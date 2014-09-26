#ifndef LUV_USERDATA_H
#define LUV_USERDATA_H
#include "luv.h"

typedef struct {
  lua_State* L;
  int ref;
} luv_ref_t;

// luv_create_*(lua_State* L) {...}
// create a userdata, ref it globally and push on stack.

// loop type
static uv_loop_t* luv_create_loop(lua_State* L);

// Concrete handle types
static uv_async_t* luv_create_async(lua_State* L);
static uv_check_t* luv_create_check(lua_State* L);
static uv_fs_event_t* luv_create_fs_event(lua_State* L);
static uv_fs_poll_t* luv_create_fs_poll(lua_State* L);
static uv_idle_t* luv_create_idle(lua_State* L);
static uv_pipe_t* luv_create_pipe(lua_State* L);
static uv_poll_t* luv_create_poll(lua_State* L);
static uv_prepare_t* luv_create_prepare(lua_State* L);
static uv_process_t* luv_create_process(lua_State* L);
static uv_tcp_t* luv_create_tcp(lua_State* L);
static uv_timer_t* luv_create_timer(lua_State* L);
static uv_tty_t* luv_create_tty(lua_State* L);
static uv_udp_t* luv_create_udp(lua_State* L);
static uv_signal_t* luv_create_signal(lua_State* L);

// Concrete req types
static uv_connect_t* luv_create_connect(lua_State* L);
static uv_write_t* luv_create_write(lua_State* L);
static uv_shutdown_t* luv_create_shutdown(lua_State* L);
static uv_udp_send_t* luv_create_udp_send(lua_State* L);
static uv_fs_t* luv_create_fs(lua_State* L);
static uv_work_t* luv_create_work(lua_State* L);
static uv_getaddrinfo_t* luv_create_getaddrinfo(lua_State* L);
static uv_getnameinfo_t* luv_create_getnameinfo(lua_State* L);

// luv_check_*(lua_State* L, int index) {...}
// check a function argument and push on stack

// loop type
static uv_loop_t* luv_check_loop(lua_State* L, int index);

// All handle types
static uv_handle_t* luv_check_handle(lua_State* L, int index);
static uv_stream_t* luv_check_stream(lua_State* L, int index);
static uv_async_t* luv_check_async(lua_State* L, int index);
static uv_check_t* luv_check_check(lua_State* L, int index);
static uv_fs_event_t* luv_check_fs_event(lua_State* L, int index);
static uv_fs_poll_t* luv_check_fs_poll(lua_State* L, int index);
static uv_idle_t* luv_check_idle(lua_State* L, int index);
static uv_pipe_t* luv_check_pipe(lua_State* L, int index);
static uv_poll_t* luv_check_poll(lua_State* L, int index);
static uv_prepare_t* luv_check_prepare(lua_State* L, int index);
static uv_process_t* luv_check_process(lua_State* L, int index);
static uv_tcp_t* luv_check_tcp(lua_State* L, int index);
static uv_timer_t* luv_check_timer(lua_State* L, int index);
static uv_tty_t* luv_check_tty(lua_State* L, int index);
static uv_udp_t* luv_check_udp(lua_State* L, int index);
static uv_signal_t* luv_check_signal(lua_State* L, int index);

// Concrete req types
static uv_connect_t* luv_check_connect(lua_State* L, int index);
static uv_write_t* luv_check_write(lua_State* L, int index);
static uv_shutdown_t* luv_check_shutdown(lua_State* L, int index);
static uv_udp_send_t* luv_check_udp_send(lua_State* L, int index);
static uv_fs_t* luv_check_fs(lua_State* L, int index);
static uv_work_t* luv_check_work(lua_State* L, int index);
static uv_getaddrinfo_t* luv_check_getaddrinfo(lua_State* L, int index);
static uv_getnameinfo_t* luv_check_getnameinfo(lua_State* L, int index);


// luv_find_*(uv_*_t* ptr) -> lua_State* L
// Given a pointer push the userdata on the stack and return lua_State
static lua_State* luv_find_req(uv_req_t* req);
static lua_State* luv_find_handle(uv_handle_t* handle);

// luv_unref_*(uv_*_t* ptr) -> lua_State* L
// Given a pointer unref it's userdata return lua_State
// Also free and NULL it's `data` member.
static lua_State* luv_unref_loop(uv_loop_t* loop);
static lua_State* luv_unref_req(uv_req_t* req);
static lua_State* luv_unref_handle(uv_handle_t* handle);

#endif
