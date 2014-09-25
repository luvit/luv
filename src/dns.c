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

// static void on_addrinfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
//   luv_callback_t* callback = req->data;
//   lua_State* L = callback->L;
//   struct addrinfo* curr = res;
//   char ip[INET6_ADDRSTRLEN];
//   const char *addr;
//   int port;
//   int i = 1;
//   /* Get the callback and remove the reference to it. */
//   lua_rawgeti(L, LUA_REGISTRYINDEX, callback->ref);
//   luaL_unref(L, LUA_REGISTRYINDEX, callback->ref);
//   free(callback);
//   free(req);

//   lua_newtable(L);
//   for (curr = res; curr; curr = curr->ai_next) {
//     if (curr->ai_family == AF_INET || curr->ai_family == AF_INET6) {
//       lua_newtable(L);
//       if (curr->ai_family == AF_INET) {
//         addr = (char*) &((struct sockaddr_in*) curr->ai_addr)->sin_addr;
//         port = ((struct sockaddr_in*) curr->ai_addr)->sin_port;
//         lua_pushstring(L, "IPv4");
//         lua_setfield(L, -2, "family");
//       } else {
//         addr = (char*) &((struct sockaddr_in6*) curr->ai_addr)->sin6_addr;
//         port = ((struct sockaddr_in6*) curr->ai_addr)->sin6_port;
//         lua_pushstring(L, "IPv6");
//         lua_setfield(L, -2, "family");
//       }
//       uv_inet_ntop(curr->ai_family, addr, ip, INET6_ADDRSTRLEN);
//       lua_pushstring(L, ip);
//       lua_setfield(L, -2, "addr");
//       if (ntohs(port)) {
//         lua_pushinteger(L, ntohs(port));
//         lua_setfield(L, -2, "port");
//       }
//       if (curr->ai_socktype == SOCK_STREAM) {
//         lua_pushstring(L, "STREAM");
//         lua_setfield(L, -2, "socktype");
//       }
//       else if (curr->ai_socktype == SOCK_DGRAM) {
//         lua_pushstring(L, "DGRAM");
//         lua_setfield(L, -2, "socktype");
//       }
//       switch (curr->ai_protocol) {
// #ifdef AF_UNIX
//         case AF_UNIX:
//           lua_pushstring(L, "UNIX");
//           break;
// #endif
// #ifdef AF_INET
//         case AF_INET:
//           lua_pushstring(L, "INET");
//           break;
// #endif
// #ifdef AF_INET6
//         case AF_INET6:
//           lua_pushstring(L, "INET6");
//           break;
// #endif
// #ifdef AF_IPX
//         case AF_IPX:
//           lua_pushstring(L, "IPX");
//           break;
// #endif
// #ifdef AF_NETLINK
//         case AF_NETLINK:
//           lua_pushstring(L, "NETLINK");
//           break;
// #endif
// #ifdef AF_X25
//         case AF_X25:
//           lua_pushstring(L, "X25");
//           break;
// #endif
// #ifdef AF_AX25
//         case AF_AX25:
//           lua_pushstring(L, "AX25");
//           break;
// #endif
// #ifdef AF_ATMPVC
//         case AF_ATMPVC:
//           lua_pushstring(L, "ATMPVC");
//           break;
// #endif
// #ifdef AF_APPLETALK
//         case AF_APPLETALK:
//           lua_pushstring(L, "APPLETALK");
//           break;
// #endif
// #ifdef AF_PACKET
//         case AF_PACKET:
//           lua_pushstring(L, "PACKET");
//           break;
// #endif
//         default:
//           lua_pushstring(L, NULL);
//       }
//       lua_setfield(L, -2, "protocol");
//       lua_pushstring(L, curr->ai_canonname);
//       lua_setfield(L, -2, "canonname");
//       lua_rawseti(L, -2, i++);
//     }
//   }
//   uv_freeaddrinfo(res);

//   luv_call(L, 1, 0);
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

//   if (hints) {
//     /* Initialize the hints */
//     memset(hints, 0, sizeof(struct addrinfo));

//     /* Process the `family` hint. */
//     lua_getfield(L, 3, "family");
//     switch (lua_tointeger(L, -1)) {
//       case 0:
//         hints->ai_family = AF_UNSPEC;
//         break;
//       case 4:
//         hints->ai_family = AF_INET;
//         break;
//       case 6:
//         hints->ai_family = AF_INET6;
//         break;
//       default:
//         return luaL_error(L, "`family` must be integer 0 (any), 4 (IPv4), or 6 (IPv6)");
//         break;
//     }
//     lua_pop(L, 1);

