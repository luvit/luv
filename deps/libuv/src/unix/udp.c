/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "internal.h"

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


static void uv__udp_run_completed(uv_udp_t* handle);
static void uv__udp_run_pending(uv_udp_t* handle);
static void uv__udp_recvmsg(uv_loop_t* loop, uv__io_t* w, int revents);
static void uv__udp_sendmsg(uv_loop_t* loop, uv__io_t* w, int revents);
static int uv__udp_maybe_deferred_bind(uv_udp_t* handle, int domain);
static int uv__udp_send(uv_udp_send_t* req, uv_udp_t* handle, uv_buf_t bufs[],
    int bufcnt, struct sockaddr* addr, socklen_t addrlen, uv_udp_send_cb send_cb);


static void uv__udp_start_watcher(uv_udp_t* handle,
                                  uv__io_t* w,
                                  uv__io_cb cb,
                                  int events) {
  if (uv__io_active(w)) return;
  uv__io_init(w, cb, handle->fd, events);
  uv__io_start(handle->loop, w);
  uv__handle_start(handle);
}


static void uv__udp_stop_watcher(uv_udp_t* handle, uv__io_t* w) {
  if (!uv__io_active(w)) return;
  uv__io_stop(handle->loop, w);

  if (!uv__io_active(&handle->read_watcher) &&
      !uv__io_active(&handle->write_watcher))
  {
    uv__handle_stop(handle);
  }
}


void uv__udp_close(uv_udp_t* handle) {
  uv__udp_stop_watcher(handle, &handle->write_watcher);
  uv__udp_stop_watcher(handle, &handle->read_watcher);
  close(handle->fd);
  handle->fd = -1;
}


void uv__udp_finish_close(uv_udp_t* handle) {
  uv_udp_send_t* req;
  ngx_queue_t* q;

  assert(!uv__io_active(&handle->write_watcher));
  assert(!uv__io_active(&handle->read_watcher));
  assert(handle->fd == -1);

  uv__udp_run_completed(handle);

  while (!ngx_queue_empty(&handle->write_queue)) {
    q = ngx_queue_head(&handle->write_queue);
    ngx_queue_remove(q);

    req = ngx_queue_data(q, uv_udp_send_t, queue);
    uv__req_unregister(handle->loop, req);

    if (req->bufs != req->bufsml)
      free(req->bufs);
    req->bufs = NULL;

    if (req->send_cb) {
      /* FIXME proper error code like UV_EABORTED */
      uv__set_artificial_error(handle->loop, UV_EINTR);
      req->send_cb(req, -1);
    }
  }

  /* Now tear down the handle. */
  handle->flags = 0;
  handle->recv_cb = NULL;
  handle->alloc_cb = NULL;
  /* but _do not_ touch close_cb */
}


static void uv__udp_run_pending(uv_udp_t* handle) {
  uv_udp_send_t* req;
  ngx_queue_t* q;
  struct msghdr h;
  ssize_t size;

  while (!ngx_queue_empty(&handle->write_queue)) {
    q = ngx_queue_head(&handle->write_queue);
    assert(q != NULL);

    req = ngx_queue_data(q, uv_udp_send_t, queue);
    assert(req != NULL);

    memset(&h, 0, sizeof h);
    h.msg_name = &req->addr;
    h.msg_namelen = (req->addr.sin6_family == AF_INET6 ?
      sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in));
    h.msg_iov = (struct iovec*)req->bufs;
    h.msg_iovlen = req->bufcnt;

    do {
      size = sendmsg(handle->fd, &h, 0);
    }
    while (size == -1 && errno == EINTR);

    /* TODO try to write once or twice more in the
     * hope that the socket becomes readable again?
     */
    if (size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
      break;

    req->status = (size == -1 ? -errno : size);

#ifndef NDEBUG
    /* Sanity check. */
    if (size != -1) {
      ssize_t nbytes;
      int i;

      for (nbytes = i = 0; i < req->bufcnt; i++)
        nbytes += req->bufs[i].len;

      assert(size == nbytes);
    }
#endif

    /* Sending a datagram is an atomic operation: either all data
     * is written or nothing is (and EMSGSIZE is raised). That is
     * why we don't handle partial writes. Just pop the request
     * off the write queue and onto the completed queue, done.
     */
    ngx_queue_remove(&req->queue);
    ngx_queue_insert_tail(&handle->write_completed_queue, &req->queue);
  }
}


