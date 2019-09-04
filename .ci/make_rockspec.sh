#!/bin/sh

version=$1
if [ -z "$version" ]; then
  echo "must specify a version" >&2
  exit 1
fi

# .rockspec
cp luv-scm-0.rockspec luv-${version}.rockspec
cp rockspecs/luv-scm-0.rockspec rockspecs/luv-${version}.rockspec
script="/^version/s@\"[^\"]\\+\"@\"${version}\"@"
sed -e "${script}" -i luv-${version}.rockspec
sed -e "${script}" -i rockspecs/luv-${version}.rockspec
script="s@git://github.com/luvit/luv.git@https://github.com/luvit/luv/releases/download/'..version..'/luv-'..version..'.tar.gz@"
sed -e "${script}" -i luv-${version}.rockspec
sed -e "${script}" -i rockspecs/luv-${version}.rockspec

# .tar.gz
rm -rf luv-${version}
mkdir -p luv-${version}/deps
cp -r src cmake CMakeLists.txt LICENSE.txt README.md docs.md libluv.pc.in luv-${version}/
cp -r deps/libuv deps/lua-compat-5.3 deps/*.cmake deps/lua_one.c luv-${version}/deps/
COPYFILE_DISABLE=true tar -czvf luv-${version}.tar.gz luv-${version}
rm -rf luv-${version}
