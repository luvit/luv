#ifndef LIB_LUV_FUNCTIONS
#define LIB_LUV_FUNCTIONS
#include "common.h"

static int new_tcp(lua_State* L) {
  uv_tcp_t* handle = luv_create_tcp(L);
  if (uv_tcp_init(uv_default_loop(), handle)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "new_tcp: %s", uv_strerror(err));
  }
//  fprintf(stderr, "new tcp \tlhandle=%p handle=%p\n", handle->data, handle);
  return 1;
}

static int new_timer(lua_State* L) {
  uv_timer_t* handle = luv_create_timer(L);
  if (uv_timer_init(uv_default_loop(), handle)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "new_timer: %s", uv_strerror(err));
  }
//  fprintf(stderr, "new timer \tlhandle=%p handle=%p\n", handle->data, handle);
  return 1;
}

static int new_tty(lua_State* L) {
  uv_tty_t* handle = luv_create_tty(L);
  uv_file fd = luaL_checkint(L, 1);
  int readable = lua_toboolean(L, 2);
  if (uv_tty_init(uv_default_loop(), handle, fd, readable)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "new_tty: %s", uv_strerror(err));
  }
//  fprintf(stderr, "new timer \tlhandle=%p handle=%p\n", handle->data, handle);
  return 1;
}

static int new_pipe(lua_State* L) {
  uv_pipe_t* handle = luv_create_pipe(L);
  int ipc = lua_toboolean(L, 1);
  if (uv_pipe_init(uv_default_loop(), handle, ipc)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "new_pipe: %s", uv_strerror(err));
  }
  return 1;
}

static int luv_run_once(lua_State* L) {
  int ret = uv_run_once(uv_default_loop());
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_run(lua_State* L) {
  uv_run(uv_default_loop());
  return 0;
}

static int luv_guess_handle(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  switch (uv_guess_handle(file)) {
#define XX(uc, lc) case UV_##uc: lua_pushstring(L, #uc); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    case UV_FILE: lua_pushstring(L, "FILE"); break;
    default: lua_pushstring(L, "UNKNOWN"); break;
  }
  return 1;
}

static int luv_update_time(lua_State* L) {
  uv_update_time(uv_default_loop());
  return 0;
}

static int luv_now(lua_State* L) {
  lua_pushnumber(L, uv_now(uv_default_loop()));
  return 1;
}

/******************************************************************************/

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

static int luv_is_active(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_handle_t* handle = luv_get_handle(L, 1);
  lua_pushboolean(L, uv_is_active(handle));
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}

static void on_walk(uv_handle_t* handle, void* arg) {
  luv_callback_t* callback = (luv_callback_t*)arg;
  lua_State* L = callback->L;
  /* Get the callback and push the type */
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->ref);
  luv_handle_t* lhandle = (luv_handle_t*)handle->data;
  assert(L == lhandle->L); // Make sure the lua states match
  lua_rawgeti(L, LUA_REGISTRYINDEX, lhandle->ref);

  switch (lhandle->handle->type) {
#define XX(uc, lc) case UV_##uc: lua_pushfstring(L, "uv_%s_t: %p", #lc, lhandle->handle); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default: lua_pushfstring(L, "userdata: %p", lhandle->handle); break;
  }

  lua_call(L, 2, 0);
}

