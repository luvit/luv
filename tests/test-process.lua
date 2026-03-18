return require('lib/tap')(function (test)

  local isWindows = require('lib/utils').isWindows
  local privilege_drop_child_code = string.dump(function ()
    local uv = require('luv')
    local target_id = 1
    local groups

    assert(uv.getuid() == 0, "child must start as root")

    uv.setgroups({})
    uv.setgid(target_id)
    uv.setuid(target_id)

    groups = uv.getgroups()
    assert(uv.getuid() == target_id)
    assert(uv.getgid() == target_id)
    assert(#groups == 0)

    io.write(string.format("dropped:%d:%d:%d\n", uv.getuid(), uv.getgid(), #groups))
  end)

  test("test disable_stdio_inheritance", function (print, p, expect, uv)
    uv.disable_stdio_inheritance()
  end)

  test("process stdout", function (print, p, expect, uv)
    local stdout = uv.new_pipe(false)

    local input = "Hello World"
    local cmd, args, expectedOutput
    if isWindows then
      cmd = "cmd.exe"
      args = {"/c", "echo "..input}
      expectedOutput = input .. "\r\n"
    else
      cmd = "echo"
      args = {input}
      expectedOutput = input .. "\n"
    end

    local handle, pid
    handle, pid = uv.spawn(cmd, {
      args = args,
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
      assert(chunk == expectedOutput)
      uv.close(stdout)
    end))

  end)

  local longRunning = {}
  if isWindows then
    longRunning.cmd = "cmd.exe"
    longRunning.options = { args = {"/c","pause"} }
    longRunning.expect_status = 1
  else
    longRunning.cmd = 'sleep'
    longRunning.options = { args = {1} }
    longRunning.expect_status = 0
  end

  test("spawn and kill by pid", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn(longRunning.cmd, longRunning.options, expect(function (status, signal)
      p("exit", handle, {status=status,signal=signal})
      assert(status == longRunning.expect_status)
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
    handle, pid = uv.spawn(longRunning.cmd, longRunning.options, expect(function (status, signal)
      p("exit", handle, {status=status,signal=signal})
      assert(status == longRunning.expect_status)
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

    local input = "Hello World"
    local cmd, args, expectedOutput
    if isWindows then
      cmd = "cmd.exe"
      args = {"/c", "set /p output=&call echo %output%"}
      expectedOutput = input .. "\r\n"
    else
      cmd = "cat"
      args = {"-"}
      expectedOutput = input
    end

    local handle, pid
    handle, pid = uv.spawn(cmd, {
      args = args,
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
      assert(chunk == expectedOutput)
      uv.close(stdout)
    end))

    uv.write(stdin, input)
    uv.shutdown(stdin, expect(function ()
      uv.close(stdin)
    end))

  end)

  test("get pid", function (print, p, expect, uv)
    local handle, pid
    handle, pid = uv.spawn(longRunning.cmd, longRunning.options, expect(function(status, signal)
      handle:close()
    end))
    assert(handle:get_pid() == pid)
    handle:kill("sigterm")
  end, "1.19.0")

  test("spawned child can drop privileges in POS36-C order", function (print, p, expect, uv)
    local child, pid
    local input, output

    if isWindows or not uv.getuid or uv.getuid() ~= 0 then
      print("skipping: requires Unix root privileges")
      return
    end

    input = uv.new_pipe(false)
    output = uv.new_pipe(false)

    child, pid = assert(uv.spawn(uv.exepath(), {
      args = {"-"},
      stdio = {input, output, 2},
    }, expect(function (code, signal)
      p("exit", {pid = pid, code = code, signal = signal})
      assert(code == 0)
      assert(signal == 0)
      uv.close(input)
      uv.close(output)
      uv.close(child)
    end)))

    uv.read_start(output, expect(function (err, chunk)
      assert(not err, err)
      if chunk then
        p(chunk)
        assert(chunk == "dropped:1:1:0\n")
        uv.read_stop(output)
      end
    end, 1))

    uv.write(input, privilege_drop_child_code)
    uv.shutdown(input)
  end)

end)
