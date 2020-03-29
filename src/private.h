#ifndef LUV_PRIVATE_H
#define LUV_PRIVATE_H

#include <lua.h>
#if (LUA_VERSION_NUM != 503)
#include "compat-5.3.h"
#endif

#include "luv.h"
#include "util.h"
#include "lhandle.h"
#include "lreq.h"
#include "lthreadpool.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

/* From stream.c */
static uv_stream_t* luv_check_stream(lua_State* L, int index);
static void luv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

/* From lhandle.c */
/* Traceback for lua_pcall */
static int luv_traceback (lua_State *L);

/* Setup the handle at the top of the stack */
static luv_handle_t* luv_setup_handle(lua_State* L, luv_ctx_t* ctx);

/* Store a lua callback in a luv_handle for future callbacks.
   Either replace an existing callback by id or append a new one at the end.
*/
static void luv_check_callback(lua_State* L, luv_handle_t* data, luv_callback_id id, int index);

/* Lookup a function and call it with nargs
   If there is no such function, pop the args.
*/
static void luv_call_callback(lua_State* L, luv_handle_t* data, luv_callback_id id, int nargs);

/* Push a userdata on the stack from a handle */
static void luv_find_handle(lua_State* L, luv_handle_t* data);

/* Unref the handle from the lua world, allowing it to GC */
static void luv_unref_handle(lua_State* L, luv_handle_t* data);

/* From lreq.c */
/* Used in the top of a setup function to check the arg
   and ref the callback to an integer.
*/
static int luv_check_continuation(lua_State* L, int index);

/* setup a luv_req_t.  The userdata is assumed to be at the
   top of the stack.
*/
static luv_req_t* luv_setup_req(lua_State* L, luv_ctx_t* ctx, int ref);
static void luv_fulfill_req(lua_State* L, luv_req_t* data, int nargs);
static void luv_cleanup_req(lua_State* L, luv_req_t* data);

/* From handle.c */
static void* luv_checkudata(lua_State* L, int ud, const char* tname);
static void* luv_newuserdata(lua_State* L, size_t sz);


/* From misc.c */
static void luv_prep_buf(lua_State *L, int idx, uv_buf_t *pbuf);
static uv_buf_t* luv_prep_bufs(lua_State* L, int index, size_t *count, int **refs);
static uv_buf_t* luv_check_bufs(lua_State* L, int index, size_t *count, luv_req_t* req_data);
static uv_buf_t* luv_check_bufs_noref(lua_State* L, int index, size_t *count);

/* From tcp.c */
static void parse_sockaddr(lua_State* L, struct sockaddr_storage* address);
static void luv_connect_cb(uv_connect_t* req, int status);

/* From fs.c */
static void luv_push_stats_table(lua_State* L, const uv_stat_t* s);

/* From constants.c */
static int luv_af_string_to_num(const char* string);
static const char* luv_af_num_to_string(const int num);
static int luv_sock_string_to_num(const char* string);
static const char* luv_sock_num_to_string(const int num);
static int luv_sig_string_to_num(const char* string);
static const char* luv_sig_num_to_string(const int num);

/* From util.c */
// Push a Libuv error code onto the Lua stack
static int luv_error(lua_State* L, int status);

// Common error handling pattern for binding uv functions that only return success/error.
// If the binding returns a value other than success/error, this function should not be used.
static int luv_result(lua_State* L, int status);

// Push the error name onto the stack if status is an error code,
// or push nil onto the stack if it's not an error code
static void luv_status(lua_State* L, int status);

// Return true if the object is a function or a callable table
static int luv_is_callable(lua_State* L, int index);

// Check if the argument is callable and throw an error if it's not
static void luv_check_callable(lua_State* L, int index);

static int luv_optboolean(lua_State*L, int idx, int defaultval);

/* From thread.c */
static lua_State* luv_thread_acquire_vm();

/* From work.c */
static const char* luv_thread_dumped(lua_State* L, int idx, size_t* l);
static const char* luv_getmtname(lua_State *L, int idx);
static int luv_thread_arg_set(lua_State* L, luv_thread_arg_t* args, int idx, int top, int flags);
static int luv_thread_arg_push(lua_State* L, luv_thread_arg_t* args, int flags);
static void luv_thread_arg_clear(lua_State* L, luv_thread_arg_t* args, int flags);

static luv_acquire_vm acquire_vm_cb = NULL;
static luv_release_vm release_vm_cb = NULL;

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
