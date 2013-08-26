luv
===

Bare [libuv][] bindings for [lua][]

This is [lua][]/[luajit][] bindings for [libuv][], the evented library that
powers [node.js][].  If you want a more opinionated framework with an improved
module system over what stock lua provides, look at the [luvit][] project.

## Build Instructions

This is a native module using the lua C addon API.  Libuv is included as a git
submodule and is required to build.

To use, first setup a lua build environment.  The easiest way on linux is to
download the latest lua or luajit (I recommend luajit) release and build from
source and install to `/usr/local`.  Or if you use your system's native package
management make sure to install the development package for lua.

Once you have a lua build environment, use the included `Makefile` to build
`luv.so`.  This single file will contain all of libuv along with the nessecary
lua bindings to make it a lua module.

## API

Luv tries to stick as close to the libuv API as possible while still being easy
to use.  This means it's mostly a flat list of functions.

I'll show some basic examples and go over the lua API here, but extensive docs
can be found at the [uv book][] site.

## Types

Libuv is a sort-of object oriented API.  There is a hierchary of object types
(implemented as structs internally).

 - `uv_handle_t` - The base abstract type.  All subtypes can also be used in functions that expect handles.
   - `uv_timer_t` - The type for timeouts and intervals.  Create instances using `new_timer()`.
   - `uv_stream_t` - A shared abstract type for all stream-like types.
     - `uv_tcp_t` - Handle type for TCP servers and clients.  Create using `new_tcp()`.
     - `uv_tty_t` - A special stream for wrapping TTY file descriptors.  Create using `new_tty(fd, readable)`.
     - `uv_pipe` - A generic stream.  Can represent inter-process pipes, named pipes, anonymous pipes, etc...
   - `uv_process_t` - The type for spawned child processes.

## Handles

The base type for all libuv types is the handle.  These functions are common
for all libuv types.

### ref(handle)

Increment the refcount of a handle manually.  Only handles with positive
refcounts will hold the main event loop open.

### unref(handle)

Decrement the refcount of a handle manually.  Only handles with positive
refcounts will hold the main event loop open.

This is useful for things like a background interval that doesn't prevent the
process from exiting naturally.

### close(handle)

Tell a handle.  Attach an `onclose` handler if you wish to be notified when it's
complete.

```lua
function handle:onclose()
  -- Now it's closed
end
uv.close(handle)
```

### is_closing(handle) -> boolean

Lets you know if a handle is already closing.

## is_active(handle) -> boolean

Returns true if the prepare/check/idle/timer handle has been started, false
otherwise. For other handle types this always returns true.

### walk(callback)

Walk all active handles.

```lua
uv.walk(function (handle, description)
  -- Do something with this information.
end)
```

## Timers

Luv provides non-blocking timers so that you can schedule code to run on an
interval or after a period of timeout.

The `uv_timer_t` handle type if a direct desccendent of `uv_handle_t`.

Here is an example of how to implement a JavaScript style `setTimeout` function:

```lua
local function setTimeout(fn, ms)
  local handle = uv.new_timer()
  function handle:ontimeout()
    fn()
    uv.timer_stop(handle)
    ub.close(handle)
  end
  uv.timer_start(handle)
end
```

And here is an example of implementing a JavaScript style `setInterval`
function:

```lua
local function setInterval(fn, ms)
  local handle = uv.new_timer()
  function handle:ontimeout()
    fn();
  end
  uv.timer_start(handle, ms, ms)
  return handle
end

local clearTimer(handle)
  uv.timer_stop(handle)
  uv.close(handle)
end
```

And here is a more advanced example that creates a repeating timer that halves
the delay each iteration till it's down to 1ms.

```lua
local handle = uv.new_timer()
local delay = 1024
function handle:ontimeout()
  p("tick", delay)
  delay = delay / 2
  if delay >= 1 then
    uv.timer_set_repeat(handle, delay)
    uv.timer_again(handle)
  else
    uv.timer_stop(handle)
    uv.close(handle)
    p("done")
  end
end
uv.timer_start(handle, delay, 0)
```

