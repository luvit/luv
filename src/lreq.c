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

static int luv_check_continuation(lua_State* L, int index) {
#ifdef LUV_FORCE_COROUTINE_CONTINUATION
  // Uses the current thread unless it is the main thread
  if (lua_isnoneornil(L, index)) {
    int ismain = lua_pushthread(L);
    if (ismain) {
      lua_pop(L, 1);
      return LUA_NOREF;
    }
    return luaL_ref(L, LUA_REGISTRYINDEX);
  }
#else
  if (lua_isnoneornil(L, index))
    return LUA_NOREF;
#endif

  lua_State *thread = lua_tothread(L, index);
  if (thread) {
    if (lua_status(thread) != LUA_OK && lua_status(thread) != LUA_YIELD)
      luaL_argerror(L, index, "expected non-suspended coroutine");
  } else
    luv_check_callable(L, index);
  lua_pushvalue(L, index);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}

// Store a lua callback in a luv_req for the continuation.
// The uv_req_t is assumed to be at the top of the stack
static luv_req_t* luv_setup_req_with_mt(lua_State* L, luv_ctx_t* ctx, int cb_ref, const char* mt_name) {
  luv_req_t* data;

  luaL_checktype(L, -1, LUA_TUSERDATA);

  data = (luv_req_t*)malloc(sizeof(*data));
  if (!data) luaL_error(L, "Problem allocating luv request");

  luaL_getmetatable(L, mt_name);
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  data->req_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  data->callback_ref = cb_ref;
  data->data_ref = LUA_NOREF;
  data->ctx = ctx;
  data->data = NULL;

  return data;
}

static luv_req_t* luv_setup_req(lua_State* L, luv_ctx_t* ctx, int cb_ref) {
  return luv_setup_req_with_mt(L, ctx, cb_ref, "uv_req");
}


static void luv_fulfill_req(lua_State* L, luv_req_t* data, int nargs) {
  if (data->callback_ref == LUA_NOREF) {
    lua_pop(L, nargs);
  }
  else {
    // Get the callback
    lua_rawgeti(L, LUA_REGISTRYINDEX, data->callback_ref);
    lua_State *thread = lua_tothread(L, -1);
    
    if (thread) {
      // We need to resume the thread
      lua_pop(L, 1);

      if (lua_status(thread) != LUA_OK && lua_status(thread) != LUA_YIELD)
        luaL_error(L, "cannot resume suspended coroutine");

      lua_xmove(L, thread, nargs);

#if LUA_VERSION_NUM >= 504
      int nres;
      int status = lua_resume(thread, NULL, nargs, &nres);
#else
      int status = lua_resume(thread, NULL, nargs);
#endif
      if (status == LUA_OK || status == LUA_YIELD) {
        // We've handled control back to lua, hopefully we don't need to do anything here.
        lua_pop(L, lua_gettop(L));
      } else {
#if LUA_VERSION_NUM >= 504
        status = lua_resetthread(thread);  /* close its tbc variables */
        lua_assert(status != LUA_OK);
#endif
        const char *msg = lua_tostring(thread, -1);
        luaL_traceback(L, thread, msg, 0);

        lua_error(L);
      }
    } else {
      // insert the callback before the args if there are any.
      if (nargs) {
        lua_insert(L, -1 - nargs);
      }

      data->ctx->cb_pcall(L, nargs, 0, 0);
    }
  }
}

static void luv_fulfill_req_status(lua_State *L, luv_req_t *data, int status) {
  int is_thread = luv_is_sync_req(L, data);

  if (is_thread) {
    int nargs = luv_result(L, status);
    luv_fulfill_req(L, data, nargs);
  } else {
    luv_status(L, status);
    luv_fulfill_req(L, data, 1);
  }
}

static void luv_cleanup_req(lua_State* L, luv_req_t* data) {
  int i;
  luaL_unref(L, LUA_REGISTRYINDEX, data->req_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callback_ref);
  if (data->data_ref == LUV_REQ_MULTIREF) {
    for (i = 0; ((int*)(data->data))[i] != LUA_NOREF; i++) {
      luaL_unref(L, LUA_REGISTRYINDEX, ((int*)(data->data))[i]);
    }
  }
  else
    luaL_unref(L, LUA_REGISTRYINDEX, data->data_ref);
  free(data->data);
  free(data);
}

static int luv_is_sync_req(lua_State *L, luv_req_t *data) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, (data)->callback_ref);
  int isth = lua_isthread(L, -1);
  lua_pop(L, 1);

  return isth;
}