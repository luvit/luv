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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

// http://broken.build/2012/11/10/magical-container_of-macro/
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

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
#include <lauxlib.h>
#include "uv.h"

#if LUA_VERSION_NUM < 502
#	define lua_rawlen lua_objlen
/* lua_...uservalue: Something very different, but it should get the job done */
#	define lua_getuservalue lua_getfenv
#	define lua_setuservalue lua_setfenv
#	define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#	define luaL_setfuncs(L,l,n) (assert(n==0), luaL_register(L,NULL,l))
#else
# define lua_resume(L,n) lua_resume(L,NULL,n)
#endif

#include "util.c"
#include "loop.c"
#include "handle.c"
#include "timer.c"

// #include "misc.c"
// #include "dns.c"
// #include "stream.c"
// #include "tcp.c"
// #include "tty.c"
// #include "pipe.c"
// #include "process.c"
// #include "fs.c"

static const luaL_Reg luv_functions[] = {
  // loop.c
  {"new_loop", new_loop},
  {"loop_close", luv_loop_close},
  {"run", luv_run},
  {"loop_alive", luv_loop_alive},
  {"stop", luv_stop},
  {"backend_fd", luv_backend_fd},
  {"backend_timeout", luv_backend_timeout},
  {"now", luv_now},
  {"update_time", luv_update_time},
  {"walk", luv_walk},

  // handle.c
  {"is_active", luv_is_active},
  {"is_closing", luv_is_closing},
  {"close", luv_close},
  {"ref", luv_ref},
  {"unref", luv_unref},
  {"has_ref", luv_has_ref},
  {"send_buffer_size", luv_send_buffer_size},
  {"recv_buffer_size", luv_recv_buffer_size},
  {"fileno", luv_fileno},

  // timer.c
  {"new_timer", new_timer},
  {"timer_start", luv_timer_start},
  {"timer_stop", luv_timer_stop},
  {"timer_again", luv_timer_again},
  {"timer_set_repeat", luv_timer_set_repeat},


  // {"new_tcp", new_tcp},
  // {"new_timer", new_timer},
  // {"new_tty", new_tty},
  // {"new_udp", new_udp},
  // {"new_pipe", new_pipe},
  // {"guess_handle", luv_guess_handle},
  // {"update_time", luv_update_time},
  // {"now", luv_now},
  // {"loadavg", luv_loadavg},
  // {"execpath", luv_execpath},
  // {"cwd", luv_cwd},
  // {"chdir", luv_chdir},
  // {"get_free_memory", luv_get_free_memory},
  // {"get_total_memory", luv_get_total_memory},
  // {"get_process_title", luv_get_process_title},
  // {"set_process_title", luv_set_process_title},
  // {"hrtime", luv_hrtime},
  // {"uptime", luv_uptime},
  // {"cpu_info", luv_cpu_info},
  // {"interface_addresses", luv_interface_addresses},
  //
  // {"getaddrinfo", luv_getaddrinfo},
  //
  // {"is_active", luv_is_active},
  // {"walk", luv_walk},
  // {"close", luv_close},
  // {"ref", luv_ref},
  // {"unref", luv_unref},
  // {"is_closing", luv_is_closing},
  //
  // {"timer_start", luv_timer_start},
  // {"timer_stop", luv_timer_stop},
  // {"timer_again", luv_timer_again},
  // {"timer_set_repeat", luv_timer_set_repeat},
  // {"timer_get_repeat", luv_timer_get_repeat},
  //
  // {"write", luv_write},
  // {"write2", luv_write2},
  // {"shutdown", luv_shutdown},
  // {"read_start", luv_read_start},
  // {"read2_start", luv_read2_start},
  // {"read_stop", luv_read_stop},
  // {"listen", luv_listen},
  // {"accept", luv_accept},
  // {"is_readable", luv_is_readable},
  // {"is_writable", luv_is_writable},
  //
  // {"tcp_bind", luv_tcp_bind},
  // {"tcp_getsockname", luv_tcp_getsockname},
  // {"tcp_getpeername", luv_tcp_getpeername},
  // {"tcp_connect", luv_tcp_connect},
  // {"tcp_open", luv_tcp_open},
  // {"tcp_nodelay", luv_tcp_nodelay},
  // {"tcp_keepalive", luv_tcp_keepalive},
  //
  // {"tty_set_mode", luv_tty_set_mode},
  // {"tty_reset_mode", luv_tty_reset_mode},
  // {"tty_get_winsize", luv_tty_get_winsize},
  //
  // {"pipe_open", luv_pipe_open},
  // {"pipe_bind", luv_pipe_bind},
  // {"pipe_connect", luv_pipe_connect},
  //
  // {"spawn", luv_spawn},
  // {"kill", luv_kill},
  // {"process_kill", luv_process_kill},
  //
  // {"fs_open", luv_fs_open},
  // {"fs_close", luv_fs_close},
  // {"fs_read", luv_fs_read},
  // {"fs_write", luv_fs_write},
  // {"fs_stat", luv_fs_stat},
  // {"fs_fstat", luv_fs_fstat},
  // {"fs_lstat", luv_fs_lstat},
  // {"fs_unlink", luv_fs_unlink},
  // {"fs_mkdir", luv_fs_mkdir},
  // {"fs_rmdir", luv_fs_rmdir},
  // {"fs_readdir", luv_fs_readdir},
  // {"fs_rename", luv_fs_rename},
  // {"fs_fsync", luv_fs_fsync},
  // {"fs_fdatasync", luv_fs_fdatasync},
  // {"fs_ftruncate", luv_fs_ftruncate},
  // {"fs_sendfile", luv_fs_sendfile},
  // {"fs_chmod", luv_fs_chmod},
  // {"fs_utime", luv_fs_utime},
  // {"fs_futime", luv_fs_futime},
  // {"fs_link", luv_fs_link},
  // {"fs_symlink", luv_fs_symlink},
  // {"fs_readlink", luv_fs_readlink},
  // {"fs_fchmod", luv_fs_fchmod},
  // {"fs_chown", luv_fs_chown},
  // {"fs_fchown", luv_fs_fchown},

  {NULL, NULL}
};

