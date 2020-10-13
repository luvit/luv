return require('lib/tap')(function (test)

	test("uv.loop_mode", function (print, p, expect, uv)
		assert(uv.loop_mode() == nil)
    local timer = uv.new_timer()
    uv.timer_start(timer, 100, 0, expect(function ()
      assert(uv.loop_mode() == "default")
      uv.timer_stop(timer)
      uv.close(timer)
    end))
  end)

end)
