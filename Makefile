all: build/Makefile
	$(MAKE) -C build

build:
	mkdir -p build

build/Makefile: build
	cd build && cmake ..

clean:
	rm -rf build

# CFLAGS+= -Ilibuv/include -I/usr/local/include/luajit-2.0 -I/usr/include/luajit-2.0\
# 	-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64\
# 	-Werror -Wall -Wextra -Wstrict-prototypes\
# 	-Wold-style-definition -Wmissing-prototypes\
# 	-Wmissing-declarations -Wdeclaration-after-statement\
# 	-Wno-unused-parameter

# uname_S=$(shell uname -s)
# ifeq (Darwin, $(uname_S))
# 	LDFLAGS+=-bundle -bundle_loader /usr/local/bin/luajit \
# 		-framework CoreServices \
# 		-L/usr/local/lib/
# else
# 	CFLAGS+= -fPIC
# 	LDFLAGS+= -shared -lrt
# endif

# SOURCE_FILES=\
# 	src/async.c\
# 	src/check.c\
# 	src/dns.c\
# 	src/fs.c\
# 	src/fs_event.c\
# 	src/fs_poll.c\
# 	src/handle.c\
# 	src/idle.c\
# 	src/lhandle.c\
# 	src/lhandle.h\
# 	src/loop.c\
# 	src/lreq.c\
# 	src/lreq.h\
# 	src/luv.c\
# 	src/luv.h\
# 	src/misc.c\
# 	src/pipe.c\
# 	src/poll.c\
# 	src/prepare.c\
# 	src/process.c\
# 	src/req.c\
# 	src/signal.c\
# 	src/stream.c\
# 	src/tcp.c\
# 	src/timer.c\
# 	src/tty.c\
# 	src/udp.c\
# 	src/util.c\
# 	src/util.h

# all: luv.so

# libuv/gyp_uv.py:
# 	git submodule update --init libuv

# libuv/build/gyp: libuv/gyp_uv.py
# 	mkdir -p libuv/build
# 	git clone https://git.chromium.org/external/gyp.git libuv/build/gyp

# libuv/out/Release/libuv.a: libuv/gyp_uv.py libuv/build/gyp
# 	cd libuv && ./gyp_uv.py && BUILDTYPE=Release CFLAGS=-fPIC  ${MAKE} -C out && cd ..

# libuv/out/Debug/libuv.a: libuv/gyp_uv.py libuv/build/gyp
# 	cd libuv && ./gyp_uv.py && BUILDTYPE=Debug CFLAGS=-fPIC ${MAKE} -C out && cd ..

# luv.o: ${SOURCE_FILES} libuv/gyp_uv.py
# 	$(CC) -c src/luv.c ${CFLAGS} -o $@

# luv.so: luv.o libuv/out/Release/libuv.a
# 	$(CC) $^ ${LDFLAGS} -o $@

# clean:
# 	rm -f luv.so *.o

# clean-all: clean
# 	rm -rf libuv/out

# test: luv.so
# 	luajit tests/run.lua

# test-all: test
# 	luajit tests/manual-test-cluster.lua

# install-luajit:
# 	curl http://luajit.org/download/LuaJIT-2.0.3.tar.gz -O
# 	tar -xzf LuaJIT-2.0.3.tar.gz
# 	LUAJIT_ENABLE_LUA52COMPAT=1 make -C LuaJIT-2.0.3 -j4
# 	sudo make -C LuaJIT-2.0.3 install
