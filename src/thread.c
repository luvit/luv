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

  // private fields, avoid thread be released before it done
  lua_State *L;
  int ref;
  uv_async_t notify;
} luv_thread_t;

static lua_State* luv_thread_acquire_vm(void) {
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

static const size_t luv_table_len(lua_State* L, int idx) {
  size_t len = 0;
  lua_pushnil(L);
  while (lua_next(L, idx) != 0) {
    len++;
    lua_pop(L, 1); // remove value, keep key for next iteration
  }
  return len;
}

// Count the number of upvalues of the closure at idx.
static const size_t luv_upvalues_len(lua_State* L, int idx) {
  size_t len = 0;
  while (lua_getupvalue(L, idx, len + 1) != NULL) {
    lua_pop(L, 1);  // Remove the upvalue from the stack
    len++;
  }
  return len;
}

static int thread_arg_table_set(lua_State* L, int idx, luv_table_t** table, int cache_idx, int flags);
static void thread_arg_table_clear(lua_State* L, luv_table_t* table, int cache_idx, int args_flags, int flags);
static void thread_arg_table_push(lua_State* L, luv_table_t* table, int cache_idx, int flags);

static int thread_arg_set(lua_State* L, int idx, luv_val_t* arg, int cache_idx, int flags) {
  int side = LUVF_THREAD_SIDE(flags);
  int async = LUVF_THREAD_ASYNC(flags);

  arg->type = lua_type(L, idx);
  arg->ref[0] = arg->ref[1] = LUA_NOREF;
  switch (arg->type)
  {
    case LUA_TNIL:
      break;
    case LUA_TBOOLEAN:
      arg->val.boolean = lua_toboolean(L, idx);
      break;
    case LUA_TNUMBER:
      arg->val.num = lua_tonumber(L, idx);
      break;
    case LUA_TSTRING:
      if (async)
      {
        const char* p = lua_tolstring(L, idx, &arg->val.str.len);
        arg->val.str.base = malloc(arg->val.str.len);
        memcpy((void*)arg->val.str.base, p, arg->val.str.len);
      } else {
        arg->val.str.base = lua_tolstring(L, idx, &arg->val.str.len);
        lua_pushvalue(L, idx);
        arg->ref[side] = luaL_ref(L, LUA_REGISTRYINDEX);
      }
      break;
    case LUA_TUSERDATA:
      arg->val.udata.data = lua_topointer(L, idx);
      arg->val.udata.size = lua_rawlen(L, idx);
      arg->val.udata.metaname = luv_getmtname(L, idx);

      if (arg->val.udata.size) {
        lua_pushvalue(L, idx);
        arg->ref[side] = luaL_ref(L, LUA_REGISTRYINDEX);
      }
      break;
    case LUA_TFUNCTION:
      if (lua_iscfunction(L, idx)) {
        arg->val.function.code_len = 0;
        arg->val.function.code = lua_topointer(L, idx);
        break;
      }

      lua_pushvalue(L, idx);
      lua_gettable(L, cache_idx);
      if (!lua_isnil(L, -1)) {
        arg->val.function.code_len = 1;
        arg->val.function.code = lua_touserdata(L, -1);
        lua_pop(L, 1);
        break;
      }
      lua_pop(L, 1);

      luv_thread_dumped(L, idx);
      arg->val.function.code_len = lua_rawlen(L, -1);
      arg->val.function.code = malloc(arg->val.function.code_len);
      memcpy((void*)arg->val.function.code, lua_tostring(L, -1), arg->val.function.code_len);
      lua_pop(L, 1);

      lua_pushvalue(L, idx);
      lua_pushlightuserdata(L, (void*)arg->val.function.code);
      lua_settable(L, cache_idx);

      arg->val.function.upvalues_len = luv_upvalues_len(L, idx);
      if (arg->val.function.upvalues_len) {
        arg->val.function.upvalues = malloc(arg->val.function.upvalues_len * sizeof(luv_val_t));
        int top = lua_gettop(L);
        int i = 0;
        while(lua_getupvalue(L, idx, i + 1) != NULL) {
          if (thread_arg_set(L, top + 1, arg->val.function.upvalues + i, cache_idx, flags)) return 1;
          lua_pop(L, 1);
          i++;
        }
      } else {
        arg->val.function.upvalues = NULL;
      }
      break;
    case LUA_TTABLE: {
      if (thread_arg_table_set(L, idx, &arg->val.table, cache_idx, flags)) return 1;
      break;
    }
    default:
      return 1;
  }
  return 0;
}

static int thread_arg_table_set(lua_State* L, int idx, luv_table_t** table, int cache_idx, int flags) {
  // check if the table is reused.
  lua_pushvalue(L, idx);
  lua_gettable(L, cache_idx);
  if (!lua_isnil(L, -1)) {
    *table = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return 0;
  }
  lua_pop(L, 1);

  size_t len = luv_table_len(L, idx);
  *table = malloc(sizeof(**table) + len * sizeof(luv_table_pair_t));
  (*table)->len = len;

  // set the cache before recursion to poperly support self referencing tables.
  lua_pushvalue(L, idx);
  lua_pushlightuserdata(L, *table);
  lua_settable(L, cache_idx);

  int top = lua_gettop(L);
  int i = 0;
  lua_pushnil(L);
  while (lua_next(L, idx) != 0) {
    if (thread_arg_set(L, top + 1, &(*table)->pairs[i].key, cache_idx, flags)) return 1;
    if (thread_arg_set(L, top + 2, &(*table)->pairs[i].value, cache_idx, flags)) return 1;
    lua_pop(L, 1);
    i++;
  }

  if (lua_getmetatable(L, idx)) {
    if (thread_arg_table_set(L, top + 1, &(*table)->metatable, cache_idx, flags)) return 1;
    lua_pop(L, 1);
  } else {
    (*table)->metatable = NULL;
  }
  return 0;
}

static int luv_thread_arg_set(lua_State* L, luv_thread_arg_t* args, int idx, int top, int flags) {
  idx = idx > 0 ? idx : 1;
  int i = idx;
  args->flags = flags;
  luv_val_t* arg = args->argv;

  lua_newtable(L);
  int cache_idx = lua_gettop(L);

  while (i <= top && i < LUV_THREAD_MAXNUM_ARG + idx) {
    if (thread_arg_set(L, i, arg, cache_idx, flags)) {
      args->argc = i - idx;
      lua_settop(L, cache_idx - 1);
      lua_pushinteger(L, arg->type);
      lua_pushinteger(L, i - idx + 1);
      return -1;
    }
    i++;
    arg++;
  }
  lua_remove(L, cache_idx);
  args->argc = i - idx;
  return args->argc;
}

static void thread_arg_clear(lua_State* L, luv_val_t* arg, int cache_idx, int args_flags, int flags) {
  int side = LUVF_THREAD_SIDE(flags);
  int set = LUVF_THREAD_SIDE(args_flags);
  int async = LUVF_THREAD_ASYNC(args_flags);

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
          lua_pop(L, 1);
        }
        luaL_unref(L, LUA_REGISTRYINDEX, arg->ref[side]);
        arg->ref[side] = LUA_NOREF;
      }
      break;
    case LUA_TFUNCTION:
      if (set != side && arg->val.function.code_len) {
        lua_pushlightuserdata(L, (void*)arg->val.function.code);
        lua_gettable(L, cache_idx);
        if (!lua_isnil(L, -1)) {
          lua_pop(L, 1);
          break;
        }
        lua_pop(L, 1);

        lua_pushlightuserdata(L, (void*)arg->val.function.code);
        lua_pushboolean(L, 1);
        lua_settable(L, cache_idx);

        free((void*)arg->val.function.code);
        arg->val.function.code = NULL;
        for (int i = 0; i < arg->val.function.upvalues_len; i++) {
          thread_arg_clear(L, arg->val.function.upvalues + i, cache_idx, args_flags, flags);
        }
        if (arg->val.function.upvalues_len) {
          free(arg->val.function.upvalues);
        }
      }
      break;
    case LUA_TTABLE:
      if (side != set) {
        thread_arg_table_clear(L, arg->val.table, cache_idx, args_flags, flags);
      }
      break;
    default:
      break;
  }
}

