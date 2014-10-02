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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


static void luv_getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
  struct addrinfo* curr = res;
  char ip[INET6_ADDRSTRLEN];
  const char *addr;
  int port, nargs, i = 0;

  if (status < 0) {
    luv_status(R, status);
    nargs = 1;
  }
  else {
    lua_pushnil(R);
    lua_newtable(R);
    nargs = 2;
    for (curr = res; curr; curr = curr->ai_next) {
      if (curr->ai_family == AF_INET || curr->ai_family == AF_INET6) {
        lua_newtable(R);
        if (curr->ai_family == AF_INET) {
          addr = (char*) &((struct sockaddr_in*) curr->ai_addr)->sin_addr;
          port = ((struct sockaddr_in*) curr->ai_addr)->sin_port;
          lua_pushstring(R, "inet");
          lua_setfield(R, -2, "family");
        } else {
          addr = (char*) &((struct sockaddr_in6*) curr->ai_addr)->sin6_addr;
          port = ((struct sockaddr_in6*) curr->ai_addr)->sin6_port;
          lua_pushstring(R, "inet6");
          lua_setfield(R, -2, "family");
        }
        uv_inet_ntop(curr->ai_family, addr, ip, INET6_ADDRSTRLEN);
        lua_pushstring(R, ip);
        lua_setfield(R, -2, "addr");
        if (ntohs(port)) {
          lua_pushinteger(R, ntohs(port));
          lua_setfield(R, -2, "port");
        }
        if (curr->ai_socktype == SOCK_STREAM) {
          lua_pushstring(R, "stream");
          lua_setfield(R, -2, "socktype");
        }
        else if (curr->ai_socktype == SOCK_DGRAM) {
          lua_pushstring(R, "dgram");
          lua_setfield(R, -2, "socktype");
        }
        lua_pushstring(R, luv_family_to_string(curr->ai_protocol));
        lua_setfield(R, -2, "protocol");
        lua_pushstring(R, curr->ai_canonname);
        lua_setfield(R, -2, "canonname");
        lua_rawseti(R, -2, ++i);
      }
    }
  }
  luv_fulfill_req(R, req->data, nargs);
  luv_cleanup_req(R, req->data);
  req->data = NULL;
  if (res) uv_freeaddrinfo(res);
}


