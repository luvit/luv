return require('lib/tap')(function (test)

  local function bench(uv, p, count, fn)
    collectgarbage()
    local before
    local notify = count / 8
    for i = 1, count do
      fn()
      if i % notify == 0 then
        uv.run()
        collectgarbage()
        local now = uv.resident_set_memory()
        if not before then before = now end
        p(i, now)
      end
    end
    uv.run()
    collectgarbage()
    local after = uv.resident_set_memory()
    p{
      before = before,
      after = after,
    }
    assert(after < before * 1.5)
  end

  test("lots-o-timers", function (print, p, expect, uv)
    bench(uv, p, 0x10000, function ()
      local timer = uv.new_timer()
      uv.close(timer)
    end)
  end)

  test("lots-o-timers with canceled callbacks", function (print, p, expect, uv)
    bench(uv, p, 0x10000, function ()
      local timer = uv.new_timer()
      uv.timer_start(timer, 100, 100, function ()
      end)
      uv.timer_stop(timer)
      uv.close(timer, function ()
      end)
      uv.run()
    end)
  end)

  test("lots-o-timers with real timeouts", function (print, p, expect, uv)
    bench(uv, p, 0x500, function ()
      local timer = uv.new_timer()
      uv.timer_start(timer, 10, 0, expect(function ()
        uv.timer_stop(timer)
        uv.close(timer, function ()
        end)
      end))
    end)
  end)

  test("reading file async", function (print, p, expect, uv)
    local mode = tonumber("644", 8)
    bench(uv, p, 0x500, function ()
      local onOpen, onStat, onRead, onClose
      local fd, stat

      onOpen = expect(function (err, result)
        assert(not err, err)
        fd = result
        uv.fs_fstat(fd, onStat)
      end)

      onStat = expect(function (err, result)
        assert(not err, err)
        stat = result
        uv.fs_read(fd, stat.size, 0, onRead)
      end)

      onRead = expect(function (err, data)
        assert(not err, err)
        assert(#data == stat.size)
        uv.fs_close(fd, onClose)
      end)

      onClose = expect(function (err)
        assert(not err, err)
      end)

      assert(uv.fs_open("README.md", "r", mode, onOpen))
    end)
  end)

  test("reading file sync", function (print, p, expect, uv)
    local mode = tonumber("644", 8)
    bench(uv, p, 0x2000, function ()
      local fd = assert(uv.fs_open("README.md", "r", mode))
      local stat = assert(uv.fs_fstat(fd))
      local data = assert(uv.fs_read(fd, stat.size, 0))
      assert(#data == stat.size)
      assert(uv.fs_close(fd))
    end)
  end)

  test("invalid file", function (print, p, expect, uv)
    local mode = tonumber("644", 8)
    bench(uv, p, 0x1500, function ()
      local req = uv.fs_open("BAD_FILE", "r", mode, expect(function (err, fd)
        assert(not fd)
        assert(err)
      end))
    end)
  end)

  test("invalid file sync", function (print, p, expect, uv)
    local mode = tonumber("644", 8)
    bench(uv, p, 0x20000, function ()
      local fd, err = uv.fs_open("BAD_FILE", "r", mode)
      assert(not fd)
      assert(err)
    end)
  end)

  test("invalid spawn args", function (print, p, expect, uv)
    -- Regression test for #73
    bench(uv, p, 0x10000, function ()
      local ret, err = pcall(function ()
        return uv.spawn("ls", {
          args = {"-l", "-h"},
          stdio = {0, 1, 2},
          env = {"EXTRA=true"},
          gid = false, -- Should be integer
        })
      end)
      assert(not ret)
      assert(err)
    end)
  end)

  test("pipe writing with vectors", function (print, p, expect, uv)
    local port = 0
    local data = {}
    for i = 0, 255 do
      data[i + 1] = string.rep(string.char(i), 100)
    end
    bench(uv, p, 0x800, function ()
      local server = uv.new_tcp()
      server:bind("::1", port)
      server:listen(1, expect(function (err)
        assert(not err, err)
        local client = uv.new_pipe(false)
        server:accept(client)
        client:write(data)
        client:close()
        server:close()
      end))
      local address = server:getsockname()
      port = address.port
      local socket = uv.new_tcp()
      socket:connect(address.ip, port, expect(function (err)
        assert(not err, err)
        socket:read_start(expect(function (err, chunk)
          assert(not err, err)
          assert(chunk)
          socket:close()
        end))
      end))
      uv.run()
    end)
  end)

end)
