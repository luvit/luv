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

#include <lua.h>
#if (LUA_VERSION_NUM < 503)
#include "compat-5.3.h"
#endif
#include "luv.h"

#include "util.c"
#include "lhandle.c"
#include "lreq.c"
#include "loop.c"
#include "req.c"
#include "handle.c"
#include "timer.c"
#include "prepare.c"
#include "check.c"
#include "idle.c"
#include "async.c"
#include "poll.c"
#include "signal.c"
#include "process.c"
#include "stream.c"
#include "tcp.c"
#include "pipe.c"
#include "tty.c"
#include "udp.c"
#include "fs_event.c"
#include "fs_poll.c"
#include "fs.c"
#include "dns.c"
#include "thread.c"
#include "work.c"
#include "misc.c"
#include "constants.c"
#include "metrics.c"

static const luaL_Reg luv_functions[] = {
  // loop.c
  {"loop_close", luv_loop_close},
  {"run", luv_run},
  {"loop_mode", luv_loop_mode},
  {"loop_alive", luv_loop_alive},
  {"stop", luv_stop},
  {"backend_fd", luv_backend_fd},
  {"backend_timeout", luv_backend_timeout},
  {"now", luv_now},
  {"update_time", luv_update_time},
  {"walk", luv_walk},
#if LUV_UV_VERSION_GEQ(1, 0, 2)
  {"loop_configure", luv_loop_configure},
#endif

  // req.c
  {"cancel", luv_cancel},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"req_get_type", luv_req_get_type},
#endif

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
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"handle_get_type", luv_handle_get_type},
#endif

  // timer.c
  {"new_timer", luv_new_timer},
  {"timer_start", luv_timer_start},
  {"timer_stop", luv_timer_stop},
  {"timer_again", luv_timer_again},
  {"timer_set_repeat", luv_timer_set_repeat},
  {"timer_get_repeat", luv_timer_get_repeat},
#if LUV_UV_VERSION_GEQ(1, 40, 0)
  {"timer_get_due_in", luv_timer_get_due_in},
#endif

  // prepare.c
  {"new_prepare", luv_new_prepare},
  {"prepare_start", luv_prepare_start},
  {"prepare_stop", luv_prepare_stop},

  // check.c
  {"new_check", luv_new_check},
  {"check_start", luv_check_start},
  {"check_stop", luv_check_stop},

  // idle.c
  {"new_idle", luv_new_idle},
  {"idle_start", luv_idle_start},
  {"idle_stop", luv_idle_stop},

  // async.c
  {"new_async", luv_new_async},
  {"async_send", luv_async_send},

  // poll.c
  {"new_poll", luv_new_poll},
  {"new_socket_poll", luv_new_socket_poll},
  {"poll_start", luv_poll_start},
  {"poll_stop", luv_poll_stop},

  // signal.c
  {"new_signal", luv_new_signal},
  {"signal_start", luv_signal_start},
#if LUV_UV_VERSION_GEQ(1, 12, 0)
  {"signal_start_oneshot", luv_signal_start_oneshot},
#endif
  {"signal_stop", luv_signal_stop},

  // process.c
  {"disable_stdio_inheritance", luv_disable_stdio_inheritance},
  {"spawn", luv_spawn},
  {"process_kill", luv_process_kill},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"process_get_pid", luv_process_get_pid},
#endif
  {"kill", luv_kill},

  // stream.c
  {"shutdown", luv_shutdown},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"read_start", luv_read_start},
  {"read_stop", luv_read_stop},
  {"write", luv_write},
  {"write2", luv_write2},
  {"try_write", luv_try_write},
  {"is_readable", luv_is_readable},
  {"is_writable", luv_is_writable},
  {"stream_set_blocking", luv_stream_set_blocking},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"stream_get_write_queue_size", luv_stream_get_write_queue_size},
