
all: luv

deps/libuv/include:
	git submodule update --init deps/libuv

deps/luajit/src:
	git submodule update --init deps/luajit

build/Makefile: deps/libuv/include deps/luajit/src
	cmake -H. -Bbuild

luv: build/Makefile
	cmake --build build --config Debug
	ln -sf build/luv.so

clean:
	rm -rf build luv.so

test: luv
	build/luajit tests/run.lua

