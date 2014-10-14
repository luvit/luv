

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

luajit/src/luajit:
	LUAJIT_ENABLE_LUA52COMPAT=1 $(MAKE) -C luajit

test: luv.so luajit/src/luajit
	luajit/src/luajit tests/run.lua

install-luajit: luajit/src/luajit
	sudo make -C luajit install
