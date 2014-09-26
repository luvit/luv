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

// Metamethod to allow storing anything in the userdata's environment
static int handle_newindex(lua_State* L) {
  lua_getuservalue(L, 1);
  lua_pushvalue(L, 2);
  lua_pushvalue(L, 3);
  lua_rawset(L, -3);
  lua_pop(L, 1);
  return 0;
}

// Metamethod to allow reading things out of the environment.
// Special read-only "type".
static int handle_index(lua_State* L) {
  /* Get handle type if requested */
  const char* key = lua_tostring(L, 2);
  if (strcmp(key, "type") == 0) {
    uv_handle_t* handle = luaL_checkudata(L, 1, "uv_handle");
    switch (handle->type) {
#define XX(uc, lc) case UV_##uc: lua_pushstring(L, #uc); break;
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
      default: lua_pushstring(L, "UNKNOWN"); break;
    }
    return 1;
  }

  lua_getuservalue(L, 1);
  lua_pushvalue(L, 2);
  lua_rawget(L, -2);
  lua_remove(L, -2);
  return 1;
}

// Show the libuv type instead of generic "userdata"
static int handle_tostring(lua_State* L) {
  luv_handle_t* handle = luaL_checkudata(L, 1, "uv_handle");
  switch (handle->data.type) {
#define XX(uc, lc) case UV_##uc: lua_pushfstring(L, "uv_"#lc"_t: %p", handle); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default: lua_pushfstring(L, "uv_handle_t: %p", handle); break;
  }
  return 1;
}


// Reuse the builtin pairs on the uservalue table.
static int handle_pairs(lua_State* L) {
  lua_getglobal(L, "pairs");
  lua_getuservalue(L, 1);
  lua_call(L, 1, 3);
  return 3;
}


static void handle_init(lua_State* L) {
  luaL_newmetatable (L, "uv_handle");
  lua_pushcfunction(L, handle_newindex);
  lua_setfield(L, -2, "__newindex");
  lua_pushcfunction(L, handle_index);
  lua_setfield(L, -2, "__index");
  lua_pushcfunction(L, handle_pairs);
  lua_setfield(L, -2, "__pairs");
  lua_pushcfunction(L, handle_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pop(L, 1);
}


static int luv_is_active(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  int ret = uv_is_active(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_is_closing(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  int ret = uv_is_closing(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static void close_cb(uv_handle_t* handle) {
  lua_State* L = handle->data;
  luv_unref_handle(L, handle);
  luv_resume(L, 0);
}

static int luv_close(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  handle->data = L;
  uv_close(handle, close_cb);
  return luv_wait(L, 0);
}

static int luv_ref(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  uv_ref(handle);
  return 0;
}

static int luv_unref(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  uv_unref(handle);
  return 0;
}

static int luv_has_ref(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  int ret = uv_has_ref(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_send_buffer_size(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  int value;
  int ret;
  if (lua_isnoneornil(L, 2)) {
    value = 0;
  }
  else {
    value = luaL_checkinteger(L, 2);
  }
  ret = uv_send_buffer_size(handle, &value);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_recv_buffer_size(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  int value;
  int ret;
  if (lua_isnoneornil(L, 2)) {
    value = 0;
  }
  else {
    value = luaL_checkinteger(L, 2);
  }
  ret = uv_recv_buffer_size(handle, &value);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_fileno(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  uv_os_fd_t fd;
  int ret = uv_fileno(handle, &fd);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, fd);
  return 1;
}

// static int new_tcp(lua_State* L) {
//   uv_tcp_t* handle = luv_create_tcp(L);
//   if (uv_tcp_init(uv_default_loop(), handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_tcp: %s", uv_strerror(err));
//   }
//   return 1;
// }
//
// static int new_udp(lua_State* L) {
//   uv_udp_t* handle = luv_create_udp(L);
//   if (uv_udp_init(uv_default_loop(), handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_udp: %s", uv_strerror(err));
//   }
//   return 1;
// }
//
// static int new_timer(lua_State* L) {
//   uv_timer_t* handle = luv_create_timer(L);
//   if (uv_timer_init(uv_default_loop(), handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_timer: %s", uv_strerror(err));
//   }
//   return 1;
// }
//
// static int new_tty(lua_State* L) {
//   uv_tty_t* handle = luv_create_tty(L);
//   uv_file fd = luaL_checkint(L, 1);
//   int readable;
//   luaL_checktype(L, 2, LUA_TBOOLEAN);
//   readable = lua_toboolean(L, 2);
//   if (uv_tty_init(uv_default_loop(), handle, fd, readable)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_tty: %s", uv_strerror(err));
//   }
//   return 1;
// }