static int luv_walk(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  luaL_checktype(L, 1, LUA_TFUNCTION);
  luv_callback_t callback;
  /* Store the callback as a ref */
  callback.L = L;
  lua_pushvalue(L, 1);
  callback.ref = luaL_ref(L, LUA_REGISTRYINDEX);

  /* Walk the list */
  uv_walk(uv_default_loop(), on_walk, &callback);

  /* unref the callback */
  luaL_unref(L, LUA_REGISTRYINDEX, callback.ref);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_ref(lua_State* L) {
  uv_ref(luv_get_handle(L, 1));
  return 0;
}

static int luv_unref(lua_State* L) {
  uv_unref(luv_get_handle(L, 1));
  return 0;
}

static int luv_is_closing(lua_State* L) {
  lua_pushboolean(L, uv_is_closing(luv_get_handle(L, 1)));
  return 1;
}

/******************************************************************************/

static void on_timeout(uv_timer_t* handle, int status) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (luv_get_callback(L, -1, "ontimeout")) {
    lua_call(L, 1, 0);
  }
  lua_pop(L, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static int luv_timer_start(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_timer_t* handle = luv_get_timer(L, 1);
  int64_t timeout = luaL_checkinteger(L, 2);
  int64_t repeat = luaL_checkinteger(L, 3);
  if (uv_timer_start(handle, on_timeout, timeout, repeat)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "timer_start: %s", uv_strerror(err));
  }
  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_timer_stop(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_timer_t* handle = luv_get_timer(L, 1);
  luv_handle_unref(L, handle->data);
  if (uv_timer_stop(handle)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "timer_stop: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_timer_again(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_timer_t* handle = luv_get_timer(L, 1);
  if (uv_timer_again(handle)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "timer_again: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_timer_set_repeat(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_timer_t* handle = luv_get_timer(L, 1);
  int64_t repeat = luaL_checkint(L, 2);
  uv_timer_set_repeat(handle, repeat);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_timer_get_repeat(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_timer_t* handle = luv_get_timer(L, 1);
  lua_pushinteger(L, uv_timer_get_repeat(handle));
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}



/******************************************************************************/

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

    if (luv_get_callback(L, -1, "ondata")) {
      lua_pushlstring (L, buf.base, nread);
      lua_call(L, 2, 0);
    }

  } else {
    uv_err_t err = uv_last_error(uv_default_loop());
    if (err.code == UV_EOF) {
      if (luv_get_callback(L, -1, "onend")) {
        lua_call(L, 1, 0);
      }
    } else if (err.code != UV_ECONNRESET) {
      uv_close((uv_handle_t*)handle, NULL);
      /* TODO: route reset events somewhere so the user knows about them */
      fprintf(stderr, "TODO: Implement async error handling\n");
      assert(0);
    }
  }
  /* Release the userdata */
  lua_pop(L, 1);

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
  if (luv_get_callback(L, -1, "onconnection")) {
    lua_call(L, 1, 0);
  }
  lua_pop(L, 1);
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
    lua_call(L, 0, 0);
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
    lua_call(L, 0, 0);
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

  luv_handle_ref(L, handle->data, 1);

  if (lua_istable(L, 2)) {
    int length, i;
    uv_buf_t* bufs;
    length = lua_objlen(L, 2);
    bufs = (uv_buf_t*)malloc(sizeof(uv_buf_t) * length);
    for (i = 0; i < length; i++) {
      lua_rawgeti(L, 2, i + 1);
      size_t len;
      const char* chunk = luaL_checklstring(L, -1, &len);
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

static int luv_shutdown(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_stream_t* handle = luv_get_stream(L, 1);

  uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
  luv_req_t* lreq = (luv_req_t*)malloc(sizeof(luv_req_t));

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

/******************************************************************************/

static void luv_after_connect(uv_connect_t* req, int status) {
  lua_State* L = luv_prepare_callback(req->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (lua_isfunction(L, -1)) {
     lua_call(L, 0, 0);
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

static int luv_tcp_bind(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tcp_t* handle = luv_get_tcp(L, 1);
  const char* host = luaL_checkstring(L, 2);
  int port = luaL_checkint(L, 3);

  struct sockaddr_in address = uv_ip4_addr(host, port);

  if (uv_tcp_bind(handle, address)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tcp_bind: %s", uv_strerror(err));
  }

#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_tcp_getsockname(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tcp_t* handle = luv_get_tcp(L, 1);
  int port = 0;
  char ip[INET6_ADDRSTRLEN];
  int family;

  struct sockaddr_storage address;
  int addrlen = sizeof(address);

  if (uv_tcp_getsockname(handle, (struct sockaddr*)(&address), &addrlen)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tcp_getsockname: %s", uv_strerror(err));
  }

  family = address.ss_family;
  if (family == AF_INET) {
    struct sockaddr_in* addrin = (struct sockaddr_in*)&address;
    uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin->sin_port);
  } else if (family == AF_INET6) {
    struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)&address;
    uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin6->sin6_port);
  }

  lua_newtable(L);
  lua_pushnumber(L, port);
  lua_setfield(L, -2, "port");
  lua_pushnumber(L, family);
  lua_setfield(L, -2, "family");
  lua_pushstring(L, ip);
  lua_setfield(L, -2, "address");

#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}


static int luv_tcp_getpeername(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tcp_t* handle = luv_get_tcp(L, 1);
  int port = 0;
  char ip[INET6_ADDRSTRLEN];
  int family;

  struct sockaddr_storage address;
  int addrlen = sizeof(address);

  if (uv_tcp_getpeername(handle, (struct sockaddr*)(&address), &addrlen)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tcp_getpeername: %s", uv_strerror(err));
  }

  family = address.ss_family;
  if (family == AF_INET) {
    struct sockaddr_in* addrin = (struct sockaddr_in*)&address;
    uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin->sin_port);
  } else if (family == AF_INET6) {
    struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)&address;
    uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin6->sin6_port);
  }

  lua_newtable(L);
  lua_pushnumber(L, port);
  lua_setfield(L, -2, "port");
  lua_pushnumber(L, family);
  lua_setfield(L, -2, "family");
  lua_pushstring(L, ip);
  lua_setfield(L, -2, "address");

#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}

static int luv_tcp_connect(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tcp_t* handle = luv_get_tcp(L, 1);

  const char* ip_address = luaL_checkstring(L, 2);
  int port = luaL_checkint(L, 3);

  struct sockaddr_in address = uv_ip4_addr(ip_address, port);

  uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
  luv_req_t* lreq = (luv_req_t*)malloc(sizeof(luv_req_t));

  req->data = (void*)lreq;

  lreq->lhandle = handle->data;

  if (uv_tcp_connect(req, handle, address, luv_after_connect)) {
    free(req->data);
    free(req);
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tcp_connect: %s", uv_strerror(err));
  }

  lreq->data_ref = LUA_NOREF;
  lua_pushvalue(L, 4);
  lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_tcp_open(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tcp_t* handle = luv_get_tcp(L, 1);
  uv_os_sock_t sock = luaL_checkint(L, 2);
  if (uv_tcp_open(handle, sock)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tcp_open: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

/* Enable/disable Nagle's algorithm. */
static int luv_tcp_nodelay(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tcp_t* handle = luv_get_tcp(L, 1);
  luaL_checkany(L, 2);
  int enable = lua_toboolean(L, 2);
  if (uv_tcp_nodelay(handle, enable)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tcp_nodelay: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}



/* Enable/disable TCP keep-alive. */
static int luv_tcp_keepalive(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tcp_t* handle = luv_get_tcp(L, 1);
  luaL_checkany(L, 2);
  int enable = lua_toboolean(L, 2);
  unsigned int delay;
  if (enable) {
    delay = luaL_checkint(L, 3);
  }
  if (uv_tcp_keepalive(handle, enable, delay)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tcp_keepalive: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

/******************************************************************************/

static int luv_tty_set_mode(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tty_t* handle = luv_get_tty(L, 1);
  int mode = luaL_checkint(L, 2);
  if (uv_tty_set_mode(handle, mode)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tty_set_mode: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_tty_reset_mode(lua_State* L) {
  uv_tty_reset_mode();
  return 0;
}

static int luv_tty_get_winsize(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tty_t* handle = luv_get_tty(L, 1);
  int width, height;
  if(uv_tty_get_winsize(handle, &width, &height)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tty_get_winsize: %s", uv_strerror(err));
  }
  lua_pushinteger(L, width);
  lua_pushinteger(L, height);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 2);
#endif
  return 2;
}

/******************************************************************************/

static int luv_pipe_open(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_pipe_t* handle = luv_get_pipe(L, 1);
  uv_file file = luaL_checkint(L, 2);
  if (uv_pipe_open(handle, file)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "pipe_open: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_pipe_bind(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_pipe_t* handle = luv_get_pipe(L, 1);
  const char* name = luaL_checkstring(L, 2);
  if (uv_pipe_bind(handle, name)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "pipe_name: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_pipe_connect(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_pipe_t* handle = luv_get_pipe(L, 1);
  const char* name = luaL_checkstring(L, 2);

  uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
  luv_req_t* lreq = (luv_req_t*)malloc(sizeof(luv_req_t));
  req->data = (void*)lreq;
  lreq->lhandle = handle->data;

  uv_pipe_connect(req, handle, name, luv_after_connect);

  lreq->data_ref = LUA_NOREF;
  lua_pushvalue(L, 3);
  lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

/******************************************************************************/

static void luv_push_stats_table(lua_State* L, uv_statbuf_t* s) {
  lua_newtable(L);
  lua_pushinteger(L, s->st_dev);
  lua_setfield(L, -2, "dev");
  lua_pushinteger(L, s->st_ino);
  lua_setfield(L, -2, "ino");
  lua_pushinteger(L, s->st_mode);
  lua_setfield(L, -2, "mode");
  lua_pushinteger(L, s->st_nlink);
  lua_setfield(L, -2, "nlink");
  lua_pushinteger(L, s->st_uid);
  lua_setfield(L, -2, "uid");
  lua_pushinteger(L, s->st_gid);
  lua_setfield(L, -2, "gid");
  lua_pushinteger(L, s->st_rdev);
  lua_setfield(L, -2, "rdev");
  lua_pushinteger(L, s->st_size);
  lua_setfield(L, -2, "size");
  lua_pushinteger(L, s->st_atime);
  lua_setfield(L, -2, "atime");
  lua_pushinteger(L, s->st_mtime);
  lua_setfield(L, -2, "mtime");
  lua_pushinteger(L, s->st_ctime);
  lua_setfield(L, -2, "ctime");
  lua_pushboolean(L, S_ISREG(s->st_mode));
  lua_setfield(L, -2, "is_file");
  lua_pushboolean(L, S_ISDIR(s->st_mode));
  lua_setfield(L, -2, "is_directory");
  lua_pushboolean(L, S_ISCHR(s->st_mode));
  lua_setfield(L, -2, "is_character_device");
  lua_pushboolean(L, S_ISBLK(s->st_mode));
  lua_setfield(L, -2, "is_block_device");
  lua_pushboolean(L, S_ISFIFO(s->st_mode));
  lua_setfield(L, -2, "is_fifo");
  lua_pushboolean(L, S_ISLNK(s->st_mode));
  lua_setfield(L, -2, "is_symbolic_link");
  lua_pushboolean(L, S_ISSOCK(s->st_mode));
  lua_setfield(L, -2, "is_socket");
#ifdef __POSIX__
  lua_pushinteger(L, s->st_blksize);
  lua_setfield(L, -2, "blksize");
  lua_pushinteger(L, s->st_blocks);
  lua_setfield(L, -2, "blocks");
#endif
}

static int luv_string_to_flags(lua_State* L, const char* string) {
  if (strcmp(string, "r") == 0) return O_RDONLY;
  if (strcmp(string, "r+") == 0) return O_RDWR;
  if (strcmp(string, "w") == 0) return O_CREAT | O_TRUNC | O_WRONLY;
  if (strcmp(string, "w+") == 0) return O_CREAT | O_TRUNC | O_RDWR;
  if (strcmp(string, "a") == 0) return O_APPEND | O_CREAT | O_WRONLY;
  if (strcmp(string, "a+") == 0) return O_APPEND | O_CREAT | O_RDWR;
  return luaL_error(L, "Unknown file open flag '%s'", string);
}

// Processes a result and pushes the data onto the stack
// returns the number of items pushed
static int push_fs_result(lua_State* L, uv_fs_t* req) {

  switch (req->fs_type) {
    case UV_FS_CLOSE:
    case UV_FS_RENAME:
    case UV_FS_UNLINK:
    case UV_FS_RMDIR:
    case UV_FS_MKDIR:
    case UV_FS_FTRUNCATE:
    case UV_FS_FSYNC:
    case UV_FS_FDATASYNC:
    case UV_FS_LINK:
    case UV_FS_SYMLINK:
    case UV_FS_CHMOD:
    case UV_FS_FCHMOD:
    case UV_FS_CHOWN:
    case UV_FS_FCHOWN:
    case UV_FS_UTIME:
    case UV_FS_FUTIME:
      return 0;

    case UV_FS_OPEN:
    case UV_FS_SENDFILE:
    case UV_FS_WRITE:
      lua_pushinteger(L, req->result);
      return 1;

    case UV_FS_STAT:
    case UV_FS_LSTAT:
    case UV_FS_FSTAT:
      luv_push_stats_table(L, (uv_statbuf_t*)req->ptr);
      return 1;

    case UV_FS_READLINK:
      lua_pushstring(L, (char*)req->ptr);
      return 1;

    case UV_FS_READ:
      lua_pushlstring(L, req->ptr, req->result);
      return 1;

    case UV_FS_READDIR:
      {
        int i;
        char* namebuf = (char*)req->ptr;
        int nnames = req->result;

        lua_createtable(L, nnames, 0);
        for (i = 0; i < nnames; i++) {
          lua_pushstring(L, namebuf);
          lua_rawseti(L, -2, i + 1);
          namebuf += strlen(namebuf);
          assert(*namebuf == '\0');
          namebuf += 1;
        }
      }
      return 1;

    default:
      fprintf(stderr, "UNKNOWN FS TYPE %d\n", req->fs_type);
      return 0;
  }

}

// Pushes a formatted error string onto the stack
static void push_fs_error(lua_State* L, uv_fs_t* req) {
  uv_err_t err;
  memset(&err, 0, sizeof err);
  err.code = (uv_err_code)req->errorno;
  if (req->path) {
    lua_pushfstring(L, "%s: %s \"%s\"", uv_err_name(err), uv_strerror(err), req->path);
  }
  else {
    lua_pushfstring(L, "%s: %s", uv_err_name(err), uv_strerror(err));
  }
}

static void on_fs(uv_fs_t *req) {
  // Get the lua state
  luv_callback_t* callback = (luv_callback_t*)req->data;
  lua_State* L = callback->L;

  // Get the callback and push on the lua stack
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, callback->ref);
  free(callback);

  int argc;
  if (req->result == -1) {
    push_fs_error(L, req);
    argc = 1;
  }
  else {
    lua_pushnil(L);
    argc = 1 + push_fs_result(L, req);
  }

  // Cleanup the req
  uv_fs_req_cleanup(req);
  free(req);

  lua_call(L, argc, 0);
}

#define FS_CALL(func, index, ...)                                              \
  uv_fs_t* req = (uv_fs_t*)malloc(sizeof(uv_fs_t));                            \
  if (lua_isnone(L, index)) {                                                  \
    if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, NULL) < 0) {         \
      push_fs_error(L, req);                                                   \
      uv_fs_req_cleanup(req);                                                  \
      free(req);                                                               \
      return lua_error(L);                                                     \
    }                                                                          \
    int argc = push_fs_result(L, req);                                         \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    return argc;                                                               \
  }                                                                            \
  luaL_checktype(L, index, LUA_TFUNCTION);                                     \
  luv_callback_t* callback = (luv_callback_t*)malloc(sizeof(luv_callback_t));  \
  callback->L = L;                                                             \
  lua_pushvalue(L, index);                                                     \
  callback->ref = luaL_ref(L, LUA_REGISTRYINDEX);                              \
  req->data = (void*)callback;                                                 \
  if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, on_fs) < 0) {          \
    push_fs_error(L, req);                                                     \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    return lua_error(L);                                                       \
  }                                                                            \
  return 0;                                                                    \

// HACK: Hacked version that patches req->ptr to hold buffer
// TODO: get this into libuv itself, there is no reason it couldn't store this
// for us.
#define FS_CALL2(func, index, ...)                                             \
  uv_fs_t* req = (uv_fs_t*)malloc(sizeof(uv_fs_t));                            \
  if (lua_isnone(L, index)) {                                                  \
    if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, NULL) < 0) {         \
      push_fs_error(L, req);                                                   \
      uv_fs_req_cleanup(req);                                                  \
      free(req);                                                               \
      return lua_error(L);                                                     \
    }                                                                          \
    req->ptr = buffer;                                              /* HACK */ \
    int argc = push_fs_result(L, req);                                         \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    return argc;                                                               \
  }                                                                            \
  luaL_checktype(L, index, LUA_TFUNCTION);                                     \
  luv_callback_t* callback = (luv_callback_t*)malloc(sizeof(luv_callback_t));  \
  callback->L = L;                                                             \
  lua_pushvalue(L, index);                                                     \
  callback->ref = luaL_ref(L, LUA_REGISTRYINDEX);                              \
  req->data = (void*)callback;                                                 \
  if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, on_fs) < 0) {          \
    push_fs_error(L, req);                                                     \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    return lua_error(L);                                                       \
  }                                                                            \
  req->ptr = buffer;                                                /* HACK */ \
  return 0;                                                                    \


static int luv_fs_open(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int flags = luv_string_to_flags(L, luaL_checkstring(L, 2));
  int mode = luaL_checkint(L, 3);
  FS_CALL(open, 4, path, flags, mode);
}

static int luv_fs_close(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(close, 4, file);
}

static int luv_fs_read(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  size_t length = luaL_checkint(L, 2);
  off_t offset = -1;
  if (!lua_isnil(L, 3)) {
    offset = luaL_checkint(L, 3);
  }
  char* buffer = (char*)malloc(length + 1);
  FS_CALL2(read, 4, file, buffer, length, offset);
}

static int luv_fs_write(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  size_t length;
  const char* string = luaL_checklstring(L, 2, &length);
  char* buffer = (char*)malloc(length + 1);
  uv_strlcpy(buffer, string, length + 1);
  off_t offset = -1;
  if (!lua_isnil(L, 3)) {
    offset = luaL_checkint(L, 3);
  }
  FS_CALL2(write, 4, file, buffer, length, offset);
}

static int luv_fs_unlink(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(unlink, 2, path);
}

static int luv_fs_mkdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  FS_CALL(mkdir, 3, path, mode);
}

static int luv_fs_rmdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(rmdir, 2, path);
}

static int luv_fs_readdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(readdir, 2, path, 0);
}

static int luv_fs_stat(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(stat, 2, path);
}

static int luv_fs_fstat(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(fstat, 2, file);
}

static int luv_fs_rename(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  FS_CALL(rename, 3, path, new_path);
}

static int luv_fs_fsync(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(fsync, 2, file);
}

static int luv_fs_fdatasync(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(fdatasync, 2, file);
}

static int luv_fs_ftruncate(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  off_t offset = luaL_checkint(L, 2);
  FS_CALL(ftruncate, 3, file, offset);
}

static int luv_fs_sendfile(lua_State* L) {
  uv_file out_fd = luaL_checkint(L, 1);
  uv_file in_fd = luaL_checkint(L, 2);
  off_t in_offset = luaL_checkint(L, 3);
  size_t length = luaL_checkint(L, 4);
  FS_CALL(sendfile, 5, out_fd, in_fd, in_offset, length);
}

static int luv_fs_chmod(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  FS_CALL(chmod, 3, path, mode);
}

static int luv_fs_utime(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);
  FS_CALL(utime, 4, path, atime, mtime);
}

static int luv_fs_futime(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);
  FS_CALL(futime, 4, file, atime, mtime);
}

static int luv_fs_lstat(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(lstat, 2, path);
}

static int luv_fs_link(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  FS_CALL(link, 3, path, new_path);
}

static int luv_fs_symlink(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  int flags = luv_string_to_flags(L, luaL_checkstring(L, 3));
  FS_CALL(symlink, 4, path, new_path, flags);
}

static int luv_fs_readlink(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(readlink, 2, path);
}

static int luv_fs_fchmod(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  FS_CALL(fchmod, 3, file, mode);
}

static int luv_fs_chown(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);
  FS_CALL(chown, 4, path, uid, gid);
}

static int luv_fs_fchown(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);
  FS_CALL(fchown, 4, file, uid, gid);
}


