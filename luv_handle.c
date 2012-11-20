#ifndef LIB_LUV_HANDLE
#define LIB_LUV_HANDLE
#include "common.h"

static void on_close(uv_handle_t* handle) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (luv_get_callback(L, -1, "onclose")) {
    lua_call(L, 1, 0);
  }
  lua_pop(L, 1);
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
