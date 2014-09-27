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
#include "userdata.h"

// Given a userdata on top of the stack, give it a metatable and an environment.
// +0 to stack
static luv_ref_t* setup_udata(lua_State* L, const char* type) {

  // Record the lua_State and ref the userdata.
  luv_ref_t* data = malloc(sizeof(*data));
  lua_pushvalue(L, -1);
  data->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  data->L = L;

  // Tag the userdata with the given type.
  luaL_getmetatable(L, type);
  lua_setmetatable(L, -2);

  // Create a new environment for storing stuff.
  lua_newtable(L);
  lua_setuservalue(L, -2);

  return data;
}

static uv_loop_t* luv_create_loop(lua_State* L) {
  uv_loop_t* loop = lua_newuserdata(L, sizeof(*loop));
  // Tag the userdata with the given type.
  // Don't store ref in registry because of https://github.com/joyent/libuv/issues/1503
  luaL_getmetatable(L, "uv_loop");
  lua_setmetatable(L, -2);
  return loop;
}
static uv_timer_t* luv_create_timer(lua_State* L) {
  uv_timer_t* timer = lua_newuserdata(L, sizeof(*timer));
  timer->data = setup_udata(L, "uv_handle");
  return timer;
}
static uv_async_t* luv_create_async(lua_State* L) {
  uv_async_t* async = lua_newuserdata(L, sizeof(*async));
  async->data = setup_udata(L, "uv_handle");
  return async;
}
static uv_check_t* luv_create_check(lua_State* L) {
  uv_check_t* check = lua_newuserdata(L, sizeof(*check));
  check->data = setup_udata(L, "uv_handle");
  return check;
}
static uv_fs_event_t* luv_create_fs_event(lua_State* L) {
  uv_fs_event_t* fs_event = lua_newuserdata(L, sizeof(*fs_event));
  fs_event->data = setup_udata(L, "uv_handle");
  return fs_event;
}
static uv_fs_poll_t* luv_create_fs_poll(lua_State* L) {
  uv_fs_poll_t* fs_poll = lua_newuserdata(L, sizeof(*fs_poll));
  fs_poll->data = setup_udata(L, "uv_handle");
  return fs_poll;
}
static uv_idle_t* luv_create_idle(lua_State* L) {
  uv_idle_t* idle = lua_newuserdata(L, sizeof(*idle));
  idle->data = setup_udata(L, "uv_handle");
  return idle;
}
static uv_pipe_t* luv_create_pipe(lua_State* L) {
  uv_pipe_t* pipe = lua_newuserdata(L, sizeof(*pipe));
  pipe->data = setup_udata(L, "uv_handle");
  return pipe;
}
static uv_poll_t* luv_create_poll(lua_State* L) {
  uv_poll_t* poll = lua_newuserdata(L, sizeof(*poll));
  poll->data = setup_udata(L, "uv_handle");
  return poll;
}
static uv_prepare_t* luv_create_prepare(lua_State* L) {
  uv_prepare_t* prepare = lua_newuserdata(L, sizeof(*prepare));
  prepare->data = setup_udata(L, "uv_handle");
  return prepare;
}
static uv_process_t* luv_create_process(lua_State* L) {
  uv_process_t* process = lua_newuserdata(L, sizeof(*process));
  process->data = setup_udata(L, "uv_handle");
  return process;
}
static uv_tcp_t* luv_create_tcp(lua_State* L) {
  uv_tcp_t* tcp = lua_newuserdata(L, sizeof(*tcp));
  tcp->data = setup_udata(L, "uv_handle");
  return tcp;
}
static uv_tty_t* luv_create_tty(lua_State* L) {
  uv_tty_t* tty = lua_newuserdata(L, sizeof(*tty));
  tty->data = setup_udata(L, "uv_handle");
  return tty;
}
static uv_udp_t* luv_create_udp(lua_State* L) {
  uv_udp_t* udp = lua_newuserdata(L, sizeof(*udp));
  udp->data = setup_udata(L, "uv_handle");
  return udp;
}
static uv_signal_t* luv_create_signal(lua_State* L) {
  uv_signal_t* signal = lua_newuserdata(L, sizeof(*signal));
  signal->data = setup_udata(L, "uv_handle");
  return signal;
}
static uv_connect_t* luv_create_connect(lua_State* L) {
  uv_connect_t* connect = lua_newuserdata(L, sizeof(*connect));
  connect->data = setup_udata(L, "uv_req");
  return connect;
}
static uv_write_t* luv_create_write(lua_State* L) {
  uv_write_t* write = lua_newuserdata(L, sizeof(*write));
  write->data = setup_udata(L, "uv_req");
  return write;
}
static uv_shutdown_t* luv_create_shutdown(lua_State* L) {
  uv_shutdown_t* shutdown = lua_newuserdata(L, sizeof(*shutdown));
  shutdown->data = setup_udata(L, "uv_req");
  return shutdown;
}
static uv_udp_send_t* luv_create_udp_send(lua_State* L) {
  uv_udp_send_t* udp_send = lua_newuserdata(L, sizeof(*udp_send));
  udp_send->data = setup_udata(L, "uv_req");
  return udp_send;
}
static uv_fs_t* luv_create_fs(lua_State* L) {
  uv_fs_t* fs = lua_newuserdata(L, sizeof(*fs));
  fs->data = setup_udata(L, "uv_req");
  return fs;
}
static uv_work_t* luv_create_work(lua_State* L) {
  uv_work_t* work = lua_newuserdata(L, sizeof(*work));
  work->data = setup_udata(L, "uv_req");
  return work;
}
static uv_getaddrinfo_t* luv_create_getaddrinfo(lua_State* L) {
  uv_getaddrinfo_t* getaddrinfo = lua_newuserdata(L, sizeof(*getaddrinfo));
  getaddrinfo->data = setup_udata(L, "uv_req");
  return getaddrinfo;
}
static uv_getnameinfo_t* luv_create_getnameinfo(lua_State* L) {
  uv_getnameinfo_t* getnameinfo = lua_newuserdata(L, sizeof(*getnameinfo));
  getnameinfo->data = setup_udata(L, "uv_req");
  return getnameinfo;
}


