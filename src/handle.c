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



static int luv_newindex(lua_State* L) {
  lua_getuservalue(L, 1);
  lua_pushvalue(L, 2);
  lua_pushvalue(L, 3);
  lua_rawset(L, -3);
  lua_pop(L, 1);
  return 0;
}

static int luv_index(lua_State* L) {

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

static int luv_tostring(lua_State* L) {
  uv_handle_t* handle = luaL_checkudata(L, 1, "uv_handle");
  switch (handle->type) {
#define XX(uc, lc) case UV_##uc: lua_pushfstring(L, "uv_"#lc"_t: %p", handle); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default: lua_pushfstring(L, "userdata: %p", handle); break;
  }
  return 1;
}

static void handle_init(lua_State* L) {
  luaL_newmetatable (L, "uv_handle");
  lua_pushcfunction(L, luv_newindex);
  lua_setfield(L, -2, "__newindex");
  lua_pushcfunction(L, luv_index);
  lua_setfield(L, -2, "__index");
  lua_pushcfunction(L, luv_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pop(L, 1);
}

static uv_handle_t* luv_check_handle(lua_State* L, int index) {
  return luaL_checkudata(L, index, "uv_handle");
}

// static uv_stream_t* luv_check_stream(lua_State* L, int index) {
//   uv_stream_t* handle = luaL_checkudata(L, index, "uv_handle");
//   luaL_argcheck(L,
//     handle->type == UV_TCP ||
//     handle->type == UV_TTY ||
//     handle->type == UV_NAMED_PIPE ||
//     handle->type == UV_UDP, index, "uv_stream_t subclass required");
//   return handle;
// }

// static uv_tcp_t* luv_check_tcp(lua_State* L, int index) {
//   uv_tcp_t* handle = luaL_checkudata(L, index, "uv_handle");
//   luaL_argcheck(L, handle->type == UV_TCP, index, "uv_tcp_t required");
//   return handle;
// }
//
// static uv_tty_t* luv_check_tty(lua_State* L, int index) {
//   uv_tty_t* handle = luaL_checkudata(L, index, "uv_handle");
//   luaL_argcheck(L, handle->type == UV_TTY, index, "uv_tty_t required");
//   return handle;
// }
//
// static uv_pipe_t* luv_check_pipe(lua_State* L, int index) {
//   uv_pipe_t* handle = luaL_checkudata(L, index, "uv_handle");
//   luaL_argcheck(L, handle->type == UV_NAMED_PIPE, index, "uv_pipe_t required");
//   return handle;
// }
//
// static uv_udp_t* luv_check_udp(lua_State* L, int index) {
//   uv_udp_t* handle = luaL_checkudata(L, index, "uv_handle");
//   luaL_argcheck(L, handle->type == UV_UDP, index, "uv_udp_t required");
//   return handle;
// }
//
// static uv_timer_t* luv_check_timer(lua_State* L, int index) {
//   uv_timer_t* handle = luaL_checkudata(L, index, "uv_handle");
//   luaL_argcheck(L, handle->type == UV_TIMER, index, "uv_timer_t required");
//   return handle;
// }
//
// static uv_process_t* luv_check_process(lua_State* L, int index) {
//   uv_process_t* handle = luaL_checkudata(L, index, "uv_handle");
//   luaL_argcheck(L, handle->type == UV_PROCESS, index, "uv_process_t required");
//   return handle;
// }

static int luv_is_active(lua_State* L) {
  uv_handle_t* handle = luv_check_handle(L, 1);
  int ret = uv_is_active(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int new_timer(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_timer_t* handle = lua_newuserdata(L, sizeof(*handle));
  setup_udata(L, handle, "uv_handle");
  int ret = uv_timer_init(loop, handle);
  if (ret < 0) return luv_error(L, ret);
  return 1;
}


//   luv_handle_t* lhandle = (luv_handle_t*)lua_newuserdata(L, sizeof(luv_handle_t));
//
//   /* Create a local environment for storing stuff */
//   lua_newtable(L);
//   lua_setuservalue(L, -2);
//
//   /* Initialize and return the lhandle */
//   lhandle->handle = (uv_handle_t*)malloc(size);
//   lhandle->handle->data = lhandle; /* Point back to lhandle from handle */
//   lhandle->refCount = 0;
//   lhandle->ref = LUA_NOREF;
//   lhandle->mask = mask;
//   lhandle->L = L;


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
