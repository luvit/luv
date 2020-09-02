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
#include <math.h>

static int luv_disable_stdio_inheritance(lua_State* L) {
  (void)L;
  uv_disable_stdio_inheritance();
  return 0;
}

static uv_process_t* luv_check_process(lua_State* L, int index) {
  uv_process_t* handle = (uv_process_t*)luv_checkudata(L, index, "uv_process");
  luaL_argcheck(L, handle->type == UV_PROCESS && handle->data, index, "Expected uv_process_t");
  return handle;
}

static void exit_cb(uv_process_t* handle, int64_t exit_status, int term_signal) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  lua_pushinteger(L, exit_status);
  lua_pushinteger(L, term_signal);
  luv_call_callback(L, data, LUV_EXIT, 2);
}

static void luv_spawn_close_cb(uv_handle_t* handle) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  luv_unref_handle(L, (luv_handle_t*)handle->data);
}

static void luv_clean_options(lua_State* L, uv_process_options_t* options, int* args_refs) {
  free(options->args);
  free(options->stdio);
  free(options->env);
  if (args_refs) {
    int i;
    for (i = 0; args_refs[i] != LUA_NOREF; i++) {
      luaL_unref(L, LUA_REGISTRYINDEX, args_refs[i]);
    }
    free(args_refs);
  }
}

// iterates over the tbl to find the max integer
// key of the table that is >= 1. If any key is
// NOT an integer, simply call lua_rawlen().
static int sparse_rawlen(lua_State* L, int tbl) {
  int len = 0;
  tbl = lua_absindex(L, tbl);

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    if (lua_type(L, -2) == LUA_TNUMBER) {
      int idx = lua_tonumber(L, -2);
      if (floor(idx) == idx && idx >= 1) {
        if (idx > len) len = idx;
        lua_pop(L, 1);
        continue;
      }
    }
    lua_pop(L, 2);
    return lua_rawlen(L, tbl);
  }
  return len;
}

static int luv_spawn(lua_State* L) {
  uv_process_t* handle;
  uv_process_options_t options;
  int* args_refs = NULL;
  size_t i, len = 0;
  int ret;
  luv_ctx_t* ctx = luv_context(L);

  memset(&options, 0, sizeof(options));
  options.exit_cb = exit_cb;
  options.file = luaL_checkstring(L, 1);
  options.flags = 0;

  // Make sure the 2nd argument is a table
  luaL_checktype(L, 2, LUA_TTABLE);

  // get the args list
  lua_getfield(L, 2, "args");
  // +1 for inserted command at front
  if (lua_type(L, -1) == LUA_TTABLE) {
    len = 1 + lua_rawlen(L, -1);
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luv_clean_options(L, &options, args_refs);
    return luaL_argerror(L, 3, "args option must be table");
  }
  else {
    len = 1;
  }
  // +1 for null terminator at end
  options.args = (char**)malloc((len + 1) * sizeof(*options.args));

  // args must be referenced to ensure that they don't get garbage
  // collected between now and when they are used in uv_spawn.
  // However, we don't need to ref args[0] since we don't pop that
  // from the stack. Note: args_refs is a LUA_NOREF-terminated array
  // when it is non-NULL 
  if (len > 1) {
    args_refs = (int*)malloc(len * sizeof(int));
    if (args_refs)
      args_refs[len-1] = LUA_NOREF;
  }

  if (!options.args || (len > 1 && !args_refs)) {
    luv_clean_options(L, &options, args_refs);
    return luaL_error(L, "Problem allocating args");
  }
  options.args[0] = (char*)options.file;
  for (i = 1; i < len; ++i) {
    lua_rawgeti(L, -1, i);
    options.args[i] = (char*)lua_tostring(L, -1);
    args_refs[i - 1] = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  options.args[len] = NULL;
  lua_pop(L, 1); // pop the args field (either table or nil)

  // get the stdio list
  lua_getfield(L, 2, "stdio");
  if (lua_type(L, -1) == LUA_TTABLE) {
    options.stdio_count = len = sparse_rawlen(L, -1);
    options.stdio = (uv_stdio_container_t*)malloc(len * sizeof(*options.stdio));
    if (!options.stdio) {
      luv_clean_options(L, &options, args_refs);
      return luaL_error(L, "Problem allocating stdio");
    }
    for (i = 0; i < len; ++i) {
      lua_rawgeti(L, -1, i + 1);
      // integers are assumed to be file descripters
      if (lua_type(L, -1) == LUA_TNUMBER) {
        options.stdio[i].flags = UV_INHERIT_FD;
        options.stdio[i].data.fd = lua_tointeger(L, -1);
      }
      // userdata is assumed to be a uv_stream_t instance
      else if (lua_type(L, -1) == LUA_TUSERDATA) {
        uv_os_fd_t fd;
        uv_stream_t* stream = luv_check_stream(L, -1);
        int err = uv_fileno((uv_handle_t*)stream, &fd);
        if (err == UV_EINVAL || err == UV_EBADF) {
          // stdin (fd 0) is read-only, stdout and stderr (fds 1 & 2) are
          // write-only, and all fds > 2 are read-write
          int flags = UV_CREATE_PIPE;
          if (i == 0 || i > 2)
            flags |= UV_READABLE_PIPE;
          if (i != 0)
            flags |= UV_WRITABLE_PIPE;
          options.stdio[i].flags = (uv_stdio_flags)flags;
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
        luv_clean_options(L, &options, args_refs);
        return luaL_argerror(L, 2, "stdio table entries must be nil, uv_stream_t, or integer");
      }
      lua_pop(L, 1);
    }
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luv_clean_options(L, &options, args_refs);
    return luaL_argerror(L, 2, "stdio option must be table");
  }
  lua_pop(L, 1);

  // Get the env
  lua_getfield(L, 2, "env");
  if (lua_type(L, -1) == LUA_TTABLE) {
    len = lua_rawlen(L, -1);
    options.env = (char**)malloc((len + 1) * sizeof(*options.env));
    if (!options.env) {
      luv_clean_options(L, &options, args_refs);
      return luaL_error(L, "Problem allocating env");
    }
    for (i = 0; i < len; ++i) {
      lua_rawgeti(L, -1, i + 1);
      options.env[i] = (char*)lua_tostring(L, -1);
      lua_pop(L, 1);
    }
    options.env[len] = NULL;
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luv_clean_options(L, &options, args_refs);
    return luaL_argerror(L, 2, "env option must be table");
  }
  lua_pop(L, 1);

  // Get the cwd
  lua_getfield(L, 2, "cwd");
  if (lua_type(L, -1) == LUA_TSTRING) {
    options.cwd = (char*)lua_tostring(L, -1);
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luv_clean_options(L, &options, args_refs);
    return luaL_argerror(L, 2, "cwd option must be string");
  }
  lua_pop(L, 1);

  // Check for uid
  lua_getfield(L, 2, "uid");
  if (lua_type(L, -1) == LUA_TNUMBER) {
    options.uid = lua_tointeger(L, -1);
    options.flags |= UV_PROCESS_SETUID;
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luv_clean_options(L, &options, args_refs);
    return luaL_argerror(L, 2, "uid option must be number");
  }
  lua_pop(L, 1);

  // Check for gid
  lua_getfield(L, 2, "gid");
  if (lua_type(L, -1) == LUA_TNUMBER) {
    options.gid = lua_tointeger(L, -1);
    options.flags |= UV_PROCESS_SETGID;
  }
  else if (lua_type(L, -1) != LUA_TNIL) {
    luv_clean_options(L, &options, args_refs);
    return luaL_argerror(L, 2, "gid option must be number");
  }
  lua_pop(L, 1);

  // Check for the boolean flags
  lua_getfield(L, 2, "verbatim");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS;
  }
  lua_pop(L, 1);
  lua_getfield(L, 2, "detached");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_DETACHED;
  }
  lua_pop(L, 1);
  lua_getfield(L, 2, "hide");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_WINDOWS_HIDE;
  }
  lua_pop(L, 1);
