-- Alternate rockspec that uses luarocks builtin builder
package = "luv"
version = "scm-0"
source = {
  url = 'git://github.com/luvit/luv.git'
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
    header = 'uv.h',
    library = 'uv',
  },
  LUA_COMPAT53 = {
    header = "compat-5.3.h"
  }
}

build = {
  type = 'builtin',
  -- default (platform-agnostic) configuration
  modules = {
    ['luv'] = {
      sources = {'src/luv.c'},
      libraries = {'uv'},
      incdirs = {"$(LIBUV_INCDIR)","$(LUA_COMPAT53_INCDIR)"},
      libdirs = {"$(LIBUV_LIBDIR)"}
    }
  };
  -- per-platform overrides
  platforms = {
    linux = {
      modules = {
        ['luv'] = {
          libraries = {
            nil;
            'pthread';
            'rt';
            'dl';
          };
        };
      };
    };
    freebsd = {
      modules = {
        ['luv'] = {
          libraries = {
            nil;
            'pthread';
            'kvm';
          };
        };
      };
    };
    windows = {
      modules = {
        ['luv'] = {
          libraries = {
            nil;
            'User32';
            'psapi';
            'iphlpapi';
            'userenv';
            'ws2_32';
            'advapi32';
          };
        };
      };
    };
  }
}
