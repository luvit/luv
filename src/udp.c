/*
 *  Copyright 2014 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#include "private.h"

static uv_udp_t* luv_check_udp(lua_State* L, int index) {
  uv_udp_t* handle = (uv_udp_t*)luv_checkudata(L, index, "uv_udp");
  luaL_argcheck(L, handle->type == UV_UDP && handle->data, index, "Expected uv_udp_t");
  return handle;
}

static int luv_new_udp(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  lua_settop(L, 1);
  uv_udp_t* handle = (uv_udp_t*)luv_newuserdata(L, uv_handle_size(UV_UDP));
  int ret;
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  // TODO: This default can potentially be increased, but it's
  //       not clear what the best default would be, or if unconditionally
  //       using recvmmsg is always an improvement.
  //
  //       Would probably need to do some extensive benchmarking to
  //       figure out what a good default might be.
  int mmsg_num_msgs = 1;
#endif
#if LUV_UV_VERSION_GEQ(1, 7, 0)
  unsigned int flags = AF_UNSPEC;
  if (!lua_isnoneornil(L, 1)) {
    if (lua_isnumber(L, 1)) {
      flags = lua_tointeger(L, 1);
    }
    else if (lua_isstring(L, 1)) {
      const char* family = lua_tostring(L, 1);
      flags = luv_af_string_to_num(family);
    } else if (lua_istable(L, 1)) {
      lua_getfield(L, 1, "family");
      if (lua_isnumber(L, -1)) {
        // The lower 8 bits of the flags parameter are used as the socket domain
        flags = lua_tointeger(L, -1) & 0xFF;
      }
      else if (lua_isstring(L, -1)) {
        flags = luv_af_string_to_num(lua_tostring(L, -1));
      }
      else if (!lua_isnil(L, -1)) {
        luaL_argerror(L, 1, "family must be string or integer if set");
      }
      lua_pop(L, 1);

#if LUV_UV_VERSION_GEQ(1, 39, 0)
      lua_getfield(L, 1, "mmsgs");
      if (lua_isnumber(L, -1)) {
        mmsg_num_msgs = lua_tonumber(L, -1);
      } else if (!lua_isnil(L, -1)) {
        luaL_argerror(L, 1, "mmsgs must be integer if set");
      }
      lua_pop(L, 1);
#endif
    }
    else {
      luaL_argerror(L, 1, "expected table, string, or integer");
    }
  }
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  // Libuv intended to enable this by default, but it caused a backwards-incompatibility with how
  // the buffer is freed in udp_recv_cb, so it had to be put behind a flag to avoid breaking
  // existing libuv users. However, because luv handles UV_UDP_MMSG_CHUNK in luv_udp_recv_cb, we can
  // always enable this flag and get the benefits of recvmmsg for platforms that support it.
  //
  // Relevant links:
  // - https://github.com/libuv/libuv/issues/2791
  // - https://github.com/libuv/libuv/pull/2792
  // - https://github.com/libuv/libuv/pull/2532
  // - https://github.com/libuv/libuv/issues/419
  //
  // But we should only set the flag if we can actually take advantage of it.
  if (mmsg_num_msgs > 1)
    flags |= UV_UDP_RECVMMSG;
#endif
  ret = uv_udp_init_ex(ctx->loop, handle, flags);
#else
  ret = uv_udp_init(ctx->loop, handle);
#endif
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  handle->data = luv_setup_handle(L, ctx);
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  if (flags & UV_UDP_RECVMMSG) {
    // store the number of msgs to be received for use in alloc_cb
    int* extra_data = malloc(sizeof(int));
    assert(extra_data);
    *extra_data = mmsg_num_msgs;
    ((luv_handle_t*)handle->data)->extra = extra_data;
    ((luv_handle_t*)handle->data)->extra_gc = free;
  }
#endif
  return 1;
}

static int luv_udp_get_send_queue_size(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  lua_pushinteger(L, handle->send_queue_size);
  return 1;
}

static int luv_udp_get_send_queue_count(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  lua_pushinteger(L, handle->send_queue_count);
  return 1;
}

static int luv_udp_open(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  uv_os_sock_t sock = luaL_checkinteger(L, 2);
  int ret = uv_udp_open(handle, sock);
  return luv_result(L, ret);
}

static int luv_udp_bind(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  const char* host = luaL_checkstring(L, 2);
  int port = luaL_checkinteger(L, 3);
  unsigned int flags = 0;
  struct sockaddr_storage addr;
  int ret;
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    return luaL_error(L, "Invalid IP address or port [%s:%d]", host, port);
  }
  if (lua_type(L, 4) == LUA_TTABLE) {
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_getfield(L, 4, "reuseaddr");
    if (lua_toboolean(L, -1)) flags |= UV_UDP_REUSEADDR;
    lua_pop(L, 1);
    lua_getfield(L, 4, "ipv6only");
    if (lua_toboolean(L, -1)) flags |= UV_UDP_IPV6ONLY;
    lua_pop(L, 1);
  }
  ret = uv_udp_bind(handle, (struct sockaddr*)&addr, flags);
  return luv_result(L, ret);
}

static int luv_udp_getsockname(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  struct sockaddr_storage address;
  int addrlen = sizeof(address);
  int ret = uv_udp_getsockname(handle, (struct sockaddr*)&address, &addrlen);
  if (ret < 0) return luv_error(L, ret);
  parse_sockaddr(L, &address);
  return 1;
}

// These are the same order as uv_membership which also starts at 0
static const char *const luv_membership_opts[] = {
  "leave", "join", NULL
};

static int luv_udp_set_membership(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  const char* multicast_addr = luaL_checkstring(L, 2);
  const char* interface_addr = lua_isstring(L, 3) ? lua_tostring(L, 3) : NULL;
  luaL_argcheck(L, lua_isstring(L, 3) || lua_isnil(L, 3), 3, "expected string or nil");
  uv_membership membership = (uv_membership)luaL_checkoption(L, 4, NULL, luv_membership_opts);
  int ret = uv_udp_set_membership(handle, multicast_addr, interface_addr, membership);
  return luv_result(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 32, 0)
static int luv_udp_set_source_membership(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  const char* multicast_addr = luaL_checkstring(L, 2);
  const char* interface_addr = lua_isstring(L, 3) ? lua_tostring(L, 3) : NULL;
  luaL_argcheck(L, lua_isstring(L, 3) || lua_isnil(L, 3), 3, "expected string or nil");
  const char* source_addr = luaL_checkstring(L, 4);
  uv_membership membership = (uv_membership)luaL_checkoption(L, 5, NULL, luv_membership_opts);
  int ret = uv_udp_set_source_membership(handle, multicast_addr, interface_addr, source_addr, membership);
  return luv_result(L, ret);
}
#endif

static int luv_udp_set_multicast_loop(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int on, ret;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  on = lua_toboolean(L, 2);
  ret = uv_udp_set_multicast_loop(handle, on);
  return luv_result(L, ret);
}

static int luv_udp_set_multicast_ttl(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ttl, ret;
  ttl = luaL_checkinteger(L, 2);
  ret = uv_udp_set_multicast_ttl(handle, ttl);
  return luv_result(L, ret);
}

static int luv_udp_set_multicast_interface(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  const char* interface_addr = luaL_checkstring(L, 2);
  int ret = uv_udp_set_multicast_interface(handle, interface_addr);
  return luv_result(L, ret);
}

static int luv_udp_set_broadcast(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int on, ret;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  on = lua_toboolean(L, 2);
  ret =uv_udp_set_broadcast(handle, on);
  return luv_result(L, ret);
}

static int luv_udp_set_ttl(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ttl, ret;
  ttl = luaL_checknumber(L, 2);
  ret = uv_udp_set_ttl(handle, ttl);
  return luv_result(L, ret);
}

static void luv_udp_send_cb(uv_udp_send_t* req, int status) {
  luv_req_t* data = (luv_req_t*)req->data;
  lua_State* L = data->ctx->L;
  luv_fulfill_req_status(L, (luv_req_t*)req->data, status);
  luv_cleanup_req(L, (luv_req_t*)req->data);
  req->data = NULL;
}

static struct sockaddr* luv_check_addr(lua_State *L, struct sockaddr_storage* addr, int hostidx, int portidx) {
  const char* host;
  int port;
#if LUV_UV_VERSION_GEQ(1, 27, 0)
  int host_type, port_type;
  host_type = lua_type(L, hostidx);
  port_type = lua_type(L, portidx);
  if (host_type == LUA_TNIL && port_type == LUA_TNIL) {
    return NULL;
  }
  host = lua_tostring(L, hostidx);
  port = lua_tointeger(L, portidx);
  if (host_type == LUA_TSTRING && port_type == LUA_TNUMBER) {
    if (uv_ip4_addr(host, port, (struct sockaddr_in*)addr) &&
        uv_ip6_addr(host, port, (struct sockaddr_in6*)addr)) {
      luaL_error(L, "Invalid IP address or port [%s:%d]", host, port);
      return NULL;
    }
    return (struct sockaddr*)addr;
  }
  else {
    if (host_type == LUA_TNIL || port_type == LUA_TNIL) {
      luaL_argerror(L, host_type == LUA_TNIL ? portidx : hostidx,
        "Both host and port must be nil if one is nil");
    }
    luaL_argcheck(L, host_type == LUA_TNIL || host_type == LUA_TSTRING, hostidx,
      "Host must be string or nil");
    luaL_argcheck(L, port_type == LUA_TNIL || port_type == LUA_TNUMBER, portidx,
      "Port must be number or nil");
    return NULL;
  }
#else
  host = luaL_checkstring(L, hostidx);
  port = luaL_checkinteger(L, portidx);
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)addr)) {
    luaL_error(L, "Invalid IP address or port [%s:%d]", host, port);
    return NULL;
  }
  return (struct sockaddr*)addr;
#endif
}

static int luv_udp_send(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  uv_udp_send_t* req;
  int ret, ref;
  struct sockaddr_storage addr;
  struct sockaddr* addr_ptr;
  luv_handle_t* lhandle = handle->data;
  addr_ptr = luv_check_addr(L, &addr, 3, 4);
  ref = luv_check_continuation(L, 5);
  req = (uv_udp_send_t*)lua_newuserdata(L, uv_req_size(UV_UDP_SEND));
  req->data = luv_setup_req(L, lhandle->ctx, ref);
  size_t count;
  uv_buf_t* bufs = luv_check_bufs(L, 2, &count, (luv_req_t*)req->data);
  ret = uv_udp_send(req, handle, bufs, count, addr_ptr, luv_udp_send_cb);
  free(bufs);
  if (ret < 0) {
    luv_cleanup_req(L, (luv_req_t*)req->data);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  luv_yield_req(L, (luv_req_t*)req->data);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_try_send(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int err_or_num_bytes;
  struct sockaddr_storage addr;
  struct sockaddr* addr_ptr;
  size_t count;
  uv_buf_t* bufs = luv_check_bufs_noref(L, 2, &count);
  addr_ptr = luv_check_addr(L, &addr, 3, 4);
  err_or_num_bytes = uv_udp_try_send(handle, bufs, count, addr_ptr);
  free(bufs);
  if (err_or_num_bytes < 0) return luv_error(L, err_or_num_bytes);
  lua_pushinteger(L, err_or_num_bytes);
  return 1;
}

static void luv_udp_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;

#if LUV_UV_VERSION_GEQ(1, 40, 0)
  // If UV_UDP_MMSG_FREE is set, we can skip calling the callback
  // and return early because we know the only purpose of this recv_cb call
  // is to free the buffer that was being used by recvmmsg
  if (flags & UV_UDP_MMSG_FREE) {
    free(buf->base);
    return;
  }
#endif

  // err
  if (nread < 0) {
    luv_status(L, nread);
  }
  else {
    lua_pushnil(L);
  }

  // data
  if (nread == 0) {
    if (addr) {
      lua_pushstring(L, "");
    }
    else {
      lua_pushnil(L);
    }
  }
  else if (nread > 0) {
    lua_pushlstring(L, buf->base, nread);
  }
  else {
    lua_pushnil(L);
  }
#if LUV_UV_VERSION_GEQ(1, 35, 0)
  // UV_UDP_MMSG_CHUNK Indicates that the message was received by recvmmsg, so the buffer provided
  // must not be freed by the recv_cb callback.
  if (buf && !(flags & UV_UDP_MMSG_CHUNK)) {
    free(buf->base);
  }
#else
  if (buf) free(buf->base);
#endif

  // address
  if (addr) {
    // Force the sockaddr to be properly aligned
    struct sockaddr_storage address = *(struct sockaddr_storage*)addr;
    parse_sockaddr(L, &address);
  }
  else {
    lua_pushnil(L);
  }

  // flags
  lua_newtable(L);
  if (flags & UV_UDP_PARTIAL) {
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "partial");
  }
#if LUV_UV_VERSION_GEQ(1, 35, 0)
  if (flags & UV_UDP_MMSG_CHUNK) {
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "mmsg_chunk");
  }
#endif

  luv_call_callback(L, (luv_handle_t*)handle->data, LUV_RECV, 4);
}

#if LUV_UV_VERSION_GEQ(1, 39, 0)
#define MAX_DGRAM_SIZE (64*1024)

static void luv_udp_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  size_t buffer_size = suggested_size;
  if (uv_udp_using_recvmmsg((uv_udp_t*)handle)) {
    int num_msgs = *(int*)(((luv_handle_t*)handle->data)->extra);
    buffer_size = MAX_DGRAM_SIZE * num_msgs;
  }
  buf->base = (char*)malloc(buffer_size);
  assert(buf->base);
  buf->len = buffer_size;
}
#endif

static int luv_udp_recv_start(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ret;
  luv_check_callback(L, (luv_handle_t*)handle->data, LUV_RECV, 2);
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  ret = uv_udp_recv_start(handle, luv_udp_alloc_cb, luv_udp_recv_cb);
#else
  ret = uv_udp_recv_start(handle, luv_alloc_cb, luv_udp_recv_cb);
#endif
#if LUV_UV_VERSION_LEQ(1, 23, 0)
#if LUV_UV_VERSION_GEQ(1, 10, 0)
  // in Libuv <= 1.23.0, uv_udp_recv_start will return untranslated error codes on Windows
  // (uv_translate_sys_error was only made public in libuv >= 1.10.0)
  ret = uv_translate_sys_error(ret);
#endif
#endif
  return luv_result(L, ret);
}

static int luv_udp_recv_stop(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ret = uv_udp_recv_stop(handle);
  return luv_result(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 27, 0)
static int luv_udp_connect(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  struct sockaddr_storage addr;
  struct sockaddr* addr_ptr = luv_check_addr(L, &addr, 2, 3);
  int ret = uv_udp_connect(handle, addr_ptr);
  return luv_result(L, ret);
}

static int luv_udp_getpeername(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  struct sockaddr_storage address;
  int addrlen = sizeof(address);
  int ret = uv_udp_getpeername(handle, (struct sockaddr*)&address, &addrlen);
  if (ret < 0) return luv_error(L, ret);
  parse_sockaddr(L, &address);
  return 1;
}
#endif
