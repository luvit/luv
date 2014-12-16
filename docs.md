# LibUV in Lua

The [luv][] project provides access to the multi-platform support library
[libuv][] to lua code.  It was primariliy developed for the [luvit][] project as
the `uv` builtin module, but can be used in other lua environments.

### TCP Echo Server Example

Here is a small example showing a TCP echo server:

```lua
local uv = require('uv')

local server = uv.new_tcp()
server:bind("127.0.0.1", 1337)
server:listen(128, function (err)
  assert(not err, err)
  local client = uv.new_tcp()
  server:accept(client)
  client:read_start(function (err, chunk)
    assert(not err, err)
    if chunk then
      client:write(chunk)
    else
      client:shutdown()
      client:close()
    end
  end)
end)
print("TCP server listening at 127.0.0.1 port 1337")
uv.run()
```

### Methods vs Functions

As a quick note, [libuv][] is a C library and as such, there are no such things
as methods.  The [luv][] bindings allow calling the libuv functions as either
functions or methods.  For example, calling `server:bind(host, port)` is
equivalent to calling `uv.tcp_bind(server, host, port)`.  All wrapped uv types
in lua have method shortcuts where is makes sense.  Some are even renamed
shorter like the `tcp_` prefix that removed in method form.  Under the hood it's
the exact same C function.

## Table Of Contents

The rest of the docs are organized by libuv type.  There is some hierarchy as
most types are considered handles and some are considered streams.

 - [`uv_loop_t`][] — Event loop
 - [`uv_handle_t`][] — Base handle
   - [`uv_timer_t`][] — Timer handle
   - [`uv_prepare_t`][] — Prepare handle
   - [`uv_check_t`][] — Check handle
   - [`uv_idle_t`][] — Idle handle
   - [`uv_async_t`][] — Async handle
   - [`uv_poll_t`][] — Poll handle
   - [`uv_signal_t`][] — Signal handle
   - [`uv_process_t`][] — Process handle
   - [`uv_stream_t`][] — Stream handle
     - [`uv_tcp_t`][] — TCP handle
     - [`uv_pipe_t`][] — Pipe handle
     - [`uv_tty_t`][] — TTY handle
   - [`uv_udp_t`][] — UDP handle
   - [`uv_fs_event_t`][] — FS Event handle
   - [`uv_fs_poll_t`][] — FS Poll handle
 - [Filesystem operations][]
 - [DNS utility functions][]
 - [Miscellaneous utilities][]

## `uv_loop_t` — Event loop

[`uv_loop_t`]: #uv_loop_t--event-loop

The event loop is the central part of libuv’s functionality. It takes care of
polling for i/o and scheduling callbacks to be run based on different sources of
events.

In [luv][], there is an implicit uv loop for every lua state that loads the
library.  You can use this library in an multithreaded environment as long as
each thread has it's own lua state with corresponsding own uv loop.

### `uv.loop_close()`

Closes all internal loop resources. This function must only be called once the
loop has finished its execution or it will raise a UV_EBUSY error.

### `uv.run([mode])`

> optional `mode` defaults to `"default"`

This function runs the event loop. It will act differently depending on the
specified mode:

 - `"default"`: Runs the event loop until there are no more active and
   referenced handles or requests. Always returns `false`.

 - `"once"`: Poll for i/o once. Note that this function blocks if there are no
   pending callbacks. Returns `false` when done (no active handles or requests
   left), or `true` if more callbacks are expected (meaning you should run
   the event loop again sometime in the future).

 - `"nowait"`: Poll for i/o once but don’t block if there are no
   pending callbacks. Returns `false` if done (no active handles or requests
   left), or `true` if more callbacks are expected (meaning you should run
   the event loop again sometime in the future).

Luvit will implicitly call `uv.run()` after loading user code, but if you use
the `luv` bindings directly, you need to call this after registering your
initial set of event callbacks to start the event loop.

### `uv.loop_alive()`

Returns true if there are active handles or request in the loop.

### `uv.stop()`

Stop the event loop, causing `uv_run()` to end as soon as possible. This
will happen not sooner than the next loop iteration. If this function was called
before blocking for i/o, the loop won’t block for i/o on this iteration.

