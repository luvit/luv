
return require('lib/tap')(function (test)

  test("metrics_info", function (print, p, expect, uv)
    local metrics = assert(uv.metrics_info())
    p(metrics)
    assert(metrics.loop_count >= 0)
    assert(metrics.events >= 0)
    assert(metrics.events_waiting >= 0)
    local init_count = metrics.loop_count

    local timer = uv.new_timer()
    timer:start(0, 0, expect(function()
      timer:close()
    end))

    uv.run()

    metrics = assert(uv.metrics_info())
    p(metrics)
    assert(metrics.loop_count > init_count)
    assert(metrics.events >= 0)
    assert(metrics.events_waiting >= 0)
  end, "1.45.0")

end)
