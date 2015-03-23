LUV_TAG=$(shell git describe --tags)

all: luv

deps/libuv/include:
	git submodule update --init deps/libuv

deps/luajit/src:
	git submodule update --init deps/luajit

build/Makefile: deps/libuv/include deps/luajit/src
	cmake -H. -Bbuild

luv: build/Makefile
	cmake --build build --config Debug
	ln -sf build/luv.so

clean:
	rm -rf build luv.so

test: luv
	build/luajit tests/run.lua

reset:
	git submodule update --init --recursive && \
	  git clean -f -d && \
	  git checkout .

publish-src: reset
	tar -czvf luv-src.tar.gz \
	  --exclude 'luv-src.tar.gz' --exclude '.git*' --exclude build . && \
	  github-release upload --user luvit --repo luv --tag ${LUV_TAG} \
	  --file luv-src.tar.gz --name luv-src.tar.gz