#endif

  // tcp.c
  {"new_tcp", luv_new_tcp},
  {"tcp_open", luv_tcp_open},
  {"tcp_nodelay", luv_tcp_nodelay},
  {"tcp_keepalive", luv_tcp_keepalive},
  {"tcp_simultaneous_accepts", luv_tcp_simultaneous_accepts},
  {"tcp_bind", luv_tcp_bind},
  {"tcp_getpeername", luv_tcp_getpeername},
  {"tcp_getsockname", luv_tcp_getsockname},
  {"tcp_connect", luv_tcp_connect},
  {"tcp_write_queue_size", luv_write_queue_size},
#if LUV_UV_VERSION_GEQ(1, 32, 0)
  {"tcp_close_reset", luv_tcp_close_reset},
#endif
#if LUV_UV_VERSION_GEQ(1, 41, 0)
  {"socketpair", luv_socketpair},
#endif

  // pipe.c
  {"new_pipe", luv_new_pipe},
  {"pipe_open", luv_pipe_open},
  {"pipe_bind", luv_pipe_bind},
#if LUV_UV_VERSION_GEQ(1, 16, 0)
  {"pipe_chmod", luv_pipe_chmod},
#endif
  {"pipe_connect", luv_pipe_connect},
  {"pipe_getsockname", luv_pipe_getsockname},
#if LUV_UV_VERSION_GEQ(1, 3, 0)
  {"pipe_getpeername", luv_pipe_getpeername},
#endif
  {"pipe_pending_instances", luv_pipe_pending_instances},
  {"pipe_pending_count", luv_pipe_pending_count},
  {"pipe_pending_type", luv_pipe_pending_type},
#if LUV_UV_VERSION_GEQ(1, 41, 0)
  {"pipe", luv_pipe},
#endif

  // tty.c
  {"new_tty", luv_new_tty},
  {"tty_set_mode", luv_tty_set_mode},
  {"tty_reset_mode", luv_tty_reset_mode},
  {"tty_get_winsize", luv_tty_get_winsize},
#if LUV_UV_VERSION_GEQ(1, 33, 0)
  {"tty_set_vterm_state", luv_tty_set_vterm_state},
  {"tty_get_vterm_state", luv_tty_get_vterm_state},
#endif

  // udp.c
  {"new_udp", luv_new_udp},
  {"udp_get_send_queue_size", luv_udp_get_send_queue_size},
  {"udp_get_send_queue_count", luv_udp_get_send_queue_count},
  {"udp_open", luv_udp_open},
  {"udp_bind", luv_udp_bind},
  {"udp_getsockname", luv_udp_getsockname},
  {"udp_set_membership", luv_udp_set_membership},
#if LUV_UV_VERSION_GEQ(1, 32, 0)
  {"udp_set_source_membership", luv_udp_set_source_membership},
#endif
  {"udp_set_multicast_loop", luv_udp_set_multicast_loop},
  {"udp_set_multicast_ttl", luv_udp_set_multicast_ttl},
  {"udp_set_multicast_interface", luv_udp_set_multicast_interface},
  {"udp_set_broadcast", luv_udp_set_broadcast},
  {"udp_set_ttl", luv_udp_set_ttl},
  {"udp_send", luv_udp_send},
  {"udp_try_send", luv_udp_try_send},
  {"udp_recv_start", luv_udp_recv_start},
  {"udp_recv_stop", luv_udp_recv_stop},
#if LUV_UV_VERSION_GEQ(1, 27, 0)
  {"udp_connect", luv_udp_connect},
  {"udp_getpeername", luv_udp_getpeername},
#endif

  // fs_event.c
  {"new_fs_event", luv_new_fs_event},
  {"fs_event_start", luv_fs_event_start},
  {"fs_event_stop", luv_fs_event_stop},
  {"fs_event_getpath", luv_fs_event_getpath},

  // fs_poll.c
  {"new_fs_poll", luv_new_fs_poll},
  {"fs_poll_start", luv_fs_poll_start},
  {"fs_poll_stop", luv_fs_poll_stop},
  {"fs_poll_getpath", luv_fs_poll_getpath},

  // fs.c
  {"fs_close", luv_fs_close},
  {"fs_open", luv_fs_open},
  {"fs_read", luv_fs_read},
  {"fs_unlink", luv_fs_unlink},
  {"fs_write", luv_fs_write},
  {"fs_mkdir", luv_fs_mkdir},
  {"fs_mkdtemp", luv_fs_mkdtemp},
