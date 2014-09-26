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
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_tcp_t* handle = lua_newuserdata(L, sizeof(*handle));
  int ret;
  setup_udata(L, handle, "uv_handle");
  ret = uv_tcp_init(loop, handle);
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

// static void luv_after_connect(uv_connect_t* req, int status) {
//   lua_State* L = luv_prepare_callback(req->data);
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L) - 1;
// #endif
//   if (lua_isfunction(L, -1)) {
//      luv_call(L, 0, 0);
//   } else {
//     lua_pop(L, 1);
//   }
//
//   luv_handle_unref(L, req->handle->data);
//   free(req->data);
//   free(req);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
// }
//
// static int luv_tcp_bind(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_tcp_t* handle = luv_get_tcp(L, 1);
//   const char* host = luaL_checkstring(L, 2);
//   int port = luaL_checkint(L, 3);
//
//   struct sockaddr_in address = uv_ip4_addr(host, port);
//
//   if (uv_tcp_bind(handle, address)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "tcp_bind: %s", uv_strerror(err));
//   }
//
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
// static int luv_tcp_getsockname(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_tcp_t* handle = luv_get_tcp(L, 1);
//   int port = 0;
//   char ip[INET6_ADDRSTRLEN];
//   int family;
//
//   struct sockaddr_storage address;
//   int addrlen = sizeof(address);
//
//   if (uv_tcp_getsockname(handle, (struct sockaddr*)(&address), &addrlen)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "tcp_getsockname: %s", uv_strerror(err));
//   }
//
//   family = address.ss_family;
//   if (family == AF_INET) {
//     struct sockaddr_in* addrin = (struct sockaddr_in*)&address;
//     uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, INET6_ADDRSTRLEN);
//     port = ntohs(addrin->sin_port);
//   } else if (family == AF_INET6) {
//     struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)&address;
//     uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, INET6_ADDRSTRLEN);
//     port = ntohs(addrin6->sin6_port);
//   }
//
//   lua_newtable(L);
//   lua_pushnumber(L, port);
//   lua_setfield(L, -2, "port");
//   lua_pushnumber(L, family);
//   lua_setfield(L, -2, "family");
//   lua_pushstring(L, ip);
//   lua_setfield(L, -2, "address");
//
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top + 1);
// #endif
//   return 1;
// }
//
//
// static int luv_tcp_getpeername(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_tcp_t* handle = luv_get_tcp(L, 1);
//   int port = 0;
//   char ip[INET6_ADDRSTRLEN];
//   int family;
//
//   struct sockaddr_storage address;
//   int addrlen = sizeof(address);
//
//   if (uv_tcp_getpeername(handle, (struct sockaddr*)(&address), &addrlen)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "tcp_getpeername: %s", uv_strerror(err));
//   }
//
//   family = address.ss_family;
//   if (family == AF_INET) {
//     struct sockaddr_in* addrin = (struct sockaddr_in*)&address;
//     uv_inet_ntop(AF_INET, &(addrin->sin_addr), ip, INET6_ADDRSTRLEN);
//     port = ntohs(addrin->sin_port);
//   } else if (family == AF_INET6) {
//     struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)&address;
//     uv_inet_ntop(AF_INET6, &(addrin6->sin6_addr), ip, INET6_ADDRSTRLEN);
//     port = ntohs(addrin6->sin6_port);
//   }
//
//   lua_newtable(L);
//   lua_pushnumber(L, port);
//   lua_setfield(L, -2, "port");
//   lua_pushnumber(L, family);
//   lua_setfield(L, -2, "family");
//   lua_pushstring(L, ip);
//   lua_setfield(L, -2, "address");
//
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top + 1);
// #endif
//   return 1;
// }
//
// static int luv_tcp_connect(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_tcp_t* handle = luv_get_tcp(L, 1);
//
//   const char* ip_address = luaL_checkstring(L, 2);
//   int port = luaL_checkint(L, 3);
//
//   struct sockaddr_in address = uv_ip4_addr(ip_address, port);
//
//   uv_connect_t* req = malloc(sizeof(*req));
//   luv_req_t* lreq = malloc(sizeof(*lreq));
//
//   req->data = (void*)lreq;
//
//   lreq->lhandle = handle->data;
//
//   if (uv_tcp_connect(req, handle, address, luv_after_connect)) {
//     uv_err_t err;
//     free(req->data);
//     free(req);
//     err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "tcp_connect: %s", uv_strerror(err));
//   }
//
//   lreq->data_ref = LUA_NOREF;
//   lua_pushvalue(L, 4);
//   lreq->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
//
//   luv_handle_ref(L, handle->data, 1);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
// static int luv_tcp_open(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_tcp_t* handle = luv_get_tcp(L, 1);
//   uv_os_sock_t sock = luaL_checkint(L, 2);
//   if (uv_tcp_open(handle, sock)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "tcp_open: %s", uv_strerror(err));
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
// /* Enable/disable Nagle's algorithm. */
// static int luv_tcp_nodelay(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_tcp_t* handle = luv_get_tcp(L, 1);
//   int enable;
//   luaL_checkany(L, 2);
//   enable = lua_toboolean(L, 2);
//   if (uv_tcp_nodelay(handle, enable)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "tcp_nodelay: %s", uv_strerror(err));
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
//
//
//
// /* Enable/disable TCP keep-alive. */
// static int luv_tcp_keepalive(lua_State* L) {
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   uv_tcp_t* handle = luv_get_tcp(L, 1);
//   unsigned int delay = 0;
//   int enable;
//   luaL_checkany(L, 2);
//   enable = lua_toboolean(L, 2);
//   if (enable) {
//     delay = luaL_checkint(L, 3);
//   }
//   if (uv_tcp_keepalive(handle, enable, delay)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "tcp_keepalive: %s", uv_strerror(err));
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top);
// #endif
//   return 0;
// }