/******************************************************************************/

static const luaL_reg luv_functions[] = {
  {"new_tcp", new_tcp},
  {"new_timer", new_timer},
  {"new_tty", new_tty},
  {"new_pipe", new_pipe},
  {"run", luv_run},
  {"run_once", luv_run_once},
  {"guess_handle", luv_guess_handle},
  {"update_time", luv_update_time},
  {"now", luv_now},

  {"is_active", luv_is_active},
  {"walk", luv_walk},
  {"close", luv_close},
  {"ref", luv_ref},
  {"unref", luv_unref},
  {"is_closing", luv_is_closing},

  {"timer_start", luv_timer_start},
  {"timer_stop", luv_timer_stop},
  {"timer_again", luv_timer_again},
  {"timer_set_repeat", luv_timer_set_repeat},
  {"timer_get_repeat", luv_timer_get_repeat},

  {"write", luv_write},
  {"shutdown", luv_shutdown},
  {"read_start", luv_read_start},
  {"read_stop", luv_read_stop},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"write", luv_write},
  {"is_readable", luv_is_readable},
  {"is_writable", luv_is_writable},

  {"tcp_bind", luv_tcp_bind},
  {"tcp_getsockname", luv_tcp_getsockname},
  {"tcp_getpeername", luv_tcp_getpeername},
  {"tcp_connect", luv_tcp_connect},
  {"tcp_open", luv_tcp_open},
  {"tcp_nodelay", luv_tcp_nodelay},
  {"tcp_keepalive", luv_tcp_keepalive},

  {"tty_set_mode", luv_tty_set_mode},
  {"tty_reset_mode", luv_tty_reset_mode},
  {"tty_get_winsize", luv_tty_get_winsize},

  {"pipe_open", luv_pipe_open},
  {"pipe_bind", luv_pipe_bind},
  {"pipe_connect", luv_pipe_connect},

  {"fs_open", luv_fs_open},
  {"fs_close", luv_fs_close},
  {"fs_read", luv_fs_read},
  {"fs_write", luv_fs_write},
  {"fs_stat", luv_fs_stat},
  {"fs_fstat", luv_fs_fstat},
  {"fs_lstat", luv_fs_lstat},
  {"fs_unlink", luv_fs_unlink},
  {"fs_mkdir", luv_fs_mkdir},
  {"fs_rmdir", luv_fs_rmdir},
  {"fs_readdir", luv_fs_readdir},
  {"fs_rename", luv_fs_rename},
  {"fs_fsync", luv_fs_fsync},
  {"fs_fdatasync", luv_fs_fdatasync},
  {"fs_ftruncate", luv_fs_ftruncate},
  {"fs_sendfile", luv_fs_sendfile},
  {"fs_chmod", luv_fs_chmod},
  {"fs_utime", luv_fs_utime},
  {"fs_futime", luv_fs_futime},
  {"fs_link", luv_fs_link},
  {"fs_symlink", luv_fs_symlink},
  {"fs_readlink", luv_fs_readlink},
  {"fs_fchmod", luv_fs_fchmod},
  {"fs_chown", luv_fs_chown},
  {"fs_fchown", luv_fs_fchown},

  {NULL, NULL}
};

#endif
