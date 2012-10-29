#ifndef LIB_LUV_COMMON
#define LIB_LUV_COMMON

#include <lua.h>
#include <lauxlib.h>
#include "uv.h"
#include "stdlib.h"
#include "assert.h"


/* Unique type codes for each uv type */
enum luv_type {
  LUV_HANDLE = 1,
  LUV_TIMER  = 2,
  LUV_STREAM = 4,
  LUV_TCP    = 8
};

/* Mask is the types that can be extracted from this concrete type */
enum luv_mask {
  LUV_TIMER_MASK = LUV_HANDLE | LUV_TIMER,
  LUV_TCP_MASK   = LUV_HANDLE | LUV_STREAM | LUV_TCP
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

void luv_setfuncs(lua_State *L, const luaL_Reg *l);

lua_State* luv_main_thread;

uv_timer_t* luv_create_timer(lua_State* L);
uv_tcp_t* luv_create_tcp(lua_State* L);

uv_handle_t* luv_get_handle(lua_State* L, int index);
uv_timer_t* luv_get_timer(lua_State* L, int index);
uv_stream_t* luv_get_stream(lua_State* L, int index);
uv_tcp_t* luv_get_tcp(lua_State* L, int index);

void luv_handle_ref(lua_State* L, luv_handle_t* lhandle, int index, const char *TAG);
void luv_handle_unref(lua_State* L, luv_handle_t* lhandle, const char *TAG);

lua_State* luv_prepare_event(luv_handle_t* lhandle);
lua_State* luv_prepare_callback(luv_req_t* lreq);
int luv_get_callback(lua_State* L, int index, const char* name);


#endif
