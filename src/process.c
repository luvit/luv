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

static int luv_disable_stdio_inheritance(__attribute__((unused)) lua_State* L) {
  uv_disable_stdio_inheritance();
  return 0;
}

static void exit_cb(uv_process_t* handle, int64_t exit_status, int term_signal) {
  lua_State* L = (lua_State*)handle->data;
  luv_find_process(L, handle);
  lua_pushinteger(L, exit_status);
  lua_pushinteger(L, term_signal);
  luv_emit_event(L, "onexit", 3);
}

static int luv_spawn(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_process_t* handle = luv_create_process(L);
  uv_process_options_t options;
  size_t i, len = 0;
  int ret;

  memset(&options, 0, sizeof(options));
  options.exit_cb = exit_cb;
  options.file = luaL_checkstring(L, 2);;
  options.flags = 0;

  // Make sure the 3rd argument is a table
  luaL_checktype(L, 3, LUA_TTABLE);

  // get the args list
  lua_getfield(L, 3, "args");
  // +1 for inserted command at front
  if (lua_type(L, -1) == LUA_TTABLE) {
    len = 1 + lua_rawlen(L, -1);
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luaL_argerror(L, 3, "args option must be table");
  }
  else {
    len = 1;
  }
  // +1 for null terminator at end
  options.args = malloc((len + 1) * sizeof(*options.args));
  options.args[0] = (char*)options.file;
  for (i = 1; i < len; ++i) {
    lua_rawgeti(L, -1, i);
    options.args[i] = (char*)lua_tostring(L, -1);
    lua_pop(L, 1);
  }
  options.args[len] = NULL;
  lua_pop(L, 1);

  // get the stdio list
  lua_getfield(L, 3, "stdio");
  if (lua_type(L, -1) == LUA_TTABLE) {
    options.stdio_count = len = lua_rawlen(L, -1);
    options.stdio = malloc(len * sizeof(*options.stdio));
    for (i = 0; i < len; ++i) {
      lua_rawgeti(L, -1, i + 1);
      // integers are assumed to be file descripters
      if (lua_type(L, -1) == LUA_TNUMBER) {
        options.stdio[i].flags = UV_INHERIT_FD;
        options.stdio[i].data.fd = lua_tointeger(L, -1);
      }
      // userdata is assumed to be a uv_stream_t instance
      else if (lua_type(L, -1) == LUA_TUSERDATA) {
        int fd;
        uv_stream_t* stream = luv_check_stream(L, -1);
        int err = uv_fileno((uv_handle_t*)stream, &fd);
        if (err == UV_EINVAL || err == UV_EBADF) {
          options.stdio[i].flags = UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE;
        }
        else {
          options.stdio[i].flags = UV_INHERIT_STREAM;
        }
        options.stdio[i].data.stream = stream;
      }
      else if (lua_type(L, -1) == LUA_TNIL) {
        options.stdio[i].flags = UV_IGNORE;
      }
      else {
        luaL_argerror(L, 3, "stdio table entries must be nil, uv_stream_t, or integer");
      }
      lua_pop(L, 1);
    }
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luaL_argerror(L, 3, "stdio option must be table");
  }
  lua_pop(L, 1);

  // Get the env
  lua_getfield(L, 3, "env");
  if (lua_type(L, -1) == LUA_TTABLE) {
    len = lua_rawlen(L, -1);
    options.env = malloc((len + 1) * sizeof(*options.env));
    for (i = 0; i < len; ++i) {
      lua_rawgeti(L, -1, i + 1);
      options.env[i] = (char*)lua_tostring(L, -1);
      lua_pop(L, 1);
    }
    options.env[len] = NULL;
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luaL_argerror(L, 3, "env option must be table");
  }
  lua_pop(L, 1);

  // Get the cwd
  lua_getfield(L, 3, "cwd");
  if (lua_type(L, 01) == LUA_TSTRING) {
    options.cwd = (char*)lua_tostring(L, -1);
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luaL_argerror(L, 3, "cwd option must be string");
  }
  lua_pop(L, 1);

  // Check for uid
  lua_getfield(L, 3, "uid");
  if (lua_type(L, -1) == LUA_TNUMBER) {
    options.uid = lua_tointeger(L, -1);
    options.flags |= UV_PROCESS_SETUID;
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luaL_argerror(L, 3, "uid option must be number");
  }
  lua_pop(L, 1);

  // Check for gid
  lua_getfield(L, 3, "gid");
  if (lua_type(L, -1) == LUA_TNUMBER) {
    options.gid = lua_tointeger(L, -1);
    options.flags |= UV_PROCESS_SETGID;
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luaL_argerror(L, 3, "gid option must be number");
  }
  lua_pop(L, 1);

  // Check for the boolean flags
  lua_getfield(L, 3, "verbatim");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS;
  }
  lua_pop(L, 1);
  lua_getfield(L, 3, "detached");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_DETACHED;
  }
  lua_pop(L, 1);
  lua_getfield(L, 3, "hide");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_WINDOWS_HIDE;
  }
  lua_pop(L, 1);

  handle->data = L;
  ret = uv_spawn(loop, handle, &options);
  free(options.args);
  free(options.stdio);
  free(options.env);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, handle->pid);
  return 2;
}
