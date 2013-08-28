uname_S=$(shell uname -s)
ifeq (Darwin, $(uname_S))
  CFLAGS=-Ilibuv/include -g -I/usr/local/include/luajit-2.0 -DLUV_STACK_CHECK -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lluajit-5.1 -framework CoreServices -framework Cocoa -L/usr/local/lib/
  SHARED_LIB_FLAGS=-bundle -o luv.so luv.o libuv/libuv.a common.o
else
  CFLAGS=-Ilibuv/include -g -I/usr/local/include/luajit-2.0 -DLUV_STACK_CHECK -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lrt
  SHARED_LIB_FLAGS=-shared -o luv.so luv.o libuv/libuv.a common.o
endif

all: luv.so

libuv/libuv.a:
	CPPFLAGS=-fPIC $(MAKE) -C libuv

common.o: common.c common.h
	$(CC) -c $< -o $@ ${CFLAGS}

luv.o: luv.c luv.h luv_functions.c
	$(CC) -c $< -o $@ ${CFLAGS}

luv.so: luv.o libuv/libuv.a common.o
	$(CC) ${SHARED_LIB_FLAGS} ${LIBS}

clean:
	rm -f *.so *.o
