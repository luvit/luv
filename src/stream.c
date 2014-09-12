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

static uv_buf_t luv_on_alloc(uv_handle_t* handle, size_t suggested_size) {
  uv_buf_t buf;
  buf.base = malloc(suggested_size);
  buf.len = suggested_size;
  return buf;
}

static void luv_on_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (nread >= 0) {
    if (luv_get_callback(L, "ondata")) {
      lua_pushlstring (L, buf.base, nread);
      luv_call(L, 2, 0);
    }
  } else {
    uv_err_t err = uv_last_error(uv_default_loop());
    if (err.code == UV_EOF) {
      if (luv_get_callback(L, "onend")) {
        luv_call(L, 1, 0);
      }
    } else if (err.code != UV_ECONNRESET) {
      uv_close((uv_handle_t*)handle, NULL);
      if (luv_get_callback(L, "onerror")) {
        lua_pushstring(L, uv_strerror(err));
        luv_call(L, 2, 0);
      }
    }
  }

  free(buf.base);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static void luv_on_read2(uv_pipe_t* handle, ssize_t nread, uv_buf_t buf, uv_handle_type pending) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (nread >= 0) {
    if (luv_get_callback(L, "ondata")) {
      lua_pushlstring(L, buf.base, nread);
      lua_pushstring(L, type_to_string(pending));
      luv_call(L, 3, 0);
    }
  } else {
    uv_err_t err = uv_last_error(uv_default_loop());
    if (err.code == UV_EOF) {
      if (luv_get_callback(L, "onend")) {
        luv_call(L, 1, 0);
      }
    } else if (err.code != UV_ECONNRESET) {
      uv_close((uv_handle_t*)handle, NULL);
      if (luv_get_callback(L, "onerror")) {
        lua_pushstring(L, uv_strerror(err));
        luv_call(L, 2, 0);
      }
    }
  }

  free(buf.base);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static void luv_on_connection(uv_stream_t* handle, int status) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (luv_get_callback(L, "onconnection")) {
    luv_call(L, 1, 0);
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static void luv_after_write(uv_write_t* req, int status) {
  lua_State* L = luv_prepare_callback(req->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (lua_isfunction(L, -1)) {
    if (status == -1) {
      uv_err_t err = uv_last_error(uv_default_loop());
      lua_pushstring(L, uv_strerror(err));
    } else {
      lua_pushnil(L);
    }
    luv_call(L, 1, 0);
  } else {
    lua_pop(L, 1);
  }

  luv_handle_unref(L, req->handle->data);
  free(req->data);
  free(req);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static void luv_after_shutdown(uv_shutdown_t* req, int status) {
  lua_State* L = luv_prepare_callback(req->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (lua_isfunction(L, -1)) {
    luv_call(L, 0, 0);
  } else {
    lua_pop(L, 1);
  }

  luv_handle_unref(L, req->handle->data);
  free(req->data);
  free(req);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static int luv_read_start(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);
  uv_read_start(handle, luv_on_alloc, luv_on_read);
  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_read2_start(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);
  uv_read2_start(handle, luv_on_alloc, luv_on_read2);
  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_read_stop(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);
  luv_handle_unref(L, handle->data);
  uv_read_stop(handle);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_listen(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);
  int backlog_size = luaL_optint(L, 2, 128);

  if (uv_listen(handle, backlog_size, luv_on_connection)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "listen: %s", uv_strerror(err));
  }

  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_accept(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* server = luv_get_stream(L, 1);
  uv_stream_t* client = luv_get_stream(L, 2);
  if (uv_accept(server, client)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "accept: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_write(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);

  uv_write_t* req = malloc(sizeof(*req));
  luv_req_t* lreq = malloc(sizeof(*lreq));

  req->data = (void*)lreq;

  lreq->lhandle = handle->data;

  /* Reference the string in the registry */
  lua_pushvalue(L, 2);
  lreq->data_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  /* Reference the callback in the registry */
  lua_pushvalue(L, 3);
  lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_handle_ref(L, handle->data, 1);

  if (lua_istable(L, 2)) {
    int length, i;
    uv_buf_t* bufs;
    length = lua_rawlen(L, 2);
    bufs = malloc(sizeof(uv_buf_t) * length);
    for (i = 0; i < length; i++) {
      size_t len;
      const char* chunk;
      lua_rawgeti(L, 2, i + 1);
      chunk = luaL_checklstring(L, -1, &len);
      bufs[i] = uv_buf_init((char*)chunk, len);
      lua_pop(L, 1);
    }
    uv_write(req, handle, bufs, length, luv_after_write);
    /* TODO: find out if it's safe to free this soon */
    free(bufs);
  }
  else {
    size_t len;
    const char* chunk = luaL_checklstring(L, 2, &len);
    uv_buf_t buf = uv_buf_init((char*)chunk, len);
    uv_write(req, handle, &buf, 1, luv_after_write);
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_write2(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);

  uv_write_t* req = malloc(sizeof(*req));
  luv_req_t* lreq = malloc(sizeof(*lreq));

  uv_stream_t* send_handle;

  req->data = (void*)lreq;

  lreq->lhandle = handle->data;

  /* Reference the string in the registry */
  lua_pushvalue(L, 2);
  lreq->data_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  /* Get the stream handle to send */
  send_handle = luv_get_stream(L, 3);

  /* Reference the callback in the registry */
  lua_pushvalue(L, 4);
  lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_handle_ref(L, handle->data, 1);

  if (lua_istable(L, 2)) {
    int length, i;
    uv_buf_t* bufs;
    length = lua_rawlen(L, 2);
    bufs = malloc(sizeof(uv_buf_t) * length);
    for (i = 0; i < length; i++) {
      size_t len;
      const char* chunk;
      lua_rawgeti(L, 2, i + 1);
      chunk = luaL_checklstring(L, -1, &len);
      bufs[i] = uv_buf_init((char*)chunk, len);
      lua_pop(L, 1);
    }
    uv_write2(req, handle, bufs, length, send_handle, luv_after_write);
    /* TODO: find out if it's safe to free this soon */
    free(bufs);
  }
  else {
    size_t len;
    const char* chunk = luaL_checklstring(L, 2, &len);
    uv_buf_t buf = uv_buf_init((char*)chunk, len);
    uv_write2(req, handle, &buf, 1, send_handle, luv_after_write);
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_shutdown(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);

  uv_shutdown_t* req = malloc(sizeof(*req));
  luv_req_t* lreq = malloc(sizeof(*lreq));

  req->data = (void*)lreq;

  lreq->lhandle = handle->data;
  lreq->data_ref = LUA_NOREF;
  lua_pushvalue(L, 2);
  lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_handle_ref(L, handle->data, 1);

  uv_shutdown(req, handle, luv_after_shutdown);

#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_is_readable(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);
  lua_pushboolean(L, uv_is_readable(handle));
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}

static int luv_is_writable(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);
  lua_pushboolean(L, uv_is_writable(handle));
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}
