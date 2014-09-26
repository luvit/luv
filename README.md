luv
===

libuv bindings for luajit and lua 5.1/5.2.

This module tries to expose all of libuv in a sane manner to lua code. It takes advantage of lua's coroutine primitive and blocks the currently running coroutine for many I/O operations.

```lua
local uv = require('luv')

-- Create a new event loop
local loop = uv.new_loop()
-- Create a timer in that loop
local timer = uv.new_timer(loop)
function timer:ontimeout()
  -- Register a event handler
  -- Event handlers run in a new coroutine
  print("Timeout!")
  -- uv.close will block this coroutine and wait for uv_close's callback
  uv.close(timer)
end
-- Start a one-shot timer
uv.timer_start(timer, 1000, 0)
print("Starting a timer")

-- Start the event loop blocking the main thread.
uv.run(loop)

-- Clean up the loop
uv.loop_close(loop)
```

## Userdata Design

The libuv types are stored directly as lua userdata values.  The memory is allocated and managed by the lua vm.

This means that we need to be careful to prevent the GC from collecting our values before they are done.  Lua can't see C internal to libuv callbacks.

Also many of the libuv APIs (mostly callbacks) give us nothing more than a uv struct pointer for context.  We need to get the appropriate `lua_State*` and the actual userdata value i nthe lua VM to handle these callbacks.

The libuv types all have a `void*` data member for exactly this kind of usage.  We still store a small C malloced struct containing:

```c
typedef struct {
  lua_State* L;
  int ref;
} luv_ref_t;
```

The lua state here will be the last known state to handle this userdata.  The ref will be a integer to get the userdata from `LUA_REGISTRYINDEX` in the lua state `L`.

We will create the ref upon userdata creation and only clear it when it's done.  This is `uv_close` for `uv_handle_t` subclasses, `uv_loop_close` for `uv_loop_t` and in the callback for `uv_req_t` subclasses.

See src/luv.h for the helper functions that are used internally by the bindings.
In general there is `luv_create_*(lua_State* L) {...}`, `luv_check_*(lua_State* L, int index) {...}`, `luv_find_*(uv_*_t* ptr) -> lua_State* L`, and `luv_unref_*(uv_*_t* ptr) -> lua_State* L` for dealing with userdata.

## Control Flow Design

TODO: document conventions and helpers for internal control-flow.


