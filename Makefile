
all: luv.so

libuv/include:
	git submodule update --init libuv

luajit/src:
	git submodule update --init luajit

build/luv.so: build/Makefile libuv/include luajit/src
	$(MAKE) -C build

luv.so: build/luv.so
	ln -sf build/luv.so

build:
	mkdir -p build

build/Makefile: build
	cd build && cmake ..

clean:
	rm -rf build luv.so

test: luv.so
	build/luajit tests/run.lua

