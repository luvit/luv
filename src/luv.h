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
#ifndef LUV_H
#define LUV_H
#include <lua.h>
#include <lauxlib.h>
#include "uv.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#if defined(_WIN32)
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# define S_ISREG(x)  (((x) & _S_IFMT) == _S_IFREG)
# define S_ISDIR(x)  (((x) & _S_IFMT) == _S_IFDIR)
# define S_ISFIFO(x) (((x) & _S_IFMT) == _S_IFIFO)
# define S_ISCHR(x)  (((x) & _S_IFMT) == _S_IFCHR)
# define S_ISBLK(x)  0
# define S_ISLNK(x)  0
# define S_ISSOCK(x) 0
#endif

#ifndef PATH_MAX
#define PATH_MAX (8096)
#endif

#ifndef MAX_TITLE_LENGTH
#define MAX_TITLE_LENGTH (8192)
#endif

#if LUA_VERSION_NUM < 502
# define lua_rawlen lua_objlen
/* lua_...uservalue: Something very different, but it should get the job done */
# define lua_getuservalue lua_getfenv
# define lua_setuservalue lua_setfenv
# define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
# define luaL_setfuncs(L,l,n) (assert(n==0), luaL_register(L,NULL,l))
# define lua_resume(L,F,n) lua_resume(L,n)
#endif

LUALIB_API int luaopen_luv (lua_State *L);

// From util.c
static void util_init(lua_State* L);
static int on_panic(lua_State* L);
static void luv_stack_dump(lua_State* L, const char* name);

#include "userdata.h"

static int luv_error(lua_State* L, int ret);

// Utilities for hanging on to lua states while waiting for events.
static void luv_ref_state(luv_ref_t* data, lua_State* L);
static void luv_unref_state(luv_ref_t* data);

static void luv_ccall(lua_State* L, luv_ref_t* ref, int nargs);
static void luv_emit_event(lua_State* L, luv_ref_t* ref, const char* name, int nargs);

static int luv_wait(lua_State* L, luv_ref_t* ref, int status);
static void luv_resume(lua_State* L, int nargs);
static void luv_resume_with_status(lua_State* L, int status, int nargs);

#endif
