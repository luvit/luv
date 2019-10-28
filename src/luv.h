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
#include <lualib.h>
#include <lauxlib.h>
#include "uv.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#if defined(_WIN32)
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# ifndef __MINGW32__
#   define S_ISREG(x)  (((x) & _S_IFMT) == _S_IFREG)
#   define S_ISDIR(x)  (((x) & _S_IFMT) == _S_IFDIR)
#   define S_ISFIFO(x) (((x) & _S_IFMT) == _S_IFIFO)
#   define S_ISCHR(x)  (((x) & _S_IFMT) == _S_IFCHR)
#   define S_ISBLK(x)  0
# endif
# define S_ISLNK(x)  (((x) & S_IFLNK) == S_IFLNK)
# define S_ISSOCK(x) 0
#else
# include <unistd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX (8096)
#endif

#ifndef MAX_TITLE_LENGTH
#define MAX_TITLE_LENGTH (8192)
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

// luv flags to control luv_CFpcall routine
#define LUVF_CALLBACK_NOEXIT       0x01       // Don't exit when LUA_ERRMEM
#define LUVF_CALLBACK_NOTRACEBACK  0x02       // Don't traceback when error
#define LUVF_CALLBACK_NOERRMSG     0x04       // Don't output err message

/* Prototype of external callback routine.
 * The caller and the implementer exchanges data by the lua vm stack.
 * The caller push a lua function and nargs values onto the stack, then call it.
 * The implementer remove nargs(argument)+1(function) values from vm stack,
 * push all returned values by lua function onto the stack, and return an
 * integer as result code. If the result >= 0, that means the number of
 * values leave on the stack, or the callback routine error, nothing leave on
 * the stack, -result is the error value returned by lua_pcall.
 *
 * When LUVF_CALLBACK_NOEXIT is set, the implementer should not exit.
 * When LUVF_CALLBACK_NOTRACEBACK is set, the implementer will not do traceback.
 *
 * Need to notice that the implementer must balance the lua vm stack, and maybe
 * exit when memory allocation error.
 */
typedef int (*luv_CFpcall) (lua_State* L, int nargs, int nresults, int flags);

/* Default implemention of event callback */
LUALIB_API int luv_cfpcall(lua_State* L, int nargs, int nresult, int flags);

typedef struct {
  uv_loop_t*   loop;        /* main loop */
  lua_State*   L;           /* main thread,ensure coroutines works */
  luv_CFpcall  pcall;       /* luv event callback function in protected mode */

  void* extra;              /* extra data */
} luv_ctx_t;

/* Retrieve all the luv context from a lua_State */
LUALIB_API luv_ctx_t* luv_context(lua_State* L);

/* Retrieve the main thread of the given lua_State */
LUALIB_API lua_State* luv_state(lua_State* L);

/* Retrieve the uv_loop_t set for the given lua_State
   Note: Each lua_State can have a custom uv_loop_t
*/
LUALIB_API uv_loop_t* luv_loop(lua_State* L);

/* Set or clear an external uv_loop_t in a lua_State
   When using a custom/external loop, this must be called before luaopen_luv
   (otherwise luv will create and use its own loop)
*/
LUALIB_API void luv_set_loop(lua_State* L, uv_loop_t* loop);

/* Set or clear an external c routine for luv event callback
   When using a custom/external function, this must be called before luaopen_luv
   (otherwise luv will use the default callback function: luv_cfpcall)
*/
LUALIB_API void luv_set_callback(lua_State* L, luv_CFpcall pcall);

/* This is the main hook to load the library.
   This can be called multiple times in a process as long
   as you use a different lua_State and thread for each.
*/
LUALIB_API int luaopen_luv (lua_State *L);

#include "util.h"
#include "lhandle.h"
#include "lreq.h"

#ifdef LUV_SOURCE
/* From stream.c */
static uv_stream_t* luv_check_stream(lua_State* L, int index);
static void luv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
static void luv_check_buf(lua_State *L, int idx, uv_buf_t *pbuf);
static void luv_prep_buf(lua_State *L, int idx, uv_buf_t *pbuf);
static uv_buf_t* luv_prep_bufs(lua_State* L, int index, size_t *count);

/* from tcp.c */
static void parse_sockaddr(lua_State* L, struct sockaddr_storage* address);
static void luv_connect_cb(uv_connect_t* req, int status);

/* From fs.c */
static void luv_push_stats_table(lua_State* L, const uv_stat_t* s);

/* from constants.c */
static int luv_af_string_to_num(const char* string);
static const char* luv_af_num_to_string(const int num);
static int luv_sock_string_to_num(const char* string);
static const char* luv_sock_num_to_string(const int num);
static int luv_sig_string_to_num(const char* string);
static const char* luv_sig_num_to_string(const int num);
#endif

typedef lua_State* (*luv_acquire_vm)();
typedef void (*luv_release_vm)(lua_State* L);
LUALIB_API void luv_set_thread_cb(luv_acquire_vm acquire, luv_release_vm release);

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif
