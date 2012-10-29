#ifndef LIB_LUV_STREAM
#define LIB_LUV_STREAM
#include "common.h"

static uv_buf_t luv_on_alloc(uv_handle_t* handle, size_t suggested_size) {
  uv_buf_t buf;
  buf.base = malloc(suggested_size);
  buf.len = suggested_size;
  return buf;
}

static void luv_on_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  lua_State* L = luv_prepare_event(handle->data);

  if (nread >= 0) {

    if (luv_get_callback(L, -1, "ondata")) {
      lua_pushlstring (L, buf.base, nread);
      lua_call(L, 2, 0);
    }

  } else {
    uv_err_t err = uv_last_error(uv_default_loop());
    uv_close((uv_handle_t*)handle, NULL);
    if (err.code == UV_EOF) {
      if (luv_get_callback(L, -1, "onend")) {
        lua_call(L, 1, 0);
      }
    } else if (err.code != UV_ECONNRESET) {
      fprintf(stderr, "TODO: Implement async error handling\n");
      assert(0);
    }
  }

  free(buf.base);
}

static void luv_on_connection(uv_stream_t* handle, int status) {
  lua_State* L = luv_prepare_event(handle->data);
  if (luv_get_callback(L, -1, "onconnection")) {
    lua_call(L, 1, 0);
  }
}

static void luv_after_write(uv_write_t* req, int status) {
  lua_State* L = luv_prepare_callback(req->data);
  if (lua_isfunction(L, -1)) {
     lua_call(L, 0, 0);
  } else {
    lua_pop(L, 1);
  }

  luv_handle_unref(L, req->handle->data, "AFTER_WRITE");
  free(req->data);
  free(req);
}

static void luv_after_shutdown(uv_shutdown_t* req, int status) {
  lua_State* L = luv_prepare_callback(req->data);
  if (lua_isfunction(L, -1)) {
     lua_call(L, 0, 0);
  } else {
    lua_pop(L, 1);
  }

  luv_handle_unref(L, req->handle->data, "AFTER_SHUTDOWN");
  free(req->data);
  free(req);
}

static int luv_read_start(lua_State* L) {
  uv_stream_t* handle = luv_get_stream(L, 1);
  uv_read_start(handle, luv_on_alloc, luv_on_read);
  luv_handle_ref(L, handle->data, 1, "READ_START");
  return 0;
}

static int luv_read_stop(lua_State* L) {
  uv_stream_t* handle = luv_get_stream(L, 1);
  luv_handle_unref(L, handle->data, "READ_STOP");
  uv_read_stop(handle);
  return 0;
}

static int luv_listen(lua_State* L) {
  uv_stream_t* handle = luv_get_stream(L, 1);
  int backlog_size = luaL_optint(L, 2, 128);

  if (uv_listen(handle, backlog_size, luv_on_connection)) {
    luaL_error(L, "Problem listening");
  }

  luv_handle_ref(L, handle->data, 1, "LISTEN");
  return 0;
}

static int luv_accept(lua_State* L) {
  uv_stream_t* server = luv_get_stream(L, 1);
  uv_stream_t* client = luv_get_stream(L, 2);
  if (uv_accept(server, client)) {
    luaL_error(L, "Problem accepting client");
  }
  return 0;
}

static int luv_write(lua_State* L) {
  uv_stream_t* handle = luv_get_stream(L, 1);

  size_t len;
  const char* chunk = luaL_checklstring(L, 2, &len);
  uv_buf_t buf = uv_buf_init((char*)chunk, len);

  uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
  luv_req_t* lreq = (luv_req_t*)malloc(sizeof(luv_req_t));

  req->data = (void*)lreq;

  lreq->lhandle = handle->data;

  // Reference the string in the registry
  lua_pushvalue(L, 2);
  lreq->data_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  // Reference the callback in the registry
  lua_pushvalue(L, 3);
  lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_handle_ref(L, handle->data, 1, "WRITE");

  uv_write(req, handle, &buf, 1, luv_after_write);
  return 0;
}

static int luv_shutdown(lua_State* L) {
  luv_read_stop(L);

  uv_stream_t* handle = luv_get_stream(L, 1);

  uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
  luv_req_t* lreq = (luv_req_t*)malloc(sizeof(luv_req_t));

  req->data = (void*)lreq;

  lreq->lhandle = handle->data;
  lreq->data_ref = LUA_NOREF;
  lua_pushvalue(L, 2);
  lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_handle_ref(L, handle->data, 1, "SHUTDOWN");

  uv_shutdown(req, handle, luv_after_shutdown);

  return 0;
}

static int luv_is_readable(lua_State* L) {
  uv_stream_t* handle = luv_get_stream(L, 1);
  lua_pushboolean(L, uv_is_readable(handle));
  return 1;
}

static int luv_is_writable(lua_State* L) {
  uv_stream_t* handle = luv_get_stream(L, 1);
  lua_pushboolean(L, uv_is_writable(handle));
  return 1;
}

static const luaL_reg luv_stream_m[] = {
  {"write", luv_write},
  {"shutdown", luv_shutdown},
  {"readStart", luv_read_start},
  {"readStop", luv_read_stop},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"write", luv_write},
  {"isReadable", luv_is_readable},
  {"isWritable", luv_is_writable},
  {NULL, NULL}
};

#endif

