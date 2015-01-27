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
#include <lj_arch.h>

static int luv_platform(lua_State* L) {
  lua_newtable(L);

  lua_pushliteral(L, LJ_OS_NAME);
  lua_setfield(L, -2, "os");

  lua_pushliteral(L, LJ_ARCH_NAME);
  lua_setfield(L, -2, "arch");

  lua_pushinteger(L, LJ_ARCH_BITS);
  lua_setfield(L, -2, "bits");

  if(LJ_ARCH_ENDIAN == 0) {
    lua_pushliteral(L, "little");
  } else {
    lua_pushliteral(L, "big");
  }

  lua_setfield(L, -2, "byteorder");

  return 1;
}
