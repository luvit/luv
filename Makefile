LUV_TAG=$(shell git describe --tags)

ifdef WITHOUT_AMALG
	CMAKE_OPTIONS+= -DWITH_AMALG=OFF
endif

BUILD_SHARED_LIBS ?= OFF
WITH_SHARED_LIBUV ?= OFF
WITH_SHARED_LUAJIT ?= OFF

CMAKE_OPTIONS += \
	-DBUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) \
	-DWITH_SHARED_LIBUV=$(WITH_SHARED_LIBUV) \
	-DWITH_SHARED_LUAJIT=$(WITH_SHARED_LUAJIT)

all: luv

deps/libuv/include:
	git submodule update --init deps/libuv

deps/luajit/src:
	git submodule update --init deps/luajit

build/Makefile: deps/libuv/include deps/luajit/src
	cmake -H. -Bbuild ${CMAKE_OPTIONS} -DWITH_AMALG=OFF

luv: build/Makefile
	cmake --build build --config Debug
	ln -sf build/luv.so

clean:
	rm -rf build luv.so

test: luv
	build/luajit tests/run.lua

reset:
	git submodule update --init --recursive && \
	  git clean -f -d && \
	  git checkout .

publish-luarocks: reset
	rm -rf luv-${LUV_TAG}
	mkdir -p luv-${LUV_TAG}/deps
	cp -r src CMakeLists.txt LICENSE.txt README.md docs.md luv-${LUV_TAG}/
	cp -r deps/libuv deps/uv.cmake luv-${LUV_TAG}/deps/
	tar -czvf luv-${LUV_TAG}.tar.gz luv-${LUV_TAG}
	github-release upload --user luvit --repo luv --tag ${LUV_TAG} \
	  --file luv-${LUV_TAG}.tar.gz --name luv-${LUV_TAG}.tar.gz