static void uv__udp_run_completed(uv_udp_t* handle) {
  uv_udp_send_t* req;
  ngx_queue_t* q;

  while (!ngx_queue_empty(&handle->write_completed_queue)) {
    q = ngx_queue_head(&handle->write_completed_queue);
    ngx_queue_remove(q);

    req = ngx_queue_data(q, uv_udp_send_t, queue);
    uv__req_unregister(handle->loop, req);

    if (req->bufs != req->bufsml)
      free(req->bufs);
    req->bufs = NULL;

    if (req->send_cb == NULL)
      continue;

    /* req->status >= 0 == bytes written
     * req->status <  0 == errno
     */
    if (req->status >= 0) {
      req->send_cb(req, 0);
    }
    else {
      uv__set_sys_error(handle->loop, -req->status);
      req->send_cb(req, -1);
    }
  }
}


static void uv__udp_recvmsg(uv_loop_t* loop, uv__io_t* w, int revents) {
  struct sockaddr_storage peer;
  struct msghdr h;
  uv_udp_t* handle;
  ssize_t nread;
  uv_buf_t buf;
  int flags;
  int count;

  handle = container_of(w, uv_udp_t, read_watcher);
  assert(handle->type == UV_UDP);
  assert(revents & UV__IO_READ);

  assert(handle->recv_cb != NULL);
  assert(handle->alloc_cb != NULL);

  /* Prevent loop starvation when the data comes in as fast as (or faster than)
   * we can read it. XXX Need to rearm fd if we switch to edge-triggered I/O.
   */
  count = 32;

  memset(&h, 0, sizeof(h));
  h.msg_name = &peer;

  do {
    buf = handle->alloc_cb((uv_handle_t*)handle, 64 * 1024);
    assert(buf.len > 0);
    assert(buf.base != NULL);

    h.msg_namelen = sizeof(peer);
    h.msg_iov = (struct iovec*)&buf;
    h.msg_iovlen = 1;

    do {
      nread = recvmsg(handle->fd, &h, 0);
    }
    while (nread == -1 && errno == EINTR);

    if (nread == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        uv__set_sys_error(handle->loop, EAGAIN);
        handle->recv_cb(handle, 0, buf, NULL, 0);
      }
      else {
        uv__set_sys_error(handle->loop, errno);
        handle->recv_cb(handle, -1, buf, NULL, 0);
      }
    }
    else {
      flags = 0;

      if (h.msg_flags & MSG_TRUNC)
        flags |= UV_UDP_PARTIAL;

      handle->recv_cb(handle,
                      nread,
                      buf,
                      (struct sockaddr*)&peer,
                      flags);
    }
  }
  /* recv_cb callback may decide to pause or close the handle */
  while (nread != -1
      && count-- > 0
      && handle->fd != -1
      && handle->recv_cb != NULL);
}


static void uv__udp_sendmsg(uv_loop_t* loop, uv__io_t* w, int revents) {
  uv_udp_t* handle;

  handle = container_of(w, uv_udp_t, write_watcher);
  assert(handle->type == UV_UDP);
  assert(revents & UV__IO_WRITE);

  assert(!ngx_queue_empty(&handle->write_queue)
      || !ngx_queue_empty(&handle->write_completed_queue));

  /* Write out pending data first. */
  uv__udp_run_pending(handle);

  /* Drain 'request completed' queue. */
  uv__udp_run_completed(handle);

  if (!ngx_queue_empty(&handle->write_completed_queue)) {
    /* Schedule completion callbacks. */
    uv__io_feed(handle->loop, &handle->write_watcher, UV__IO_WRITE);
  }
  else if (ngx_queue_empty(&handle->write_queue)) {
    /* Pending queue and completion queue empty, stop watcher. */
    uv__udp_stop_watcher(handle, &handle->write_watcher);
  }
}