#if LUV_UV_VERSION_GEQ(1, 34, 0)
  {"fs_mkstemp", luv_fs_mkstemp},
#endif
  {"fs_rmdir", luv_fs_rmdir},
  {"fs_scandir", luv_fs_scandir},
  {"fs_scandir_next", luv_fs_scandir_next},
  {"fs_stat", luv_fs_stat},
  {"fs_fstat", luv_fs_fstat},
  {"fs_lstat", luv_fs_lstat},
  {"fs_rename", luv_fs_rename},
  {"fs_fsync", luv_fs_fsync},
  {"fs_fdatasync", luv_fs_fdatasync},
  {"fs_ftruncate", luv_fs_ftruncate},
  {"fs_sendfile", luv_fs_sendfile},
  {"fs_access", luv_fs_access},
  {"fs_chmod", luv_fs_chmod},
  {"fs_fchmod", luv_fs_fchmod},
  {"fs_utime", luv_fs_utime},
  {"fs_futime", luv_fs_futime},
#if LUV_UV_VERSION_GEQ(1, 36, 0)
  {"fs_lutime", luv_fs_lutime},
#endif
  {"fs_link", luv_fs_link},
  {"fs_symlink", luv_fs_symlink},
  {"fs_readlink", luv_fs_readlink},
#if LUV_UV_VERSION_GEQ(1, 8, 0)
  {"fs_realpath", luv_fs_realpath},
#endif
  {"fs_chown", luv_fs_chown},
  {"fs_fchown", luv_fs_fchown},
#if LUV_UV_VERSION_GEQ(1, 21, 0)
  {"fs_lchown", luv_fs_lchown},
#endif
#if LUV_UV_VERSION_GEQ(1, 14, 0)
  {"fs_copyfile", luv_fs_copyfile },
#endif
#if LUV_UV_VERSION_GEQ(1, 28, 0)
  {"fs_opendir", luv_fs_opendir},
  {"fs_readdir", luv_fs_readdir},
  {"fs_closedir", luv_fs_closedir},
#endif
#if LUV_UV_VERSION_GEQ(1, 31, 0)
  {"fs_statfs", luv_fs_statfs},
#endif

  // dns.c
  {"getaddrinfo", luv_getaddrinfo},
  {"getnameinfo", luv_getnameinfo},

  // misc.c
  {"chdir", luv_chdir},
#if LUV_UV_VERSION_GEQ(1, 9, 0)
  {"os_homedir", luv_os_homedir},
  {"os_tmpdir", luv_os_tmpdir},
  {"os_get_passwd", luv_os_get_passwd},
#endif
  {"cpu_info", luv_cpu_info},
  {"cwd", luv_cwd},
  {"exepath", luv_exepath},
  {"get_process_title", luv_get_process_title},
#if LUV_UV_VERSION_GEQ(1, 29, 0)
  {"get_constrained_memory", luv_get_constrained_memory},
#endif
  {"get_total_memory", luv_get_total_memory},
  {"get_free_memory", luv_get_free_memory},
  {"getpid", luv_getpid},
#ifndef _WIN32
  {"getuid", luv_getuid},
  {"setuid", luv_setuid},
  {"getgid", luv_getgid},
  {"setgid", luv_setgid},
#endif
  {"getrusage", luv_getrusage},
  {"guess_handle", luv_guess_handle},
  {"hrtime", luv_hrtime},
  {"interface_addresses", luv_interface_addresses},
  {"loadavg", luv_loadavg},
  {"resident_set_memory", luv_resident_set_memory},
  {"set_process_title", luv_set_process_title},
  {"uptime", luv_uptime},
  {"version", luv_version},
  {"version_string", luv_version_string},
#ifndef _WIN32
#if LUV_UV_VERSION_GEQ(1, 8, 0)
  {"print_all_handles", luv_print_all_handles},
  {"print_active_handles", luv_print_active_handles},
