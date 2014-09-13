local uv = require('luv')
local loop = uv.new_loop()
local function test()
  coroutine.wrap(function ()
    local timer = uv.new_timer(loop)
    function timer:ontimeout()
      print("Closing", self, "timer == self", timer == self)
      uv.close(self)
      print("Closed")
    end
    -- error "TEST ME2"
    uv.timer_start(timer, 200, 0)
  end)()
end

test()
print("blocking")
uv.run(loop)
print("done")
uv.loop_close(loop)
