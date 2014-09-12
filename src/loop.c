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

typedef struct {
  uv_loop_t* loop;   /* The actual uv handle. memory managed by luv */
  lua_State* L;        /* L and ref together form a reference to the userdata */
  int threadref;       /* hold reference to coroutine if created in one */
  int ref;             /* ref is null when refCount is 0 meaning we're weak */
} luv_loop_t;

static int new_loop(lua_State* L) {
  return 0;

}