// static int luv_newindex(lua_State* L) {
//   lua_getuservalue(L, 1);
//   lua_pushvalue(L, 2);
//   lua_pushvalue(L, 3);
//   lua_rawset(L, -3);
//   lua_pop(L, 1);
//   return 0;
// }
//
// static int luv_index(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//
//   /* Get handle type if requested */
//   const char* key = lua_tostring(L, 2);
//   if (strcmp(key, "type") == 0) {
//     luv_handle_t* lhandle = (luv_handle_t*)luaL_checkudata(L, 1, "luv_handle");
//     switch (lhandle->handle->type) {
// #define XX(uc, lc) case UV_##uc: lua_pushstring(L, #uc); break;
//     UV_HANDLE_TYPE_MAP(XX)
// #undef XX
//       default: lua_pushstring(L, "UNKNOWN"); break;
//     }
//     return 1;
//   }
//
//   lua_getuservalue(L, 1);
//   lua_pushvalue(L, 2);
//   lua_rawget(L, -2);
//   lua_remove(L, -2);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top + 1);
// #endif
//   return 1;
// }

// static int luv_tostring(lua_State* L) {
//   luv_handle_t* lhandle = (luv_handle_t*)luaL_checkudata(L, 1, "luv_handle");
//   switch (lhandle->handle->type) {
// #define XX(uc, lc) case UV_##uc: lua_pushfstring(L, "uv_%s_t: %p", #lc, lhandle->handle); break;
//   UV_HANDLE_TYPE_MAP(XX)
// #undef XX
//     default: lua_pushfstring(L, "userdata: %p", lhandle->handle); break;
//   }
//   return 1;
// }


LUALIB_API int luaopen_luv (lua_State *L) {
  util_init(L);
  loop_init(L);
  handle_init(L);
  luaL_newlib(L, luv_functions);
  return 1;
}
