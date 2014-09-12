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


static int luv_error(lua_State* L, int ret) {
  lua_pushnil(L);
  fprintf(stderr, "libuv %s: %s\n", uv_err_name(ret), uv_strerror(ret));
  lua_pushstring(L, uv_err_name(ret));
  return 2;
}

// /* Unique type codes for each uv type */
// enum luv_type {
//   LUV_HANDLE  = 0x01,
//   LUV_TIMER   = 0x02,
//   LUV_STREAM  = 0x04,
//   LUV_TCP     = 0x08,
//   LUV_UDP     = 0x10,
//   LUV_TTY     = 0x20,
//   LUV_PIPE    = 0x30,
//   LUV_PROCESS = 0x80
// };
//
// /* Mask is the types that can be extracted from this concrete type */
// enum luv_mask {
//   LUV_TIMER_MASK    = LUV_HANDLE | LUV_TIMER,
//   LUV_TCP_MASK      = LUV_HANDLE | LUV_STREAM | LUV_TCP,
//   LUV_UDP_MASK      = LUV_HANDLE | LUV_STREAM | LUV_UDP,
//   LUV_TTY_MASK      = LUV_HANDLE | LUV_STREAM | LUV_TTY,
//   LUV_PIPE_MASK     = LUV_HANDLE | LUV_STREAM | LUV_PIPE,
//   LUV_PROCESS_MASK  = LUV_HANDLE | LUV_PROCESS
// };
//
// /* luv handles are used as the userdata type that points to uv handles.
//  * The luv handle is considered strong when it's "active" or has non-zero
//  * reqCount.  When this happens ref will contain a luaL_ref to the userdata.
//  */
// typedef struct {
//   uv_handle_t* handle; /* The actual uv handle. memory managed by luv */
//   int refCount;        /* a count of all pending request to know strength */
//   lua_State* L;        /* L and ref together form a reference to the userdata */
//   int threadref;       /* hold reference to coroutine if created in one */
//   int ref;             /* ref is null when refCount is 0 meaning we're weak */
//   int mask;
// } luv_handle_t;
//
// typedef struct {
//   luv_handle_t* lhandle;
//   int data_ref;
//   int callback_ref;
// } luv_req_t;
//
// typedef struct {
//   lua_State* L;
//   int ref;
// } luv_callback_t;
//
// /* Initialize a new lhandle and push the new userdata on the stack. */
// static luv_handle_t* luv_handle_create(lua_State* L, size_t size, int mask) {
//
//   /* Create the userdata and set it's metatable */
//   luv_handle_t* lhandle = (luv_handle_t*)lua_newuserdata(L, sizeof(luv_handle_t));
//   luaL_getmetatable(L, "luv_handle");
//   lua_setmetatable(L, -2);
//
//   /* Create a local environment for storing stuff */
//   lua_newtable(L);
//   lua_setuservalue(L, -2);
//
//   /* Initialize and return the lhandle */
//   lhandle->handle = (uv_handle_t*)malloc(size);
//   lhandle->handle->data = lhandle; /* Point back to lhandle from handle */
//   lhandle->refCount = 0;
//   lhandle->ref = LUA_NOREF;
//   lhandle->mask = mask;
//   lhandle->L = L;
//
//   /* if handle create in a coroutine, we need hold the coroutine */
//   if (lua_pushthread(L)) {
//     /* L is mainthread */
//     lua_pop(L, 1);
//     lhandle->threadref = LUA_NOREF;
//   } else {
//     lhandle->threadref = luaL_ref(L, LUA_REGISTRYINDEX);
//   }
//
//   return lhandle;
// }
//
// static uv_timer_t* luv_create_timer(lua_State* L) {
//   luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_timer_t), LUV_TIMER_MASK);
//   return (uv_timer_t*)lhandle->handle;
// }
//
// static uv_tcp_t* luv_create_tcp(lua_State* L) {
//   luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_tcp_t), LUV_TCP_MASK);
//   return (uv_tcp_t*)lhandle->handle;
// }
//
// static uv_udp_t* luv_create_udp(lua_State* L) {
//   luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_udp_t), LUV_UDP_MASK);
//   return (uv_udp_t*)lhandle->handle;
// }
//
// static uv_tty_t* luv_create_tty(lua_State* L) {
//   luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_tty_t), LUV_TTY_MASK);
//   return (uv_tty_t*)lhandle->handle;
// }
//
// static uv_pipe_t* luv_create_pipe(lua_State* L) {
//   luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_pipe_t), LUV_PIPE_MASK);
//   return (uv_pipe_t*)lhandle->handle;
// }
//
// static uv_process_t* luv_create_process(lua_State* L) {
//   luv_handle_t* lhandle = luv_handle_create(L, sizeof(uv_process_t), LUV_PROCESS_MASK);
//   return (uv_process_t*)lhandle->handle;
// }
//
// static luv_handle_t* luv_get_lhandle(lua_State* L, int index, int type) {
//   luv_handle_t* lhandle;
//   luaL_checktype(L, index, LUA_TUSERDATA);
//   lhandle = (luv_handle_t*)luaL_checkudata(L, index, "luv_handle");
//   if ((lhandle->mask & type) == 0) {
//     luaL_error(L, "Invalid type for userdata %d not in %d", type, lhandle->mask);
//   }
//   return lhandle;
// }
//
// static uv_handle_t* luv_get_handle(lua_State* L, int index) {
//   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_HANDLE);
//   return (uv_handle_t*)lhandle->handle;
// }
//
// static uv_timer_t* luv_get_timer(lua_State* L, int index) {
//   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_TIMER);
//   return (uv_timer_t*)lhandle->handle;
// }
//
// static uv_stream_t* luv_get_stream(lua_State* L, int index) {
//   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_STREAM);
//   return (uv_stream_t*)lhandle->handle;
// }
//
// static uv_tcp_t* luv_get_tcp(lua_State* L, int index) {
//   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_TCP);
//   return (uv_tcp_t*)lhandle->handle;
// }
//
// // static uv_udp_t* luv_get_udp(lua_State* L, int index) {
// //   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_UDP);
// //   return (uv_udp_t*)lhandle->handle;
// // }
//
// static uv_tty_t* luv_get_tty(lua_State* L, int index) {
//   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_TTY);
//   return (uv_tty_t*)lhandle->handle;
// }
//
// static uv_pipe_t* luv_get_pipe(lua_State* L, int index) {
//   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_PIPE);
//   return (uv_pipe_t*)lhandle->handle;
// }
//
// static uv_process_t* luv_get_process(lua_State* L, int index) {
//   luv_handle_t* lhandle = luv_get_lhandle(L, index, LUV_PROCESS);
//   return (uv_process_t*)lhandle->handle;
// }
//
// /* This needs to be called when an async function is started on a lhandle. */
// static void luv_handle_ref(lua_State* L, luv_handle_t* lhandle, int index) {
//   /* If it's inactive, store a ref. */
//   if (!lhandle->refCount) {
//     lua_pushvalue(L, index);
//     lhandle->ref = luaL_ref(L, LUA_REGISTRYINDEX);
//   }
//   lhandle->refCount++;
// }
//
// /* This needs to be called when an async callback fires on a lhandle. */
// static void luv_handle_unref(lua_State* L, luv_handle_t* lhandle) {
//   lhandle->refCount--;
//   assert(lhandle->refCount >= 0);
//   /* If it's now inactive, clear the ref */
//   if (!lhandle->refCount) {
//     luaL_unref(L, LUA_REGISTRYINDEX, lhandle->ref);
//     if (lhandle->threadref != LUA_NOREF) {
//       luaL_unref(L, LUA_REGISTRYINDEX, lhandle->threadref);
//       lhandle->threadref = LUA_NOREF;
//     }
//     lhandle->ref = LUA_NOREF;
//   }
// }
//
// /* Get L from a lhandle (moving to the main thread if needed) and push the userdata on the stack. */
// static lua_State* luv_prepare_event(luv_handle_t* lhandle) {
//   lua_State* L = lhandle->L;
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   assert(lhandle->refCount); /* sanity check */
//   assert(lhandle->ref != LUA_NOREF); /* the ref should be there */
//   lua_rawgeti(L, LUA_REGISTRYINDEX, lhandle->ref);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top + 1);
// #endif
//   return L;
// }
//
// /* Get a named callback.  If it's there, push the function and the userdata on the stack.
//    otherwise return 0
//    either way, the original userdata is removed. */
// static int luv_get_callback(lua_State* L, const char* name) {
//   int isfunc;
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   /* Get the connection handler */
//   lua_getuservalue(L, -1);
//   lua_getfield(L, -1, name);
//   lua_remove(L, -2); /* Remove the uservalue */
//
//   isfunc = lua_isfunction(L, -1);
//   if (isfunc) {
//     lua_pushvalue(L, -2);
//     lua_remove(L, -3); /* Remove the original userdata */
//   } else {
//     lua_pop(L, 2); /* Remove the non function and the userdata */
//   }
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top + (isfunc ? 1 : -1));
// #endif
//   return isfunc;
// }
//
// /* returns the lua_State* and pushes the callback onto the stack */
// /* STACK +1 */
// static lua_State* luv_prepare_callback(luv_req_t* lreq) {
//   luv_handle_t* lhandle = lreq->lhandle;
//   lua_State* L = lhandle->L;
// #ifdef LUV_STACK_CHECK
//   int top = lua_gettop(L);
// #endif
//   lua_rawgeti(L, LUA_REGISTRYINDEX, lreq->callback_ref);
//   luaL_unref(L, LUA_REGISTRYINDEX, lreq->data_ref);
//   luaL_unref(L, LUA_REGISTRYINDEX, lreq->callback_ref);
// #ifdef LUV_STACK_CHECK
//   assert(lua_gettop(L) == top + 1);
// #endif
//   return L;
// }
//
// static void luv_call(lua_State *C, int nargs, int nresults) {
//   lua_State* L = luv_main_thread;
//   if (L != C) {
//     /* Move the function call to the main thread if it's in a coroutine */
//     lua_xmove(C, L, 1 + nargs);
//   }
//   lua_call(L, nargs, nresults);
// }
//
// // Commented out because it's unused normally.
// // static void luv_stack_dump(lua_State* L, int top, const char* name) {
// //   int i, l;
// //   printf("\nAPI STACK DUMP: %s\n", name);
// //   for (i = top, l = lua_gettop(L); i <= l; i++) {
// //     const char* typename = lua_typename(L, lua_type(L, i));
// //     switch (lua_type(L, i)) {
// //       case LUA_TNIL:
// //         printf("  %d: %s\n", i, typename);
// //         break;
// //       case LUA_TNUMBER:
// //         printf("  %d: %s\t%f\n", i, typename, lua_tonumber(L, i));
// //         break;
// //       case LUA_TBOOLEAN:
// //         printf("  %d: %s\n\t%s", i, typename, lua_toboolean(L, i) ? "true" : "false");
// //         break;
// //       case LUA_TSTRING:
// //         printf("  %d: %s\t%s\n", i, typename, lua_tostring(L, i));
// //         break;
// //       case LUA_TTABLE:
// //         printf("  %d: %s\n", i, typename);
// //         break;
// //       case LUA_TFUNCTION:
// //         printf("  %d: %s\t%p\n", i, typename, lua_tocfunction(L, i));
// //         break;
// //       case LUA_TUSERDATA:
// //         printf("  %d: %s\t%p\n", i, typename, lua_touserdata(L, i));
// //         break;
// //       case LUA_TTHREAD:
// //         printf("  %d: %s\t%p\n", i, typename, lua_tothread(L, i));
// //         break;
// //       case LUA_TLIGHTUSERDATA:
// //         printf("  %d: %s\t%p\n", i, typename, lua_touserdata(L, i));
// //         break;
// //     }
// //   }
// //   printf("\n");
// // }
//
// static const char* type_to_string(uv_handle_type type) {
//   switch (type) {
// #define XX(uc, lc) case UV_##uc: return ""#lc"";
//     UV_HANDLE_TYPE_MAP(XX)
// #undef XX
//     case UV_UNKNOWN_HANDLE: return "unknown";
//     case UV_FILE: return "file";
//     default: return "";
//   }
// }
//
// static int new_tcp(lua_State* L) {
//   uv_tcp_t* handle = luv_create_tcp(L);
//   if (uv_tcp_init(uv_default_loop(), handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_tcp: %s", uv_strerror(err));
//   }
//   return 1;
// }
//
// static int new_udp(lua_State* L) {
//   uv_udp_t* handle = luv_create_udp(L);
//   if (uv_udp_init(uv_default_loop(), handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_udp: %s", uv_strerror(err));
//   }
//   return 1;
// }
//
// static int new_timer(lua_State* L) {
//   uv_timer_t* handle = luv_create_timer(L);
//   if (uv_timer_init(uv_default_loop(), handle)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_timer: %s", uv_strerror(err));
//   }
//   return 1;
// }
//
// static int new_tty(lua_State* L) {
//   uv_tty_t* handle = luv_create_tty(L);
//   uv_file fd = luaL_checkint(L, 1);
//   int readable;
//   luaL_checktype(L, 2, LUA_TBOOLEAN);
//   readable = lua_toboolean(L, 2);
//   if (uv_tty_init(uv_default_loop(), handle, fd, readable)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_tty: %s", uv_strerror(err));
//   }
//   return 1;
// }
//
// static int new_pipe(lua_State* L) {
//   uv_pipe_t* handle = luv_create_pipe(L);
//   int ipc;
//   luaL_checktype(L, 1, LUA_TBOOLEAN);
//   ipc = lua_toboolean(L, 1);
//   if (uv_pipe_init(uv_default_loop(), handle, ipc)) {
//     uv_err_t err = uv_last_error(uv_default_loop());
//     return luaL_error(L, "new_pipe: %s", uv_strerror(err));
//   }
//   return 1;
// }
