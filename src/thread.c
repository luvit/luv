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

typedef struct {
  uv_thread_t handle;
  char* code;
  int len;
  int argc;
  luv_thread_arg_t args;
} luv_thread_t;

static lua_State* luv_thread_acquire_vm() {
  lua_State* L = luaL_newstate();

  // Add in the lua standard libraries
  luaL_openlibs(L);

  // Get package.loaded, so we can store uv in it.
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded");
  lua_remove(L, -2); // Remove package

  // Store uv module definition at loaded.uv
  luaopen_luv(L);
  lua_setfield(L, -2, "luv");
  lua_pop(L, 1);

  return L;
}

static void luv_thread_release_vm(lua_State* L) {
  lua_close(L);
}

static const char* luv_getmtname(lua_State *L, int idx) {
  const char* name;
  lua_getmetatable(L, idx);
  lua_pushstring(L, "__name");
  lua_rawget(L, -2);
  name = lua_tostring(L, -1);
  lua_pop(L, 2);
  return name;
}

static int luv_thread_arg_set(lua_State* L, luv_thread_arg_t* args, int idx, int top, int flags) {
  int i;
  int side = LUVF_THREAD_SIDE(flags);
  int async = LUVF_THREAD_ASYNC(flags);

  idx = idx > 0 ? idx : 1;
  i = idx;
  args->flags = flags;
  while (i <= top && i <= LUV_THREAD_MAXNUM_ARG + idx)
  {
    luv_val_t *arg = args->argv + i - idx;
    arg->type = lua_type(L, i);
    arg->ref[0] = arg->ref[1] = LUA_NOREF;
    switch (arg->type)
    {
    case LUA_TNIL:
      break;
    case LUA_TBOOLEAN:
      arg->val.boolean = lua_toboolean(L, i);
      break;
    case LUA_TNUMBER:
      arg->val.num = lua_tonumber(L, i);
      break;
    case LUA_TSTRING:
      if (async)
      {
        const char* p = lua_tolstring(L, i, &arg->val.str.len);
        arg->val.str.base = malloc(arg->val.str.len);
        memcpy((void*)arg->val.str.base, p, arg->val.str.len);
      } else {
        arg->val.str.base = lua_tolstring(L, i, &arg->val.str.len);
        lua_pushvalue(L, i);
        arg->ref[side] = luaL_ref(L, LUA_REGISTRYINDEX);
      }
      break;
    case LUA_TUSERDATA:
      arg->val.udata.data = lua_topointer(L, i);
      arg->val.udata.size = lua_rawlen(L, i);
      arg->val.udata.metaname = luv_getmtname(L, i);

      if (arg->val.udata.size) {
        lua_pushvalue(L, i);
        arg->ref[side] = luaL_ref(L, LUA_REGISTRYINDEX);
      }
      break;
    default:
      fprintf(stderr, "Error: thread arg not support type '%s' at %d",
        lua_typename(L, arg->type), i);
      arg->val.str.base = NULL;
      arg->val.str.len = 0;
      break;
    }
    i++;
  }
  args->argc = i - idx;
  return args->argc;
}

static void luv_thread_arg_clear(lua_State* L, luv_thread_arg_t* args, int flags) {
  int i;
  int side = LUVF_THREAD_SIDE(flags);
  int set = LUVF_THREAD_SIDE(args->flags);
  int async = LUVF_THREAD_ASYNC(args->flags);

  if (args->argc == 0)
    return;

  for (i = 0; i < args->argc; i++) {
    luv_val_t* arg = args->argv + i;
    switch (arg->type) {
    case LUA_TSTRING:
      if (arg->ref[side] != LUA_NOREF)
      {
        luaL_unref(L, LUA_REGISTRYINDEX, arg->ref[side]);
        arg->ref[side] = LUA_NOREF;
      } else {
        if(async && set!=side)
        {
          free((void*)arg->val.str.base);
          arg->val.str.base = NULL;
          arg->val.str.len = 0;
        }
      }
      break;
    case LUA_TUSERDATA:
      if (arg->ref[side]!=LUA_NOREF)
      {
        if (side != set)
        {
          // avoid custom gc
          lua_rawgeti(L, LUA_REGISTRYINDEX, arg->ref[side]);
          lua_pushnil(L);
          lua_setmetatable(L, -2);
          lua_pop(L, -1);
        }
        luaL_unref(L, LUA_REGISTRYINDEX, arg->ref[side]);
        arg->ref[side] = LUA_NOREF;
      }
      break;
    default:
      break;
    }
  }
}