### `uv.backend_fd()`

Get backend file descriptor. Only kqueue, epoll and event ports are supported.

This can be used in conjunction with `uv_run("nowait")` to poll in one thread
and run the event loop’s callbacks in another.

> **Note**: Embedding a kqueue fd in another kqueue pollset doesn’t work on all
  platforms. It’s not an error to add the fd but it never generates events.

### `uv.backend_timeout()`

Get the poll timeout. The return value is in milliseconds, or -1 for no timeout.

### `uv.now()`

Return the current timestamp in milliseconds. The timestamp is cached at the
start of the event loop tick, see `uv.update_time()` for details and rationale.

The timestamp increases monotonically from some arbitrary point in time. Don’t
make assumptions about the starting point, you will only get disappointed.

> **Note**: Use `uv.hrtime()` if you need sub-millisecond granularity.

### `uv.update_time()`

Update the event loop’s concept of “now”. Libuv caches the current time at the
start of the event loop tick in order to reduce the number of time-related
system calls.

You won’t normally need to call this function unless you have callbacks that
block the event loop for longer periods of time, where “longer” is somewhat
subjective but probably on the order of a millisecond or more.

### `uv.walk(callback)`

Walk the list of handles: `callback` will be executed with the handle.

```lua
-- Example usage of uv.walk to close all handles that aren't already closing.
uv.walk(function (handle)
  if not handle:is_closing() then
    handle:close()
  end
end)
```

## `uv_handle_t` — Base handle

[`uv_handle_t`]: #uv_handle_t--base-handle

### `uv.is_active(handle)`

> method form `handle:is_active()`

### `uv.is_closing(handle)`

> method form `handle:is_closing()`

### `uv.close(handle, callback)`

> method form `handle:close(callback)`

### `uv.ref(handle)`

> method form `handle:ref()`

### `uv.unref(handle)`

> method form `handle:unref()`

### `uv.has_ref(handle)`

> method form `handle:has_ref()`

### `uv.send_buffer_size(handle, size)`

> method form `handle:send_buffer_size(size)`

### `uv.recv_buffer_size(handle, size)`

> method form `handle:recv_buffer_size(size)`

### `uv.fileno(handle)`

> method form `handle:fileno()`

## `uv_timer_t` — Timer handle

[`uv_timer_t`]: #uv_timer_t--timer-handle

Timer handles are used to schedule callbacks to be called in the future.

### `uv.new_timer() -> timer`

Creates and initialized a new `uv_timer_t` returns the lua userdata wrapping it.

### `uv.timer_start(timer, timeout, repeat, callback)`

> method form `timer:start(timeout, repeat, callback)`

Start the timer. `timeout` and `repeat` are in milliseconds.

If `timeout` is zero, the callback fires on the next event loop iteration. If
`repeat` is non-zero, the callback fires first after timeout milliseconds and
then repeatedly after repeat milliseconds.

### `uv.timer_stop(timer)`

> method form `timer:stop()`

Stop the timer, the callback will not be called anymore.

### `uv.timer_again(timer)`

> method form `timer:again()`

Stop the timer, and if it is repeating restart it using the repeat value as the timeout. If the timer has never been started before it raises `EINVAL`.

### `uv.timer_set_repeat(timer, repeat)`

> method form `timer:set_repeat(repeat)`

Set the repeat value in milliseconds.

> **Note**: If the repeat value is set from a timer callback it does not
  immediately take effect. If the timer was non-repeating before, it will
  have been stopped. If it was repeating, then the old repeat value will
  have been used to schedule the next timeout.

### `uv.timer_get_repeat(timer) -> repeat`

> method form `timer:get_repeat() -> repeat`

Get the timer repeat value.

## `uv_prepare_t` — Prepare handle

[`uv_prepare_t`]: #uv_prepare_t--prepare-handle


## `uv_check_t` — Check handle

[`uv_check_t`]: #uv_check_t--check-handle

## `uv_idle_t` — Idle handle

[`uv_idle_t`]: #uv_idle_t--idle-handle

## `uv_async_t` — Async handle

