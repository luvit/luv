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
#include "util.c"
#include "userdata.c"
#include "loop.c"
// #include "req.c"
#include "handle.c"
#include "timer.c"
// #include "prepare.c"
// #include "check.c"
// #include "idle.c"
// #include "async.c"
// #include "poll.c"
// #include "signal.c"
// #include "process.c"
// #include "stream.c"
// #include "tcp.c"

// #include "misc.c"
// #include "dns.c"
// #include "stream.c"
// #include "tty.c"
// #include "pipe.c"
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

  // req.c
  // {"cancel", luv_cancel},

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
  {"timer_get_repeat", luv_timer_get_repeat},

  // prepare.c
  // {"new_prepare", new_prepare},
  // {"prepare_start", luv_prepare_start},
  // {"prepare_stop", luv_prepare_stop},

  // // check.c
  // {"new_check", new_check},
  // {"check_start", luv_check_start},
  // {"check_stop", luv_check_stop},

  // // idle.c
  // {"new_idle", new_idle},
  // {"idle_start", luv_idle_start},
  // {"idle_stop", luv_idle_stop},

  // // async.c
  // {"new_async", new_async},
  // {"async_send", luv_async_send},

  // // poll.c
  // {"new_poll", new_poll},
  // {"poll_start", luv_poll_start},
  // {"poll_stop", luv_poll_stop},

  // // signal.c
  // {"new_signal", new_signal},
  // {"signal_start", luv_signal_start},
  // {"signal_stop", luv_signal_stop},

  // // process.c
  // {"disable_stdio_inheritance", luv_disable_stdio_inheritance},
  // {"spawn", luv_spawn},

  // // stream.c
  // {"shutdown_req", shutdown_req},
  // {"shutdown", luv_shutdown},
  // {"listen", luv_listen},
  // {"accept", luv_accept},
  // {"read_start", luv_read_start},
  // {"read_stop", luv_read_stop},
  // {"write_req", write_req},
  // {"write", luv_write},
  // {"write2", luv_write2},
  // {"try_write", luv_try_write},
  // {"is_readable", luv_is_readable},
  // {"is_writable", luv_is_writable},
  // {"stream_set_blocking", luv_stream_set_blocking},

  // // tcp.c
  // {"new_tcp", new_tcp},
  // {"tcp_open", luv_tcp_open},
  // {"tcp_nodelay", luv_tcp_nodelay},
  // {"tcp_keepalive", luv_tcp_keepalive},
  // {"tcp_simultaneous_accepts", luv_tcp_simultaneous_accepts},
  // {"tcp_bind", luv_tcp_bind},
  // {"tcp_getpeername", luv_tcp_getpeername},
  // {"tcp_getsockname", luv_tcp_getsockname},
  // {"connect_req", connect_req},
  // {"tcp_connect", luv_tcp_connect},

  {NULL, NULL}
};

LUALIB_API int luaopen_luv (lua_State *L) {

  util_init(L);
  loop_init(L);
  // req_init(L);
  handle_init(L);
  luaL_newlib(L, luv_functions);
  return 1;
}
