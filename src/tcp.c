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
#include "luv.h"

static int new_tcp(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_tcp_t* handle = luv_create_tcp(L);
  int ret = uv_tcp_init(loop, handle);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_tcp_open(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  uv_os_sock_t sock = luaL_checkinteger(L, 2);
  int ret = uv_tcp_open(handle, sock);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_tcp_nodelay(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  int ret, enable;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  enable = lua_toboolean(L, 2);
  ret = uv_tcp_nodelay(handle, enable);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_tcp_keepalive(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  int ret, enable;
  unsigned int delay;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  enable = lua_toboolean(L, 2);
  delay = luaL_checkinteger(L, 3);
  ret = uv_tcp_keepalive(handle, enable, delay);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_tcp_simultaneous_accepts(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  int ret, enable;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  enable = lua_toboolean(L, 2);
  ret = uv_tcp_simultaneous_accepts(handle, enable);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
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
    return luaL_argerror(L, 2, "Invalid IP address or port");
  }
  ret = uv_tcp_bind(handle, (struct sockaddr*)&addr, flags);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static void parse_sockaddr(lua_State* L, struct sockaddr_storage* address, int addrlen) {
  char ip[INET6_ADDRSTRLEN];
  int port = 0;
  lua_newtable(L);
  if (address->ss_family == AF_INET) {
    struct sockaddr_in* addrin = (struct sockaddr_in*)address;
    uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, addrlen);
    port = ntohs(addrin->sin_port);
  } else if (address->ss_family == AF_INET6) {
    struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)address;
    uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, addrlen);
    port = ntohs(addrin6->sin6_port);
  }

  lua_pushinteger(L, address->ss_family);
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
  parse_sockaddr(L, &address, addrlen);
  return 1;
}

static int luv_tcp_getpeername(lua_State* L) {
  uv_tcp_t* handle = luv_check_tcp(L, 1);
  struct sockaddr_storage address;
  int addrlen = sizeof(address);
  int ret = uv_tcp_getpeername(handle, (struct sockaddr*)&address, &addrlen);
  if (ret < 0) return luv_error(L, ret);
  parse_sockaddr(L, &address, addrlen);
  return 1;
}

static int luv_tcp_connect(lua_State* L) {
  uv_connect_t* req = luv_check_connect(L, 1);
  uv_tcp_t* handle = luv_check_tcp(L, 2);
  const char* host = luaL_checkstring(L, 3);
  int port = luaL_checkinteger(L, 4);
  struct sockaddr_storage addr;
  int ret;
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    return luaL_argerror(L, 3, "Invalid IP address or port");
  }
  ret = uv_tcp_connect(req, handle, (struct sockaddr*)&addr, connect_cb);
  return luv_wait(L, req->data, ret);
}