#endif
#endif
#if LUV_UV_VERSION_GEQ(1, 12, 0)
  {"os_getenv", luv_os_getenv},
  {"os_setenv", luv_os_setenv},
  {"os_unsetenv", luv_os_unsetenv},
  {"os_gethostname", luv_os_gethostname},
#endif
#if LUV_UV_VERSION_GEQ(1, 16, 0)
  {"if_indextoname", luv_if_indextoname},
  {"if_indextoiid", luv_if_indextoiid},
  {"os_getppid", luv_os_getppid },
#endif
#if LUV_UV_VERSION_GEQ(1, 18, 0)
  {"os_getpid", luv_os_getpid},
#endif
#if LUV_UV_VERSION_GEQ(1, 23, 0)
  {"os_getpriority", luv_os_getpriority},
  {"os_setpriority", luv_os_setpriority},
#endif
#if LUV_UV_VERSION_GEQ(1, 25, 0)
  {"os_uname", luv_os_uname},
#endif
#if LUV_UV_VERSION_GEQ(1, 28, 0)
  {"gettimeofday", luv_gettimeofday},
#endif
#if LUV_UV_VERSION_GEQ(1, 31, 0)
  {"os_environ", luv_os_environ},
#endif
#if LUV_UV_VERSION_GEQ(1, 33, 0)
  {"random", luv_random},
#endif
  {"sleep", luv_sleep},

  // thread.c
  {"new_thread", luv_new_thread},
  {"thread_equal", luv_thread_equal},
  {"thread_self", luv_thread_self},
  {"thread_join", luv_thread_join},

  // work.c
  {"new_work", luv_new_work},
  {"queue_work", luv_queue_work},

  // util.c
#if LUV_UV_VERSION_GEQ(1, 10, 0)
  {"translate_sys_error", luv_translate_sys_error},
#endif

  // metrics.c
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  {"metrics_idle_time", luv_metrics_idle_time},
#endif

  {NULL, NULL}
};

static const luaL_Reg luv_handle_methods[] = {
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
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"get_type", luv_handle_get_type},
#endif
  {NULL, NULL}
};

static const luaL_Reg luv_async_methods[] = {
  {"send", luv_async_send},
  {NULL, NULL}
};

static const luaL_Reg luv_check_methods[] = {
  {"start", luv_check_start},
  {"stop", luv_check_stop},
  {NULL, NULL}
};

static const luaL_Reg luv_fs_event_methods[] = {
  {"start", luv_fs_event_start},
  {"stop", luv_fs_event_stop},
  {"getpath", luv_fs_event_getpath},
  {NULL, NULL}
};

static const luaL_Reg luv_fs_poll_methods[] = {
  {"start", luv_fs_poll_start},
  {"stop", luv_fs_poll_stop},
  {"getpath", luv_fs_poll_getpath},
  {NULL, NULL}
};

static const luaL_Reg luv_idle_methods[] = {
  {"start", luv_idle_start},
  {"stop", luv_idle_stop},
  {NULL, NULL}
};

static const luaL_Reg luv_stream_methods[] = {
  {"shutdown", luv_shutdown},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"read_start", luv_read_start},
  {"read_stop", luv_read_stop},
  {"write", luv_write},
  {"write2", luv_write2},
  {"try_write", luv_try_write},
  {"is_readable", luv_is_readable},
  {"is_writable", luv_is_writable},
  {"set_blocking", luv_stream_set_blocking},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"get_write_queue_size", luv_stream_get_write_queue_size},
#endif
  {NULL, NULL}
};

static const luaL_Reg luv_pipe_methods[] = {
  {"open", luv_pipe_open},
  {"bind", luv_pipe_bind},
#if LUV_UV_VERSION_GEQ(1, 16, 0)
  {"chmod", luv_pipe_chmod},
#endif
  {"connect", luv_pipe_connect},
  {"getsockname", luv_pipe_getsockname},
#if LUV_UV_VERSION_GEQ(1, 3, 0)
  {"getpeername", luv_pipe_getpeername},
#endif
  {"pending_instances", luv_pipe_pending_instances},
  {"pending_count", luv_pipe_pending_count},
  {"pending_type", luv_pipe_pending_type},
  {NULL, NULL}
};

