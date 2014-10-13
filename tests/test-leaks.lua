return require('lib/tap')(function (test)

  test("lots-o-timers", function (print, p, expect, uv)
    collectgarbage()
    local before
    local timer
    for i = 1, 0x10000 do
      timer = uv.new_timer()
      uv.close(timer)
      if i % 0x1000 == 0 then
        timer = nil
        uv.run()
        collectgarbage()
        local now = uv.resident_set_memory()
        if not before then before = now end
        p(i, now)
      end
    end
    uv.run()
    collectgarbage()
    local after = uv.resident_set_memory()
    p{
      before = before,
      after = after,
    }
    assert(after < before * 1.1, "Leak in timers")
  end)

end)
