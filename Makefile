uname_S=$(shell uname -s)
ifeq (Darwin, $(uname_S))
  CFLAGS=-Ilibuv/include -g -I/usr/local/include/luajit-2.0 -DLUV_STACK_CHECK -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lluajit-5.1 -framework CoreServices -framework Cocoa -L/usr/local/lib/
  SHARED_LIB_FLAGS=-bundle -o luv.so temp/luv.o libuv/libuv.a temp/common.o
else
  CFLAGS=-Ilibuv/include -g -I/usr/local/include/luajit-2.0 -DLUV_STACK_CHECK -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lrt
  SHARED_LIB_FLAGS=-shared -o luv.so temp/luv.o libuv/libuv.a temp/common.o
endif

all: luv.so

libuv/libuv.a:
	CPPFLAGS=-fPIC $(MAKE) -C libuv

temp:
	mkdir temp

temp/common.o: src/common.c src/common.h temp
	$(CC) -c $< -o $@ ${CFLAGS}

temp/luv.o: src/luv.c src/luv.h src/luv_functions.c temp
	$(CC) -c $< -o $@ ${CFLAGS}

luv.so: temp/luv.o libuv/libuv.a temp/common.o
	$(CC) ${SHARED_LIB_FLAGS} ${LIBS}

clean:
	make -C libuv clean
	rm -rf temp
	rm -f luv.so
