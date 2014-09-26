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

// Create a weak table for mapping pointers to userdata.
static void util_init(lua_State* L) {
  // Create a table for storing active coroutines.
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "active_threads");

  lua_atpanic(L, on_panic);
}

static int on_panic(lua_State* L) {
  luv_stack_dump(L, "PANIC");
  return 0;
}

static void luv_stack_dump(lua_State* L, const char* name) {
  int i, l;
  printf("\nAPI STACK DUMP %p: %s\n", L, name);
  for (i = 1, l = lua_gettop(L); i <= l; i++) {
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, i);
    lua_call(L, 1, 1);
    printf("  %d %s\n", i, lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  assert(l == lua_gettop(L));
}

// Given a userdata on top of the stack, give it a metatable and an environment.
// +0 to stack
static void setup_udata(lua_State* L, const char* type) {
  int top = lua_gettop(L);
  luv_stack_dump(L, "setup_udata1");

  // Tag the userdata with the given type.
  luaL_getmetatable(L, type);
  lua_setmetatable(L, -2);

  // Create a new environment for storing stuff.
  lua_newtable(L);
  lua_setuservalue(L, -2);

  luv_stack_dump(L, "setup_udata2");
  assert(top == lua_gettop(L));
}

// Define loop constructor
static uv_loop_t* luv_create_loop(lua_State* L) {
  luv_loop_t* lloop = lua_newuserdata(L, sizeof(*lloop));
  setup_udata(L, "uv_loop");
  lua_pushvalue(L, -1);
  lloop->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  return &(lloop->data);
}

// Define req constructors
#define XX(uc, lc)                                        \
static uv_##lc##_t* luv_create_##lc(lua_State* L) {       \
  luv_##lc##_t* lreq = lua_newuserdata(L, sizeof(*lreq)); \
  setup_udata(L, "uv_req");                               \
  lua_pushvalue(L, -1);                                   \
  lreq->ref = luaL_ref(L, LUA_REGISTRYINDEX);             \
  return &(lreq->data);                                   \
}
  LUV_CONCRETE_REQ_TYPE_MAP(XX)
#undef XX

// Define handle constructors
#define XX(uc, lc)                                              \
static uv_##lc##_t* luv_create_##lc(lua_State* L) {             \
  luv_##lc##_t* lhandle = lua_newuserdata(L, sizeof(*lhandle)); \
  setup_udata(L, "uv_handle");                                  \
  lua_pushvalue(L, -1);                                         \
  lhandle->ref = luaL_ref(L, LUA_REGISTRYINDEX);                \
  return &(lhandle->data);                                      \
}
  LUV_CONCRETE_HANDLE_TYPE_MAP(XX)
#undef XX

// Define loop checker
static uv_loop_t* luv_check_loop(lua_State* L, int index) {
  luv_loop_t* lloop = luaL_checkudata(L, index, "uv_loop");
  return &(lloop->data);
}

// Define req checkers
#define XX(uc, lc)                                            \
static uv_##lc##_t* luv_check_##lc(lua_State* L, int index) { \
  luv_##lc##_t* lreq = luaL_checkudata(L, index, "uv_req");   \
  uv_##lc##_t* req = &(lreq->data);                           \
  if (req->type != UV_##uc) {                                 \
    luaL_argerror(L, index, "uv_"#lc"_t required");           \
  }                                                           \
  return req;                                                 \
}
  LUV_CONCRETE_REQ_TYPE_MAP(XX)
#undef XX

// Define handle checkers
#define XX(uc, lc)                                                \
static uv_##lc##_t* luv_check_##lc(lua_State* L, int index) {     \
  luv_##lc##_t* lhandle = luaL_checkudata(L, index, "uv_handle"); \
  uv_##lc##_t* handle = &(lhandle->data);                         \
  if (handle->type != UV_##uc) {                                  \
    luaL_argerror(L, index, "uv_"#lc"_t required");               \
  }                                                               \
  return handle;                                                  \
}
  LUV_CONCRETE_HANDLE_TYPE_MAP(XX)
#undef XX

// Define abstract handle checkers
static uv_handle_t* luv_check_handle(lua_State* L, int index) {
  luv_handle_t* lhandle = luaL_checkudata(L, index, "uv_handle");
  return &(lhandle->data);
}
static uv_stream_t* luv_check_stream(lua_State* L, int index) {
  luv_stream_t* lhandle = luaL_checkudata(L, index, "uv_handle");
  uv_stream_t* handle = &(lhandle->data);
  luaL_argcheck(L,
    handle->type == UV_TCP ||
    handle->type == UV_TTY ||
    handle->type == UV_NAMED_PIPE,
    index, "uv_stream_t subclass required");
  return handle;
}

