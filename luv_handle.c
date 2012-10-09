#ifndef LIB_LUV_HANDLE
#define LIB_LUV_HANDLE
#include "common.h"

static void on_close(uv_handle_t* handle) {
  fprintf(stderr, "on_close \tlhandle=%p handle=%p\n", handle->data, handle);
}

static int luv_close(lua_State* L) {
  uv_handle_t* handle = luv_get_handle(L, 1);
  fprintf(stderr, "close \tlhandle=%p handle=%p\n", handle->data, handle);

  if (uv_is_closing(handle)) {
    fprintf(stderr, "WARNING: Handle already closing \tlhandle=%p handle=%p\n", handle->data, handle);
    return 0;
  }

  uv_close(handle, on_close);
  return 0;
}

static const luaL_reg luv_handle_m[] = {
  {"close", luv_close},
  {NULL, NULL}
};

#endif
