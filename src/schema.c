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
#include "schema.h"

// Platform specefic file handle
static int luv_isfile(lua_State* L, int index) {
  // TODO: this should act different on windows maybe.
  return lua_isnumber(L, index);
}

// Optional function
static int luv_iscontinuation(lua_State* L, int index) {
  return lua_isnone(L, index) || lua_isfunction(L, index);
}

static int luv_ispositive(lua_State* L, int index) {
  return lua_isnumber(L, index) && lua_tonumber(L, index) >= 0;
}

void lschema_check(lua_State* L, lschema_entry schema[]) {
  int num = lua_gettop(L) - 1;
  int i;
  for (i = 0; schema[i].name;) {
    lschema_entry entry = schema[i++];
    luaL_argcheck(L, entry.checker(L, i), i, entry.name);
  }
  if (num > i) {
    luaL_argerror(L, i, "Too many arguments");
  }
}
