return require('lib/tap')(function (test)

  local isWindows = require('lib/utils').isWindows

  local function errorAllowed(err)
    -- allowed errors  from gnulib's test-getaddrinfo.c
    return err == "EAI_AGAIN" -- offline/no network connection
      or err == "EAI_NONAME"  -- IRIX returns this for "https"
      or err == "EAI_SERVICE" -- Solaris returns this for "http"/"https"
      or err == "EAI_NODATA"  -- AIX returns this for "https"
  end

  test("Get all local http addresses", function (print, p, expect, uv)
    assert(uv.getaddrinfo(nil, "http", nil, expect(function (err, res)
      if errorAllowed(err) then
        print(err, "skipping")
        return
      end
      assert(not err, err)
      p(res, #res)
      assert(res[1].port == 80)
    end)))
  end)

  test("Get all local http addresses sync", function (print, p, expect, uv)
    local res, errstr, err = uv.getaddrinfo(nil, "http")
    if errorAllowed(err) then
      print(err, "skipping")
      return
    end
    assert(res, errstr)
    p(res, #res)
    assert(res[1].port == 80)
  end, "1.3.0")

  test("Get only ipv4 tcp adresses for luvit.io", function (print, p, expect, uv)
    assert(uv.getaddrinfo("luvit.io", nil, {
      socktype = "stream",
      family = "inet",
    }, expect(function (err, res)
      if errorAllowed(err) then
        print(err, "skipping")
        return
      end
      assert(not err, err)
      p(res, #res)
      assert(#res > 0)
    end)))
  end)

  -- FIXME: this test always fails on AppVeyor for some reason
  if isWindows and not os.getenv'APPVEYOR' then
    test("Get only ipv6 tcp adresses for luvit.io", function (print, p, expect, uv)
      assert(uv.getaddrinfo("luvit.io", nil, {
        socktype = "stream",
        family = "inet6",
      }, expect(function (err, res)
        if errorAllowed(err) then
          print(err, "skipping")
          return
        end
        assert(not err, err)
        p(res, #res)
        assert(#res == 1)
      end)))
    end)
  end

  test("Get ipv4 and ipv6 tcp adresses for luvit.io", function (print, p, expect, uv)
    assert(uv.getaddrinfo("luvit.io", nil, {
      socktype = "stream",
    }, expect(function (err, res)
      if errorAllowed(err) then
        print(err, "skipping")
        return
      end
      assert(not err, err)
      p(res, #res)
      assert(#res > 0)
    end)))
  end)

  test("Get all adresses for luvit.io", function (print, p, expect, uv)
    assert(uv.getaddrinfo("luvit.io", nil, nil, expect(function (err, res)
      if errorAllowed(err) then
        print(err, "skipping")
        return
      end
      assert(not err, err)
      p(res, #res)
      assert(#res > 0)
    end)))
  end)

  test("Lookup local ipv4 address", function (print, p, expect, uv)
    assert(uv.getnameinfo({
      family = "inet",
    }, expect(function (err, hostname, service)
      if errorAllowed(err) then
        print(err, "skipping")
        return
      end
      p{err=err,hostname=hostname,service=service}
      assert(not err, err)
      assert(hostname)
      assert(service)
    end)))
  end)

  test("Lookup local ipv4 address sync", function (print, p, expect, uv)
    local hostname, service, err = uv.getnameinfo({
      family = "inet",
    })
    if errorAllowed(err) then
      print(err, "skipping")
      return
    end
    p{hostname=hostname,service=service}
    assert(hostname)
    assert(service)
  end, "1.3.0")

  test("Lookup local 127.0.0.1 ipv4 address", function (print, p, expect, uv)
    assert(uv.getnameinfo({
      ip = "127.0.0.1",
    }, expect(function (err, hostname, service)
      p{err=err,hostname=hostname,service=service}
      assert(not err, err)
      assert(hostname)
      assert(service)
    end)))
  end)

  test("Lookup local ipv6 address", function (print, p, expect, uv)
    assert(uv.getnameinfo({
      family = "inet6",
    }, expect(function (err, hostname, service)
      p{err=err,hostname=hostname,service=service}
      assert(not err, err)
      assert(hostname)
      assert(service)
    end)))
  end)

  test("Lookup local ::1 ipv6 address", function (print, p, expect, uv)
    assert(uv.getnameinfo({
      ip = "::1",
    }, expect(function (err, hostname, service)
      p{err=err,hostname=hostname,service=service}
      assert(not err, err)
      assert(hostname)
      assert(service)
    end)))
  end)

  test("Lookup local port 80 service", function (print, p, expect, uv)
    assert(uv.getnameinfo({
      port = 80,
      family = "inet6",
    }, expect(function (err, hostname, service)
      p{err=err,hostname=hostname,service=service}
      assert(not err, err)
      assert(hostname)
      assert(service == "http")
    end)))
  end)

end)