### new_timer() -> uv_timer_t

Create a new timer userdata.  Later this can be turned into a timeout or
interval using the timer functions.

### timer_start(timer, timeout, repeat)

Given a timer handle, start it with a timeout and repeat.  To create a one-shot
timeout, set repeat to zero.  For a recurring interval, set the same value to
repeat.  Attach the `ontimeout` listener to know when it timeouts.

### timer_stop(timer)

Stop a timer.  Use this to cancel timeouts or intervals.

### timer_again(timer)

Use this to resume a stopped timer.

### timer_set_repeat(timer, repeat)

Give the timer a new repeat value.  If it was stopped, you'll need to resume it
as well after setting this.

### timer_get_repeat(timer) -> repeat

Read the repeat value out of a timer instance

## Streams

Stream is a common interface between several libuv types.  Concrete types that
are also streams include `uv_tty_t`, `uv_tcp_t`, and `uv_pipe_t`.

The handle functions also accept streams as handles.

### write(stream, value[s], [callback])

Write a value (or array of values) to a stream.  The optional callback is to know when
it's done writing.

### shutdown(stream, [callback])

Flush a stream and shut it down with optional completion callback.

### read_start(stream)

Tell a readable stream to start pulling from it's input source.

Data events can be caught by adding an `onread` function on the stream.

### read_stop(stream)

Tell a readable stream to stop pulling from it's input source.

### listen(stream)

For streams that are able to accept connections, this tells them to start
listening.

To handle the connections, attach an `onconnection` function to the stream.

### accept(server, client)

When a server stream gets a connection, it needs to create a client stream and
accept it.

See TCP for an example.

### is_readable(stream) -> boolean

Check if a stream is readable.

### is_writable(stream) -> boolean

Check if a stream is writable.

## TCP

TCP is for network streams.  It works with all the stream functions as well

An example server:

```lua
local server = uv.new_tcp()
uv.tcp_bind(server, "0.0.0.0", 8080)
function server:onconnection()
  local client = uv.new_tcp()
  uv.accept(server, client)
  print("A client connected")
  -- now do something with the client...
end
uv.listen(server)
```

An example client:

```lua
local client = uv.new_tcp()
function client:ondata(chunk)
  -- handle data
end
function client:onend()
  -- handle end
  uv.close(client)
end
uv.tcp_connect(client, "127.0.0.1", 8080, function ()
  uv.read_start(client)
  uv.write(client, "Hello")
  uv.write(client, "World", function ()
    -- both were written
  end)
end)
```

### new_tcp() -> uv_tcp_t

Create a new tcp userdata.  This can later be turned into a TCP server or
client.

### tcp_bind(tcp, host, port)

Bind to a TCP port on the system.  Used mostly for creating servers.

### tcp_getsockname(tcp) -> sockname

Get the local address of a TCP socket or server.

```lua
> uv.tcp_getsockname(server)
{ address = "0.0.0.0", family = 2, port = 5000 }
```

### tcp_getpeername(tcp)

Get the remote address from a TCP socket.

### tcp_connect(tcp, host, port, [callback])

Connect to a remote TCP server.  The optional callback will be called when the
connection finishes.

### tcp_open(tcp, fd)

Open an existing file descriptor.  Used to share TCP connections between processes.

### tcp_nodelay(tcp, value)

Enable or disable [nagle] on the TCP socket.

### tcp_keepalive(tcp, value)

Enable or disable TCP keepalive on the socket.

## TTY

TTY is for stdio file descriptors that are connected to interactive terminals
and exposes them as streams.

### new_tty(fd, readable) -> uv_tty_t

Create a new tty userdata.  This is good for wrapping stdin, stdout, and stderr
when the process is used via tty.  The tty type inherits from the stream type in
libuv.

`fd is an integer (0 for stdin, 1 for stdout, 2 for stderr). readable is a
`boolean (usually only for stdin).