static const luaL_Reg luv_poll_methods[] = {
  {"start", luv_poll_start},
  {"stop", luv_poll_stop},
  {NULL, NULL}
};

static const luaL_Reg luv_prepare_methods[] = {
  {"start", luv_prepare_start},
  {"stop", luv_prepare_stop},
  {NULL, NULL}
};

static const luaL_Reg luv_process_methods[] = {
  {"kill", luv_process_kill},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"get_pid", luv_process_get_pid},
#endif
  {NULL, NULL}
};

static const luaL_Reg luv_tcp_methods[] = {
  {"open", luv_tcp_open},
  {"nodelay", luv_tcp_nodelay},
  {"keepalive", luv_tcp_keepalive},
  {"simultaneous_accepts", luv_tcp_simultaneous_accepts},
  {"bind", luv_tcp_bind},
  {"getpeername", luv_tcp_getpeername},
  {"getsockname", luv_tcp_getsockname},
  {"connect", luv_tcp_connect},
  {"write_queue_size", luv_write_queue_size},
#if LUV_UV_VERSION_GEQ(1, 32, 0)
  {"close_reset", luv_tcp_close_reset},
#endif
  {NULL, NULL}
};

static const luaL_Reg luv_timer_methods[] = {
  {"start", luv_timer_start},
  {"stop", luv_timer_stop},
  {"again", luv_timer_again},
  {"set_repeat", luv_timer_set_repeat},
  {"get_repeat", luv_timer_get_repeat},
#if LUV_UV_VERSION_GEQ(1, 40, 0)
  {"get_due_in", luv_timer_get_due_in},
#endif
  {NULL, NULL}
};

static const luaL_Reg luv_tty_methods[] = {
  {"set_mode", luv_tty_set_mode},
  {"get_winsize", luv_tty_get_winsize},
  {NULL, NULL}
};

static const luaL_Reg luv_udp_methods[] = {
  {"get_send_queue_size", luv_udp_get_send_queue_size},
  {"get_send_queue_count", luv_udp_get_send_queue_count},
  {"open", luv_udp_open},
  {"bind", luv_udp_bind},
  {"getsockname", luv_udp_getsockname},
  {"set_membership", luv_udp_set_membership},
#if LUV_UV_VERSION_GEQ(1, 32, 0)
  {"set_source_membership", luv_udp_set_source_membership},
#endif
  {"set_multicast_loop", luv_udp_set_multicast_loop},
  {"set_multicast_ttl", luv_udp_set_multicast_ttl},
  {"set_multicast_interface", luv_udp_set_multicast_interface},
  {"set_broadcast", luv_udp_set_broadcast},
  {"set_ttl", luv_udp_set_ttl},
  {"send", luv_udp_send},
  {"try_send", luv_udp_try_send},
  {"recv_start", luv_udp_recv_start},
  {"recv_stop", luv_udp_recv_stop},
#if LUV_UV_VERSION_GEQ(1, 27, 0)
  {"connect", luv_udp_connect},
  {"getpeername", luv_udp_getpeername},
#endif
  {NULL, NULL}
};

static const luaL_Reg luv_signal_methods[] = {
  {"start", luv_signal_start},
  {"stop", luv_signal_stop},
  {NULL, NULL}
};

#if LUV_UV_VERSION_GEQ(1, 28, 0)
static const luaL_Reg luv_dir_methods[] = {
  {"readdir", luv_fs_readdir},
  {"closedir", luv_fs_closedir},
  {NULL, NULL}
};

static void luv_dir_init(lua_State* L) {
  luaL_newmetatable(L, "uv_dir");
  lua_pushcfunction(L, luv_fs_dir_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_fs_dir_gc);
  lua_setfield(L, -2, "__gc");
  luaL_newlib(L, luv_dir_methods);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);
}
#endif

