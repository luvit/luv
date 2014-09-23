local uv = require('luv')
local loop = uv.new_loop()
local dump = require('lib/utils').dump
local p = require('lib/utils').prettyPrint

local function test()
  coroutine.wrap(function ()
    local timer = uv.new_timer(loop)
    function timer:ontimeout()
      assert(self == timer)
      p{["repeat"]= uv.timer_get_repeat(timer)}
      uv.timer_set_repeat(timer, 200)
      uv.timer_again(timer)
      function timer:ontimeout()
        p{["repeat"]= uv.timer_get_repeat(timer)}
        assert(self == timer)
        uv.timer_stop(self)
        print(self, "Closing")
        uv.close(self)
        print(self, "Closed")
      end
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
