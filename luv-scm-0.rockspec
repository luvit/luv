package = "luv"
version = "scm-0"
source = {
  url = 'git://github.com/luvit/luv.git'
}
rockspec_format = "3.0"

description = {
  summary = "Bare libuv bindings for lua",
  detailed = [[
libuv bindings for luajit and lua 5.1/5.2/5.3/5.4.

This library makes libuv available to lua scripts. It was made for the luvit
project but should usable from nearly any lua project.
  ]],
  homepage = "https://github.com/luvit/luv",
  license = "Apache 2.0"
}

dependencies = {
  "lua >= 5.1"
}

build = {
  type = 'cmake',
  variables = {
     CMAKE_C_FLAGS="$(CFLAGS)",
     CMAKE_MODULE_LINKER_FLAGS="$(LIBFLAG)",
     LUA_LIBDIR="$(LUA_LIBDIR)",
     LUA_INCDIR="$(LUA_INCDIR)",
     LUA_LIBFILE="$(LUALIB)",
     LUA="$(LUA)",
     LIBDIR="$(LIBDIR)",
     LUADIR="$(LUADIR)",
  },
}

test = {
  type = "command",
  script = "tests/run.lua",
}
