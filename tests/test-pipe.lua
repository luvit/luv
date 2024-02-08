local TEST_PIPENAME = "\\\\?\\pipe\\luv-test"

return require('lib/tap')(function (test)
  test("pipe chmod", function (print, p, expect, uv)
    local pipe = assert(uv.new_pipe())
    assert(pipe:bind(TEST_PIPENAME))
    local _, err, errname = pipe:chmod("r")
    if errname == "EPERM" then
      print("Insufficient privileges to alter pipe fmode, skipping")
      pipe:close()
      return
    end
    assert(not err, err)
    assert(pipe:chmod("w"))
    assert(pipe:chmod("rw"))
    assert(pipe:chmod("wr"))

    local ok = pcall(function() uv.pipe_chmod(pipe, "bad flags") end)
    assert(not ok)

    pipe:close()
  end, "1.16.0")

  test("pipe ping pong", function(print, p, expect, uv)
    local PING = "PING\n"
    local NUM_PINGS = 4

    local fds = assert(uv.pipe({nonblock=true}, {nonblock=true}))
    assert(uv.guess_handle(fds.read) == "pipe")
    assert(uv.guess_handle(fds.write) == "pipe")

    local ponger = assert(uv.new_pipe())
    assert(ponger:open(fds.read))

    local pinger = assert(uv.new_pipe())
    assert(pinger:open(fds.write))

    local ping = function()
      assert(pinger:write(PING, expect(function(err)
        assert(not err, err)
      end)))
    end

    local pongs = 0
    ping()

    assert(ponger:read_start(function(err, chunk)
      assert(not err, err)
      if chunk == nil then
        return
      end
      assert(chunk == PING)
      pongs = pongs + 1
      if pongs == NUM_PINGS then
        ponger:close()
        pinger:close()
      else
        ping()
      end
    end))
  end, "1.41.0")

  test("pipe close fd", function(print, p, expect, uv)
    local fds = assert(uv.pipe())

    local pipe_handle = assert(uv.new_pipe())
    assert(pipe_handle:open(fds.read))
    -- pipe_open takes ownership of the file descriptor
    fds.read = nil

    assert(uv.fs_write(fds.write, "PING"))
    assert(uv.fs_close(fds.write))
    fds.write = nil

    local read_cb_called = 0
    local read_cb = expect(function(err, chunk)
      assert(not err, err)
      read_cb_called = read_cb_called + 1
      if read_cb_called == 1 then
        assert(chunk == "PING")
        pipe_handle:read_stop()
      elseif read_cb_called == 2 then
        assert(chunk == nil)
        pipe_handle:close()
      end
    end, 2)

    assert(pipe_handle:read_start(read_cb))
    uv.run()
    assert(read_cb_called == 1)

    assert(pipe_handle:read_start(read_cb))
    uv.run()
    assert(read_cb_called == 2)

    assert(pipe_handle:is_closing())
  end, "1.41.0")

  test("pipe getsockname abstract", function(print, p, expect, uv)
    -- https://github.com/libuv/libuv/blob/v1.x/test/test-pipe-getsockname.c#L164-L210
    local isWindows = require('lib/utils').isWindows
    local isLinux = require('lib/utils').isLinux

    local pipe_name = isWindows and "\\\\.\\pipe\\uv-test"
                                or "/tmp/uv-test-sock"

    local close_cnt, connected_cb = 0, 0

    local function close_cb()
      close_cnt = close_cnt + 1
    end

    local server = assert(uv.new_pipe(false))

    if isLinux then
      assert(server:bind2('\0' .. pipe_name))
      local name = server:getsockname()
      assert(#name == #pipe_name + 1 )
      assert(name:sub(1, #pipe_name + 1) == '\0' .. pipe_name)
      pipe_name = name
    else
      local _, err = server:bind2('\0' .. pipe_name)
      assert(err:match("^EINVAL:"))
    end

    server:listen(128, function(err)
      assert(not err, err)
      local cli = uv.new_tcp()
      assert(uv.accept(server, cli))
      cli:close()
      server:close(close_cb)
      connected_cb = connected_cb + 1
    end)

    local client = assert(uv.new_pipe(false))

    client:connect2(pipe_name, nil, function(err)
      if isLinux then
        assert(not err, err)
      else
        assert(err == 'ENOENT', err)
      end
      client:close(close_cb)
    end)

    if not isLinux then
      server:close(close_cb)
    end
    uv.run()
    assert(close_cnt == 2)
    if isLinux then
      assert(connected_cb == 1)
    else
      assert(connected_cb == 0)
    end
  end, "1.46.0")

end)
