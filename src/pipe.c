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

static uv_pipe_t* luv_check_pipe(lua_State* L, int index) {
  uv_pipe_t* handle = (uv_pipe_t*)luv_checkudata(L, index, "uv_pipe");
  luaL_argcheck(L, handle->type == UV_NAMED_PIPE && handle->data, index, "Expected uv_pipe_t");
  return handle;
}

static int luv_new_pipe(lua_State* L) {
  uv_pipe_t* handle;
  int ipc, ret;
  luv_ctx_t* ctx = luv_context(L);
  ipc = luv_optboolean(L, 1, 0);
  handle = (uv_pipe_t*)luv_newuserdata(L, sizeof(*handle));
  ret = uv_pipe_init(ctx->loop, handle, ipc);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  handle->data = luv_setup_handle(L, ctx);
  return 1;
}

static int luv_pipe_open(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  uv_file file = luaL_checkinteger(L, 2);
  int ret = uv_pipe_open(handle, file);
  return luv_result(L, ret);
}

static int luv_pipe_bind(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  const char* name = luaL_checkstring(L, 2);
  int ret = uv_pipe_bind(handle, name);
  return luv_result(L, ret);
}

static int luv_pipe_connect(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  const char* name = luaL_checkstring(L, 2);
  int ref = luv_check_continuation(L, 3);
  uv_connect_t* req = (uv_connect_t*)lua_newuserdata(L, sizeof(*req));
  req->data = luv_setup_req(L, ctx, ref);
  uv_pipe_connect(req, handle, name, luv_connect_cb);
  return 1;
}

static int luv_pipe_getsockname(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  size_t len = 2*PATH_MAX;
  char buf[2*PATH_MAX];
  int ret = uv_pipe_getsockname(handle, buf, &len);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, buf, len);
  return 1;
}

#if LUV_UV_VERSION_GEQ(1, 3, 0)
static int luv_pipe_getpeername(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  size_t len = 2*PATH_MAX;
  char buf[2*PATH_MAX];
  int ret = uv_pipe_getpeername(handle, buf, &len);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, buf, len);
  return 1;
}
#endif

static int luv_pipe_pending_instances(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  int count = luaL_checkinteger(L, 2);
  uv_pipe_pending_instances(handle, count);
  return 0;
}

static int luv_pipe_pending_count(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  lua_pushinteger(L, uv_pipe_pending_count(handle));
  return 1;
}

static int luv_pipe_pending_type(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  uv_handle_type type = uv_pipe_pending_type(handle);
  const char* type_name;
  switch (type) {
#define XX(uc, lc) \
    case UV_##uc: type_name = #lc; break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default: return 0;
  }
  lua_pushstring(L, type_name);
  return 1;
}

#if LUV_UV_VERSION_GEQ(1,16,0)
static const char *const luv_pipe_chmod_flags[] = {
  "r", "w", "rw", "wr", NULL
};

static int luv_pipe_chmod(lua_State* L) {
  uv_pipe_t* handle = luv_check_pipe(L, 1);
  int flags;
  switch (luaL_checkoption(L, 2, NULL, luv_pipe_chmod_flags)) {
  case 0: flags = UV_READABLE; break;
  case 1: flags = UV_WRITABLE; break;
  case 2: case 3: flags = UV_READABLE | UV_WRITABLE; break;
  default: flags = 0; /* unreachable */
  }
  int ret = uv_pipe_chmod(handle, flags);
  return luv_result(L, ret);
}
#endif

#if LUV_UV_VERSION_GEQ(1,41,0)
static int luv_pipe(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  int read_flags = 0, write_flags = 0;
  uv_file fds[2];
  int ret;
  if (lua_type(L, 1) == LUA_TTABLE) {
    lua_getfield(L, 1, "nonblock");
    if (lua_toboolean(L, -1)) read_flags |= UV_NONBLOCK_PIPE;
    lua_pop(L, 1);
  } else if (!lua_isnoneornil(L, 1)) {
    luv_arg_type_error(L, 1, "table or nil expected, got %s");
  }
  if (lua_type(L, 2) == LUA_TTABLE) {
    lua_getfield(L, 2, "nonblock");
    if (lua_toboolean(L, -1)) write_flags |= UV_NONBLOCK_PIPE;
    lua_pop(L, 1);
  } else if (!lua_isnoneornil(L, 2)) {
    luv_arg_type_error(L, 2, "table or nil expected, got %s");
  }
  ret = uv_pipe(fds, read_flags, write_flags);
  if (ret < 0) return luv_error(L, ret);
  // return as a table with 'read' and 'write' keys containing the corresponding fds
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, fds[0]);
  lua_setfield(L, -2, "read");
  lua_pushinteger(L, fds[1]);
  lua_setfield(L, -2, "write");
  return 1;
}
#endif