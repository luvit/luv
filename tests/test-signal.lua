local child_code = [[
local uv = require('luv')
local signal = uv.new_signal()
uv.ref(signal)
uv.signal_start(signal, "sigint", function (self)
  assert(self == signal)
  print("# SIGINT caught")
  uv.unref(signal)
end)
print("# blocking")
uv.run()
print("# done")
]]

require('lib/tap')(function (test)
  test("Catch SIGINT", function (print, p, expect, uv)
    local child, pid
    child, pid = uv.spawn(uv.execpath(), {
      args = {"-e", child_code},
      stdio = {nil,2}
    }, expect(function (self, code, signal)
      p("exit", {pid=pid,code=code,signal=signal})
      assert(self == child)
      assert(code == 0)
      assert(signal == 0)
      uv.close(child)
    end))
    uv.timer_start(uv.new_timer(), 200, 0, function (timer)
      print("Sending child SIGINT")
      uv.process_kill(child, "sigint")
      uv.close(timer)
    end)
  end)

end)
