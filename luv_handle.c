#ifndef LIB_LUV_HANDLE
#define LIB_LUV_HANDLE
#include "common.h"

static void on_close(uv_handle_t* handle) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif

  luv_handle_unref(L, handle->data);

  lua_getfenv(L, -1);
  lua_getfield(L, -1, "onclose");
  lua_remove(L, -2);

  if (lua_isfunction(L, -1)) {
    lua_pushvalue(L, -2); // push self
    lua_call(L, 1, 0);
  } else {
    lua_pop(L, 1);
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static int luv_close(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_handle_t* handle = luv_get_handle(L, 1);
//  fprintf(stderr, "close \tlhandle=%p handle=%p\n", handle->data, handle);

  if (uv_is_closing(handle)) {
    fprintf(stderr, "WARNING: Handle already closing \tlhandle=%p handle=%p\n", handle->data, handle);
    return 0;
  }

  uv_close(handle, on_close);
  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static const luaL_reg luv_handle_m[] = {
  {"close", luv_close},
  {NULL, NULL}
};

#endif
