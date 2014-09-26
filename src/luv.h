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

#include "luv.h"

// This is a subset of UV_HANDLE_TYPE_MAP without stream and handle
// since you never actually create instances of those.
#define LUV_CONCRETE_HANDLE_TYPE_MAP(XX)                                                \
  XX(ASYNC, async)                                                            \
  XX(CHECK, check)                                                            \
  XX(FS_EVENT, fs_event)                                                      \
  XX(FS_POLL, fs_poll)                                                        \
  XX(IDLE, idle)                                                              \
  XX(NAMED_PIPE, pipe)                                                        \
  XX(POLL, poll)                                                              \
  XX(PREPARE, prepare)                                                        \
  XX(PROCESS, process)                                                        \
  XX(TCP, tcp)                                                                \
  XX(TIMER, timer)                                                            \
  XX(TTY, tty)                                                                \
  XX(UDP, udp)                                                                \
  XX(SIGNAL, signal)

#define LUV_CONCRETE_REQ_TYPE_MAP(XX)                                         \
  XX(CONNECT, connect)                                                        \
  XX(WRITE, write)                                                            \
  XX(SHUTDOWN, shutdown)                                                      \
  XX(UDP_SEND, udp_send)                                                      \
  XX(FS, fs)                                                                  \
  XX(WORK, work)                                                              \
  XX(GETADDRINFO, getaddrinfo)                                                \
  XX(GETNAMEINFO, getnameinfo)                                                \

// wrapper for uv_loop_t so we can store ref
typedef struct {
  int ref;
  uv_loop_t data;
} luv_loop_t;

// Define the luv handles for the various handle types
#define XX(uc, lc)    \
typedef struct{       \
  int ref;            \
  uv_##lc##_t data;   \
} luv_##lc##_t;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX

// Define the luv requests for the various request types
#define XX(uc, lc)    \
typedef struct{       \
  int ref;            \
  uv_##lc##_t data;   \
} luv_##lc##_t;
  UV_REQ_TYPE_MAP(XX)
#undef XX

// The presence of these unions force similar struct layout.
#define XX(uc, lc)    \
  luv_##lc##_t lc;
union luv_any_handle {
  UV_HANDLE_TYPE_MAP(XX)
};
union luv_any_req {
  UV_REQ_TYPE_MAP(XX)
};
#undef XX

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

// create, check, find, and unref for all uv types (loop, req*, handle*)
#define XX(uc, lc)                                           \
static uv_##lc##_t* luv_create_##lc(lua_State* L);
  XX(LOOP, loop)
  LUV_CONCRETE_REQ_TYPE_MAP(XX)
  LUV_CONCRETE_HANDLE_TYPE_MAP(XX)
#undef XX

#define XX(uc, lc)                                           \
static uv_##lc##_t* luv_check_##lc(lua_State* L, int index); \
static void luv_find_##lc(lua_State* L, uv_##lc##_t* ptr);
  XX(LOOP, loop)
  LUV_CONCRETE_REQ_TYPE_MAP(XX)
  UV_HANDLE_TYPE_MAP(XX)
#undef XX

#define XX(uc, lc)                                           \
static void luv_unref_##lc(lua_State* L, uv_##lc##_t* ptr);
  XX(LOOP, loop)
  LUV_CONCRETE_REQ_TYPE_MAP(XX)
  XX(HANDLE, handle)
#undef XX

static int luv_error(lua_State* L, int ret);
static void luv_ccall(lua_State* L, int nargs);
static void luv_emit_event(lua_State* L, const char* name, int nargs);

static int luv_wait(lua_State* L, int status);
static void luv_resume(lua_State* L, int nargs);
static void luv_resume_with_status(lua_State* L, int status, int nargs);

#endif
