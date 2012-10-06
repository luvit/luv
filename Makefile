CFLAGS=-Ilibuv/include -I/usr/local/include/luvit/luajit -I/usr/local/include/luvit/http_parser -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
LIBS=-shared -lm libuv/uv.a -lpthread -lrt

all: luv.so

libuv/uv.a:
	CPPFLAGS=-fPIC $(MAKE) -C libuv

%.o: %.c %.h libuv/uv.a
	$(CC) -c $< -o $@ ${CFLAGS}

luv.so: luv.o
	$(CC) -o luv.so luv.o ${LIBS}

clean:
	rm -f luv.so luv.o
