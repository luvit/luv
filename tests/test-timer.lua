require('lib/tap')(function (test)

  test("simple timeout", function (print, p, expect, uv)
    local timer = uv.new_timer()
    local function onclose(self)
      assert(self == timer)
      p("closed", timer)
    end
    local function ontimeout(self)
      assert(self == timer)
      p("timeout", timer)
      uv.close(timer, expect(onclose))
    end
    uv.timer_start(timer, expect(ontimeout), 10, 0)
  end)

  test("simple interval", function (print, p, expect, uv)
    local timer = uv.new_timer()
    local count = 5
    local function onclose(self)
      assert(self == timer)
      p("closed", timer)
    end
    local function oninterval(self)
      assert(self == timer)
      p("interval", timer)
      count = count - 1
      if count == 0 then
        uv.close(timer, expect(onclose))
      end
    end
    uv.timer_start(timer, expect(oninterval, count), 10, 10)
  end)

  test("timeout with interval", function (print, p, expect, uv)
    local a = uv.new_timer()
    local b = uv.new_timer()
    uv.timer_start(a, expect(function ()
      p("timeout", a)
      uv.timer_stop(b)
      uv.close(a)
      uv.close(b)
    end), 45, 0)
    uv.timer_start(b, expect(function ()
      p("interval", b)
    end, 4), 10, 10)
  end)

  test("shrinking interval", function (print, p, expect, uv)
    local timer = uv.new_timer()
    uv.timer_start(timer, expect(function ()
      local r = uv.timer_get_repeat(timer)
      p("interval", timer, r)
      if r == 0 then
        uv.timer_set_repeat(timer, 128)
        uv.timer_again(timer)
      elseif r == 1 then
        uv.timer_stop(timer)
        uv.close(timer)
      else
        uv.timer_set_repeat(timer, r / 2)
      end
    end, 9), 10, 0)
  end)

end)
