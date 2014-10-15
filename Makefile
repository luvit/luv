
all: luv.so

libuv/include:
	git submodule update --init libuv

luajit/src:
	git submodule update --init luajit

build/luv.so: build/Makefile
	$(MAKE) -C build

luv.so: build/luv.so
	ln -sf build/luv.so

build:
	mkdir -p build

build/Makefile: build libuv/include luajit/src
	cmake -H. -Bbuild
	cmake --build build --config Release

clean:
	rm -rf build luv.so

test: luv.so
	build/luajit tests/run.lua

