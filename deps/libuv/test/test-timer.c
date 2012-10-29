/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "task.h"


static int once_cb_called = 0;
static int once_close_cb_called = 0;
static int repeat_cb_called = 0;
static int repeat_close_cb_called = 0;

static int64_t start_time;


static void once_close_cb(uv_handle_t* handle) {
  printf("ONCE_CLOSE_CB\n");

  ASSERT(handle != NULL);
  ASSERT(!uv_is_active(handle));

  once_close_cb_called++;
}


static void once_cb(uv_timer_t* handle, int status) {
  printf("ONCE_CB %d\n", once_cb_called);

  ASSERT(handle != NULL);
  ASSERT(status == 0);
  ASSERT(!uv_is_active((uv_handle_t*)handle));

  once_cb_called++;

  uv_close((uv_handle_t*)handle, once_close_cb);

  /* Just call this randomly for the code coverage. */
  uv_update_time(uv_default_loop());
}


static void repeat_close_cb(uv_handle_t* handle) {
  printf("REPEAT_CLOSE_CB\n");

  ASSERT(handle != NULL);

  repeat_close_cb_called++;
}


static void repeat_cb(uv_timer_t* handle, int status) {
  printf("REPEAT_CB\n");

  ASSERT(handle != NULL);
  ASSERT(status == 0);
  ASSERT(uv_is_active((uv_handle_t*)handle));

  repeat_cb_called++;

  if (repeat_cb_called == 5) {
    uv_close((uv_handle_t*)handle, repeat_close_cb);
  }
}


static void never_cb(uv_timer_t* handle, int status) {
  FATAL("never_cb should never be called");
}


TEST_IMPL(timer) {
  uv_timer_t once_timers[10];
  uv_timer_t *once;
  uv_timer_t repeat, never;
  int i, r;

  start_time = uv_now(uv_default_loop());
  ASSERT(0 < start_time);

  /* Let 10 timers time out in 500 ms total. */
  for (i = 0; i < ARRAY_SIZE(once_timers); i++) {
    once = once_timers + i;
    r = uv_timer_init(uv_default_loop(), once);
    ASSERT(r == 0);
    r = uv_timer_start(once, once_cb, i * 50, 0);
    ASSERT(r == 0);
  }

  /* The 11th timer is a repeating timer that runs 4 times */
  r = uv_timer_init(uv_default_loop(), &repeat);
  ASSERT(r == 0);
  r = uv_timer_start(&repeat, repeat_cb, 100, 100);
  ASSERT(r == 0);

  /* The 12th timer should not do anything. */
  r = uv_timer_init(uv_default_loop(), &never);
  ASSERT(r == 0);
  r = uv_timer_start(&never, never_cb, 100, 100);
  ASSERT(r == 0);
  r = uv_timer_stop(&never);
  ASSERT(r == 0);
  uv_unref((uv_handle_t*)&never);

  uv_run(uv_default_loop());

  ASSERT(once_cb_called == 10);
  ASSERT(once_close_cb_called == 10);
  printf("repeat_cb_called %d\n", repeat_cb_called);
  ASSERT(repeat_cb_called == 5);
  ASSERT(repeat_close_cb_called == 1);

  ASSERT(500 <= uv_now(uv_default_loop()) - start_time);

  MAKE_VALGRIND_HAPPY();
  return 0;
}


TEST_IMPL(timer_start_twice) {
  uv_timer_t once;
  int r;

  r = uv_timer_init(uv_default_loop(), &once);
  ASSERT(r == 0);
  r = uv_timer_start(&once, never_cb, 86400 * 1000, 0);
  ASSERT(r == 0);
  r = uv_timer_start(&once, once_cb, 10, 0);
  ASSERT(r == 0);
  r = uv_run(uv_default_loop());
  ASSERT(r == 0);

  ASSERT(once_cb_called == 1);

  MAKE_VALGRIND_HAPPY();
  return 0;
}
