LUV_TAG=$(shell git describe --tags)

ifdef WITHOUT_AMALG
	CMAKE_OPTIONS+= -DWITH_AMALG=OFF
endif

BUILD_MODULE ?= ON
BUILD_SHARED_LIBS ?= OFF
WITH_SHARED_LIBUV ?= OFF
WITH_LUA_ENGINE ?= LuaJIT
LUA_BUILD_TYPE ?= Static
LUA_COMPAT53_DIR ?= deps/lua-compat-5.3
BUILD_DIR ?= build
# options: Release, Debug, RelWithDebInfo, MinSizeRel
BUILD_TYPE ?= RelWithDebInfo

ifeq ($(WITH_LUA_ENGINE), LuaJIT)
  LUABIN=$(BUILD_DIR)/luajit
else
  LUABIN=$(BUILD_DIR)/lua
endif

CMAKE_OPTIONS += \
	-DBUILD_MODULE=$(BUILD_MODULE) \
	-DBUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) \
	-DWITH_SHARED_LIBUV=$(WITH_SHARED_LIBUV) \
	-DWITH_LUA_ENGINE=$(WITH_LUA_ENGINE) \
	-DLUA_BUILD_TYPE=$(LUA_BUILD_TYPE) \
	-DLUA_COMPAT53_DIR=$(LUA_COMPAT53_DIR) \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

ifeq ($(MAKE),mingw32-make)
CMAKE_OPTIONS += -G"MinGW Makefiles"
LUV_EXT ?= .dll
LUV_CP  ?= cp -f
endif

LUV_EXT ?= .so
LUV_CP  ?= ln -sf

all: luv

deps/libuv/include:
	git submodule update --init deps/libuv

deps/luajit/src:
	git submodule update --init deps/luajit

deps/lua-compat-5.3/c-api:
	git submodule update --init deps/lua-compat-5.3

$(BUILD_DIR)/Makefile: deps/libuv/include deps/luajit/src deps/lua-compat-5.3/c-api
	cmake -H. -B$(BUILD_DIR) ${CMAKE_OPTIONS}

luv: $(BUILD_DIR)/Makefile
	cmake --build $(BUILD_DIR)
	$(LUV_CP) $(BUILD_DIR)/luv$(LUV_EXT) luv$(LUV_EXT)

install: luv
	$(MAKE) -C $(BUILD_DIR) install

clean:
	rm -rf $(BUILD_DIR) luv$(LUV_EXT)

test: luv
	${LUABIN} tests/run.lua

reset:
	git submodule update --init --recursive && \
	  git clean -f -d && \
	  git checkout .

publish-luarocks:
	github-release upload --user luvit --repo luv --tag ${LUV_TAG} \
	  --file luv-${LUV_TAG}.tar.gz --name luv-${LUV_TAG}.tar.gz
	luarocks upload luv-${LUV_TAG}.rockspec --api-key=${LUAROCKS_TOKEN}

# vim: ts=8 sw=8 noet tw=79 fen fdm=marker
