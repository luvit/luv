-- Alternate rockspec that uses luarocks builtin builder
package = "luv"
version = "1.8.0-4"
source = {
  url = 'https://github.com/luvit/luv/releases/download/'..version..'/luv-'..version..'.tar.gz'
}

description = {
  summary = "Bare libuv bindings for lua",
  detailed = [[
libuv bindings for luajit and lua 5.1/5.2/5.3.

This library makes libuv available to lua scripts. It was made for the luvit
project but should usable from nearly any lua project.
  ]],
  homepage = "https://github.com/luvit/luv",
  license = "Apache 2.0"
}

dependencies = {
  "lua >= 5.1"
}

external_dependencies = {
  LIBUV = {
    header = 'uv.h'
  }
}

local function make_modules()
  return {
    ['luv'] = {
      sources = {'src/luv.c'},
      libraries = {'uv'},
      incdirs = {"$(LIBUV_INCDIR)"},
      libdirs = {"$(LIBUV_LIBDIR)"}
    }
  }
end

local function make_plat(plat)
  local modules = make_modules()
  local libs = modules['luv'].libraries

  if plat == 'windows' then
    libs[#libs + 1] = 'psapi'
    libs[#libs + 1] = 'iphlpapi'
    libs[#libs + 1] = 'userenv'
    libs[#libs + 1] = 'ws2_32'
    libs[#libs + 1] = 'advapi32'
  else
    libs[#libs + 1] = 'pthread'
  end

  if plat == 'freebsd' then
    libs[#libs + 1] = 'kvm'
  end

  if plat == 'linux' then
    libs[#libs + 1] = 'rt'
    libs[#libs + 1] = 'dl'
  end

  return { modules = modules }
end

build = {
  type = 'builtin',
  -- default (platform-agnostic) configuration
  modules = make_modules(),

  -- per-platform overrides
  -- https://github.com/keplerproject/luarocks/wiki/Platform-agnostic-external-dependencies
  platforms = {
    linux = make_plat('linux'),
    freebsd = make_plat('freebsd'),
    windows = make_plat('windows')
  }
}
