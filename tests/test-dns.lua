require('lib/tap')(function (test)

  test("Get only ipv4 tcp adresses for luvit.io", function (print, p, expect, uv)
    local req = uv.getaddrinfo("luvit.io", nil, {
      socktype = "stream",
      family = "inet",
    }, function (err, res)
      assert(not err, err)
      p(res, #res)
      assert(#res == 1)
    end)
  end)

  test("Get only ipv6 tcp adresses for luvit.io", function (print, p, expect, uv)
    local req = uv.getaddrinfo("luvit.io", nil, {
      socktype = "stream",
      family = "inet6",
    }, function (err, res)
      assert(not err, err)
      p(res, #res)
      assert(#res == 1)
    end)
  end)

  test("Get ipv4 and ipv6 tcp adresses for luvit.io", function (print, p, expect, uv)
    local req = uv.getaddrinfo("luvit.io", nil, {
      socktype = "stream",
    }, function (err, res)
      assert(not err, err)
      p(res, #res)
      assert(#res == 2)
    end)
  end)

  test("Get all adresses for luvit.io", function (print, p, expect, uv)
    local req = uv.getaddrinfo("luvit.io", nil, nil, function (err, res)
      assert(not err, err)
      p(res, #res)
      assert(#res == 6)
    end)
  end)




end)
