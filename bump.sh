#!/bin/sh

version=$1
if [ -z "$version" ]; then
  echo "must specify a version" >&2
  exit 1
fi

script="/^version/s@\"[^\"]\\+\"@\"${version}\"@"
sed -e "${script}" -i luv-*.rockspec
sed -e "${script}" -i rockspecs/luv-*.rockspec
git mv luv-*.rockspec luv-${version}.rockspec
git mv rockspecs/luv-*.rockspec rockspecs/luv-${version}.rockspec
git add luv-${version}.rockspec rockspecs/luv-${version}.rockspec