// called only in thread
static int luv_thread_arg_push(lua_State* L, luv_thread_arg_t* args, int flags) {
  int i = 0;
  int side = LUVF_THREAD_SIDE(flags);

  while (i < args->argc) {
    luv_val_t* arg = args->argv + i;
    switch (arg->type) {
    case LUA_TNIL:
      lua_pushnil(L);
      break;
    case LUA_TBOOLEAN:
      lua_pushboolean(L, arg->val.boolean);
      break;
    case LUA_TNUMBER:
      lua_pushnumber(L, arg->val.num);
      break;
    case LUA_TSTRING:
      lua_pushlstring(L, arg->val.str.base, arg->val.str.len);
      break;
    case LUA_TUSERDATA:
      if (arg->val.udata.size)
      {
        char *p = lua_newuserdata(L, arg->val.udata.size);
        memcpy(p, arg->val.udata.data, arg->val.udata.size);
        if (arg->val.udata.metaname)
        {
          luaL_getmetatable(L, arg->val.udata.metaname);
          lua_setmetatable(L, -2);
        }
        lua_pushvalue(L, -1);
        arg->ref[side] = luaL_ref(L, LUA_REGISTRYINDEX);
      }else{
        lua_pushlightuserdata(L, (void*)arg->val.udata.data);
      }
      break;
    default:
      fprintf(stderr, "Error: thread arg not support type %s at %d",
        lua_typename(L, arg->type), i + 1);
    }
    i++;
  };
  return i;
}

int thread_dump(lua_State* L, const void* p, size_t sz, void* B) {
  (void)L;
  luaL_addlstring((luaL_Buffer*) B, (const char*) p, sz);
  return 0;
}

static const char* luv_thread_dumped(lua_State* L, int idx, size_t* l) {
  if (lua_isstring(L, idx)) {
    return lua_tolstring(L, idx, l);
  } else {
    const char* buff = NULL;
    int top = lua_gettop(L);
    luaL_Buffer b;
    int test_lua_dump;
    luaL_checktype(L, idx, LUA_TFUNCTION);
    lua_pushvalue(L, idx);
    luaL_buffinit(L, &b);
    test_lua_dump = (lua_dump(L, thread_dump, &b, 1) == 0);
    if (test_lua_dump) {
      luaL_pushresult(&b);
      buff = lua_tolstring(L, -1, l);
    } else
      luaL_error(L, "Error: unable to dump given function");
    lua_settop(L, top);

    return buff;
  }
}

static luv_thread_t* luv_check_thread(lua_State* L, int index) {
  luv_thread_t* thread = (luv_thread_t*)luaL_checkudata(L, index, "uv_thread");
  return thread;
}

static int luv_thread_gc(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  free(tid->code);
  tid->code = NULL;
  tid->len = 0;
  luv_thread_arg_clear(L, &tid->args, LUVF_THREAD_SIDE_MAIN);
  return 0;
}

static int luv_thread_tostring(lua_State* L)
{
  luv_thread_t* thd = luv_check_thread(L, 1);
  lua_pushfstring(L, "uv_thread_t: %p", (void*)thd->handle);
  return 1;
}

