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

static int new_timer(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_timer_t handle = lua_newuserdata(L, sizeof(handle));
  luaL_getmetatable(L, "uv_handle");
  lua_setmetatable(L, -2);
  int ret = uv_timer_init(loop, handle);
  if (ret < 0) return luv_error(L, ret);
  return 1;
}

// static void on_timeout(uv_timer_t* handle, int status) {
//   lua_State* L = luv_prepare_event(handle->data);
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L) - 1;
// #endif
//   if (luv_get_callback(L, "ontimeout")) {
//     luv_call(L, 1, 0);
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
// }
//
// static int luv_timer_start(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_timer_t* handle = luv_get_timer(L, 1);
//   int64_t timeout = luaL_checkinteger(L, 2);
//   int64_t repeat = luaL_checkinteger(L, 3);
//   if (uv_timer_start(handle, on_timeout, timeout, repeat)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "timer_start: %s", uv_strerror(err));
//   }
//   luv_handle_ref(L, handle->data, 1);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
// static int luv_timer_stop(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_timer_t* handle = luv_get_timer(L, 1);
//   luv_handle_unref(L, handle->data);
//   if (uv_timer_stop(handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "timer_stop: %s", uv_strerror(err));
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
// static int luv_timer_again(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_timer_t* handle = luv_get_timer(L, 1);
//   if (uv_timer_again(handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "timer_again: %s", uv_strerror(err));
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
// static int luv_timer_set_repeat(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_timer_t* handle = luv_get_timer(L, 1);
//   int64_t repeat = luaL_checkint(L, 2);
//   uv_timer_set_repeat(handle, repeat);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
// static int luv_timer_get_repeat(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_timer_t* handle = luv_get_timer(L, 1);
//   lua_pushinteger(L, uv_timer_get_repeat(handle));
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top + 1);
// #endif
//   return 1;
// }
