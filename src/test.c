#include "luv.h"
#include "util.h"
#include "lhandle.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <uv.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#if (LUA_VERSION_NUM < 503)
#include "compat-5.3.h"
#endif

#define ASSERT_BASE(a, operator, b, type, conv)              \
 do {                                                        \
  volatile type eval_a = (type) (a);                         \
  volatile type eval_b = (type) (b);                         \
  if (!(eval_a operator eval_b)) {                           \
    fprintf(stderr,                                          \
            "Assertion failed in %s on line %d: `%s %s %s` " \
            "(%"conv" %s %"conv")\n",                        \
            __FILE__,                                        \
            __LINE__,                                        \
            #a,                                              \
            #operator,                                       \
            #b,                                              \
            eval_a,                                          \
            #operator,                                       \
            eval_b);                                         \
    abort();                                                 \
  }                                                          \
 } while (0)

#define ASSERT_OK(a) ASSERT_BASE(a, ==, 0, int64_t, PRId64)

static lua_State *vm_acquire(uv_loop_t* loop) {
  lua_State* L = luaL_newstate();
  if (L == NULL)
    return L;

  // Add in the lua standard and compat libraries
  luaL_openlibs(L);

  if (loop)
    luv_set_loop(L, loop);

  // Get package.loaded so that we can load modules
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded");

  // load luv into uv in advance so that the metatables for async work.
  luaL_requiref(L, "uv", luaopen_luv, 0);
  lua_setfield(L, -2, "luv");

  // remove package.loaded
  lua_remove(L, -1);

  return L;
}

static void vm_release(lua_State* L) { lua_close(L); }


static lua_State* luv_thread_acquire_vm(void) {
  lua_State* L = vm_acquire(NULL);  /* create state */

  lua_pushboolean(L, 1);
  lua_setglobal(L, "_THREAD");

  return L;
}

static void luv_thread_release_vm(lua_State* L) {
  vm_release(L);
}

static int pmain(lua_State* L)
{
  const char* fn = luaL_checkstring(L, 1);
  return luaL_dofile(L, fn);
}

static void walk_cb(uv_handle_t* handle, void* arg) {
  luv_ctx_t* ctx = (luv_ctx_t*)arg;
  lua_State* L = ctx->L;
  luv_handle_t* data = (luv_handle_t*)handle->data;

  // Skip foreign handles (shared event loop)
  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->ht_ref);
  lua_rawgetp(L, -1, data);

#if LUV_UV_VERSION_GEQ(1, 19, 0)
  printf("handle(%s:%p) can be process by %s\n",
    uv_handle_type_name(handle->type), handle,
    (lua_isnil(L, -1) ? "C" : "Lua" ));
#else
  printf("handle(%d:%p) can be process by %s\n",
    handle->type, handle, (lua_isnil(L, -1) ? "C" : "Lua" ));
#endif
  lua_pop(L, 2);
}

static void call_walk(uv_timer_t* handle) {
  luv_ctx_t* ctx = (luv_ctx_t*)handle->data;
  uv_walk(ctx->loop, walk_cb, ctx);
  uv_close((uv_handle_t*)handle, NULL);
}

int main(int argc, char *argv[]) {

  lua_State* L;
  int res;
  uv_loop_t loop;
  uv_timer_t timer_handle;

  if (argc != 2) {
    printf("usage: %s luafile\n", argv[0]);
    return 1;
  };

  // Hooks in libuv that need to be done in main.
  argv = uv_setup_args(argc, argv);
  uv_loop_init(&loop);

  luv_set_thread_cb(luv_thread_acquire_vm, luv_thread_release_vm);
  // Create the lua state.
  L = vm_acquire(&loop);
  if (L == NULL) {
    fprintf(stderr, "luaL_newstate has failed\n");
    return 1;
  };


  ASSERT_OK(uv_timer_init(&loop, &timer_handle));
  timer_handle.data = luv_context(L);
  ASSERT_OK(uv_timer_start(&timer_handle, (uv_timer_cb) call_walk, 800, 0));
  uv_unref((uv_handle_t*) &timer_handle);

  lua_pushcfunction(L, pmain);
  lua_pushstring(L, argv[1]);
  res = lua_pcall(L, 1, 0, 0);
  if (res != 0) {
    fprintf(stderr, "lua error: code(%d) with message: %s", res, lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  uv_run(&loop, UV_RUN_DEFAULT);

  vm_release(L);
  uv_loop_close(&loop);
  return res;
}
