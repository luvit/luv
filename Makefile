

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
	luajit tests/run.lua

install-luajit:
	curl http://luajit.org/download/LuaJIT-2.0.3.tar.gz -O
	tar -xzf LuaJIT-2.0.3.tar.gz
	LUAJIT_ENABLE_LUA52COMPAT=1 make -C LuaJIT-2.0.3 -j4
	sudo make -C LuaJIT-2.0.3 install
