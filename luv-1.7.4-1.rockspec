package = "luv"
version = "1.7.4-1"
source = {
  url = 'https://github.com/luvit/luv/releases/download/1.7.4-1/luv-1.7.4-1.tar.gz',
}

description = {
  summary = "Bare libuv bindings for lua",
  detailed = [[
libuv bindings for luajit and lua 5.1/5.3.

This library makes libuv available to lua scripts. It was made for the luvit project but should usable from nearly any lua project.
  ]],
  homepage = "https://github.com/luvit/luv",
  license = "Apache 2.0"
}

dependencies = {
  "lua >= 5.1"
}

-- cmake -Bbuild -H. -DBUILD_SHARED_LIBS=ON
-- cmake --build build --target install --config Release
build = {
  type = 'cmake',
  build_variables = {
     CFLAGS="$(CFLAGS)",
     LIBFLAG="$(LIBFLAG)",
     LUA_LIBDIR="$(LUA_LIBDIR)",
     LUA_BINDIR="$(LUA_BINDIR)",
     LUA_INCDIR="$(LUA_INCDIR)",
     LUA="$(LUA)",
     BUILD_SHARED_LIBS="ON",
  },
  install_variables = {
     PREFIX="$(PREFIX)",
     INSTALL_BINDIR="$(BINDIR)",
     INSTALL_LIBDIR="$(LIBDIR)",
     INSTALL_LUADIR="$(LUADIR)",
     INSTALL_CONFDIR="$(CONFDIR)",
  },
}