static uv_loop_t* luv_check_loop(lua_State* L, int index) {
  return luaL_checkudata(L, index, "uv_loop");
}
static uv_handle_t* luv_check_handle(lua_State* L, int index) {
  return luaL_checkudata(L, index, "uv_handle");
}
static uv_stream_t* luv_check_stream(lua_State* L, int index) {
  uv_stream_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L,
    handle->type == UV_TCP ||
    handle->type == UV_TTY ||
    handle->type == UV_NAMED_PIPE ||
    handle->type == UV_UDP, index, "uv_stream_t subclass required");
  return handle;
}
static uv_async_t* luv_check_async(lua_State* L, int index) {
  uv_async_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_ASYNC, index, "uv_async_t required");
  return handle;
}
static uv_check_t* luv_check_check(lua_State* L, int index) {
  uv_check_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_CHECK, index, "uv_check_t required");
  return handle;
}
static uv_fs_event_t* luv_check_fs_event(lua_State* L, int index) {
  uv_fs_event_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_FS_EVENT, index, "uv_fs_event_t required");
  return handle;
}
static uv_fs_poll_t* luv_check_fs_poll(lua_State* L, int index) {
  uv_fs_poll_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_FS_POLL, index, "uv_fs_poll_t required");
  return handle;
}
static uv_idle_t* luv_check_idle(lua_State* L, int index) {
  uv_idle_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_IDLE, index, "uv_idle_t required");
  return handle;
}
static uv_pipe_t* luv_check_pipe(lua_State* L, int index) {
  uv_pipe_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_NAMED_PIPE, index, "uv_pipe_t required");
  return handle;
}
static uv_poll_t* luv_check_poll(lua_State* L, int index) {
  uv_poll_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_POLL, index, "uv_poll_t required");
  return handle;
}
static uv_prepare_t* luv_check_prepare(lua_State* L, int index) {
  uv_prepare_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_PREPARE, index, "uv_prepare_t required");
  return handle;
}
static uv_process_t* luv_check_process(lua_State* L, int index) {
  uv_process_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_PROCESS, index, "uv_process_t required");
  return handle;
}
static uv_tcp_t* luv_check_tcp(lua_State* L, int index) {
  uv_tcp_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_TCP, index, "uv_tcp_t required");
  return handle;
}
static uv_timer_t* luv_check_timer(lua_State* L, int index) {
  uv_timer_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_TIMER, index, "uv_timer_t required");
  return handle;
}
static uv_tty_t* luv_check_tty(lua_State* L, int index) {
  uv_tty_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_TTY, index, "uv_tty_t required");
  return handle;
}
static uv_udp_t* luv_check_udp(lua_State* L, int index) {
  uv_udp_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_UDP, index, "uv_udp_t required");
  return handle;
}
static uv_signal_t* luv_check_signal(lua_State* L, int index) {
  uv_signal_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type == UV_SIGNAL, index, "uv_signal_t required");
  return handle;
}
static uv_connect_t* luv_check_connect(lua_State* L, int index) {
  uv_connect_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_CONNECT, index, "uv_connect_t required");
  return req;
}
static uv_write_t* luv_check_write(lua_State* L, int index) {
  uv_write_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_WRITE, index, "uv_write_t required");
  return req;
}
static uv_shutdown_t* luv_check_shutdown(lua_State* L, int index) {
  uv_shutdown_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_SHUTDOWN, index, "uv_shutdown_t required");
  return req;
}
static uv_udp_send_t* luv_check_udp_send(lua_State* L, int index) {
  uv_udp_send_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_UDP_SEND, index, "uv_udp_send_t required");
  return req;
}
static uv_fs_t* luv_check_fs(lua_State* L, int index) {
  uv_fs_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_FS, index, "uv_fs_t required");
  return req;
}
static uv_work_t* luv_check_work(lua_State* L, int index) {
  uv_work_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_WORK, index, "uv_work_t required");
  return req;
}
static uv_getaddrinfo_t* luv_check_getaddrinfo(lua_State* L, int index) {
  uv_getaddrinfo_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_GETADDRINFO, index, "uv_getaddrinfo_t required");
  return req;
}
static uv_getnameinfo_t* luv_check_getnameinfo(lua_State* L, int index) {
  uv_getnameinfo_t* req = luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_GETNAMEINFO, index, "uv_getnameinfo_t required");
  return req;
}


static lua_State* luv_find(luv_ref_t* data) {
  if (!data) return NULL;
  assert(data->ref < 0x1000); // probably invalid data
  lua_rawgeti(data->L, LUA_REGISTRYINDEX, data->ref);
  return data->L;
}

static lua_State* luv_unref_data(luv_ref_t* data) {
  lua_State* L;
  if (!data) return NULL;
  L = data->L;
  luaL_unref(L, LUA_REGISTRYINDEX, data->ref);
  free(data);
  return L;
}

static lua_State* luv_unref_req(uv_req_t* req) {
  lua_State* L = luv_unref_data(req->data);
  req->data = NULL;
  return L;
}
static lua_State* luv_unref_handle(uv_handle_t* handle) {
  lua_State* L = luv_unref_data(handle->data);
  handle->data = NULL;
  return L;
}
