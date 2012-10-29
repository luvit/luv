/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/event.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

static void uv__fs_event(EV_P_ ev_io* w, int revents);


static void uv__fs_event_start(uv_fs_event_t* handle) {
  ev_io_init(&handle->event_watcher,
             uv__fs_event,
             handle->fd,
             EV_LIBUV_KQUEUE_HACK);
  ev_io_start(handle->loop->ev, &handle->event_watcher);
}


static void uv__fs_event_stop(uv_fs_event_t* handle) {
  ev_io_stop(handle->loop->ev, &handle->event_watcher);
}


static void uv__fs_event(EV_P_ ev_io* w, int revents) {
  uv_fs_event_t* handle;
  int events;

  assert(revents == EV_LIBUV_KQUEUE_HACK);

  handle = container_of(w, uv_fs_event_t, event_watcher);

  if (handle->fflags & (NOTE_ATTRIB | NOTE_EXTEND))
    events = UV_CHANGE;
  else
    events = UV_RENAME;

  handle->cb(handle, NULL, events, 0);

  if (handle->fd == -1)
    return;

  /* File watcher operates in one-shot mode, re-arm it. */
  uv__fs_event_stop(handle);
  uv__fs_event_start(handle);
}


/* Called by libev, don't touch. */
void uv__kqueue_hack(EV_P_ int fflags, ev_io *w) {
  uv_fs_event_t* handle;

  handle = container_of(w, uv_fs_event_t, event_watcher);
  handle->fflags = fflags;
}


int uv_fs_event_init(uv_loop_t* loop,
                     uv_fs_event_t* handle,
                     const char* filename,
                     uv_fs_event_cb cb,
                     int flags) {
  int fd;
#if defined(__APPLE__)
  struct stat statbuf;
#endif /* defined(__APPLE__) */

  /* TODO open asynchronously - but how do we report back errors? */
  if ((fd = open(filename, O_RDONLY)) == -1) {
    uv__set_sys_error(loop, errno);
    return -1;
  }

  uv__handle_init(loop, (uv_handle_t*)handle, UV_FS_EVENT);
  uv__handle_start(handle); /* FIXME shouldn't start automatically */
  handle->filename = strdup(filename);
  handle->fflags = 0;
  handle->cb = cb;
  handle->fd = fd;

#if defined(__APPLE__)
  /* Nullify field to perform checks later */
  handle->cf_eventstream = NULL;
  handle->realpath = NULL;
  handle->realpath_len = 0;
  handle->cf_flags = flags;

  if (fstat(fd, &statbuf))
    goto fallback;
  /* FSEvents works only with directories */
  if (!(statbuf.st_mode & S_IFDIR))
    goto fallback;

  return uv__fsevents_init(handle);

fallback:
#endif /* defined(__APPLE__) */

  uv__fs_event_start(handle);

  return 0;
}


void uv__fs_event_close(uv_fs_event_t* handle) {
#if defined(__APPLE__)
  if (uv__fsevents_close(handle))
    uv__fs_event_stop(handle);
#else
  uv__fs_event_stop(handle);
#endif /* defined(__APPLE__) */

  uv__handle_stop(handle);
  free(handle->filename);
  close(handle->fd);
  handle->fd = -1;
}
