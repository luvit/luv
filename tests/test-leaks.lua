return require('lib/tap')(function (test)

  test("lots-o-timers", function (print, p, expect, uv)
    collectgarbage()
    local before = uv.resident_set_memory()
    local timer
    for i = 1, 0x10000 do
      timer = uv.new_timer()
      uv.close(timer)
      if i % 0x1000 == 0 then
        timer = nil
        collectgarbage()
        p(i, uv.resident_set_memory())
      end
    end
    collectgarbage()
    local after = uv.resident_set_memory()
    p{
      before = before,
      after = after,
    }
    assert(after < before * 1.1, "Leak in timers")
  end)

end)