static void thread_arg_table_clear(lua_State* L, luv_table_t* table, int cache_idx, int args_flags, int flags) {
  // check if the table is already freed (for reused tables).
  lua_pushlightuserdata(L, table);
  lua_gettable(L, cache_idx);
  if (!lua_isnil(L, -1)) {
    lua_pop(L, 1);
    return;
  }
  lua_pop(L, 1);

  lua_pushlightuserdata(L, table);
  lua_pushboolean(L, 1);
  lua_settable(L, cache_idx);

  if (table->metatable) {
    thread_arg_table_clear(L, table->metatable, cache_idx, args_flags, flags);
  }
  for (int i = 0; i < table->len; i++) {
    thread_arg_clear(L, &table->pairs[i].key, cache_idx, args_flags, flags);
    thread_arg_clear(L, &table->pairs[i].value, cache_idx, args_flags, flags);
  }
  free(table);
}

static void luv_thread_arg_clear(lua_State* L, luv_thread_arg_t* args, int flags) {
  if (args->argc == 0)
    return;

  lua_newtable(L);
  int cache_idx = lua_gettop(L);
  for (int i = 0; i < args->argc; i++) {
    thread_arg_clear(L, args->argv + i, cache_idx, args->flags, flags);
  }
  lua_remove(L, cache_idx);
}

