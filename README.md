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



## Semantics
