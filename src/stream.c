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
#include "luv.h"

static uv_stream_t* luv_check_stream(lua_State* L, int index) {
  uv_stream_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L,
    handle->type == UV_TCP ||
    handle->type == UV_TTY ||
    handle->type == UV_NAMED_PIPE,
    index, "uv_stream_t subclass required");
  return handle;
}

static void luv_shutdown_cb(uv_shutdown_t* req, int status) {
  luv_find_handle(R, req->handle->data);
  luv_status(R, status);
  luv_fulfill_req(R, req->data, 2);
  luv_cleanup_req(R, req->data);
  req->data = NULL;
}

static int luv_shutdown(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int ref = luv_check_continuation(L, 2);
  uv_shutdown_t* req = lua_newuserdata(L, sizeof(*req));
  int ret;
  req->data = luv_setup_req(L, ref);
  ret = uv_shutdown(req, handle, luv_shutdown_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static void luv_connection_cb(uv_stream_t* handle, int status) {
  luv_find_handle(R, handle->data);
  luv_status(R, status);
  luv_call_callback(R, handle->data, LUV_CONNECTION, 2);
}

static int luv_listen(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int backlog = luaL_checkinteger(L, 2);
  int ret;
  luv_check_callback(L, handle->data, LUV_CONNECTION, 3);
  ret = uv_listen(handle, backlog, luv_connection_cb);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_accept(lua_State* L) {
  uv_stream_t* server = luv_check_stream(L, 1);
  uv_stream_t* client = luv_check_stream(L, 2);
  int ret = uv_accept(server, client);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static void luv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

static void luv_read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
  luv_find_handle(R, handle->data);

  if (nread >= 0) {
    lua_pushnil(R);
    lua_pushlstring(R, buf->base, nread);
  }

  free(buf->base);
  if (nread == 0) return;

  if (nread == UV_EOF) {
    lua_pushnil(R); // no error
    lua_pushnil(R); // nil value to signify EOF
  }
  else if (nread < 0) {
    luv_status(R, nread);
  }

  luv_call_callback(R, handle->data, LUV_READ, 3);
}

static int luv_read_start(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int ret;
  luv_check_callback(L, handle->data, LUV_READ, 2);
  ret = uv_read_start(handle, luv_alloc_cb, luv_read_cb);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_read_stop(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int ret = uv_read_stop(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static void luv_write_cb(uv_write_t* req, int status) {
  luv_find_handle(R, req->handle->data);
  luv_status(R, status);
  luv_fulfill_req(R, req->data, 2);
  luv_cleanup_req(R, req->data);
  req->data = NULL;
}

static int luv_write(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  uv_write_t* req;
  uv_buf_t buf;
  int ret, ref;
  buf.base = (char*) luaL_checklstring(L, 2, &buf.len);
  ref = luv_check_continuation(L, 3);
  req = lua_newuserdata(L, sizeof(*req));
  req->data = luv_setup_req(L, ref);
  ret = uv_write(req, handle, &buf, 1, luv_write_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_write2(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  uv_write_t* req;
  uv_buf_t buf;
  int ret, ref;
  uv_stream_t* send_handle;
  buf.base = (char*) luaL_checklstring(L, 2, &buf.len);
  send_handle = luv_check_stream(L, 3);
  ref = luv_check_continuation(L, 4);
  req = lua_newuserdata(L, sizeof(*req));
  req->data = luv_setup_req(L, ref);
  ret = uv_write2(req, handle, &buf, 1, send_handle, luv_write_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_try_write(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  uv_buf_t buf;
  int ret;
  buf.base = (char*) luaL_checklstring(L, 2, &buf.len);
  ret = uv_try_write(handle, &buf, 1);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_is_readable(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  lua_pushboolean(L, uv_is_readable(handle));
  return 1;
}

static int luv_is_writable(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  lua_pushboolean(L, uv_is_writable(handle));
  return 1;
}

static int luv_stream_set_blocking(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int blocking, ret;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  blocking = lua_toboolean(L, 2);
  ret = uv_stream_set_blocking(handle, blocking);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

