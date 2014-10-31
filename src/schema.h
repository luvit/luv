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
#ifndef LSCHEMA_H
#define LSCHEMA_H

#include "luv.h"

typedef const struct {
  const char* name;
  int (*checker)(lua_State* L, int index);
} lschema_entry;

static int luv_isfile(lua_State* L, int index);
static uv_file luv_tofile(lua_State* L, int index);
static int luv_iscontinuation(lua_State* L, int index);
static int luv_ispositive(lua_State* L, int index);
void lschema_check(lua_State* L, lschema_entry schema[]);

#endif
