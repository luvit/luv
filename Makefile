
all: luv.so

build/luv.so: build/Makefile
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
	luajit/src/luajit tests/run.lua