static void luv_handle_init(lua_State* L) {

  lua_newtable(L);
#define XX(uc, lc)                             \
    luaL_newmetatable (L, "uv_"#lc);           \
    lua_pushcfunction(L, luv_handle_tostring); \
    lua_setfield(L, -2, "__tostring");         \
    lua_pushcfunction(L, luv_handle_gc);       \
    lua_setfield(L, -2, "__gc");               \
    luaL_newlib(L, luv_##lc##_methods);        \
    luaL_setfuncs(L, luv_handle_methods, 0);   \
    lua_setfield(L, -2, "__index");            \
    lua_pushboolean(L, 1);                     \
    lua_rawset(L, -3);

  UV_HANDLE_TYPE_MAP(XX)
#undef XX
  lua_setfield(L, LUA_REGISTRYINDEX, "uv_handle");

  lua_newtable(L);

  luaL_getmetatable(L, "uv_pipe");
  lua_getfield(L, -1, "__index");
  luaL_setfuncs(L, luv_stream_methods, 0);
  lua_pop(L, 1);
  lua_pushboolean(L, 1);
  lua_rawset(L, -3);

  luaL_getmetatable(L, "uv_tcp");
  lua_getfield(L, -1, "__index");
  luaL_setfuncs(L, luv_stream_methods, 0);
  lua_pop(L, 1);
  lua_pushboolean(L, 1);
  lua_rawset(L, -3);

  luaL_getmetatable(L, "uv_tty");
  lua_getfield(L, -1, "__index");
  luaL_setfuncs(L, luv_stream_methods, 0);
  lua_pop(L, 1);
  lua_pushboolean(L, 1);
  lua_rawset(L, -3);

  lua_setfield(L, LUA_REGISTRYINDEX, "uv_stream");
}

static const luaL_Reg luv_req_methods[] = {
  // req.c
  {"cancel", luv_cancel},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"get_type", luv_req_get_type},
#endif
  {NULL, NULL}
};

static void luv_req_init(lua_State* L) {
  luaL_newmetatable(L, "uv_req");
  lua_pushcfunction(L, luv_req_tostring);
  lua_setfield(L, -2, "__tostring");
  luaL_newlib(L, luv_req_methods);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);
}

// Call lua function, will pop nargs values from top of vm stack and push some
// values according to nresults. When error occurs, it will print error message
// to stderr, and memory allocation error will cause exit.
LUALIB_API int luv_cfpcall(lua_State* L, int nargs, int nresult, int flags) {
  int ret, top, errfunc;

  // Get the traceback function in case of error
  if ((flags & (LUVF_CALLBACK_NOTRACEBACK|LUVF_CALLBACK_NOERRMSG) ) == 0)
  {
    lua_pushcfunction(L, luv_traceback);
    errfunc = lua_gettop(L);
    // And insert it before the function and args
    lua_insert(L, -2 - nargs);
    errfunc -= (nargs+1);
  }else
    errfunc = 0;
  top  = lua_gettop(L);

  ret = lua_pcall(L, nargs, nresult, errfunc);
  switch (ret) {
  case LUA_OK:
    break;
  case LUA_ERRMEM:
    if ((flags & LUVF_CALLBACK_NOERRMSG) == 0)
      fprintf(stderr, "System Error: %s\n", lua_tostring(L, -1));
    if ((flags & LUVF_CALLBACK_NOEXIT) == 0)
      exit(-1);
    lua_pop(L, 1);
    ret = -ret;
    break;
  case LUA_ERRRUN:
  case LUA_ERRERR:
  default:
    if ((flags & LUVF_CALLBACK_NOERRMSG) == 0)
      fprintf(stderr, "Uncaught Error: %s\n", lua_tostring(L, -1));
    if ((flags & LUVF_CALLBACK_NOEXIT) == 0)
      exit(-1);
    lua_pop(L, 1);
    ret = -ret;
    break;
  }
  if ((flags & (LUVF_CALLBACK_NOTRACEBACK|LUVF_CALLBACK_NOERRMSG) ) == 0)
  {
    lua_remove(L, errfunc);
  }
  if (ret == LUA_OK) {
    if(nresult == LUA_MULTRET)
      nresult = lua_gettop(L) - top + nargs + 1;
    return nresult;
  }
  return ret;
}

