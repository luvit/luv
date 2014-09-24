CFLAGS+=-Ilibuv/include -g -I/usr/local/include \
	-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
	-Werror -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wdeclaration-after-statement

uname_S=$(shell uname -s)
ifeq (Darwin, $(uname_S))
	LDFLAGS+=-bundle -bundle_loader /usr/local/bin/lua \
		-framework CoreServices \
		-L/usr/local/lib/
else
	CFLAGS+= -fPIC
	LDFLAGS+=-shared -lrt
endif

SOURCE_FILES=\
	src/luv.c \
	src/loop.c \
	src/dns.c \
	src/fs.c \
	src/handle.c \
	src/luv.h \
	src/misc.c \
	src/pipe.c \
	src/stream.c \
	src/tcp.c \
	src/timer.c \
	src/prepare.c \
	src/check.c \
	src/idle.c \
	src/async.c \
	src/poll.c \
	src/signal.c \
	src/process.c \
	src/tty.c \
	src/util.c

all: luv.so

libuv/out/Release/libuv.a:
	cd libuv && ./gyp_uv.py && BUILDTYPE=Release CFLAGS=-fPIC  ${MAKE} -C out && cd ..

libuv/out/Debug/libuv.a:
	cd libuv && ./gyp_uv.py && BUILDTYPE=Debug CFLAGS=-fPIC  ${MAKE} -C out && cd ..

luv.o: ${SOURCE_FILES}
	$(CC) -c $< ${CFLAGS} -o $@

luv.so: luv.o libuv/out/Release/libuv.a
	$(CC) $^ ${LDFLAGS} -o $@

clean:
	rm -f luv.so *.o
