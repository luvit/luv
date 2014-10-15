luv
===

[![Linux Build Status](https://travis-ci.org/luvit/luv.svg?branch=master)](https://travis-ci.org/luvit/luv)

[![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/luvit/luv?branch=master&svg=true)](https://ci.appveyor.com/project/creationix/luv)

[libuv](https://github.com/joyent/libuv) bindings for [luajit](http://luajit.org/) and [lua](http://www.lua.org/) [5.1](http://www.lua.org/manual/5.1/manual.html)/[5.2](http://www.lua.org/manual/5.2/manual.html).

This library makes libuv available to lua scripts.  It was made for the [luvit](http://luvit.io/) project but should usable from nearly any lua project.

The library can be used by multiple threads at once.  Each thread is assumed to load the library from a different `lua_State`.  Luv will create a unique `uv_loop_t` for each state.  You can't share uv handles between states/loops.

The best docs currently are the [libuv docs](http://docs.libuv.org/) themselves.  Hopfully soon we'll have a copy locally tailored for lua.

```lua
local uv = require('luv')

-- This will wait 1000ms and then continue inside the callback
uv.timer_start(uv.new_timer(), 1000, 0, function (timer)
  -- timer here is the value we passed in before from new_timer.

  print ("Awake!")

  -- You must always close your uv handles or you'll leak memory
  -- We can't depend on the GC since it doesn't know enough about libuv.
  uv.close(timer)
end)

print("Sleeping");

-- uv.run will block and wait for all events to run.
-- When there are no longer any active handles, it will return
uv.run()
```


Here is an example of an TCP echo server
```lua
local function create_server(host, port, on_connection)

  local server = uv.new_tcp()
  uv.tcp_bind(server, host, port)

  uv.listen(server, 128, function(self)
    -- self is the same as server
    assert(self == server)

    -- Accept the client
    local client = uv.new_tcp()
    uv.accept(server, client)

    on_connection(client)
  end)

  return server
end

create_server("0.0.0.0", 0, function (client)

  uv.read_start(client, function (self, err, chunk)

    -- Crash on errors
    assert(not err, err)

    if chunk then
      -- Echo anything heard
      uv.write(client, chunk)
    else
      -- When the stream ends, close the socket
      uv.close(client)
    end
  end)
end)
```

More examples can be found in the [examples](examples) and [tests](tests) folders.

## Building From Source

To build, first install your compiler tools.

### Get a Compiler

On linux this probably means `gcc` and `make`.  On Ubuntu, the `build-essential`
package is good for this.

On OSX, you probably want XCode which comes with `clang` and `make` and friends.

For windows the free Visual Studio Express works.  If you get the 2013 edition,
make sure to get the `Windows Deskop` edition.  The `Windows` version doesn't
include a working C compiler.  Make sure to run all of setup including getting a
free license.

### Install CMake

Now install Cmake.  The version in `brew` on OSX or most Linux package managers
is good.  The version on Travis CI is too old and so I use a PPA there.  On
windows use the installer and make sure to add cmake to your command prompt
path.

### Install Git

If you haven't already, install git and make sure it's in your path.  This comes
with XCode on OSX.  On Linux it's in your package manager.  For windows, use the
installer at <http://git-scm.com>.  Make sure it's available to your windows
command prompt.

### Clone the Code

Now open a terminal and clone the code.  For windows I recommend the special
developer command prompt that came with Visual Studio.

```
git clone https://github.com/luvit/luv.git --recursive
cd luv
```

### Build the Code and Test

On windows I wrote a small batch file that runs the correct cmake commands and
copies the output files for easy access.

```
C:\Code\luv> msvcbuild.bat
C:\Code\luv> luajit tests\run.lua
```

On unix systems, use the Makefile.

```
~/Code/luv> make test
```
