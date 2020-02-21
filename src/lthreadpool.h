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
#ifndef LUV_LTHREADPOOL_H
#define LUV_LTHREADPOOL_H

#include "luv.h"

#define LUV_THREAD_MAXNUM_ARG 9

typedef struct {
  // support basic lua type LUA_TNIL, LUA_TBOOLEAN, LUA_TNUMBER, LUA_TSTRING
  // and support uv_handle_t userdata
  int type;
  union
  {
    lua_Number num;
    int boolean;
    struct {
      const char* base;
      size_t len;
    } str;
    struct {
      const void* data;
      size_t size;
      const char* metaname;
    } udata;
  } val;
  int ref[2];          // ref of string or userdata
} luv_val_t;

typedef struct {
  int argc;
  int flags;          // control gc

  lua_State *L;
  luv_val_t argv[LUV_THREAD_MAXNUM_ARG];
} luv_thread_arg_t;

//luajit miss LUA_OK
#ifndef LUA_OK
#define LUA_OK 0
#endif

//LUV flags for thread or threadpool args
#define LUVF_THREAD_SIDE_MAIN      0x00
#define LUVF_THREAD_SIDE_CHILD     0x01
#define LUVF_THREAD_MODE_ASYNC     0x02
#define LUVF_THREAD_SIDE(i)        ((i)&0x01)
#define LUVF_THREAD_ASYNC(i)       ((i)&0x02)

#ifdef LUV_SOURCE
static const char* luv_getmtname(lua_State *L, int idx);
static int luv_thread_arg_set(lua_State* L, luv_thread_arg_t* args, int idx, int top, int flags);
static int luv_thread_arg_push(lua_State* L, luv_thread_arg_t* args, int flags);
static void luv_thread_arg_clear(lua_State* L, luv_thread_arg_t* args, int flags);
#endif

#endif //LUV_LTHREADPOOL_H
