CFLAGS=-Ilibuv/include -I/usr/local/include/luvit/luajit -I/usr/local/include/luvit/http_parser -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
LIBS=-lm -lpthread -lrt

all: luv.so

libuv/uv.a:
	CPPFLAGS=-fPIC $(MAKE) -C libuv

luv.o: luv.c luv.h luv_functions.c luv_handle.c luv_stream.c luv_tcp.c luv_timer.c
	$(CC) -c $< -o $@ ${CFLAGS}

luv.so: luv.o libuv/uv.a
	$(CC) -o luv.so luv.o libuv/uv.a ${LIBS} -shared

clean:
	rm -f *.so *.o
