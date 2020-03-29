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

static uv_stream_t* luv_check_stream(lua_State* L, int index) {
  int isStream;
  void *udata;
  uv_stream_t* handle;
  if (!(udata = lua_touserdata(L, index))) { goto fail; }
  if (!(handle = *(uv_stream_t**) udata)) { goto fail; }
  if (!handle->data) { goto fail; }
  lua_getfield(L, LUA_REGISTRYINDEX, "uv_stream");
  lua_getmetatable(L, index < 0 ? index - 1 : index);
  lua_rawget(L, -2);
  isStream = lua_toboolean(L, -1);
  lua_pop(L, 2);
  if (isStream) { return handle; }
  fail: luaL_argerror(L, index, "Expected uv_stream userdata");
  return NULL;
}

static void luv_shutdown_cb(uv_shutdown_t* req, int status) {
  luv_req_t* data = (luv_req_t*)req->data;
  lua_State* L = data->ctx->L;
  luv_status(L, status);
  luv_fulfill_req(L, (luv_req_t*)req->data, 1);
  luv_cleanup_req(L, (luv_req_t*)req->data);
  req->data = NULL;
}

static int luv_shutdown(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  uv_stream_t* handle = luv_check_stream(L, 1);
  int ref = luv_check_continuation(L, 2);
  uv_shutdown_t* req = (uv_shutdown_t*)lua_newuserdata(L, sizeof(*req));
  int ret;
  req->data = luv_setup_req(L, ctx, ref);
  ret = uv_shutdown(req, handle, luv_shutdown_cb);
  if (ret < 0) {
    luv_cleanup_req(L, (luv_req_t*)req->data);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static void luv_connection_cb(uv_stream_t* handle, int status) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  luv_status(L, status);
  luv_call_callback(L, (luv_handle_t*)handle->data, LUV_CONNECTION, 1);
}

static int luv_listen(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int backlog = luaL_checkinteger(L, 2);
  int ret;
  luv_check_callback(L, (luv_handle_t*)handle->data, LUV_CONNECTION, 3);
  ret = uv_listen(handle, backlog, luv_connection_cb);
  return luv_result(L, ret);
}

static int luv_accept(lua_State* L) {
  uv_stream_t* server = luv_check_stream(L, 1);
  uv_stream_t* client = luv_check_stream(L, 2);
  int ret = uv_accept(server, client);
  return luv_result(L, ret);
}

static void luv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  (void)handle;
  buf->base = (char*)malloc(suggested_size);
  assert(buf->base);
  buf->len = suggested_size;
}

static void luv_read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  int nargs;

  if (nread > 0) {
    lua_pushnil(L);
    lua_pushlstring(L, buf->base, nread);
    nargs = 2;
  }

  free(buf->base);
  if (nread == 0) return;

  if (nread == UV_EOF) {
    nargs = 0;
  }
  else if (nread < 0) {
    luv_status(L, nread);
    nargs = 1;
  }

  luv_call_callback(L, (luv_handle_t*)handle->data, LUV_READ, nargs);
}

static int luv_read_start(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int ret;
  luv_check_callback(L, (luv_handle_t*)handle->data, LUV_READ, 2);
  ret = uv_read_start(handle, luv_alloc_cb, luv_read_cb);
  return luv_result(L, ret);
}

static int luv_read_stop(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int ret = uv_read_stop(handle);
  return luv_result(L, ret);
}

static void luv_write_cb(uv_write_t* req, int status) {
  luv_req_t* data = (luv_req_t*)req->data;
  lua_State* L = data->ctx->L;
  luv_status(L, status);
  luv_fulfill_req(L, (luv_req_t*)req->data, 1);
  luv_cleanup_req(L, (luv_req_t*)req->data);
  req->data = NULL;
}

static int luv_write(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  uv_stream_t* handle = luv_check_stream(L, 1);
  uv_write_t* req;
  int ret, ref;
  ref = luv_check_continuation(L, 3);
  req = (uv_write_t *)lua_newuserdata(L, sizeof(*req));
  req->data = (luv_req_t*)luv_setup_req(L, ctx, ref);
  size_t count;
  uv_buf_t* bufs = luv_check_bufs(L, 2, &count, (luv_req_t*)req->data);
  ret = uv_write(req, handle, bufs, count, luv_write_cb);
  free(bufs);
  if (ret < 0) {
    luv_cleanup_req(L, (luv_req_t*)req->data);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_write2(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  uv_stream_t* handle = luv_check_stream(L, 1);
  uv_write_t* req;
  int ret, ref;
  uv_stream_t* send_handle;
  send_handle = luv_check_stream(L, 3);
  ref = luv_check_continuation(L, 4);
  req = (uv_write_t *)lua_newuserdata(L, sizeof(*req));
  req->data = luv_setup_req(L, ctx, ref);
  size_t count;
  uv_buf_t* bufs = luv_check_bufs(L, 2, &count, (luv_req_t*)req->data);
  ret = uv_write2(req, handle, bufs, count, send_handle, luv_write_cb);
  free(bufs);
  if (ret < 0) {
    luv_cleanup_req(L, (luv_req_t*)req->data);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_try_write(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  int err_or_num_bytes;
  size_t count;
  uv_buf_t* bufs = luv_check_bufs_noref(L, 2, &count);
  err_or_num_bytes = uv_try_write(handle, bufs, count);
  free(bufs);
  if (err_or_num_bytes < 0) return luv_error(L, err_or_num_bytes);
  lua_pushinteger(L, err_or_num_bytes);
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
  return luv_result(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 19, 0)
static int luv_stream_get_write_queue_size(lua_State* L) {
  uv_stream_t* handle = luv_check_stream(L, 1);
  lua_pushinteger(L, uv_stream_get_write_queue_size(handle));
  return 1;
}
#endif
