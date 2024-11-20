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

typedef struct luv_async_send_s {
  luv_thread_arg_t targ;
  struct luv_async_send_s* next;
} luv_async_send_t;

typedef struct {
  luv_thread_arg_t targ;
  uv_mutex_t mutex;
  int max; // FIFO queue in case of max > 1
  int count;
  luv_async_send_t* first;
  luv_async_send_t* last;
} luv_async_arg_t;

#define luv_get_async_arg_from_handle(H) ((luv_async_arg_t *) ((luv_handle_t*) (H)->data)->extra)

static uv_async_t* luv_check_async(lua_State* L, int index) {
  uv_async_t* handle = (uv_async_t*)luv_checkudata(L, index, "uv_async");
  luaL_argcheck(L, handle->type == UV_ASYNC && handle->data, index, "Expected uv_async_t");
  return handle;
}

#define luv_is_async_queue(AA) ((AA)->max > 1)

static luv_async_send_t* luv_async_pop(luv_async_arg_t* asarg) {
  luv_async_send_t* sendarg = asarg->first;
  if (sendarg != NULL) {
    asarg->count--;
    asarg->first = sendarg->next;
    if (asarg->first == NULL) {
      asarg->last = NULL;
    }
  }
  return sendarg;
}

static luv_async_send_t* luv_async_push(luv_async_arg_t* asarg) {
  luv_async_send_t* sendarg = (luv_async_send_t*)malloc(sizeof(luv_async_send_t));
  memset(sendarg, 0, sizeof(luv_async_send_t));
  asarg->count++;
  if (asarg->last != NULL) {
    asarg->last->next = sendarg;
  }
  asarg->last = sendarg;
  if (asarg->first == NULL) {
    asarg->first = sendarg;
  }
  return sendarg;
}

static void luv_async_cb(uv_async_t* handle) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  luv_async_arg_t* asarg = luv_get_async_arg_from_handle(handle);
  uv_mutex_t *argmutex = &asarg->mutex;
  luv_thread_arg_t targcpy; // work on a copy of the arguments
  int n;
  int q = luv_is_async_queue(asarg);
  do {
    uv_mutex_lock(argmutex);
    if (q) {
      luv_async_send_t* sendarg = luv_async_pop(asarg);
      if (sendarg == NULL) {
        uv_mutex_unlock(argmutex);
        return;
      }
      targcpy = sendarg->targ;
      free(sendarg);
    } else {
      targcpy = asarg->targ;
      asarg->targ.argc = 0; // empty the shared original, nothing to clear
    }
    uv_mutex_unlock(argmutex);
    n = luv_thread_arg_push(L, &targcpy, LUVF_THREAD_SIDE_MAIN);
    if (n >= 0) {
      luv_call_callback(L, data, LUV_ASYNC, n);
    }
    luv_thread_arg_clear(L, &targcpy, LUVF_THREAD_SIDE_MAIN); // clear the copy
  } while (q);
}

static int luv_new_async(lua_State* L) {
  uv_async_t* handle;
  luv_handle_t* data;
  int ret;
  luv_ctx_t* ctx = luv_context(L);
  int max = luaL_optinteger(L, 2, 0);
  luaL_checktype(L, 1, LUA_TFUNCTION);
  handle = (uv_async_t*)luv_newuserdata(L, uv_handle_size(UV_ASYNC));
  ret = uv_async_init(ctx->loop, handle, luv_async_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  data = luv_setup_handle(L, ctx);
  luv_async_arg_t* asarg = (luv_async_arg_t*)malloc(sizeof(luv_async_arg_t));
  memset(asarg, 0, sizeof(luv_async_arg_t));
  asarg->max = max;
  ret = uv_mutex_init(&asarg->mutex);
  if (ret < 0) { // unlikely
    abort();
  }
  data->extra = asarg;
  data->extra_gc = free;
  handle->data = data;
  luv_check_callback(L, (luv_handle_t*)handle->data, LUV_ASYNC, 1);
  return 1;
}

// From handle.c
static int luv_handle_gc(lua_State* L);
// From thread.c
static void luv_thread_arg_free(luv_thread_arg_t* args);

static int luv_async_gc(lua_State* L) {
  uv_async_t* handle = *(uv_async_t**)lua_touserdata(L, 1);
  luv_async_arg_t* asarg = luv_get_async_arg_from_handle(handle);
  uv_mutex_t *argmutex = &asarg->mutex;
  uv_mutex_lock(argmutex);
  if (luv_is_async_queue(asarg)) {
    luv_async_send_t* sendarg;
    while ((sendarg = luv_async_pop(asarg)) != NULL) {
      luv_thread_arg_free(&sendarg->targ);
      free(sendarg);
    }
  } else {
    luv_thread_arg_free(&asarg->targ); // in case of a pending send
  }
  uv_mutex_unlock(argmutex);
  uv_mutex_destroy(argmutex);
  return luv_handle_gc(L);
}

static int luv_async_send(lua_State* L) {
  int ret;
  uv_async_t* handle = luv_check_async(L, 1);
  luv_async_arg_t* asarg = luv_get_async_arg_from_handle(handle);
  uv_mutex_t *argmutex = &asarg->mutex;
  luv_thread_arg_t* args;
  int n;
  uv_mutex_lock(argmutex);
  if (luv_is_async_queue(asarg)) {
    if (asarg->count >= asarg->max) {
      uv_mutex_unlock(argmutex);
      return luv_error(L, UV_ENOSPC);
    }
    luv_async_send_t* sendarg = luv_async_push(asarg);
    args = &sendarg->targ;
  } else {
    luv_thread_arg_free(&asarg->targ); // in case of a pending send
    args = &asarg->targ;
  }
  n = luv_thread_arg_set(L, args, 2, lua_gettop(L), LUVF_THREAD_MODE_ASYNC|LUVF_THREAD_SIDE_CHILD);
  uv_mutex_unlock(argmutex);
  if (n < 0) {
    return luv_thread_arg_error(L);
  }
  ret = uv_async_send(handle);
  return luv_result(L, ret);
}

static void luv_async_init(lua_State* L) {
  luaL_getmetatable(L, "uv_async");
  lua_pushcfunction(L, luv_async_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);
}
