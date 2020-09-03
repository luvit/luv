-- This is a manual test because uv.loop_configure would affect all other tests that
-- run after it. So, just to be safe, this is not included in the automatic test runner.

return require('lib/tap')(function (test)

  -- port of https://github.com/libuv/libuv/blob/fa8b4f27c023638918e645ef997c3b56a3a5e681/test/test-metrics.c#L39-L63
  test("idle time", function (print, p, expect, uv)
    local NS_TO_MS = 1000000
    local timeout = 1000
    uv.loop_configure('metrics_idle_time')
    local timer = uv.new_timer()
    local counter = 0
    timer:start(timeout, 0, function()
      counter = counter + 1
      local t = uv.hrtime()

      -- Spin for 500 ms to spin loop time out of the delta check.
      while uv.hrtime() - t < 600 * NS_TO_MS do end
      timer:close()

      local idle_time = uv.metrics_idle_time()
      assert(idle_time <= (timeout + 500) * NS_TO_MS, "idle_time larger than expected: "..idle_time)
      assert(idle_time >= (timeout - 500) * NS_TO_MS, "idle_time smaller than expected: "..idle_time)
    end)
  end)

end)
