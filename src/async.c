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

typedef struct {
  luv_thread_arg_t targ;
  uv_mutex_t mutex; // protect thread arg modifications
  lua_State* L; // sender Lua state or NULL when nothing to send
} luv_async_arg_t;

static uv_async_t* luv_check_async(lua_State* L, int index) {
  uv_async_t* handle = (uv_async_t*)luv_checkudata(L, index, "uv_async");
  luaL_argcheck(L, handle->type == UV_ASYNC && handle->data, index, "Expected uv_async_t");
  return handle;
}

static void luv_async_cb(uv_async_t* handle) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  luv_async_arg_t* asarg = (luv_async_arg_t *)((luv_handle_t*) handle->data)->extra;
  uv_mutex_t *argmutex = &asarg->mutex;
  int n = -1;
  uv_mutex_lock(argmutex);
  if (asarg->L) {
    n = luv_thread_arg_push(L, &asarg->targ, LUVF_THREAD_SIDE_MAIN);
    luv_thread_arg_clear(L, &asarg->targ, LUVF_THREAD_SIDE_MAIN);
    asarg->L = NULL;
  }
  uv_mutex_unlock(argmutex);
  if (n >= 0) {
    luv_call_callback(L, data, LUV_ASYNC, n);
  }
}

static int luv_new_async(lua_State* L) {
  uv_async_t* handle;
  luv_handle_t* data;
  luv_ref_t *ref;
  int ret;
  luv_ctx_t* ctx = luv_context(L);
  luaL_checktype(L, 1, LUA_TFUNCTION);
  ref = (luv_ref_t*)luv_newuserdata(L, sizeof(luv_ref_t));
  handle = &ref->handle.async;
  ret = uv_mutex_init(&ref->mutex);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  ret = uv_async_init(ctx->loop, handle, luv_async_cb);
  if (ret < 0) {
    uv_mutex_destroy(&ref->mutex);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  ref->count = 1;
  data = luv_setup_handle(L, ctx);
  luv_async_arg_t* asarg = (luv_async_arg_t*)malloc(sizeof(luv_async_arg_t));
  data->extra = asarg;
  data->extra_gc = free;
  memset(asarg, 0, sizeof(luv_async_arg_t));
  ret = uv_mutex_init(&asarg->mutex);
  if (ret < 0) { // unlikely
    abort();
  }
  handle->data = data;
  luv_check_callback(L, (luv_handle_t*)handle->data, LUV_ASYNC, 1);
  return 1;
}

static int luv_handle_gc(lua_State* L);

static int luv_async_gc(lua_State* L) {
  luv_ref_t* ref = *(luv_ref_t**)lua_touserdata(L, 1);
  uv_mutex_t *refmutex = &ref->mutex;
  luv_handle_t* data = (luv_handle_t*)ref->handle.async.data;
  luv_async_arg_t* asarg = (luv_async_arg_t *)data->extra;
  uv_mutex_t *argmutex = &asarg->mutex;
  int count;
  // clear arg if we are the sender
  uv_mutex_lock(argmutex);
  if (asarg->L == L) {
    luv_thread_arg_clear(L, &asarg->targ, LUVF_THREAD_SIDE_CHILD);
    asarg->L = NULL;
  }
  uv_mutex_unlock(argmutex);
  // decrease reference count
  uv_mutex_lock(refmutex);
  ref->count--;
  count = ref->count;
  uv_mutex_unlock(refmutex);
  if (count > 0) {
    return 0;
  }
  uv_mutex_destroy(argmutex);
  uv_mutex_destroy(refmutex);
  return luv_handle_gc(L);
}

static int luv_async_send(lua_State* L) {
  int ret;
  uv_async_t* handle = luv_check_async(L, 1);
  luv_async_arg_t* asarg = (luv_async_arg_t *)((luv_handle_t*) handle->data)->extra;
  uv_mutex_t *argmutex = &asarg->mutex;
  int n;
  uv_mutex_lock(argmutex);
  // clear pending arg if any and if owner
  if (asarg->L) {
    if (asarg->L != L) {
      uv_mutex_unlock(argmutex);
      return luaL_error(L, "Unable to clear pending send not owned by the current Lua state");
    }
    luv_thread_arg_clear(L, &asarg->targ, LUVF_THREAD_SIDE_CHILD);
    asarg->L = NULL;
  }
  n = luv_thread_arg_set(L, &asarg->targ, 2, lua_gettop(L), LUVF_THREAD_MODE_ASYNC|LUVF_THREAD_SIDE_CHILD);
  if (n >= 0) {
    asarg->L = L;
  }
  uv_mutex_unlock(argmutex);
  if (n < 0) {
    return luv_thread_arg_error(L);
  }
  ret = uv_async_send(handle);
  // clear on gc
  return luv_result(L, ret);
}

static void luv_async_init(lua_State* L) {
  luaL_getmetatable(L, "uv_async");
  lua_pushcfunction(L, luv_async_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);
}
