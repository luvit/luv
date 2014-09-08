CFLAGS=-Ilibuv/include -g -I/usr/local/include \
	-DLUV_STACK_CHECK -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
	-Wall -Werror -fPIC

uname_S=$(shell uname -s)
ifeq (Darwin, $(uname_S))
	LDFLAGS=-bundle -bundle_loader /usr/local/bin/lua \
		-framework CoreServices -framework Cocoa \
		-L/usr/local/lib/
else
	LDFLAGS=-shared -lrt
endif

SOURCE_FILES=\
	src/dns.c \
	src/fs.c \
	src/handle.c \
	src/luv.c \
	src/luv.h \
	src/misc.c \
	src/pipe.c \
	src/process.c \
	src/stream.c \
	src/tcp.c \
	src/timer.c \
	src/tty.c \
	src/util.c

all: luv.so

libuv/libuv.a:
	CPPFLAGS=-fPIC $(MAKE) -C libuv

luv.so: ${SOURCE_FILES} libuv/libuv.a
	$(CC) -c src/luv.c ${CFLAGS} -o luv.o
	$(CC) luv.o libuv/libuv.a ${LDFLAGS} -o $@
	rm luv.o

clean:
	rm -f luv.so