```lua
local std = {
  in = uv.new_tty(0, true),
  out = uv.new_tty(1),
  err = uv.new_tty(2)
}
```

### tty_set_mode(tty, mode)

Set the mode of a tty stream.

### tty_reset_mode(tty)

Reset the mode of a tty stream.

### tty_get_winsize(tty) -> width, height

Get the window size of a tty.

## Pipe

Pipes can be used for many things such as named pipes, anonymous pipes,
child_process stdio, or even file stream pipes.

Named pipes work much like TCP clients and servers using the same stream
functions.

Server example:

```lua
local server = uv.new_pipe()
uv.pipe_bind(server, "/tmp/myserver")
function server:onconnection()
  local client = uv.new_pipe()
  uv.accept(server, client)
  -- now do something with the client...
end
uv.listen(server)
```

Client example:

```lua
local client = uv.new_pipe()
function client:ondata(chunk)
  -- handle data
end
function client:onend()
  -- handle end
  uv.close(client)
end
uv.pipe_connect(client, "/tmp/myserver", function ()
  uv.read_start(client)
  uv.write(client, "Hello")
  uv.write(client, "World", function ()
    -- both were written
  end)
end)
```

### new_pipe(ipc) -> uv_pipe_t

Create a new pipe userdata.  `ipc` is normally `false` or omitted except for
advanced usage.

### pipe_open(pipe, fd)

Open a file descriptor as a pipe.

### pipe_bind(pipe, path)

Create a named pipe by providing a filesystem path

### pipe_connect(pipe, path)

Connect to a named pipe via a path.

## Processes

Luv has powerful support for non-blocking child-processes complete with three
streams for stdin, stdout, and stderr.

### spawn(command, [args], [options]) -> process, stdin, stdout, stderr, pid

Create a child-process.  Command is the executable to spawn.  If an args array
is provided, those are the extra arguments passed to the process.

Available options are:

 - `cwd` - The cwd to run the child process in.
 - `env` - A table of key-value pairs for a custom environment for the child.

```lua
local child, stdout, stdin, stderr, pid = uv.spawn("ls", {"-lh"}, {cwd = "/home/tim"})
-- the stdio values are pipe streams.
```

### process_kill(process, [signal])

Kill a child process.  If no signal is specified, then it will default to
`SIGTERM`.

The signal can be either an interger or a string like `"SIGTERM"`.

Note that even though this is called "kill", some signals don't actually kill
the process.

### kill(fd, signal)

Kill an arbitrary process by pid.  Otherwise same semantics as `process_kill`.

## File System

All filesystem commands can be run in blocking or non-blocking mode.

If you want non-blocking mode, provide a callback at the end and it will be
called when the operation is complete.

### fs_open(path, flags, mode, [callback]) -> fd

### fs_close(fd, [callback])

### fs_read(fd, length, offset, [callback]) -> value

### fs_write(fd, value, offset, [callback]) -> bytes written

### fs_stat(path, [callback]) -> stat

### fs_fstat(fd, [callback]) -> stat

### fs_lstat(path, [callback]) -> stat

### fs_unlink(path, [callback])

### fs_mkdir(path, mode, [callback])

### fs_rmdir(path, [callback])

### fs_rename(path, new_path, [callback])

### fs_fsync(fd, [callback])

### fs_fdatasync(fd, [callback])

### fs_ftruncate(fd, offset, [callback])

### fs_sendfile(out_fd, in_fd, in_offset, length, [callback]) -> bytes written

### fs_chmod(path, mode, [callback])

### fs_fchmod(fd, mode, [callback])

### fs_chown(path, uid, gid, [callback])

### fs_fchown(fd, uid, git, [callback])

### fs_utime(path, atime, mtime, [callback])

### fs_futime(fd, atime, mtime, [callback])

### fs_link(path, new_path, [callback])

### fs_symlink(path, new_path, flags, [callback])

### fs_readlink(path, [callback]) -> target

## Miscellaneous