// TODO: see if we can avoid using a string key for this to increase performance
static const char* luv_ctx_key = "luv_context";

// Please look at luv_ctx_t in luv.h
LUALIB_API luv_ctx_t* luv_context(lua_State* L) {
  luv_ctx_t* ctx;
  lua_pushstring(L, luv_ctx_key);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (lua_isnil(L, -1)) {
    // create it if not exist in registry
    lua_pushstring(L, luv_ctx_key);
    ctx = (luv_ctx_t*)lua_newuserdata(L, sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    lua_rawset(L, LUA_REGISTRYINDEX);
  } else {
    ctx = (luv_ctx_t*)lua_touserdata(L, -1);
  }
  lua_pop(L, 1);
  return ctx;
}

LUALIB_API lua_State* luv_state(lua_State* L) {
  return luv_context(L)->L;
}

LUALIB_API uv_loop_t* luv_loop(lua_State* L) {
  return luv_context(L)->loop;
}

// Set an external loop, before luaopen_luv
LUALIB_API void luv_set_loop(lua_State* L, uv_loop_t* loop) {
  luv_ctx_t* ctx = luv_context(L);

  ctx->loop = loop;
  ctx->L = L;
  ctx->mode = -1;
}

// Set an external event callback routine, before luaopen_luv
LUALIB_API void luv_set_callback(lua_State* L, luv_CFpcall pcall) {
  luv_ctx_t* ctx = luv_context(L);
  ctx->pcall = pcall;
}

static void walk_cb(uv_handle_t *handle, void *arg)
{
  (void)arg;
  if (!uv_is_closing(handle)) {
    uv_close(handle, luv_close_cb);
  }
}

static int loop_gc(lua_State *L) {
  luv_ctx_t *ctx = luv_context(L);
  uv_loop_t* loop = ctx->loop;
  if (loop==NULL)
    return 0;
  // Call uv_close on every active handle
  uv_walk(loop, walk_cb, NULL);
  // Run the event loop until all handles are successfully closed
  while (uv_loop_close(loop)) {
    uv_run(loop, UV_RUN_DEFAULT);
  }
  return 0;
}

LUALIB_API int luaopen_luv (lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);

  luaL_newlib(L, luv_functions);

  // loop is NULL, luv need to create an inner loop
  if (ctx->loop==NULL) {
    int ret;
    uv_loop_t* loop;

    // Setup the uv_loop meta table for a proper __gc
    luaL_newmetatable(L, "uv_loop.meta");
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, loop_gc);
    lua_settable(L, -3);
    lua_pop(L, 1);

    lua_pushstring(L, "_loop");
    loop = (uv_loop_t*)lua_newuserdata(L, sizeof(*loop));
    // setup the userdata's metatable for __gc
    luaL_getmetatable(L, "uv_loop.meta");
    lua_setmetatable(L, -2);
    // create a ref to loop, avoid __gc early
    // this puts the loop userdata into the _loop key
    // in the returned luv table
    lua_rawset(L, -3);

    ctx->loop = loop;
    ctx->L = L;
    ctx->mode = -1;

    ret = uv_loop_init(loop);
    if (ret < 0) {
      return luaL_error(L, "%s: %s\n", uv_err_name(ret), uv_strerror(ret));
    }
  }
  // pcall is NULL, luv use default callback routine
  if (ctx->pcall==NULL) {
    ctx->pcall = luv_cfpcall;
  }

  luv_req_init(L);
  luv_handle_init(L);
#if LUV_UV_VERSION_GEQ(1, 28, 0)
  luv_dir_init(L);
#endif
  luv_thread_init(L);
  luv_work_init(L);

  luv_constants(L);
  lua_setfield(L, -2, "constants");
  return 1;
}
