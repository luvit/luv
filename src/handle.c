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

static void on_close(uv_handle_t* handle) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (luv_get_callback(L, "onclose")) {
    luv_call(L, 1, 0);
  }
  luv_handle_unref(L, handle->data);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static int luv_close(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_handle_t* handle = luv_get_handle(L, 1);

  if (uv_is_closing(handle)) {
    fprintf(stderr, "WARNING: Handle already closing \tlhandle=%p handle=%p\n", handle->data, (void*)handle);
    return 0;
  }

  uv_close(handle, on_close);
  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_is_active(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_handle_t* handle = luv_get_handle(L, 1);
  lua_pushboolean(L, uv_is_active(handle));
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}

static void on_walk(uv_handle_t* handle, void* arg) {
  luv_callback_t* callback = (luv_callback_t*)arg;
  lua_State* L = callback->L;
  luv_handle_t* lhandle;
  /* Get the callback and push the type */
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->ref);
  lhandle = (luv_handle_t*)handle->data;
  assert(L == lhandle->L); /* Make sure the lua states match */
  lua_rawgeti(L, LUA_REGISTRYINDEX, lhandle->ref);

  switch (lhandle->handle->type) {
#define XX(uc, lc) case UV_##uc: lua_pushfstring(L, "uv_%s_t: %p", #lc, lhandle->handle); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default: lua_pushfstring(L, "userdata: %p", lhandle->handle); break;
  }

  lua_call(L, 2, 0);
}

static int luv_walk(lua_State* L) {
  luv_callback_t callback;
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  luaL_checktype(L, 1, LUA_TFUNCTION);
  /* Store the callback as a ref */
  callback.L = L;
  lua_pushvalue(L, 1);
  callback.ref = luaL_ref(L, LUA_REGISTRYINDEX);

  /* Walk the list */
  uv_walk(uv_default_loop(), on_walk, &callback);

  /* unref the callback */
  luaL_unref(L, LUA_REGISTRYINDEX, callback.ref);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_ref(lua_State* L) {
  uv_ref(luv_get_handle(L, 1));
  return 0;
}

static int luv_unref(lua_State* L) {
  uv_unref(luv_get_handle(L, 1));
  return 0;
}

static int luv_is_closing(lua_State* L) {
  lua_pushboolean(L, uv_is_closing(luv_get_handle(L, 1)));
  return 1;
}