static int uv__bind(uv_udp_t* handle,
                    int domain,
                    struct sockaddr* addr,
                    socklen_t len,
                    unsigned flags) {
  int saved_errno;
  int status;
  int yes;
  int fd;

  saved_errno = errno;
  status = -1;
  fd = -1;

  /* Check for bad flags. */
  if (flags & ~UV_UDP_IPV6ONLY) {
    uv__set_sys_error(handle->loop, EINVAL);
    goto out;
  }

  /* Cannot set IPv6-only mode on non-IPv6 socket. */
  if ((flags & UV_UDP_IPV6ONLY) && domain != AF_INET6) {
    uv__set_sys_error(handle->loop, EINVAL);
    goto out;
  }

  if (handle->fd == -1) {
    if ((fd = uv__socket(domain, SOCK_DGRAM, 0)) == -1) {
      uv__set_sys_error(handle->loop, errno);
      goto out;
    }
    handle->fd = fd;
  }

  fd = handle->fd;
  yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }

  /* On the BSDs, SO_REUSEADDR lets you reuse an address that's in the TIME_WAIT
   * state (i.e. was until recently tied to a socket) while SO_REUSEPORT lets
   * multiple processes bind to the same address. Yes, it's something of a
   * misnomer but then again, SO_REUSEADDR was already taken.
   *
   * None of the above applies to Linux: SO_REUSEADDR implies SO_REUSEPORT on
   * Linux and hence it does not have SO_REUSEPORT at all.
   */
#ifdef SO_REUSEPORT
  yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes) == -1) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }
#endif

  if (flags & UV_UDP_IPV6ONLY) {
#ifdef IPV6_V6ONLY
    yes = 1;
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof yes) == -1) {
      uv__set_sys_error(handle->loop, errno);
      goto out;
    }
#else
    uv__set_sys_error(handle->loop, ENOTSUP);
    goto out;
#endif
  }

  if (bind(fd, addr, len) == -1) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }

  status = 0;

out:
  if (status) {
    close(handle->fd);
    handle->fd = -1;
  }

  errno = saved_errno;
  return status;
}


