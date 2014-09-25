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

// static int luv_pipe_open(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_pipe_t* handle = luv_get_pipe(L, 1);
//   uv_file file = luaL_checkint(L, 2);
//   if (uv_pipe_open(handle, file)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "pipe_open: %s", uv_strerror(err));
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }

// static int luv_pipe_bind(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_pipe_t* handle = luv_get_pipe(L, 1);
//   const char* name = luaL_checkstring(L, 2);
//   if (uv_pipe_bind(handle, name)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "pipe_name: %s", uv_strerror(err));
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }

// static int luv_pipe_connect(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_pipe_t* handle = luv_get_pipe(L, 1);
//   const char* name = luaL_checkstring(L, 2);

//   uv_connect_t* req = malloc(sizeof(*req));
//   luv_req_t* lreq = malloc(sizeof(*lreq));
//   req->data = (void*)lreq;
//   lreq->lhandle = handle->data;

//   uv_pipe_connect(req, handle, name, luv_after_connect);

//   lreq->data_ref = LUA_NOREF;
//   lua_pushvalue(L, 3);
//   lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

//   luv_handle_ref(L, handle->data, 1);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
