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
  lua_State* L;       /* vm in main */
  char* code;         /* thread entry code */
  size_t len;

  int after_work_cb;  /* ref, run in main ,call after work cb*/
  int pool_ref;       /* ref of lua_State cache array */
} luv_work_ctx_t;

typedef struct {
  uv_work_t work;
  luv_work_ctx_t* ctx;

  luv_thread_arg_t args;
  luv_thread_arg_t rets;
  int ref;            /* ref to luv_work_ctx_t, which create a new uv_work_t*/
} luv_work_t;

static luv_work_ctx_t* luv_check_work_ctx(lua_State* L, int index) {
  luv_work_ctx_t* ctx = (luv_work_ctx_t*)luaL_checkudata(L, index, "luv_work_ctx");
  return ctx;
}

static int luv_work_ctx_gc(lua_State *L) {
  int i, n;
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  free(ctx->code);
  luaL_unref(L, LUA_REGISTRYINDEX, ctx->after_work_cb);

  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->pool_ref);
  n = lua_rawlen(L, -1);
  for (i=1; i<=n; i++) {
    lua_State *S;
    lua_rawgeti(L, -1, i);
    S = *(lua_State**)lua_touserdata(L, -1);
    release_vm_cb(S);
    lua_pop(L, 1);
  }
  luaL_unref(L, LUA_REGISTRYINDEX, ctx->pool_ref);
  return 0;
}

static int luv_work_ctx_tostring(lua_State* L) {
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  lua_pushfstring(L, "luv_work_ctx_t: %p", ctx);
  return 1;
}

static void luv_work_cb(uv_work_t* req) {
  luv_work_t* work = (luv_work_t*)req->data;
  luv_work_ctx_t* ctx = work->ctx;
  lua_State *L = work->args.L;

  int top = lua_gettop(L);

  /* push lua function */
  lua_pushlstring(L, ctx->code, ctx->len);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);

    lua_pushlstring(L, ctx->code, ctx->len);
    if (luaL_loadbuffer(L, ctx->code, ctx->len, "=pool") != 0)
    {
      fprintf(stderr, "Uncaught Error in work callback: %s\n", lua_tostring(L, -1));
      lua_pop(L, 2);

      lua_pushnil(L);
    } else {
      lua_pushvalue(L, -1);
      lua_insert(L, lua_gettop(L) - 2);
      lua_rawset(L, LUA_REGISTRYINDEX);
    }
  }

  if (lua_isfunction(L, -1)) {
    int i = luv_thread_arg_push(L, &work->args, LUVF_THREAD_SIDE_CHILD);
    i = luv_cfpcall(L, i, LUA_MULTRET, 0);
    if ( i>=0 ) {
      //clear in main threads, luv_after_work_cb
      i = luv_thread_arg_set(L, &work->rets, top + 1, lua_gettop(L), LUVF_THREAD_SIDE_CHILD);
      lua_pop(L, i);  // pop all returned value
      luv_thread_arg_clear(L, &work->rets, LUVF_THREAD_SIDE_CHILD);
    }
    luv_thread_arg_clear(L, &work->args, LUVF_THREAD_SIDE_CHILD);
  } else {
    fprintf(stderr, "Uncaught Error: %s can't be work entry\n",
            lua_typename(L, lua_type(L,-1)));
    lua_pop(L, 1);
    luv_thread_arg_clear(L, &work->args, LUVF_THREAD_SIDE_CHILD);
  }
  work->args.L = L;
  if (top!=lua_gettop(L))
    luaL_error(L, "stack not balance in luv_work_cb, need %d but %d", top, lua_gettop(L));
}

static void luv_after_work_cb(uv_work_t* req, int status) {
  luv_work_t* work = (luv_work_t*)req->data;
  luv_work_ctx_t* ctx = work->ctx;
  lua_State* L = ctx->L;
  int i;

  (void)status;

  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->after_work_cb);
  i = luv_thread_arg_push(L, &work->rets, LUVF_THREAD_SIDE_MAIN);
  luv_cfpcall(L, i, 0, 0);

  //cache lua_State to reuse
  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->pool_ref);
  i = lua_rawlen(L, -1);
  *(lua_State**)lua_newuserdata(L, sizeof(lua_State*)) = work->args.L;
  lua_rawseti(L, -2, i+1);
  lua_pop(L, 1);

  //ref down to ctx, up in luv_queue_work()
  luaL_unref(L, LUA_REGISTRYINDEX, work->ref);
  work->ref = LUA_NOREF;

  luv_thread_arg_clear(L, &work->args, LUVF_THREAD_SIDE_MAIN);
  luv_thread_arg_clear(L, &work->rets, LUVF_THREAD_SIDE_MAIN);
  free(work);
}

static int luv_new_work(lua_State* L) {
  size_t len;
  const char* buff;
  luv_work_ctx_t* ctx;

  buff = luv_thread_dumped(L, 1, &len);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  if(!lua_isnoneornil(L, 3))
    luaL_checktype(L, 3, LUA_TFUNCTION);

  ctx = (luv_work_ctx_t*)lua_newuserdata(L, sizeof(*ctx));
  memset(ctx, 0, sizeof(*ctx));

  ctx->len = len;
  ctx->code = (char*)malloc(ctx->len);
  memcpy(ctx->code, buff, len);

  lua_pushvalue(L, 2);
  ctx->after_work_cb = luaL_ref(L, LUA_REGISTRYINDEX);

  ctx->L = L;
  luaL_getmetatable(L, "luv_work_ctx");
  lua_setmetatable(L, -2);

  lua_newtable(L);
  ctx->pool_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  return 1;
}

static int luv_queue_work(lua_State* L) {
  int top = lua_gettop(L);
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  luv_work_t* work = (luv_work_t*)malloc(sizeof(*work));
  int ret, n;

  //prepare lua_State for threadpool
  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->pool_ref);
  n = lua_rawlen(L, -1);
  if (n > 0) {
    int i;
    lua_rawgeti(L, -1, 1);
    work->args.L = *(lua_State **)lua_touserdata(L, -1);
    lua_pop(L, 1);
    for(i=1; i<n; i++) {
      lua_rawgeti(L, -1, i+1);
      lua_rawseti(L, -2, i);
    }
    lua_pushnil(L);
    lua_rawseti(L, -2, n);
  }
  else
    work->args.L = acquire_vm_cb();
  lua_pop(L, 1);

  luv_thread_arg_set(L, &work->args, 2, top, LUVF_THREAD_SIDE_MAIN); //clear in sub threads,luv_work_cb
  work->ctx = ctx;
  work->work.data = work;
  ret = uv_queue_work(luv_loop(L), &work->work, luv_work_cb, luv_after_work_cb);
  if (ret < 0) {
    free(work);
    return luv_error(L, ret);
  }

  //ref up to ctx
  lua_pushvalue(L, 1);
  work->ref = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_pushboolean(L, 1);
  return 1;
}

static const luaL_Reg luv_work_ctx_methods[] = {
  {"queue", luv_queue_work},
  {NULL, NULL}
};

static void luv_work_init(lua_State* L) {
  luaL_newmetatable(L, "luv_work_ctx");
  lua_pushcfunction(L, luv_work_ctx_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_work_ctx_gc);
  lua_setfield(L, -2, "__gc");
  luaL_newlib(L, luv_work_ctx_methods);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);
}