static int uv__udp_maybe_deferred_bind(uv_udp_t* handle, int domain) {
  unsigned char taddr[sizeof(struct sockaddr_in6)];
  socklen_t addrlen;

  assert(domain == AF_INET || domain == AF_INET6);

  if (handle->fd != -1)
    return 0;

  switch (domain) {
  case AF_INET:
  {
    struct sockaddr_in* addr = (void*)&taddr;
    memset(addr, 0, sizeof *addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addrlen = sizeof *addr;
    break;
  }
  case AF_INET6:
  {
    struct sockaddr_in6* addr = (void*)&taddr;
    memset(addr, 0, sizeof *addr);
    addr->sin6_family = AF_INET6;
    addr->sin6_addr = in6addr_any;
    addrlen = sizeof *addr;
    break;
  }
  default:
    assert(0 && "unsupported address family");
    abort();
  }

  return uv__bind(handle, domain, (struct sockaddr*)&taddr, addrlen, 0);
}


static int uv__udp_send(uv_udp_send_t* req,
                        uv_udp_t* handle,
                        uv_buf_t bufs[],
                        int bufcnt,
                        struct sockaddr* addr,
                        socklen_t addrlen,
                        uv_udp_send_cb send_cb) {
  assert(bufcnt > 0);

  if (uv__udp_maybe_deferred_bind(handle, addr->sa_family))
    return -1;

  uv__req_init(handle->loop, req, UV_UDP_SEND);

  assert(addrlen <= sizeof(req->addr));
  memcpy(&req->addr, addr, addrlen);
  req->send_cb = send_cb;
  req->handle = handle;
  req->bufcnt = bufcnt;

  if (bufcnt <= (int) ARRAY_SIZE(req->bufsml)) {
    req->bufs = req->bufsml;
  }
  else if ((req->bufs = malloc(bufcnt * sizeof(bufs[0]))) == NULL) {
    uv__set_sys_error(handle->loop, ENOMEM);
    return -1;
  }
  memcpy(req->bufs, bufs, bufcnt * sizeof(bufs[0]));

  ngx_queue_insert_tail(&handle->write_queue, &req->queue);

  uv__udp_start_watcher(handle,
                        &handle->write_watcher,
                        uv__udp_sendmsg,
                        UV__IO_WRITE);

  return 0;
}


int uv_udp_init(uv_loop_t* loop, uv_udp_t* handle) {
  memset(handle, 0, sizeof *handle);

  uv__handle_init(loop, (uv_handle_t*)handle, UV_UDP);
  handle->fd = -1;
  ngx_queue_init(&handle->write_queue);
  ngx_queue_init(&handle->write_completed_queue);

  return 0;
}


int uv__udp_bind(uv_udp_t* handle, struct sockaddr_in addr, unsigned flags) {
  return uv__bind(handle,
                  AF_INET,
                  (struct sockaddr*)&addr,
                  sizeof addr,
                  flags);
}


int uv__udp_bind6(uv_udp_t* handle, struct sockaddr_in6 addr, unsigned flags) {
  return uv__bind(handle,
                  AF_INET6,
                  (struct sockaddr*)&addr,
                  sizeof addr,
                  flags);
}


int uv_udp_open(uv_udp_t* handle, uv_os_sock_t sock) {
  int saved_errno;
  int status;
  int yes;

  saved_errno = errno;
  status = -1;

  /* Check for already active socket. */
  if (handle->fd != -1) {
    uv__set_artificial_error(handle->loop, UV_EALREADY);
    goto out;
  }

  yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }

  /* On the BSDs, SO_REUSEADDR lets you reuse an address that's in the TIME_WAIT
   * state (i.e. was until recently tied to a socket) while SO_REUSEPORT lets
   * multiple processes bind to the same address. Yes, it's something of a
   * misnomer but then again, SO_REUSEADDR was already taken.
   *
   * None of the above applies to Linux: SO_REUSEADDR implies SO_REUSEPORT on
   * Linux and hence it does not have SO_REUSEPORT at all.
   */
#ifdef SO_REUSEPORT
  yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes) == -1) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }
#endif

  handle->fd = sock;
  status = 0;

out:
  errno = saved_errno;
  return status;
}


int uv_udp_set_membership(uv_udp_t* handle, const char* multicast_addr,
  const char* interface_addr, uv_membership membership) {

  int optname;
  struct ip_mreq mreq;
  memset(&mreq, 0, sizeof mreq);

  if (interface_addr) {
    mreq.imr_interface.s_addr = inet_addr(interface_addr);
  } else {
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  }

  mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);

  switch (membership) {
  case UV_JOIN_GROUP:
    optname = IP_ADD_MEMBERSHIP;
    break;
  case UV_LEAVE_GROUP:
    optname = IP_DROP_MEMBERSHIP;
    break;
  default:
    uv__set_sys_error(handle->loop, EFAULT);
    return -1;
  }

  if (setsockopt(handle->fd, IPPROTO_IP, optname, (void*) &mreq, sizeof mreq) == -1) {
    uv__set_sys_error(handle->loop, errno);
    return -1;
  }

  return 0;
}


