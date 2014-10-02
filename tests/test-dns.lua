require('lib/tap')(function (test)

  test("resolve a domain to ip", function (print, p, expect, uv)
    local req = uv.getaddrinfo("luvit.io", nil, nil, function (err, res)
      assert(not err, err)
      p(res)
    end)
  end)

end)