static int thread_arg_push(lua_State* L, luv_val_t* arg, int cache_idx, int flags) {
  int side = LUVF_THREAD_SIDE(flags);

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
    case LUA_TFUNCTION:
      if (!arg->val.function.code_len) {
        lua_pushcfunction(L, arg->val.function.code);
      } else {
        lua_pushlightuserdata(L, (void*)arg->val.function.code);
        lua_gettable(L, cache_idx);
        if (!lua_isnil(L, -1)) {
          break;
        }
        lua_pop(L, 1);

        luaL_loadbuffer(L, arg->val.function.code, arg->val.function.code_len, "=thread");

        lua_pushlightuserdata(L, (void*)arg->val.function.code);
        lua_pushvalue(L, -2);
        lua_settable(L, cache_idx);

        for (int i = 0; i < arg->val.function.upvalues_len; i++) {
          thread_arg_push(L, arg->val.function.upvalues + i, cache_idx, flags);
          lua_setupvalue(L, -2, i + 1);
        }
      }
      break;
    case LUA_TTABLE:
      thread_arg_table_push(L, arg->val.table, cache_idx, flags);
      break;
    default:
      fprintf(stderr, "Error: thread arg not support type %s", lua_typename(L, arg->type));
  }
  return 0;
}

static void thread_arg_table_push(lua_State* L, luv_table_t* table, int cache_idx, int flags) {
  lua_pushlightuserdata(L, table);
  lua_gettable(L, cache_idx);
  if (!lua_isnil(L, -1)) {
    return;
  }
  lua_pop(L, 1);

  lua_newtable(L);

  lua_pushlightuserdata(L, table);
  lua_pushvalue(L, -2); // duplicate the result table
  lua_settable(L, cache_idx);

  for (int i = 0; i < table->len; i++) {
    thread_arg_push(L, &table->pairs[i].key, cache_idx, flags);
    thread_arg_push(L, &table->pairs[i].value, cache_idx, flags);
    lua_settable(L, -3);
  }
  if (table->metatable) {
    thread_arg_table_push(L, table->metatable, cache_idx, flags);
    lua_setmetatable(L, -2);
  }
}

static int luv_thread_arg_push(lua_State* L, luv_thread_arg_t* args, int flags) {
  lua_newtable(L);
  int cache_idx = lua_gettop(L);
  int i = 0;
  while (i < args->argc) {
    thread_arg_push(L, args->argv + i, cache_idx, flags);
    i++;
  }
  lua_remove(L, cache_idx);
  return i;
}

static int luv_thread_arg_error(lua_State *L) {
  int type = lua_tointeger(L, -2);
  int pos = lua_tointeger(L, -1);
  lua_pop(L, 2);
  return luaL_error(L, "Error: thread arg not support type '%s' at %d",
    lua_typename(L, type), pos);
}