static int uv__setsockopt_maybe_char(uv_udp_t* handle, int option, int val) {
#if __sun
  char arg = val;
#else
  int arg = val;
#endif

  if (val < 0 || val > 255)
    return uv__set_sys_error(handle->loop, EINVAL);

  if (setsockopt(handle->fd, IPPROTO_IP, option, &arg, sizeof(arg)))
    return uv__set_sys_error(handle->loop, errno);

  return 0;
}


int uv_udp_set_broadcast(uv_udp_t* handle, int on) {
  if (setsockopt(handle->fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)))
    return uv__set_sys_error(handle->loop, errno);

  return 0;
}


int uv_udp_set_ttl(uv_udp_t* handle, int ttl) {
  if (ttl < 1 || ttl > 255)
    return uv__set_sys_error(handle->loop, EINVAL);

  if (setsockopt(handle->fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)))
    return uv__set_sys_error(handle->loop, errno);

  return 0;
}


int uv_udp_set_multicast_ttl(uv_udp_t* handle, int ttl) {
  return uv__setsockopt_maybe_char(handle, IP_MULTICAST_TTL, ttl);
}


int uv_udp_set_multicast_loop(uv_udp_t* handle, int on) {
  return uv__setsockopt_maybe_char(handle, IP_MULTICAST_LOOP, on);
}


int uv_udp_getsockname(uv_udp_t* handle, struct sockaddr* name, int* namelen) {
  socklen_t socklen;
  int saved_errno;
  int rv = 0;

  /* Don't clobber errno. */
  saved_errno = errno;

  if (handle->fd < 0) {
    uv__set_sys_error(handle->loop, EINVAL);
    rv = -1;
    goto out;
  }

  /* sizeof(socklen_t) != sizeof(int) on some systems. */
  socklen = (socklen_t)*namelen;

  if (getsockname(handle->fd, name, &socklen) == -1) {
    uv__set_sys_error(handle->loop, errno);
    rv = -1;
  } else {
    *namelen = (int)socklen;
  }

out:
  errno = saved_errno;
  return rv;
}


int uv_udp_send(uv_udp_send_t* req,
                uv_udp_t* handle,
                uv_buf_t bufs[],
                int bufcnt,
                struct sockaddr_in addr,
                uv_udp_send_cb send_cb) {
  return uv__udp_send(req,
                      handle,
                      bufs,
                      bufcnt,
                      (struct sockaddr*)&addr,
                      sizeof addr,
                      send_cb);
}


int uv_udp_send6(uv_udp_send_t* req,
                 uv_udp_t* handle,
                 uv_buf_t bufs[],
                 int bufcnt,
                 struct sockaddr_in6 addr,
                 uv_udp_send_cb send_cb) {
  return uv__udp_send(req,
                      handle,
                      bufs,
                      bufcnt,
                      (struct sockaddr*)&addr,
                      sizeof addr,
                      send_cb);
}


int uv_udp_recv_start(uv_udp_t* handle,
                      uv_alloc_cb alloc_cb,
                      uv_udp_recv_cb recv_cb) {
  if (alloc_cb == NULL || recv_cb == NULL) {
    uv__set_artificial_error(handle->loop, UV_EINVAL);
    return -1;
  }

  if (uv__io_active(&handle->read_watcher)) {
    uv__set_artificial_error(handle->loop, UV_EALREADY);
    return -1;
  }

  if (uv__udp_maybe_deferred_bind(handle, AF_INET))
    return -1;

  handle->alloc_cb = alloc_cb;
  handle->recv_cb = recv_cb;

  uv__udp_start_watcher(handle,
                        &handle->read_watcher,
                        uv__udp_recvmsg,
                        UV__IO_READ);

  return 0;
}


int uv_udp_recv_stop(uv_udp_t* handle) {
  uv__udp_stop_watcher(handle, &handle->read_watcher);
  handle->alloc_cb = NULL;
  handle->recv_cb = NULL;
  return 0;
}
