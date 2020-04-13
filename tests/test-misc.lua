return require('lib/tap')(function (test)

  test("uv.guess_handle", function (print, p, expect, uv)
    local types = {
      [0] = assert(uv.guess_handle(0)),
      assert(uv.guess_handle(1)),
      assert(uv.guess_handle(2)),
    }
    p("stdio fd types", types)
  end)

  test("uv.version and uv.version_string", function (print, p, expect, uv)
    local version = assert(uv.version())
    local version_string = assert(uv.version_string())
    p{version=version, version_string=version_string}
    assert(type(version) == "number")
    assert(type(version_string) == "string")
  end)

  test("memory size", function (print, p, expect, uv)
    local rss = uv.resident_set_memory()
    local total = uv.get_total_memory()
    local constrained = nil
    if uv.get_constrained_memory then
      constrained = uv.get_constrained_memory()
    end
    local free = uv.get_free_memory()
    p{rss=rss,total=total,free=free, constrained=constrained}
    assert(rss < total)
  end)

  test("uv.uptime", function (print, p, expect, uv)
    local uptime = assert(uv.uptime())
    p{uptime=uptime}
  end)

  test("uv.getrusage", function (print, p, expect, uv)
    local rusage = assert(uv.getrusage())
    p(rusage)
  end)

  test("uv.cpu_info", function (print, p, expect, uv)
    local info = assert(uv.cpu_info())
    p(info)
  end)

  test("uv.interface_addresses", function (print, p, expect, uv)
    local addresses = assert(uv.interface_addresses())
    for name, info in pairs(addresses) do
      p(name, addresses[name])
    end
  end)

  test("uv.loadavg", function (print, p, expect, uv)
    local avg = {assert(uv.loadavg())}
    p(avg)
    assert(#avg == 3)
  end)

  test("uv.exepath", function (print, p, expect, uv)
    local path = assert(uv.exepath())
    p(path)
  end)

  test("uv.os_homedir", function (print, p, expect, uv)
    local path = assert(uv.os_homedir())
    p(path)
  end, "1.9.0")

  test("uv.os_tmpdir", function (print, p, expect, uv)
    local path = assert(uv.os_tmpdir())
    p(path)
  end, "1.9.0")

  test("uv.os_get_passwd", function (print, p, expect, uv)
    local passwd = assert(uv.os_get_passwd())
    p(passwd)
  end, "1.9.0")

  test("uv.cwd and uv.chdir", function (print, p, expect, uv)
    local old = assert(uv.cwd())
    p(old)
    assert(uv.chdir("/"))
    local cwd = assert(uv.cwd())
    p(cwd)
    assert(cwd ~= old)
    assert(uv.chdir(old))
  end)

  test("uv.hrtime", function (print, p, expect, uv)
    local time = assert(uv.hrtime())
    p(time)
  end)

  test("uv.getpid", function (print, p, expect, uv)
    assert(uv.getpid())
  end)

  test("uv.os_uname", function(print, p, expect, uv)
    local uname = assert(uv.os_uname())
    p(uname)
  end, "1.25.0")

  test("uv.gettimeofday", function(print, p, expect, uv)
    local now = os.time()
    local sec, usec = assert(uv.gettimeofday())
    print('        os.time', now)
    print('uv.gettimeofday',string.format("%f",sec+usec/10^9))
    assert(type(sec)=='number')
    assert(type(usec)=='number')
  end, "1.28.0")

  test("uv.os_environ", function(print, p, expect, uv)
    local name, name2 = "LUV_TEST_FOO", "LUV_TEST_FOO2";
    local value, value2 = "123456789", ""

    assert(uv.os_setenv(name, value))
    assert(uv.os_setenv(name2, value2))

    local env = uv.os_environ();
    assert(env[name]==value)
    assert(env[name2]==value2)
  end, "1.31.0")

  test("uv.sleep", function(print, p, expect, uv)
    local val = 1000
    local begin = uv.now()

    uv.sleep(val)
    uv.update_time()

    local now = uv.now()
    assert(now-begin >= val)
  end)

  test("uv.random async", function(print, p, expect, uv)
    local len = 256
    assert(uv.random(len, {}, expect(function(err, randomBytes)
      assert(not err)
      assert(#randomBytes == len)
      -- this matches the LibUV test
      -- it can theoretically fail but its very unlikely
      assert(randomBytes ~= string.rep("\0", len))
    end)))
  end, "1.33.0")

  test("uv.random sync", function(print, p, expect, uv)
    local len = 256
    local randomBytes = assert(uv.random(len))
    assert(#randomBytes == len)
    -- this matches the LibUV test
    -- it can theoretically fail but its very unlikely
    assert(randomBytes ~= string.rep("\0", len))
  end, "1.33.0")

  test("uv.random errors", function(print, p, expect, uv)
    -- invalid flag
    local _, err = uv.random(0, -1)
    assert(err:match("^EINVAL"))

    -- invalid len
    _, err = uv.random(-1)
    assert(err:match("^E2BIG"))
  end, "1.33.0")

end)
