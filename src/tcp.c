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
#include "private.h"

static uv_tcp_t* luv_check_tcp(lua_State* L, int index) {
  uv_tcp_t* handle = (uv_tcp_t*)luv_checkudata(L, index, "uv_tcp");
  luaL_argcheck(L, handle->type == UV_TCP && handle->data, index, "Expected uv_tcp_t");
  return handle;
}

static int luv_new_tcp(lua_State* L) {
  uv_tcp_t* handle;
  int ret;
  luv_ctx_t* ctx = luv_context(L);
  lua_settop(L, 1);
  handle = (uv_tcp_t*)luv_newuserdata(L, sizeof(*handle));
#if LUV_UV_VERSION_GEQ(1, 7, 0)
  if (lua_isnoneornil(L, 1)) {
    ret = uv_tcp_init(ctx->loop, handle);
  }
  else {
    unsigned int flags = AF_UNSPEC;
    if (lua_isnumber(L, 1)) {
      flags = lua_tointeger(L, 1);
    }
    else if (lua_isstring(L, 1)) {
      const char* family = lua_tostring(L, 1);
      flags = luv_af_string_to_num(family);
      if (!flags) {
        luaL_argerror(L, 1, lua_pushfstring(L, "invalid or unknown address family: '%s'", family));
      }
    }
    else {
      luaL_argerror(L, 1, "expected string or integer");
    }
    ret = uv_tcp_init_ex(ctx->loop, handle, flags);
  }
#else
  ret = uv_tcp_init(ctx->loop, handle);
#endif
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  handle->data = luv_setup_handle(L, ctx);
  return 1;
}

static int luv_tcp_open(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  uv_os_sock_t sock = luaL_checkinteger(L, 2);
  int ret = uv_tcp_open(handle, sock);
  return luv_result(L, ret);
}

static int luv_tcp_nodelay(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  int ret, enable;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  enable = lua_toboolean(L, 2);
  ret = uv_tcp_nodelay(handle, enable);
  return luv_result(L, ret);
}

static int luv_tcp_keepalive(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  int ret, enable;
  unsigned int delay = 0;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  enable = lua_toboolean(L, 2);
  if (enable) {
    delay = luaL_checkinteger(L, 3);
  }
  ret = uv_tcp_keepalive(handle, enable, delay);
  return luv_result(L, ret);
}

static int luv_tcp_simultaneous_accepts(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  int ret, enable;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  enable = lua_toboolean(L, 2);
  ret = uv_tcp_simultaneous_accepts(handle, enable);
  return luv_result(L, ret);
}

static int luv_tcp_bind(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  const char* host = luaL_checkstring(L, 2);
  int port = luaL_checkinteger(L, 3);
  unsigned int flags = 0;
  struct sockaddr_storage addr;
  int ret;
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    return luaL_error(L, "Invalid IP address or port [%s:%d]", host, port);
  }
  if (lua_type(L, 4) == LUA_TTABLE) {
    lua_getfield(L, 4, "ipv6only");
    if (lua_toboolean(L, -1)) flags |= UV_TCP_IPV6ONLY;
    lua_pop(L, 1);
  }
  ret = uv_tcp_bind(handle, (struct sockaddr*)&addr, flags);
  return luv_result(L, ret);
}

static void parse_sockaddr(lua_State* L, struct sockaddr_storage* address) {
  char ip[INET6_ADDRSTRLEN];
  int port = 0;
  lua_newtable(L);
  if (address->ss_family == AF_INET) {
    struct sockaddr_in* addrin = (struct sockaddr_in*)address;
    uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin->sin_port);
  } else if (address->ss_family == AF_INET6) {
    struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)address;
    uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, INET6_ADDRSTRLEN);
    port = ntohs(addrin6->sin6_port);
  }

  lua_pushstring(L, luv_af_num_to_string(address->ss_family));
  lua_setfield(L, -2, "family");
  lua_pushinteger(L, port);
  lua_setfield(L, -2, "port");
  lua_pushstring(L, ip);
  lua_setfield(L, -2, "ip");
}

static int luv_tcp_getsockname(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  struct sockaddr_storage address;
  int addrlen = sizeof(address);
  int ret = uv_tcp_getsockname(handle, (struct sockaddr*)&address, &addrlen);
  if (ret < 0) return luv_error(L, ret);
  parse_sockaddr(L, &address);
  return 1;
}

static int luv_tcp_getpeername(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  struct sockaddr_storage address;
  int addrlen = sizeof(address);
  int ret = uv_tcp_getpeername(handle, (struct sockaddr*)&address, &addrlen);
  if (ret < 0) return luv_error(L, ret);
  parse_sockaddr(L, &address);
  return 1;
}

static void luv_connect_cb(uv_connect_t* req, int status) {
  luv_req_t* data = (luv_req_t*)req->data;
  lua_State* L = data->ctx->L;
  luv_status(L, status);
  luv_fulfill_req(L, (luv_req_t*)req->data, 1);
  luv_cleanup_req(L, (luv_req_t*)req->data);
  req->data = NULL;
}

// deprecated by luv_stream_get_write_queue_size
static int luv_write_queue_size(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  lua_pushinteger(L, handle->write_queue_size);
  return 1;
}

static int luv_tcp_connect(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  const char* host = luaL_checkstring(L, 2);
  int port = luaL_checkinteger(L, 3);
  struct sockaddr_storage addr;
  uv_connect_t* req;
  int ret, ref;
  luv_handle_t* lhandle = handle->data;
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    return luaL_error(L, "Invalid IP address or port [%s:%d]", host, port);
  }
  ref = luv_check_continuation(L, 4);

  req = (uv_connect_t*)lua_newuserdata(L, sizeof(*req));
  req->data = luv_setup_req(L, lhandle->ctx, ref);
  ret = uv_tcp_connect(req, handle, (struct sockaddr*)&addr, luv_connect_cb);
  if (ret < 0) {
    luv_cleanup_req(L, (luv_req_t*)req->data);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

#if LUV_UV_VERSION_GEQ(1, 32, 0)
static void luv_close_reset_cb(uv_handle_t* handle) {
  lua_State* L;
  luv_handle_t* data = (luv_handle_t*)handle->data;
  if (!data) return;
  L = data->ctx->L;
  luv_call_callback(L, data, LUV_RESET, 0);
  luv_unref_handle(L, data);
}

static int luv_tcp_close_reset(lua_State* L) {
  int ret;
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  if (!lua_isnoneornil(L, 2)) {
    luv_check_callback(L, (luv_handle_t*)handle->data, LUV_RESET, 2);
  }
  ret = uv_tcp_close_reset(handle, luv_close_reset_cb);
  return luv_result(L, ret);
}
#endif

