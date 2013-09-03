#include <stdlib.h>
#include <assert.h>
#include "common.h"

/* Initialize a new lhandle and push the new userdata on the stack. */
static luv_handle_t* luv_handle_create(lua_State* L, size_t size, int mask) {

  /* Create the userdata and set it's metatable */
  luv_handle_t* lhandle = (luv_handle_t*)lua_newuserdata(L, sizeof(luv_handle_t));
  luaL_getmetatable(L, "luv_handle");
  lua_setmetatable(L, -2);

  /* Create a local environment for storing stuff */
  lua_newtable(L);
  lua_setuservalue(L, -2);

  /* Initialize and return the lhandle */
  lhandle->handle = (uv_handle_t*)malloc(size);
  lhandle->handle->data = lhandle; /* Point back to lhandle from handle */
  lhandle->refCount = 0;
  lhandle->ref = LUA_NOREF;
  lhandle->mask = mask;
  lhandle->L = L;
//  printf("Created lhandle %p handle %p\n", lhandle, lhandle->handle);

  /* if handle create in a coroutine, we need hold the coroutine */
  if (lua_pushthread(L)) {
    // L is mainthread
    lua_pop(L, 1);
    lhandle->threadref = LUA_NOREF;
  } else {
    lhandle->threadref = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  return lhandle;
}

uv_timer_t* luv_create_timer(lua_State* L) {
  luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_timer_t), LUV_TIMER_MASK);
  return (uv_timer_t*)lhandle->handle;
}

uv_tcp_t* luv_create_tcp(lua_State* L) {
  luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_tcp_t), LUV_TCP_MASK);
  return (uv_tcp_t*)lhandle->handle;
}

uv_tty_t* luv_create_tty(lua_State* L) {
  luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_tty_t), LUV_TTY_MASK);
  return (uv_tty_t*)lhandle->handle;
}

uv_pipe_t* luv_create_pipe(lua_State* L) {
  luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_pipe_t), LUV_PIPE_MASK);
  return (uv_pipe_t*)lhandle->handle;
}

uv_process_t* luv_create_process(lua_State* L) {
  luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_process_t), LUV_PROCESS_MASK);
  return (uv_process_t*)lhandle->handle;
}

static luv_handle_t* luv_get_lhandle(lua_State* L, int index, int type) {
  luaL_checktype(L, index, LUA_TUSERDATA);
  luv_handle_t* lhandle = (luv_handle_t*)luaL_checkudata(L, index, "luv_handle");
  if ((lhandle->mask & type) == 0) {
    luaL_error(L, "Invalid type for userdata %d not in %d", type, lhandle->mask);
  }
  return lhandle;
}

uv_handle_t* luv_get_handle(lua_State* L, int index) {
  luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_HANDLE);
  return (uv_handle_t*)lhandle->handle;
}

uv_timer_t* luv_get_timer(lua_State* L, int index) {
  luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_TIMER);
  return (uv_timer_t*)lhandle->handle;
}

uv_stream_t* luv_get_stream(lua_State* L, int index) {
  luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_STREAM);
  return (uv_stream_t*)lhandle->handle;
}

uv_tcp_t* luv_get_tcp(lua_State* L, int index) {
  luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_TCP);
  return (uv_tcp_t*)lhandle->handle;
}

uv_tty_t* luv_get_tty(lua_State* L, int index) {
  luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_TTY);
  return (uv_tty_t*)lhandle->handle;
}

uv_pipe_t* luv_get_pipe(lua_State* L, int index) {
  luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_PIPE);
  return (uv_pipe_t*)lhandle->handle;
}

uv_process_t* luv_get_process(lua_State* L, int index) {
  luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_PROCESS);
  return (uv_process_t*)lhandle->handle;
}

/* This needs to be called when an async function is started on a lhandle. */
void luv_handle_ref(lua_State* L, luv_handle_t* lhandle, int index) {
//  printf("luv_handle_ref\t%d %p:%p\n", lhandle->mask, lhandle, lhandle->handle);
  /* If it's inactive, store a ref. */
  if (!lhandle->refCount) {
    lua_pushvalue(L, index);
    lhandle->ref = luaL_ref(L, LUA_REGISTRYINDEX);
//    printf("makeStrong\t%d lhandle=%p handle=%p\n", lhandle->mask, lhandle, lhandle->handle);
  }
  lhandle->refCount++;
}

/* This needs to be called when an async callback fires on a lhandle. */
void luv_handle_unref(lua_State* L, luv_handle_t* lhandle) {
//  printf("luv_handle_unref\t%d %p:%p\n", lhandle->mask, lhandle, lhandle->handle);
  lhandle->refCount--;
  assert(lhandle->refCount >= 0);
  /* If it's now inactive, clear the ref */
  if (!lhandle->refCount) {
    luaL_unref(L, LUA_REGISTRYINDEX, lhandle->ref);
    if (lhandle->threadref != LUA_NOREF) {
      luaL_unref(L, LUA_REGISTRYINDEX, lhandle->threadref);
      lhandle->threadref = LUA_NOREF;
    }
    lhandle->ref = LUA_NOREF;
//    printf("makeWeak\t%d lhandle=%p handle=%p\n", lhandle->mask, lhandle, lhandle->handle);
  }
}

// Get L from a lhandle (moving to the main thread if needed) and push the userdata on the stack.
lua_State* luv_prepare_event(luv_handle_t* lhandle) {
  lua_State* L = lhandle->L;
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  assert(lhandle->refCount); /* sanity check */
  assert(lhandle->ref != LUA_NOREF); /* the ref should be there */
  lua_rawgeti(L, LUA_REGISTRYINDEX, lhandle->ref);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return L;
}

// Get a named callback.  If it's there, push the function and the userdata on the stack.
// otherwise leave the stack clean and return 0
int luv_get_callback(lua_State* L, int index, const char* name) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  /* Get the connection handler */
  lua_getuservalue(L, index);
  lua_getfield(L, -1, name);
  lua_remove(L, -2);

  int isfunc = lua_isfunction(L, -1);
  if (isfunc) {
    if (index < 0) index--; // Relative indexes need adjusting
    lua_pushvalue(L, index);
  } else {
    lua_pop(L, 1); // Remove the non function from the stack.
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + (isfunc ? 2 : 0));
#endif
  return isfunc;
}

/* returns the lua_State* and pushes the callback onto the stack */
/* STACK +1 */
lua_State* luv_prepare_callback(luv_req_t* lreq) {
  luv_handle_t* lhandle = lreq->lhandle;
  lua_State* L = lhandle->L;
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  lua_rawgeti(L, LUA_REGISTRYINDEX, lreq->callback_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, lreq->data_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, lreq->callback_ref);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return L;
}

void luv_call(lua_State *C, int nargs, int nresults) {
  lua_State* L = luv_main_thread;
  if (L != C) {
    // Move the function call to the main thread if it's in a coroutine
    lua_xmove(C, L, 1 + nargs);
  }
  lua_call(L, nargs, nresults);
}