static int thread_dump (lua_State *L, const void *b, size_t size, void *ud) {
  luaL_Buffer *B = (luaL_Buffer *)ud;
  (void)L;

  luaL_addlstring(B, (const char *)b, size);
  return 0;
}

static int luv_thread_dumped(lua_State* L, int idx) {
  if (lua_isstring(L, idx)) {
    lua_pushvalue(L, idx);
  } else {
    int ret, top;
    luaL_Buffer B;

    // In Lua >= 5.4.3, luaL_buffinit pushes a value onto the stack, so it needs to be called
    // here to ensure that the function is at the top of the stack during the lua_dump call
    luaL_buffinit(L, &B);
    luaL_checktype(L, idx, LUA_TFUNCTION);
    lua_pushvalue(L, idx);
    top = lua_gettop(L);
    ret = lua_dump(L, thread_dump, &B, 1);
    lua_remove(L, top);
    if (ret==0) {
      luaL_pushresult(&B);
    } else
      luaL_error(L, "Error: unable to dump given function");
  }
  return 1;
}

static luv_thread_t* luv_check_thread(lua_State* L, int index) {
  luv_thread_t* thread = (luv_thread_t*)luaL_checkudata(L, index, "uv_thread");
  return thread;
}

static int luv_thread_gc(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  free(tid->code);
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
  luv_ctx_t *ctx = luv_context(L);

  lua_pushboolean(L, 1);
  lua_setglobal(L, "_THREAD");

  //push lua function, thread entry
  if (luaL_loadbuffer(L, thd->code, thd->len, "=thread") == 0) {
    //push parameter for real thread function
    int i = luv_thread_arg_push(L, &thd->args, LUVF_THREAD_SIDE_CHILD);

    ctx->thrd_pcall(L, i, 0, 0);
    luv_thread_arg_clear(L, &thd->args, LUVF_THREAD_SIDE_CHILD);
  } else {
    fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
    //pop errmsg
    lua_pop(L, 1);
  }

  uv_async_send(&thd->notify);
  release_vm_cb(L);
}

static void luv_thread_notify_close_cb(uv_handle_t *handle) {
  luv_thread_t *thread = handle->data;
  if (thread->handle != 0)
    uv_thread_join(&thread->handle);

  luaL_unref(thread->L, LUA_REGISTRYINDEX, thread->ref);
  thread->ref = LUA_NOREF;
  thread->L = NULL;
}

static void luv_thread_exit_cb(uv_async_t* handle) {
  uv_close((uv_handle_t*)handle, luv_thread_notify_close_cb);
}

static int luv_new_thread(lua_State* L) {
  int ret;
  size_t len;
  char* code;
  luv_thread_t* thread;
  int cbidx = 1;
  luv_ctx_t* ctx = luv_context(L);

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

  luv_thread_dumped(L, cbidx);
  len = lua_rawlen(L, -1);
  code = malloc(len);
  memcpy(code, lua_tostring(L, -1), len);

  thread = (luv_thread_t*)lua_newuserdata(L, sizeof(*thread));
  memset(thread, 0, sizeof(*thread));
  luaL_getmetatable(L, "uv_thread");
  lua_setmetatable(L, -2);

  thread->len = len;
  thread->code = code;
  lua_remove(L, -2);
  //clear in luv_thread_gc or in child threads
  thread->argc = luv_thread_arg_set(L, &thread->args, cbidx+1, lua_gettop(L) - 1, LUVF_THREAD_SIDE_MAIN);
  if (thread->argc < 0) {
    return luv_thread_arg_error(L);
  }
  thread->len = len;

  thread->notify.data = thread;
  thread->ref = LUA_NOREF;
  thread->L = L;
  ret = uv_async_init(ctx->loop, &thread->notify, luv_thread_exit_cb);
  if (ret < 0)
    return luv_error(L, ret);

  lua_pushvalue(L, -1);
  thread->ref = luaL_ref(L, LUA_REGISTRYINDEX);

#if LUV_UV_VERSION_GEQ(1, 26, 0)
  ret = uv_thread_create_ex(&thread->handle, &options, luv_thread_cb, thread);
#else
  ret = uv_thread_create(&thread->handle, luv_thread_cb, thread);
#endif
  if (ret < 0) {
    uv_close((uv_handle_t*)&thread->notify, luv_thread_notify_close_cb);
    return luv_error(L, ret);
  }

  return 1;
}

