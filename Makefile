
all: luv

libuv/include:
	git submodule update --init libuv

luajit/src:
	git submodule update --init luajit

build/Makefile: libuv/include luajit/src
	cmake -H. -Bbuild

luv: build/Makefile
	cmake --build build --config Debug
	ln -sf build/luv.so

clean:
	rm -rf build luv.so

test: luv
	build/luajit tests/run.lua

