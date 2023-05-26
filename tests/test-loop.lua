return require('lib/tap')(function (test)

  local function get_cmd()
    local i=1
    repeat
      i=i-1
    until not arg[i]
    return arg[i+1]
  end

  local cmd = get_cmd()
  local cwd = require('luv').cwd()
  print("cmd: ", cmd)
  print("cwd: ", cwd)

	test("uv.loop_mode", function (print, p, expect, uv)
		assert(uv.loop_mode() == nil)
    local timer = uv.new_timer()
    uv.timer_start(timer, 100, 0, expect(function ()
      assert(uv.loop_mode() == "default")
      uv.timer_stop(timer)
      uv.close(timer)
    end))
  end)

  test("issue #437, crash without uv.run", function (print, p, expect, uv)
    local handle
    local stdout = uv.new_pipe(false)

    handle = uv.spawn(cmd, {
      args = { "tests/manual-test-without-uv.run.lua" },
      cwd = cwd,
      stdio = {nil, stdout},
    },
    expect(function(status, signal)
      print('#437', status, signal)
      assert(status==0)
      assert(signal==0)
      handle:close()
    end))

    uv.read_start(stdout, expect(function (err, chunk)
      p("stdout", {err=err,chunk=chunk})
      uv.close(stdout)
    end))
  end)

  test("issue #599, crash during calling os.exit", function (print, p, expect, uv)
    local handle
    local stdout = uv.new_pipe(false)

    handle = uv.spawn(cmd, {
      args = { "tests/manual-test-exit.lua" },
      cwd = cwd,
      stdio = {nil, stdout},
    },
    expect(function(status, signal)
      print('#599', status, signal)
      assert(status==5)
      assert(signal==0)
      handle:close()
    end))

    uv.read_start(stdout, expect(function (err, chunk)
      p("stdout", {err=err,chunk=chunk})
      uv.close(stdout)
    end))
  end)

end)