[`uv_async_t`]: #uv_async_t--async-handle

## `uv_poll_t` — Poll handle

[`uv_poll_t`]: #uv_poll_t--poll-handle

## `uv_signal_t` — Signal handle

[`uv_signal_t`]: #uv_signal_t--signal-handle

## `uv_process_t` — Process handle

[`uv_process_t`]: #uv_process_t--process-handle

## `uv_stream_t` — Stream handle

[`uv_stream_t`]: #uv_stream_t--stream-handle

Stream handles provide an abstraction of a duplex communication channel.
[`uv_stream_t`][] is an abstract type, libuv provides 3 stream implementations in
the form of [`uv_tcp_t`][], [`uv_pipe_t`][] and [`uv_tty_t`][].

### `uv.shutdown(stream, callback) -> req`

> (method form `stream:shutdown(callback) -> req`)

### `uv.listen(stream, backlog, callback)`

> (method form `stream:listen(backlog, callback)`)

### `uv.accept(stream, client_stream)`

> (method form `stream:accept(client_stream)`)

### `uv.read_start(stream, callback)`

> (method form `stream:read_start(callback)`)

### `uv.read_stop(stream)`

> (method form `stream:read_stop()`)

### `uv.write(stream, data, callback)`

> (method form `stream:write(data, callback)`)

### `uv.write2(stream, data, handle, callback)`

> (method form `stream:write2(data, handle, callback)`)

### `uv.try_write(stream, data)`

> (method form `stream:try_write(data)`)

### `uv.is_readable(stream)`

> (method form `stream:is_readable()`)

### `uv.is_writable(stream)`

> (method form `stream:is_writable()`)

### `uv.stream_set_blocking(stream, blocking)`

> (method form `stream:set_blocking(blocking)`)


## `uv_tcp_t` — TCP handle

[`uv_tcp_t`]: #uv_tcp_t--tcp-handle

TCP handles are used to represent both TCP streams and servers.

`uv_tcp_t` is a ‘subclass’ of [`uv_stream_t`](#uv_stream_t--stream-handle).

### `uv.new_tcp() -> tcp`

### `uv.tcp_open(tcp, sock)`

> (method form `tcp:open(sock)`)

### `uv.tcp_nodelay(tcp, enable)`

> (method form `tcp:nodelay(enable)`)

### `uv.tcp_keepalive(tcp, enable)`

> (method form `tcp:keepalive(enable)`)

### `uv.tcp_simultaneous_accepts(tcp, enable)`

> (method form `tcp:simultaneous_accepts(enable)`)

### `uv.tcp_bind(tcp, host, port)`

> (method form `tcp:bind(host, port)`)

### `uv.tcp_getpeername(tcp)`

> (method form `tcp:getpeername()`)

### `uv.tcp_getsockname(tcp)`

> (method form `tcp:getsockname()`)

### `uv.tcp_connect(tcp, host, port, callback) -> req`

> (method form `tcp:connect(host, port, callback) -> req`)

### `uv.tcp_write_queue_size(tcp) -> size`

> (method form `tcp:write_queue_size() -> size`)


## `uv_pipe_t` — Pipe handle

[`uv_pipe_t`]: #uv_pipe_t--pipe-handle

## `uv_tty_t` — TTY handle

[`uv_tty_t`]: #uv_tty_t--tty-handle

## `uv_udp_t` — UDP handle

[`uv_udp_t`]: #uv_udp_t--udp-handle

## `uv_fs_event_t` — FS Event handle

[`uv_fs_event_t`]: #uv_fs_event_t--fs-event-handle

## `uv_fs_poll_t` — FS Poll handle

[`uv_fs_poll_t`]: #uv_fs_poll_t--fs-poll-handle

## Filesystem operations

[Filesystem operations]:#filesystem-operations

## DNS utility functions

[DNS utility functions]: #dns-utility-functions

## Miscellaneous utilities

[Miscellaneous utilities]: #miscellaneous-utilities


[luv]: https://github.com/luvit/luv
[luvit]: https://github.com/luvit/luvit
[libuv]: https://github.com/libuv/libuv

