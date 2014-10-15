local child_code = string.dump(function ()
  local uv = require('luv')
  local signal = uv.new_signal()
  uv.ref(signal)
  uv.signal_start(signal, "SIGINT", function (self)
    assert(self == signal)
    uv.unref(signal)
  end)
  uv.run()
  os.exit(7)
end)

return require('lib/tap')(function (test)

  if require('ffi').os == "Windows" then return end

  test("Catch SIGINT", function (print, p, expect, uv)
    local child, pid
    local input = uv.new_pipe(false)
    child, pid = assert(uv.spawn(uv.exepath(), {
      -- cwd = uv.cwd(),
      stdio = {input,1,2}
    }, expect(function (self, code, signal)
      p("exit", {pid=pid,code=code,signal=signal})
      assert(self == child)
      assert(code == 7)
      assert(signal == 0)
      uv.close(input)
      uv.close(child)
    end)))
    uv.write(input, child_code)
    uv.shutdown(input)
    uv.timer_start(uv.new_timer(), 200, 0, expect(function (timer)
      print("Sending child SIGINT")
      uv.process_kill(child, "SIGINT")
      uv.close(timer)
    end))
  end)

end)
