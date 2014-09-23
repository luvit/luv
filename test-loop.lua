local uv = require('luv')
local loop = uv.new_loop()
local dump = require('lib/utils').dump
local p = require('lib/utils').prettyPrint

local function closeLoop()
  print(loop, "Closing all handles in loop")
  uv.walk(loop, function (handle)
    print("Closing", handle)
    uv.close(handle)
  end)
end

local function test()
  coroutine.wrap(function ()
    local prep = uv.new_prepare(loop)
    uv.prepare_start(prep)
    function prep:onprepare()
      assert(self == prep)
      print("prep")
    end
    local check = uv.new_check(loop)
    uv.check_start(check)
    function check:oncheck()
      assert(self == check)
      print("check")
    end

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
        closeLoop()
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