//     /* Process `socktype` hint */
//     lua_getfield(L, 3, "socktype");
//     if (!lua_isnil(L, -1)) {
//       const char* socktype = luaL_checkstring(L, -1);
//       if (strcmp(socktype, "STREAM") == 0) {
//         hints->ai_socktype = SOCK_STREAM;
//       }
//       else if (strcmp(socktype, "DGRAM") == 0) {
//         hints->ai_socktype = SOCK_DGRAM;
//       }
//       else {
//         return luaL_error(L, "`socktype` must be 'STREAM' or 'DGRAM' or nil");
//       }
//     }
//     lua_pop(L, 1);

//     /* Process the `protocol` hint */
//     lua_getfield(L, 3, "protocol");
//     if (!lua_isnil(L, -1)) {
//       const char* protocol = luaL_checkstring(L, -1);
// #ifdef AF_UNIX
//       if (strcmp(protocol, "UNIX") == 0) {
//         hints->ai_protocol = AF_UNIX;
//       }
//       else
// #endif
// #ifdef AF_INET
//       if (strcmp(protocol, "INET") == 0) {
//         hints->ai_protocol = AF_INET;
//       }
//       else
// #endif
// #ifdef AF_INET6
//       if (strcmp(protocol, "INET6") == 0) {
//         hints->ai_protocol = AF_INET6;
//       }
//       else
// #endif
// #ifdef AF_IPX
//       if (strcmp(protocol, "IPX") == 0) {
//         hints->ai_protocol = AF_IPX;
//       }
//       else
// #endif
// #ifdef AF_NETLINK
//       if (strcmp(protocol, "NETLINK") == 0) {
//         hints->ai_protocol = AF_NETLINK;
//       }
//       else
// #endif
// #ifdef AF_X25
//       if (strcmp(protocol, "X25") == 0) {
//         hints->ai_protocol = AF_X25;
//       }
//       else
// #endif
// #ifdef AF_AX25
//       if (strcmp(protocol, "AX25") == 0) {
//         hints->ai_protocol = AF_AX25;
//       }
//       else
// #endif
// #ifdef AF_ATMPVC
//       if (strcmp(protocol, "ATMPVC") == 0) {
//         hints->ai_protocol = AF_ATMPVC;
//       }
//       else
// #endif
// #ifdef AF_APPLETALK
//       if (strcmp(protocol, "APPLETALK") == 0) {
//         hints->ai_protocol = AF_APPLETALK;
//       }
//       else
// #endif
// #ifdef AF_PACKET
//       if (strcmp(protocol, "PACKET") == 0) {
//         hints->ai_protocol = AF_PACKET;
//       }
//       else
// #endif
//       {
//         return luaL_error(L, "Unknown protocol");
//       }
//     }
//     lua_pop(L, 1);

//     lua_getfield(L, 3, "addrconfig");
//     if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_ADDRCONFIG;
//     lua_pop(L, 1);

//     lua_getfield(L, 3, "v4mapped");
//     if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_V4MAPPED;
//     lua_pop(L, 1);

//     lua_getfield(L, 3, "all");
//     if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_ALL;
//     lua_pop(L, 1);

//     lua_getfield(L, 3, "numerichost");
//     if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_NUMERICHOST;
//     lua_pop(L, 1);

//     lua_getfield(L, 3, "passive");
//     if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_PASSIVE;
//     lua_pop(L, 1);

//     lua_getfield(L, 3, "numericserv");
//     if (lua_toboolean(L, -1)) {
//         hints->ai_flags |=  AI_NUMERICSERV;
//         /* On OS X upto at least OSX 10.9, getaddrinfo crashes
//          * if AI_NUMERICSERV is set and the servname is NULL or "0".
//          * This workaround avoids a segfault in libsystem.
//          */
//         if(NULL == service) service = "00";
//     }
//     lua_pop(L, 1);

//     lua_getfield(L, 3, "canonname");
//     if (lua_toboolean(L, -1)) hints->ai_flags |=  AI_CANONNAME;
//     lua_pop(L, 1);
//   }

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
