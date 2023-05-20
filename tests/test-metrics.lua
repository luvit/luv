
return require('lib/tap')(function (test)

  test("idle time", function (print, p, expect, uv)
    local NS_TO_MS = 1000000
    local timeout = 1000
    local timer = uv.new_timer()
    local counter = 0
    timer:start(timeout, 0, function()
      counter = counter + 1
      local t = uv.hrtime()

      -- Spin for 500 ms to spin loop time out of the delta check.
      while uv.hrtime() - t < 600 * NS_TO_MS do end
      timer:close()

      local metrics = uv.metrics_info()
      p(metrics)
      assert(metrics.loop_count > 0)
      assert(metrics.events >= 0)
      assert(metrics.events_waiting >= 0)
    end)

    local metrics = assert(uv.metrics_info())
    p(metrics)
    assert(metrics.loop_count >= 0)
    assert(metrics.events >= 0)
    assert(metrics.events_waiting >= 0)
  end, "1.45.0")

end)