#if LUV_UV_VERSION_GEQ(1, 24, 0)
  lua_getfield(L, 2, "hide_console");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_WINDOWS_HIDE_CONSOLE;
  }
  lua_pop(L, 1);
  lua_getfield(L, 2, "hide_gui");
  if (lua_toboolean(L, -1)) {
    options.flags |= UV_PROCESS_WINDOWS_HIDE_GUI;
  }
  lua_pop(L, 1);
#endif

  // this will fill the 3rd argument with nil if it doesn't exist so that
  // the uv_process_t userdata doesn't get treated as the 3rd argument
  lua_settop(L, 3);

  handle = (uv_process_t*)luv_newuserdata(L, sizeof(*handle));
  handle->type = UV_PROCESS;
  handle->data = luv_setup_handle(L, ctx);

  if (!lua_isnoneornil(L, 3)) {
    luv_check_callback(L, (luv_handle_t*)handle->data, LUV_EXIT, 3);
  }

  ret = uv_spawn(ctx->loop, handle, &options);

  luv_clean_options(L, &options, args_refs);
  if (ret < 0) {
    /* The async callback is required here because luajit GC may reclaim the
     * luv handle before libuv is done closing it down.
     */
    uv_close((uv_handle_t*)handle, luv_spawn_close_cb);
    return luv_error(L, ret);
  }
  lua_pushinteger(L, handle->pid);
  return 2;
}

static int luv_parse_signal(lua_State* L, int slot) {
  if (lua_isnumber(L, slot)) {
    return lua_tonumber(L, slot);
  }
  if (lua_isstring(L, slot)) {
    return luv_sig_string_to_num(lua_tostring(L, slot));
  }
  return SIGTERM;
}

static int luv_process_kill(lua_State* L) {
  uv_process_t* handle = luv_check_process(L, 1);
  int signum = luv_parse_signal(L, 2);
  int ret = uv_process_kill(handle, signum);
  return luv_result(L, ret);
}

static int luv_kill(lua_State* L) {
  int pid = luaL_checkinteger(L, 1);
  int signum = luv_parse_signal(L, 2);
  int ret = uv_kill(pid, signum);
  return luv_result(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 19, 0)
static int luv_process_get_pid(lua_State* L) {
  uv_process_t* handle = luv_check_process(L, 1);
  lua_pushinteger(L, uv_process_get_pid(handle));
  return 1;
}
#endif
