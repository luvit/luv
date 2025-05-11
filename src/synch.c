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

static uv_sem_t* luv_check_sem(lua_State* L, int index) {
  return (uv_sem_t*) luv_checkudata(L, index, "uv_sem");
}

static int luv_new_sem(lua_State* L) {
  int value = luaL_optinteger(L, 1, 0);
  if (value < 0) {
    return luaL_argerror(L, 1, "value must be >= 0");
  }

  uv_sem_t *sem = (uv_sem_t*) luv_newuserdata(L, sizeof(uv_sem_t));
  luaL_getmetatable(L, "uv_sem");
  lua_setmetatable(L, -2);

  int ret = uv_sem_init(sem, value);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_sem_gc(lua_State* L) {
  uv_sem_t* sem = luv_check_sem(L, 1);

  uv_sem_destroy(sem);
  free(sem);
  return 0;
}

static int luv_sem_post(lua_State* L) {
  uv_sem_t* sem = luv_check_sem(L, 1);
  uv_sem_post(sem);
  return 0;
}

static int luv_sem_wait(lua_State* L) {
  uv_sem_t* sem = luv_check_sem(L, 1);
  uv_sem_wait(sem);
  return 0;
}

static int luv_sem_trywait(lua_State* L) {
  uv_sem_t* sem = luv_check_sem(L, 1);
  int ret = uv_sem_trywait(sem);
  lua_pushboolean(L, ret == 0);
  return 1;
}

static const luaL_Reg luv_sem_methods[] = {
  {"post", luv_sem_post},
  {"wait", luv_sem_wait},
  {"trywait", luv_sem_trywait},
  {NULL, NULL}
};

static void luv_synch_init(lua_State* L) {
  luaL_newmetatable(L, "uv_sem");
  lua_pushcfunction(L, luv_sem_gc);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  luaL_setfuncs(L, luv_sem_methods, 0);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);
}
