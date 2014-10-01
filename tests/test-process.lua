require('lib/tap')(function (test)

  test("test disable_stdio_inheritance", function (print, p, expect, uv)
    uv.disable_stdio_inheritance()
  end)

  test("spawn and kill by pid", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn("sleep", {
      args = {1},
      exit_cb = expect(function (self, status, signal)
        p("exit", handle, {status=status,signal=signal})
        assert(self == handle)
        assert(status == 0)
        assert(signal == 2)
        uv.close(handle)
      end),
    })
    p{handle=handle,pid=pid}
    uv.kill(pid, "SIGINT")
  end)

  test("spawn and kill by handle", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn("sleep", {
      args = {1},
      exit_cb = expect(function (self, status, signal)
        p("exit", handle, {status=status,signal=signal})
        assert(self == handle)
        assert(status == 0)
        assert(signal == 15)
        uv.close(handle)
      end),
    })
    p{handle=handle,pid=pid}
    uv.process_kill(handle, "SIGTERM")
  end)

end)