#if LUV_UV_VERSION_GEQ(1, 45, 0)
static int luv_thread_getaffinity(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int default_mask_size = uv_cpumask_size();
  if (default_mask_size < 0) {
    return luv_error(L, default_mask_size);
  }
  int mask_size = luaL_optinteger(L, 2, default_mask_size);
  if (mask_size < default_mask_size) {
    return luaL_argerror(L, 2, lua_pushfstring(L, "cpumask size must be >= %d (from cpumask_size()), got %d", default_mask_size, mask_size));
  }
  char* cpumask = malloc(mask_size);
  int ret = uv_thread_getaffinity(&tid->handle, cpumask, mask_size);
  if (ret < 0) {
    free(cpumask);
    return luv_error(L, ret);
  }
  lua_newtable(L);
  for (int i = 0; i < mask_size; i++) {
    lua_pushboolean(L, cpumask[i]);
    lua_rawseti(L, -2, i + 1);
  }
  free(cpumask);
  return 1;
}

static int luv_thread_setaffinity(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  int get_old_mask = lua_toboolean(L, 3);
  int min_mask_size = uv_cpumask_size();
  if (min_mask_size < 0) {
    return luv_error(L, min_mask_size);
  }
  int mask_size = lua_rawlen(L, 2);
  // If the provided table's length is not at least min_mask_size,
  // we'll use the min_mask_size and fill in any missing values with
  // false.
  if (mask_size < min_mask_size) {
    mask_size = min_mask_size;
  }
  char* cpumask = malloc(mask_size);
  for (int i = 0; i < mask_size; i++) {
    lua_rawgeti(L, 2, i+1);
    int val = lua_toboolean(L, -1);
    cpumask[i] = val;
    lua_pop(L, 1);
  }
  char* oldmask = get_old_mask ? malloc(mask_size) : NULL;
  int ret = uv_thread_setaffinity(&tid->handle, cpumask, oldmask, mask_size);
  // Done with cpumask at this point
  free(cpumask);
  if (ret < 0) {
    if (get_old_mask) free(oldmask);
    return luv_error(L, ret);
  }
  if (get_old_mask) {
    lua_newtable(L);
    for (int i = 0; i < mask_size; i++) {
      lua_pushboolean(L, oldmask[i]);
      lua_rawseti(L, -2, i + 1);
    }
    free(oldmask);
  }
  else {
    lua_pushboolean(L, 1);
  }
  return 1;
}

static int luv_thread_getcpu(lua_State* L) {
  int ret = uv_thread_getcpu();
  if (ret < 0) return luv_error(L, ret);
  // Make the returned value start at 1 to match how getaffinity/setaffinity
  // masks are implemented (they use array-like tables so the first
  // CPU is index 1).
  lua_pushinteger(L, ret + 1);
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 48, 0)
static int luv_thread_getpriority(lua_State* L) {
  int priority;
  luv_thread_t* tid = luv_check_thread(L, 1);
  int ret = uv_thread_getpriority(tid->handle, &priority);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, priority);
  return 1;
}

static int luv_thread_setpriority(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int priority = luaL_checkinteger(L, 2);
  int ret = uv_thread_setpriority(tid->handle, priority);
  if (ret < 0) return luv_error(L, ret);
  lua_pushboolean(L, 1);
  return 1;
}
#endif

static int luv_thread_join(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int ret = uv_thread_join(&tid->handle);
  if (ret < 0) return luv_error(L, ret);
  tid->handle = 0;
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
#if LUV_UV_VERSION_GEQ(1, 45, 0)
  {"getaffinity", luv_thread_getaffinity},
  {"setaffinity", luv_thread_setaffinity},
  {"getcpu", luv_thread_getcpu},
#endif
#if LUV_UV_VERSION_GEQ(1, 48, 0)
  {"getpriority", luv_thread_getpriority},
  {"setpriority", luv_thread_setpriority},
#endif
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
