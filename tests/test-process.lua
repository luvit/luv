return require('lib/tap')(function (test)

  local isWindows =  _G.isWindows

  test("test disable_stdio_inheritance", function (print, p, expect, uv)
    uv.disable_stdio_inheritance()
  end)

  test("process stdout", function (print, p, expect, uv)
    local stdout = uv.new_pipe(false)

    local handle, pid
    handle, pid = uv.spawn(uv.exepath(), {
      args = {"-e", "io.write 'Hello World'"},
      stdio = {nil, stdout},
    }, expect(function (code, signal)
      p("exit", {code=code, signal=signal})
      uv.close(handle)
    end))

    p{
      handle=handle,
      pid=pid
    }

    uv.read_start(stdout, expect(function (err, chunk)
      p("stdout", {err=err,chunk=chunk})
      assert(not err, err)
      assert(chunk == "Hello World")
      uv.close(stdout)
    end))

  end)

  local cmd, options, expect_status = 'sleep', { args = {1} }, 0
  if isWindows then
    cmd = "cmd.exe"
    options.args = {"/c","pause"}
    expect_status = 1
  end

  test("spawn and kill by pid", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn(cmd, options, expect(function (status, signal)
      p("exit", handle, {status=status,signal=signal})
      assert(status == expect_status)
      if isWindows then
        -- just call TerminateProcess, ref uv__kill in libuv/src/win/process.c
        assert(signal == 0)
      else
        assert(signal == 2)
      end
      uv.close(handle)
    end))
    p{handle=handle,pid=pid}
    uv.kill(pid, "sigint")
  end)

  test("spawn and kill by handle", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn(cmd, options, expect(function (status, signal)
      p("exit", handle, {status=status,signal=signal})
      assert(status == expect_status)
      assert(signal == 15)
      uv.close(handle)
    end))
    p{handle=handle,pid=pid}
    uv.process_kill(handle, "sigterm")
  end)

  test("invalid command", function (print, p, expect, uv)
    local handle, err
    handle, err = uv.spawn("ksjdfksjdflkjsflksdf", {}, function(exit, code)
      assert(false)
    end)
    assert(handle == nil)
    assert(err)
  end)

  test("process stdio", function (print, p, expect, uv)
    local stdin = uv.new_pipe(false)
    local stdout = uv.new_pipe(false)

    local handle, pid
    handle, pid = uv.spawn(uv.exepath(), {
      args = {"-"},
      stdio = {stdin, stdout},
    }, expect(function (code, signal)
      p("exit", {code=code, signal=signal})
      uv.close(handle)
    end))

    p{
      handle=handle,
      pid=pid
    }

    uv.read_start(stdout, expect(function (err, chunk)
      p("stdout", {err=err,chunk=chunk})
      assert(not err, err)
      assert(chunk == "Hello World")
      uv.close(stdout)
    end))

    uv.write(stdin, "io.write('Hello World')")
    uv.shutdown(stdin, expect(function ()
      uv.close(stdin)
    end))

  end)

end)