static int luv_getaddrinfo(lua_State* L) {
  uv_getaddrinfo_t* req;
  const char* node;
  const char* service;
  struct addrinfo hints_s;
  struct addrinfo* hints = &hints_s;
  int ret, ref;
  if (lua_isnil(L, 1)) node = NULL;
  else node = luaL_checkstring(L, 1);
  if (lua_isnil(L, 2)) service = NULL;
  else service = luaL_checkstring(L, 2);
  if (!lua_isnil(L, 3)) luaL_checktype(L, 3, LUA_TTABLE);
  else hints = NULL;
  ref = luv_check_continuation(L, 4);
  if (hints) {
    // Initialize the hints
    memset(hints, 0, sizeof(*hints));

    // Process the `family` hint.
    lua_getfield(L, 3, "family");
    if (lua_isstring(L, -1)) {
      const char* family = lua_tostring(L, -1);
      if (strcmp(family, "inet") == 0) {
        hints->ai_family = AF_INET;
      }
      else if (strcmp(family, "inet6") == 0) {
        hints->ai_family = AF_INET6;
      }
      else {
        luaL_argerror(L, 3, "family hint must be 'inet' or 'inet6' if set");
      }
    }
    else if (lua_isnil(L, -1)) {
      hints->ai_family = AF_UNSPEC;
    }
    else {
      luaL_argerror(L, 3, "family hint must be string if set");
    }
    lua_pop(L, 1);

    // Process `socktype` hint
    lua_getfield(L, 3, "socktype");
    if (lua_isstring(L, -1)) {
      const char* socktype = lua_tostring(L, -1);
      if (strcmp(socktype, "stream") == 0) {
        hints->ai_socktype = SOCK_STREAM;
      }
      else if (strcmp(socktype, "dgram") == 0) {
        hints->ai_socktype = SOCK_DGRAM;
      }
      else {
        return luaL_argerror(L, 3, "socktype hint must be 'stream' or 'dgram' if set");
      }
    }
    else if (!lua_isnil(L, -1)) {
      return luaL_argerror(L, 3, "socktype hint must be string if set");
    }
    lua_pop(L, 1);

    // Process the `protocol` hint
    lua_getfield(L, 3, "protocol");
    if (lua_isstring(L, -1)) {
      int protocol = luv_string_to_family(lua_tostring(L, -1));
      if (protocol) {
        hints->ai_protocol = protocol;
      }
      else {
        return luaL_argerror(L, 3, "Invalid protocol hint");
      }
    }
    else if (!lua_isnil(L, -1)) {
      return luaL_argerror(L, 3, "protocol hint must be string if set");
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "addrconfig");
    if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_ADDRCONFIG;
    lua_pop(L, 1);

    lua_getfield(L, 3, "v4mapped");
    if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_V4MAPPED;
    lua_pop(L, 1);

    lua_getfield(L, 3, "all");
    if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_ALL;
    lua_pop(L, 1);

    lua_getfield(L, 3, "numerichost");
    if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_NUMERICHOST;
    lua_pop(L, 1);

    lua_getfield(L, 3, "passive");
    if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_PASSIVE;
    lua_pop(L, 1);

    lua_getfield(L, 3, "numericserv");
    if (lua_toboolean(L, -1)) {
        hints->ai_flags |=  AI_NUMERICSERV;
        /* On OS X upto at least OSX 10.9, getaddrinfo crashes
         * if AI_NUMERICSERV is set and the servname is NULL or "0".
         * This workaround avoids a segfault in libsystem.
         */
        if (NULL == service) service = "00";
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "canonname");
    if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_CANONNAME;
    lua_pop(L, 1);
  }

  req = lua_newuserdata(L, sizeof(*req));
  req->data = luv_setup_req(L, ref);

  ret = uv_getaddrinfo(uv_default_loop(), req, luv_getaddrinfo_cb, node, service, hints);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

// static void luv_getnameinfo_cb(uv_getnameinfo_t* req, int status, const char* hostname, const char* service) {
//   printf("luv_getnameinfo_cb(req=%p, status=%d, hostname=%s, service=%s)\n", req, status, hostname, service);
// }

// static int luv_getnameinfo(lua_State* L) {
//   uv_getnameinfo_t* req;
//   struct sockaddr addr_s;
//   struct sockaddr* addr = &addr_s;
//   int flags;
//   int ret, ref;




//   ret = uv_getnameinfo(uv_default_loop(), req, luv_getnameinfo_cb, addr, flags);
//   if (ret < 0) {
//     lua_pop(L, 1);
//     return luv_error(L, ret);
//   }
//   return 1;
// }

// static int luv_getaddrinfo(lua_State* L) {
//   const char* node;
//   const char* service;
//   uv_getaddrinfo_t* req;
//   luv_callback_t* callback;
//   struct addrinfo hints_s;
//   struct addrinfo* hints = &hints_s;

//   if (!lua_isnil(L, 1)) node = luaL_checkstring(L, 1);
//   else node = NULL;
//   if (!lua_isnil(L, 2)) service = luaL_checkstring(L, 2);
//   else service = NULL;
//   if (!lua_isnil(L, 3)) luaL_checktype(L, 3, LUA_TTABLE);
//   else hints = NULL;
//   luaL_checktype(L, 4, LUA_TFUNCTION);


//   /* Store the callback */
//   callback = malloc(sizeof(*callback));
//   callback->L = L;
//   lua_pushvalue(L, 4);
//   callback->ref = luaL_ref(L, LUA_REGISTRYINDEX);

//   /* Create the request */
//   req = malloc(sizeof(*req));
//   req->data = (void*)callback;

//   /* Make the call */
//   if (uv_getaddrinfo(uv_default_loop(), req, on_addrinfo, node, service, hints)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "getaddrinfo: %s", uv_strerror(err));
//   }

//   return 0;
// }

static int luv_string_to_family(const char* protocol) {
  if (!protocol) return AF_UNSPEC;
#ifdef AF_UNIX
  if (strcmp(protocol, "unix") == 0) return AF_UNIX;
#endif
#ifdef AF_INET
  if (strcmp(protocol, "inet") == 0) return AF_INET;
#endif
#ifdef AF_INET6
  if (strcmp(protocol, "inet6") == 0) return AF_INET6;
#endif
#ifdef AF_IPX
  if (strcmp(protocol, "ipx") == 0) return AF_IPX;
#endif
#ifdef AF_NETLINK
  if (strcmp(protocol, "netlink") == 0) return AF_NETLINK;
#endif
#ifdef AF_X25
  if (strcmp(protocol, "x25") == 0) return AF_X25;
#endif
#ifdef AF_AX25
  if (strcmp(protocol, "ax25") == 0) return AF_AX25;
#endif
#ifdef AF_ATMPVC
  if (strcmp(protocol, "atmpvc") == 0) return AF_ATMPVC;
#endif
#ifdef AF_APPLETALK
  if (strcmp(protocol, "appletalk") == 0) return AF_APPLETALK;
#endif
#ifdef AF_PACKET
  if (strcmp(protocol, "packet") == 0) return AF_PACKET;
#endif
  return 0;
}

static const char* luv_family_to_string(int family) {
#ifdef AF_UNIX
  if (family == AF_UNIX) return "unix";
#endif
#ifdef AF_INET
  if (family == AF_INET) return "inet";
#endif
#ifdef AF_INET6
  if (family == AF_INET6) return "inet6";
#endif
#ifdef AF_IPX
  if (family == AF_IPX) return "ipx";
#endif
#ifdef AF_NETLINK
  if (family == AF_NETLINK) return "netlink";
#endif
#ifdef AF_X25
  if (family == AF_X25) return "x25";
#endif
#ifdef AF_AX25
  if (family == AF_AX25) return "ax25";
#endif
#ifdef AF_ATMPVC
  if (family == AF_ATMPVC) return "atmpvc";
#endif
#ifdef AF_APPLETALK
  if (family == AF_APPLETALK) return "appletalk";
#endif
#ifdef AF_PACKET
  if (family == AF_PACKET) return "packet";
#endif
  return NULL;
}
