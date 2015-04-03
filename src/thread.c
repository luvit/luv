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
  /* only support LUA_TNIL, LUA_TBOOLEAN, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TSTRING*/
  int type;
  union {
    lua_Number num;
    int bool;
    void *point;
    struct {
      const char* base;
      int len;
    } str;
  } val;
} luv_thread_arg_t;

#define LUV_THREAD_MAXNUM_ARG 9
typedef struct {
  uv_thread_t handle;
  const unsigned char* code;
  int len;
  int argc;
  luv_thread_arg_t argv[LUV_THREAD_MAXNUM_ARG];
} luv_thread_t;

static void luv_thread_arg_set(lua_State*L, luv_thread_t* thread, int idx) {
  int top = lua_gettop(L);
  int i = idx;
  while (i <= top && i <= LUV_THREAD_MAXNUM_ARG + idx)
  {
    luv_thread_arg_t *arg = thread->argv + i - idx;
    arg->type = lua_type(L, i);
    switch (arg->type)
    {
    case LUA_TNIL:
      break;
    case LUA_TBOOLEAN:
      arg->val.bool = lua_toboolean(L, i);
      break;
    case LUA_TNUMBER:
      arg->val.num = lua_tonumber(L, i);
      break;
    case LUA_TLIGHTUSERDATA:
      arg->val.point = lua_touserdata(L, i);
      break;
    case LUA_TSTRING:
      arg->val.str.base = lua_tolstring(L, i, &arg->val.str.len);
      break;
    default:
      fprintf(stderr, "Error: thread arg not support type %s at %d",
        luaL_typename(L, arg->type), i);
      exit(-1);
      break;
    }
    i++;
  }
  thread->argc = i - idx;
}

static luv_thread_t* luv_check_thread(lua_State* L, int index)
{
  luv_thread_t* thread = luaL_checkudata(L, index, "uv_thread");
  return thread;
}

static int luv_thread_gc(lua_State *L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int i;
  for (i = 0; i < tid->argc; i++ ){ 
    luv_thread_arg_t *arg = tid->argv + i;
    if (arg->type == LUA_TSTRING) {
      free(arg->val.str.base);
    }
  }
  return 0;
}

static int luv_thread_tostring(lua_State* L)
{
  luv_thread_t* thd = luaL_checkudata(L, 1, "uv_thread");
  lua_pushfstring(L, "uv_thread_t: %p", thd->handle);
  return 1;
}

static void luv_thread_cb(void* varg) {
  luv_thread_t* thd = (luv_thread_t*)varg;
  uv_thread_t tid = uv_thread_self();
  lua_State *L = luaL_newstate();

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

  if (luaL_loadbuffer(L, thd->code, thd->len, "=thread") == 0)
  {
    int i = 0;
    while (i < thd->argc)
    {
      luv_thread_arg_t* arg = thd->argv+i;
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
        break;
      default:
        fprintf(stderr, "Error: thread arg not support type %s at %d",
          luaL_typename(L, arg->type), i+1);
      }
      i++;
    }
    if (lua_pcall(L, i, 0, 0)) {
      fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
    }
  } else {
    fprintf(stderr, "Uncaught Error: %s\n", lua_tostring(L, -1));
  }
  lua_close(L);
}

static int luv_new_thread(lua_State* L) {
  luv_thread_t* thread;
  thread = lua_newuserdata(L, sizeof(*thread));
  memset(thread, 0, sizeof(*thread));
  luaL_getmetatable(L, "uv_thread");
  lua_setmetatable(L, -2);
  return 1;
}

static int luv_thread_create(lua_State* L) {
  luv_thread_t *thread;
  int ret;
  int len;
  const unsigned char* buff;

  thread = luv_check_thread(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  luv_thread_arg_set(L, thread, 3);

  lua_getfield(L, LUA_GLOBALSINDEX, "string");
  lua_getfield(L, -1, "dump");
  lua_remove(L, -2);
  lua_pushvalue(L, 2);
  if (lua_pcall(L, 1, 1, 0)) {
    fprintf(stderr, "Uncaught Error: %s\n", lua_tostring(L, -1));
    exit(-1);
  }
  buff = lua_tolstring(L, -1, &len);
  thread->len = len;
  thread->code = malloc(thread->len);
  memcpy(thread->code, buff, len);
  
  ret = uv_thread_create(&thread->handle, luv_thread_cb, thread);
  if (ret < 0) return luv_error(L, ret);
  lua_pushboolean(L, 1);
  return 1;
}

static int luv_thread_join(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int ret = uv_thread_join(&tid->handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushboolean(L, 1);
  return 1;
}

static int luv_thread_self(lua_State* L)
{
  luv_thread_t* thread;
  uv_thread_t t = uv_thread_self();
  thread = lua_newuserdata(L, sizeof(*thread));
  memset(thread, 0, sizeof(*thread));
  memcpy(&thread->handle, &t, sizeof(t));
  luaL_getmetatable(L, "uv_thread");
  lua_setmetatable(L, -2);
  return 1;
}

static int luv_thread_equal(lua_State* L)
{
  luv_thread_t* t1 = luv_check_thread(L, 1);
  luv_thread_t* t2 = luv_check_thread(L, 2);
  int ret = uv_thread_equal(&t1->handle, &t2->handle);
  lua_pushboolean(L, ret);
  return 1;
}


static void luv_thread_init(lua_State* L) {
  luaL_newmetatable(L, "uv_thread");
  lua_pushcfunction(L, luv_thread_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_thread_equal);
  lua_setfield(L, -2, "__eq");
  lua_pushcfunction(L, luv_thread_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);
}
