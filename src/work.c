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

typedef struct {
  lua_State* L;       /* main vm in loop thread */

  const unsigned char* code;
  int len;

  //int work_cb;      /* in pool,lua work cb script */
  int after_work_cb;  /* in loop thread, lua after work cb script */
} luv_work_ctx_t;

typedef struct {
  uv_work_t work;
  luv_work_ctx_t* ctx;

  int argc;
  /* paramater for work_cb, and return to after_work_cb */
  luv_thread_arg_t argv[LUV_THREAD_MAXNUM_ARG];
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
  int i;
  for (i = 0; i < work->argc; i++)
  {
    luv_thread_arg_t *arg = work->argv + i;
    if (arg->type == LUA_TSTRING) {
      free(arg->val.str.base);
    }
  }
  return 0;
}

static int luv_work_ctx_gc(lua_State *L)
{
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  free(ctx->code);
  luaL_unref(L, LUA_REGISTRYINDEX, ctx->after_work_cb);
  ctx->after_work_cb = LUA_REFNIL;
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
    L = luaL_newstate();
    // Add in the lua standard libraries
    luaL_openlibs(L);

    // Get package.preload so we can store builtins in it.
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_remove(L, -2); // Remove package

    // Store uv module definition at preload.uv
    lua_pushcfunction(L, luaopen_luv);
    lua_setfield(L, -2, "uv");
    lua_pop(L, 1);

    uv_key_set(&L_key, L);
  }
  top = lua_gettop(L);
  if (luaL_loadbuffer(L, ctx->code, ctx->len, "=pool") == 0)
  {
    int i = 0;
    while (i < work->argc)
    {
      luv_thread_arg_t* arg = work->argv + i;
      switch (arg->type) {
      case LUA_TNIL:
        lua_pushnil(L);
        break;
      case LUA_TBOOLEAN:
        lua_pushboolean(L, arg->val.bool);
        break;
      case LUA_TLIGHTUSERDATA:
        lua_pushlightuserdata(L, arg->val.point);
        break;
      case LUA_TNUMBER:
        lua_pushnumber(L, arg->val.num);
        break;
      case LUA_TSTRING:
        lua_pushlstring(L, arg->val.str.base, arg->val.str.len);
        free(arg->val.str.base);
        arg->val.str.len = 0;
        arg->val.str.base = NULL;
        break;
      default:
        fprintf(stderr, "Error: thread arg not support type %s at %d",
          luaL_typename(L, arg->type), i+1);
      }
      i++;
    }
    if (lua_pcall(L, i, LUA_MULTRET, 0)) {
      fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
    }
    work->argc = luv_thread_arg_set(L, work->argv, top, lua_gettop(L));
  } else {
    fprintf(stderr, "Uncaught Error: %s\n", lua_tostring(L, -1));
  }

  //lua_close(L);
}

static void luv_after_work_cb(uv_work_t* req, int status) {
  luv_work_t* work = req->data;
  luv_work_ctx_t* ctx = work->ctx;
  lua_State*L = ctx->L;
  int i;
  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->after_work_cb);
  i = 0;
  while (i < work->argc)
  {
    luv_thread_arg_t* arg = work->argv + i;
    switch (arg->type)
    {
    case LUA_TNIL:
      lua_pushnil(L);
      break;
    case LUA_TBOOLEAN:
      lua_pushboolean(L, arg->val.bool);
      break;
    case LUA_TLIGHTUSERDATA:
      lua_pushlightuserdata(L, arg->val.point);
      break;
    case LUA_TNUMBER:
      lua_pushnumber(L, arg->val.num);
      break;
    case LUA_TSTRING:
      lua_pushlstring(L, arg->val.str.base, arg->val.str.len);
      break;
    default:
      fprintf(stderr, "Error: thread arg not support type %s at %d",
        luaL_typename(L, arg->type), i + 1);
    }
    i++;
  }
  if (lua_pcall(L, i, 0, 0))
  {
    fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
  }
}

static int luv_new_work(lua_State* L) {
  int len;
  const unsigned char* buff;
  luv_work_ctx_t* ctx;

  ctx = lua_newuserdata(L, sizeof(*ctx));
  memset(ctx, 0, sizeof(*ctx));
  luaL_checktype(L, 1, LUA_TFUNCTION);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  lua_getfield(L, LUA_GLOBALSINDEX, "string");
  lua_getfield(L, -1, "dump");
  lua_remove(L, -2);
  lua_pushvalue(L, 1);
  if (lua_pcall(L, 1, 1, 0))
  {
    fprintf(stderr, "Uncaught Error: %s\n", lua_tostring(L, -1));
    exit(-1);
  }
  buff = lua_tolstring(L, -1, &len);
  ctx->len = len;
  ctx->code = malloc(ctx->len);
  memcpy(ctx->code, buff, len);
  lua_pop(L, 1);
  lua_pushvalue(L, 2);
  ctx->after_work_cb = luaL_ref(L, LUA_REGISTRYINDEX);
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
  ctx = luv_check_work_ctx(L, 1);
  work = lua_newuserdata(L, sizeof(*work));
  memset(work, 0, sizeof(*work));
  work->argc = luv_thread_arg_set(L, work->argv, 2, top);
  work->ctx = ctx;
  work->work.data = work;
  ret = uv_queue_work(luv_loop(L), &work->work, luv_work_cb, luv_after_work_cb);
  if (ret < 0) return luv_error(L, ret);
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
  lua_pop(L, 1);

  luaL_newmetatable(L, "luv_work");
  lua_pushcfunction(L, luv_work_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_work_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);

  uv_key_create(&L_key);
}