Libuv exposes a lot of random, but useful functions that help when interacting
with the underlying operating system.

### guess_handle(fd) -> type string

Given a file descriptor, this will guess the type of stream.  This is especially
useful to find out if stdin, stdout, and stderr are indeed TTY streams or if
they are something else.

```lua
local stdin
if uv.guess_handle(0) == "TTY" then
  stdin = uv.new_tty(0)
else
  ...
end
```

### update_time()

Force libuv to internally update it's time.

### now() -> timestamp

Get a relative timestamp. The value is in milliseconds, but is only good for
relative measurements, not dates.  Use `update_time()` if you are getting stale
values.

```lua
uv.update_time()
local start = uv.now()

-- Do something slow

uv.update_time()
local delta = uv.now() - start
```

### loadavg() -> load

Get the load average as 3 returned integers.

### execpath() -> path to executable

Gives the absolute path to the executable.  This would usually be wherever
luajit or lua is installed on your system.  Useful for writing portable
applications that embed their own copy of lua and want to load resources
relative to it in the filesystem.  This is much more reliable and useful than
simply getting `argv[0]`.

```lua
local path = uv.execpath()
```

### cwd() -> path to current working directory

```lua
local cwd = uv.cwd()
```

### chdir(path)

Change current working directory to a new path.

### get_free_memory() -> bytes of free mem

Get the number of bytes of free memory.

### get_total_memory() -> bytes of total mem

Get the number of byyed of total memory.

### get_process_title() -> title

NOTE: currently does not work on some platforms because we're a library and
can't access `argc` and `argv` directly.

Get the current process title as reported in tools like top.

### set_process_title(title)

NOTE: currently does not work on some platforms because we're a library and
can't access `argc` and `argv` directly.

Get the current process title as reported in tools like top.

### hrtime() -> timestamp

High-resolution timestamp in ms as a floating point number.  Value is relative
and not absolute like a date based timestamp.

### uptime() -> uptime

Return the computer's current uptime in seconds as a floating point number.

### cpu_info() -> info

Return detailed information about the CPUs.

```lua
> uv.cpu_info()
{
  { times = table: 0x41133530, model = "Intel(R) Core(TM) i5-3427U CPU @ 1.80GHz", speed = 1800 },
  { times = table: 0x41129e00, model = "Intel(R) Core(TM) i5-3427U CPU @ 1.80GHz", speed = 1800 },
  { times = table: 0x4112a020, model = "Intel(R) Core(TM) i5-3427U CPU @ 1.80GHz", speed = 1801 },
  { times = table: 0x4112a278, model = "Intel(R) Core(TM) i5-3427U CPU @ 1.80GHz", speed = 1800 }
}
> uv.cpu_info()[1]
{
  times = { irq = 600, user = 8959100, idle = 78622000, sys = 2754000, nice = 0 },
  model = "Intel(R) Core(TM) i5-3427U CPU @ 1.80GHz",
  speed = 1800
}
```

### interface_addresses() -> info

Return detailed information about network interfaces.

```lua
> uv.interface_addresses()
{ lo = { table: 0x4112fb90, table: 0x4112f460 }, wlan0 = { table: 0x41132360, table: 0x41132e68 } }
> uv.interface_addresses().lo
{
  { address = "127.0.0.1", internal = true, family = "IPv4" },
  { address = "::1", internal = true, family = "IPv6" }
}
> uv.interface_addresses().wlan0
{
  { address = "172.19.131.166", internal = false, family = "IPv4" },
  { address = "fe80::9e2a:70ff:fe0c:ba8b", internal = false, family = "IPv6" }
}
```


[lua]: http://www.lua.org/
[luajit]: http://luajit.org/
[libuv]: https://github.com/joyent/libuv
[node.js]: http://nodejs.org/
[luvit]: http://luvit.io/
[uv book]: http://nikhilm.github.io/uvbook/
[nagle]: http://en.wikipedia.org/wiki/Nagle's_algorithm