// Given a libuv pointer, push the corresponding userdata on the stack
// +1 to stack
#define XX(uc, lc)                                                    \
static void luv_find_##lc(lua_State* L, uv_##lc##_t* ptr) {           \
  /* Manually expanded container_of macro */                          \
  luv_##lc##_t* wrapper = ({                                          \
    const typeof( ((luv_##lc##_t *)0)->data ) *__mptr = (ptr);        \
    (luv_##lc##_t *)( (char *)__mptr - offsetof(luv_##lc##_t,data) ); \
  });                                                                 \
  lua_rawgeti(L, LUA_REGISTRYINDEX, wrapper->ref);                    \
}
  XX(LOOP, loop)
  LUV_CONCRETE_REQ_TYPE_MAP(XX)
  UV_HANDLE_TYPE_MAP(XX)
#undef XX

// Given a libuv pointer, push the corresponding userdata on the stack
// +1 to stack
#define XX(uc, lc)                                                    \
static void luv_unref_##lc(lua_State* L, uv_##lc##_t* ptr) {          \
  /* Manually expanded container_of macro */                          \
  luv_##lc##_t* wrapper = ({                                          \
    const typeof( ((luv_##lc##_t *)0)->data ) *__mptr = (ptr);        \
    (luv_##lc##_t *)( (char *)__mptr - offsetof(luv_##lc##_t,data) ); \
  });                                                                 \
  luaL_unref(L, LUA_REGISTRYINDEX, wrapper->ref);                     \
  wrapper->ref = LUA_NOREF;                                           \
}
  XX(LOOP, loop)
  LUV_CONCRETE_REQ_TYPE_MAP(XX)
  XX(HANDLE, handle)
#undef XX

static int luv_error(lua_State* L, int ret) {
  lua_pushnil(L);
  // For now log errors to stderr in case they aren't asserted or checked for.
  fprintf(stderr, "%s: %s\n", uv_err_name(ret), uv_strerror(ret));
  lua_pushstring(L, uv_err_name(ret));
  return 2;
}

static void luv_ccall(lua_State* L, int nargs) {
  int top = lua_gettop(L);
  int ret;
  lua_State* T = lua_newthread(L);

  printf("lua_newthread(L=%p) -> T=%p\n", L, T);
  lua_insert(L, -2 - nargs); // Push coroutine above function and userdata
  lua_xmove(L, T, nargs + 1);
  ret = lua_resume(T, L, nargs);
  lua_pop(L, 1);
  if (ret && ret != LUA_YIELD) {
    on_panic(T);
  }
  assert(lua_gettop(L) == top - nargs - 1);
}

static void luv_emit_event(lua_State* L, const char* name, int nargs) {
  printf("emit L=%p name=%s\n", L, name);
  lua_getuservalue(L, -nargs);
  lua_getfield(L, -1, name);
  if (lua_type(L, -1) != LUA_TFUNCTION) {
    lua_pop(L, 2 + nargs);
    return;
  }
  lua_remove(L, -2); // Remove uservalue
  lua_insert(L, -1 - nargs); // Push function above userdata
  luv_ccall(L, nargs);
}

static int luv_wait(lua_State* L, int status) {
  if (status < 0) return luv_error(L, status);

  // Store the current thread in active_threads
  lua_getfield(L, LUA_REGISTRYINDEX, "active_threads");
  lua_pushthread(L);
  lua_pushboolean(L, 1);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  printf("Pausing L=%p\n", L);
  return lua_yield(L, 0);
}

static void luv_resume(lua_State* L, int nargs) {
  int ret;

  // Remove the current thread from active_threads
  lua_getfield(L, LUA_REGISTRYINDEX, "active_threads");
  lua_pushthread(L);
  lua_pushnil(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  printf("Resuming L=%p\n", L);
  ret = lua_resume(L, NULL, nargs);
  if (ret && ret != LUA_YIELD) {
    on_panic(L);
  }
}

static void luv_resume_with_status(lua_State* L, int status, int nargs) {
  lua_getglobal(L, "_ref");
  luaL_unref(L, LUA_REGISTRYINDEX, lua_tointeger(L, -1));
  lua_pushnil(L);
  lua_setglobal(L, "_ref");
  if (status < 0) {
    fprintf(stderr, "%s: %s\n", uv_err_name(status), uv_strerror(status));
    lua_pushstring(L, uv_err_name(status));
  }
  else {
    lua_pushnil(L);
  }
  if (nargs) {
    lua_insert(L, -1 - nargs);
  }
  return luv_resume(L, 1 + nargs);
}
