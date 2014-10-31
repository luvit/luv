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
      uv.timer_start(timer, 100, 100, function (self)
        assert(self == timer)
      end)
      uv.timer_stop(timer)
      uv.close(timer, function (self)
        assert(self == timer)
      end)
      uv.run()
    end)
  end)

  test("lots-o-timers with real timeouts", function (print, p, expect, uv)
    bench(uv, p, 0x500, function ()
      local timer = uv.new_timer()
      uv.timer_start(timer, 10, 0, expect(function (self)
        assert(self == timer)
        uv.timer_stop(timer)
        uv.close(timer, function (self)
          assert(self == timer)
        end)
      end))
    end)
  end)

  test("reading valid file", function (print, p, expect, uv)
    local mode = tonumber("644", 8)
    bench(uv, p, 0x500, function ()
      local req = assert(uv.fs_open("README.md", "r", mode, expect(function (err, fd)
        assert(not err, err)
        local stat = assert(uv.fs_fstat(fd))
        assert(uv.fs_read(fd, stat.size, 0))
        assert(uv.fs_close(fd, expect(function (err)
          assert(not err, err)
        end)))
      end)))
    end)
  end)

  test("reading file sync", function (print, p, expect, uv)
    local mode = tonumber("644", 8)
    bench(uv, p, 0x2000, function ()
      local fd = assert(uv.fs_open("README.md", "r", mode))
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

end)