static void luv_thread_cb(void* varg) {
  //acquire vm and get top
  luv_thread_t* thd = (luv_thread_t*)varg;
  lua_State* L = acquire_vm_cb();

  //push lua function, thread entry
  if (luaL_loadbuffer(L, thd->code, thd->len, "=thread") == 0) {
    //push parameter for real thread function
    int i = luv_thread_arg_push(L, &thd->args, LUVF_THREAD_SIDE_CHILD);

    luv_cfpcall(L, i, 0, 0);
    luv_thread_arg_clear(L, &thd->args, LUVF_THREAD_SIDE_CHILD);
  } else {
    fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
    //pop errmsg
    lua_pop(L, 1);
  }

  release_vm_cb(L);
}

static int luv_new_thread(lua_State* L) {
  int ret;
  size_t len;
  const char* buff;
  luv_thread_t* thread;
  int cbidx = 1;
#if LUV_UV_VERSION_GEQ(1, 26, 0)
  uv_thread_options_t options;
  options.flags = UV_THREAD_NO_FLAGS;
  if (lua_type(L, 1) == LUA_TTABLE)
  {
    cbidx++;

    lua_getfield(L, 1, "stack_size");
    if (!lua_isnil(L, -1))
    {
      options.flags |= UV_THREAD_HAS_STACK_SIZE;
      if (lua_isnumber(L, -1)) {
        options.stack_size = lua_tointeger(L, -1);
      }
      else {
        return luaL_argerror(L, 1, "stack_size option must be a number if set");
      }
    }
    lua_pop(L, 1);
  }
#endif

  buff = luv_thread_dumped(L, cbidx, &len);

  thread = (luv_thread_t*)lua_newuserdata(L, sizeof(*thread));
  memset(thread, 0, sizeof(*thread));
  luaL_getmetatable(L, "uv_thread");
  lua_setmetatable(L, -2);

  //clear in luv_thread_gc or in child threads
  thread->argc = luv_thread_arg_set(L, &thread->args, cbidx+1, lua_gettop(L) - 1, LUVF_THREAD_SIDE_MAIN);
  thread->len = len;
  thread->code = (char*)malloc(thread->len);
  memcpy(thread->code, buff, len);

#if LUV_UV_VERSION_GEQ(1, 26, 0)
  ret = uv_thread_create_ex(&thread->handle, &options, luv_thread_cb, thread);
#else
  ret = uv_thread_create(&thread->handle, luv_thread_cb, thread);
#endif
  if (ret < 0) return luv_error(L, ret);

  return 1;
}

static int luv_thread_join(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int ret = uv_thread_join(&tid->handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushboolean(L, 1);
  return 1;
}

static int luv_thread_self(lua_State* L)
{
  luv_thread_t* thread;
  uv_thread_t t = uv_thread_self();
  thread = (luv_thread_t*)lua_newuserdata(L, sizeof(*thread));
  memset(thread, 0, sizeof(*thread));
  memcpy(&thread->handle, &t, sizeof(t));
  luaL_getmetatable(L, "uv_thread");
  lua_setmetatable(L, -2);
  return 1;
}

static int luv_thread_equal(lua_State* L) {
  luv_thread_t* t1 = luv_check_thread(L, 1);
  luv_thread_t* t2 = luv_check_thread(L, 2);
  int ret = uv_thread_equal(&t1->handle, &t2->handle);
  lua_pushboolean(L, ret);
  return 1;
}

static const luaL_Reg luv_thread_methods[] = {
  {"equal", luv_thread_equal},
  {"join", luv_thread_join},
  {NULL, NULL}
};

static void luv_thread_init(lua_State* L) {
  luaL_newmetatable(L, "uv_thread");
  lua_pushcfunction(L, luv_thread_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_thread_equal);
  lua_setfield(L, -2, "__eq");
  lua_pushcfunction(L, luv_thread_gc);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  luaL_setfuncs(L, luv_thread_methods, 0);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  if (acquire_vm_cb == NULL) acquire_vm_cb = luv_thread_acquire_vm;
  if (release_vm_cb == NULL) release_vm_cb = luv_thread_release_vm;
}

LUALIB_API void luv_set_thread_cb(luv_acquire_vm acquire, luv_release_vm release)
{
  acquire_vm_cb = acquire;
  release_vm_cb = release;
}
