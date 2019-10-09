local child_code = string.dump(function ()
  local uv = require('luv')
  local signal = uv.new_signal()
  uv.ref(signal)
  uv.signal_start(signal, "sigint", function ()
    print("sigint")
    uv.unref(signal)
  end)
  uv.run()
end)

return require('lib/tap')(function (test)

  if require('lib/utils').isWindows then return end

  test("Catch SIGINT", function (print, p, expect, uv)
    local child, pid
    local input = uv.new_pipe(false)
    local output = uv.new_pipe(false)
    child, pid = assert(uv.spawn(uv.exepath(), {
      args = {"-"},
      stdio = {input,output,2}
    }, expect(function (code, signal)
      p("exit", {pid=pid,code=code,signal=signal})
      assert(signal == 0)
      uv.close(input)
      uv.close(output)
      uv.close(child)
    end)))
    uv.read_start(output, expect(function(err, chunk)
      assert(not err, err)
      if chunk then
        p(chunk)
        assert(chunk=="sigint\n")
        uv.read_stop(output)
      end
    end, 1))
    uv.write(input, child_code)
    uv.shutdown(input)
    local timer = uv.new_timer()
    uv.timer_start(timer, 200, 0, expect(function ()
      print("Sending child SIGINT")
      uv.process_kill(child, "sigint")
      uv.close(timer)
    end))
  end)

end)
