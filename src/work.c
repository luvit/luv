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

#include "lthreadpool.h"

typedef struct {
  lua_State* L;       /* main vm in loop thread */
  char* code;
  size_t len;

  uv_async_t async;
  int async_cb;       /* in loop thread, call when async message recived */
  //int work_cb;      /* in pool,lua work cb script */
  int after_work_cb;  /* in loop thread, lua after work cb script */
} luv_work_ctx_t;

typedef struct {
  uv_work_t work;
  luv_work_ctx_t* ctx;

  luv_thread_arg_t arg;
} luv_work_t;

static uv_key_t L_key;
static luv_work_ctx_t* luv_check_work_ctx(lua_State* L, int index)
{
  luv_work_ctx_t* ctx = luaL_checkudata(L, index, "luv_work_ctx");
  return ctx;
}

static luv_work_t* luv_check_work(lua_State* L, int index)
{
  luv_work_t* work = luaL_checkudata(L, index, "uv_work");
  return work;
}

static int luv_work_gc(lua_State *L) {
  luv_work_t* work = luv_check_work(L, 1);
  luv_thread_arg_clear(&work->arg);
  return 0;
}

static int luv_work_ctx_gc(lua_State *L)
{
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  free(ctx->code);
  luaL_unref(L, LUA_REGISTRYINDEX, ctx->after_work_cb);
  luaL_unref(L, LUA_REGISTRYINDEX, ctx->async_cb);

   return 0;
}

static int luv_work_ctx_tostring(lua_State* L)
{
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  lua_pushfstring(L, "luv_work_ctx_t: %p", ctx);
  return 1;
}

static int luv_work_tostring(lua_State* L)
{
  luv_work_t* work = luv_check_work(L, 1);
  lua_pushfstring(L, "uv_work_t: %p", &work->work);
  return 1;
}

static void luv_work_cb(uv_work_t* req)
{
  uv_thread_t tid = uv_thread_self();
  luv_work_t* work = req->data;
  luv_work_ctx_t* ctx = work->ctx;
  lua_State *L = uv_key_get(&L_key);
  int top;
  if (L == NULL) {
    /* should vm reuse in pool? */
    L = acquire_vm_cb();

    uv_key_set(&L_key, L);
  }

  top = lua_gettop(L);
  lua_pushlstring(L, ctx->code, ctx->len);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (lua_isnil(L, -1))
  {
    lua_pop(L, 1);
    
    lua_pushlstring(L, ctx->code, ctx->len);
    if (luaL_loadbuffer(L, ctx->code, ctx->len, "=pool") != 0)
    {
      fprintf(stderr, "Uncaught Error: %s\n", lua_tostring(L, -1));
      lua_pop(L, 2);

      lua_pushnil(L);
    } else
    {
      lua_pushvalue(L, -1);
      lua_insert(L, lua_gettop(L) - 2);
      lua_rawset(L, LUA_REGISTRYINDEX);
    }
  }

  if (lua_isfunction(L, -1))
  {
    int i = luv_thread_arg_push(L, &work->arg);
    if (lua_pcall(L, i, LUA_MULTRET, 0)) {
      fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
    }
    luv_thread_arg_set(L, &work->arg, top + 1, lua_gettop(L));
  } else {
    fprintf(stderr, "Uncaught Error: %s can't be work entry\n", 
      lua_typename(L, lua_type(L,-1)));
  }
}

static void luv_after_work_cb(uv_work_t* req, int status) {
  luv_work_t* work = req->data;
  luv_work_ctx_t* ctx = work->ctx;
  lua_State*L = ctx->L;
  int i;
  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->after_work_cb);
  i = luv_thread_arg_push(L, &work->arg);
  if (lua_pcall(L, i, 0, 0))
  {
    fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
  }
  lua_pushlightuserdata(L, work->ctx);
  lua_pushnil(L);
  lua_settable(L, LUA_REGISTRYINDEX);

  lua_pushlightuserdata(L, work);
  lua_pushnil(L);
  lua_settable(L, LUA_REGISTRYINDEX);

}

static void async_cb(uv_async_t *handle)
{
  luv_work_t*work = handle->data;
  luv_work_ctx_t* ctx = work->ctx;
  lua_State*L = ctx->L;
  int i;
  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->async_cb);
  i = luv_thread_arg_push(L, &work->arg);
  if (lua_pcall(L, i, 0, 0))
  {
    fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
  }
}

static int luv_new_work(lua_State* L) {
  size_t len;
  const char* buff;
  luv_work_ctx_t* ctx;

  buff = luv_thread_dumped(L, 1, &len);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  if(!lua_isnoneornil(L, 3))
    luaL_checktype(L, 3, LUA_TFUNCTION);

  ctx = lua_newuserdata(L, sizeof(*ctx));
  memset(ctx, 0, sizeof(*ctx));

  ctx->len = len;
  ctx->code = malloc(ctx->len);
  memcpy(ctx->code, buff, len);

  lua_pushvalue(L, 2);
  ctx->after_work_cb = luaL_ref(L, LUA_REGISTRYINDEX);
  if (lua_gettop(L) == 4) {
    lua_pushvalue(L, 3);
    ctx->async_cb = luaL_ref(L, LUA_REGISTRYINDEX);
    uv_async_init(luv_loop(L), &ctx->async, async_cb);
  } else
    ctx->async_cb = LUA_REFNIL;
  ctx->L = L;
  luaL_getmetatable(L, "luv_work_ctx");
  lua_setmetatable(L, -2);
  return 1;
}

static int luv_queue_work(lua_State* L) {
  luv_work_ctx_t* ctx;
  luv_work_t* work;
  int ret;
  int top = lua_gettop(L);
  ctx = luv_check_work_ctx(L, 1);   // ctx should ref up
  work = lua_newuserdata(L, sizeof(*work)); //work should ref up
  memset(work, 0, sizeof(*work));
  luv_thread_arg_set(L, &work->arg, 2, top);
  work->ctx = ctx;
  work->work.data = work;
  ret = uv_queue_work(luv_loop(L), &work->work, luv_work_cb, luv_after_work_cb);
  if (ret < 0) return luv_error(L, ret);

  lua_pushlightuserdata(L, work->ctx);
  lua_pushvalue(L, 1);
  lua_settable(L, LUA_REGISTRYINDEX);

  lua_pushlightuserdata(L, work);
  lua_pushvalue(L, -2);
  lua_settable(L, LUA_REGISTRYINDEX);

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
  lua_newtable(L);
  luaL_setfuncs(L, luv_work_ctx_methods, 0);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "luv_work");
  lua_pushcfunction(L, luv_work_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_work_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);

  if (uv_thread_self()==0)
    uv_key_create(&L_key);
}
