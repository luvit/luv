require('lib/tap')(function (test)

  test("test disable_stdio_inheritance", function (print, p, expect, uv)
    uv.disable_stdio_inheritance()
  end)

  test("spawn and kill by pid", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn("sleep", {
      args = {1},
    }, expect(function (self, status, signal)
      p("exit", handle, {status=status,signal=signal})
      assert(self == handle)
      assert(status == 0)
      assert(signal == 2)
      uv.close(handle)
    end))
    p{handle=handle,pid=pid}
    uv.kill(pid, "SIGINT")
  end)

  test("spawn and kill by handle", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn("sleep", {
      args = {1},
    }, expect(function (self, status, signal)
      p("exit", handle, {status=status,signal=signal})
      assert(self == handle)
      assert(status == 0)
      assert(signal == 15)
      uv.close(handle)
    end))
    p{handle=handle,pid=pid}
    uv.process_kill(handle, "SIGTERM")
  end)

  test("process stdio", function (print, p, expect, uv)
    local stdin = uv.new_pipe(false)
    local stdout = uv.new_pipe(false)

    local handle, pid
    handle, pid = uv.spawn("cat", {
      stdio = {stdin, stdout},
    }, expect(function (self, code, signal)
      p("exit", {code=code, signal=signal})
      assert(self == handle)
      uv.close(handle)
    end))

    p{
      handle=handle,
      pid=pid
    }

    uv.read_start(stdout, expect(function (self, err, chunk)
      p("stdout", {err=err,chunk=chunk})
      assert(self == stdout)
      assert(not err, err)
      uv.close(stdout)
    end))

    uv.write(stdin, "Hello World")
    uv.shutdown(stdin, expect(function ()
      uv.close(stdin)
    end))

  end)

end)
