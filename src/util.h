/*
 *  Copyright 2014 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#ifndef LUV_UTIL_H
#define LUV_UTIL_H

#include "luv.h"

#define LUV_UV_VERSION_GEQ(major, minor, patch) \
  (((major)<<16 | (minor)<<8 | (patch)) <= UV_VERSION_HEX)

#define LUV_UV_VERSION_LEQ(major, minor, patch) \
  (((major)<<16 | (minor)<<8 | (patch)) >= UV_VERSION_HEX)

void luv_stack_dump(lua_State* L, const char* name);

#ifdef LUV_SOURCE
// Push a Libuv error code onto the Lua stack
static int luv_error(lua_State* L, int status);

// Common error handling pattern for binding uv functions that only return success/error.
// If the binding returns a value other than success/error, this function should not be used.
static int luv_result(lua_State* L, int status);

// Push the error name onto the stack if status is an error code,
// or push nil onto the stack if it's not an error code
static void luv_status(lua_State* L, int status);

// Return true if the object is a function or a callable table
static int luv_is_callable(lua_State* L, int index);

// Check if the argument is callable and throw an error if it's not
static void luv_check_callable(lua_State* L, int index);
#endif

#endif
