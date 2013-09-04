#ifndef LIB_LUV_COMMON
#define LIB_LUV_COMMON

#ifndef PATH_MAX
#define PATH_MAX (8096)
#endif

#ifndef MAX_TITLE_LENGTH
#define MAX_TITLE_LENGTH (8192)
#endif

#include <stdlib.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>

#include "uv.h"

#if LUA_VERSION_NUM < 502
/* lua_rawlen: Not entirely correct, but should work anyway */
#	define lua_rawlen lua_objlen
/* lua_...uservalue: Something very different, but it should get the job done */
#	define lua_getuservalue lua_getfenv
#	define lua_setuservalue lua_setfenv
#	define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#	define luaL_setfuncs(L,l,n) (assert(n==0), luaL_register(L,NULL,l))
#endif

/* Unique type codes for each uv type */
enum luv_type {
  LUV_HANDLE  = 0x01,
  LUV_TIMER   = 0x02,
  LUV_STREAM  = 0x04,
  LUV_TCP     = 0x08,
  LUV_TTY     = 0x10,
  LUV_PIPE    = 0x20,
  LUV_PROCESS = 0x40
};

/* Mask is the types that can be extracted from this concrete type */
enum luv_mask {
  LUV_TIMER_MASK    = LUV_HANDLE | LUV_TIMER,
  LUV_TCP_MASK      = LUV_HANDLE | LUV_STREAM | LUV_TCP,
  LUV_TTY_MASK      = LUV_HANDLE | LUV_STREAM | LUV_TTY,
  LUV_PIPE_MASK     = LUV_HANDLE | LUV_STREAM | LUV_PIPE,
  LUV_PROCESS_MASK  = LUV_HANDLE | LUV_PROCESS
};

/* luv handles are used as the userdata type that points to uv handles.
 * The luv handle is considered strong when it's "active" or has non-zero
 * reqCount.  When this happens ref will contain a luaL_ref to the userdata.
 */
typedef struct {
  uv_handle_t* handle; /* The actual uv handle. memory managed by luv */
  int refCount;        /* a count of all pending request to know strength */
  lua_State* L;        /* L and ref together form a reference to the userdata */
  int threadref;       /* hold reference to coroutine if created in one */
  int ref;             /* ref is null when refCount is 0 meaning we're weak */
  int mask;
} luv_handle_t;

typedef struct {
  luv_handle_t* lhandle;
  int data_ref;
  int callback_ref;
} luv_req_t;

typedef struct {
  lua_State* L;
  int ref;
} luv_callback_t;


lua_State* luv_main_thread;

uv_timer_t* luv_create_timer(lua_State* L);
uv_tcp_t* luv_create_tcp(lua_State* L);
uv_tty_t* luv_create_tty(lua_State* L);
uv_pipe_t* luv_create_pipe(lua_State* L);
uv_process_t* luv_create_process(lua_State* L);

uv_handle_t* luv_get_handle(lua_State* L, int index);
uv_timer_t* luv_get_timer(lua_State* L, int index);
uv_stream_t* luv_get_stream(lua_State* L, int index);
uv_tcp_t* luv_get_tcp(lua_State* L, int index);
uv_tty_t* luv_get_tty(lua_State* L, int index);
uv_pipe_t* luv_get_pipe(lua_State* L, int index);
uv_process_t* luv_get_process(lua_State* L, int index);

void luv_handle_ref(lua_State* L, luv_handle_t* lhandle, int index);
void luv_handle_unref(lua_State* L, luv_handle_t* lhandle);

lua_State* luv_prepare_event(luv_handle_t* lhandle);
lua_State* luv_prepare_callback(luv_req_t* lreq);
int luv_get_callback(lua_State* L, const char* name);
void luv_call (lua_State *L, int nargs, int nresults);

void luv_stack_dump(lua_State* L, int top, const char* name);

#endif
