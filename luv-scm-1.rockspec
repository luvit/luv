package = "luv"
version = "scm-1"
source = {
  url = "git://github.com/creationix/luv.git"
}
description = {
  summary  = "libuv binding for Lua",
  detailed = "",
  homepage = "https://github.com/creationix/luv",
  license  = "",
}
dependencies = {
  "lua >= 5.1"
}
build = {
  type = "command",
  build_command = "cmake -E make_directory build && cd build && cmake -D INSTALL_CMOD=$(LIBDIR) .. && $(MAKE)",
  install_command = "cd build && $(MAKE) install",
}
