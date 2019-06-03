return require('lib/tap')(function (test)

  test("coroutines", function (print, p, expect, uv)
    local touch = 0
    local function wait(ms)
      local this = coroutine.running()
      assert(this)
      local timer = uv.new_timer()
      timer:start(ms, 0, function ()
        timer:close()
        touch = touch + 1
        coroutine.resume(this)
        touch = touch + 1
      end)
      coroutine.yield()
      touch = touch + 1
      return touch
    end

    coroutine.wrap(function()
      print("begin wait")
      local touched = wait(1000)
      assert(touched==touch)
      print("end wait")
    end)()

    uv.run()

    assert(touch==3)
  end)

end)
