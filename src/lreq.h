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
#ifndef LUV_LREQ_H
#define LUV_LREQ_H

#include "luv.h"

typedef struct {
  int req_ref; /* ref for uv_req_t's userdata */
  int callback_ref; /* ref for callback */
  int data_ref; /* ref for write data */
  luv_ctx_t* ctx; /* context for callback */
  void* data; /* extra data */
} luv_req_t;

// This is an arbitrary value that we can assume will never be returned by luaL_ref
#define LUV_REQ_MULTIREF (-0x1234)

#define luv_yield_req(L, data) \
  lua_rawgeti(L, LUA_REGISTRYINDEX, (data)->callback_ref); \
  lua_State *th = lua_tothread(L, -1);                   \
  lua_pop(L, 1);                                         \
                                                         \
  if (th)                                                \
    return lua_yield(th, 0);

#endif