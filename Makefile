
all: luv.so

libuv/include:
	git submodule update --init libuv

luajit/src:
	git submodule update --init luajit

build/Makefile: libuv/include luajit/src
	cmake -H. -Bbuild

build/luv.so: build/Makefile
	cmake --build build --config Release

luv.so: build/luv.so
	ln -sf build/luv.so

clean:
	rm -rf build luv.so

test: luv.so
	build/luajit tests/run.lua

