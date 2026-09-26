return require('lib/tap')(function (test)

  test("cancel", function (print, p, expect, uv)
    -- might need to try a few times since cancel can fail with EBUSY
    for _=1,5 do
      local req = assert(uv.fs_stat('.', expect(function(err)
        if err then
          assert(err:match("^ECANCELED"))
        else
          assert(not err, err)
        end
      end)))

      local _, cancel_err = req:cancel()
      if cancel_err == nil then
        break
      end
    end
  end)

  test("get type", function (print, p, expect, uv)
    local req = uv.fs_stat('.', expect(function(err)
      assert(not err, err)
    end))

    local typename, typeid = req:get_type()
    assert(typename == "fs")

    local typename_, typeid_ = uv.req_get_type(req)
    assert(typename == typename_)
    assert(typeid == typeid_)
  end, "1.19.0")

  test("write_nwritten", function (print, p, expect, uv)
    local server = uv.new_tcp()
    assert(uv.tcp_bind(server, "127.0.0.1", 0))
    local addr = uv.tcp_getsockname(server)
    assert(uv.listen(server, 1, function () end))

    local client = uv.new_tcp()
    local data = "Hello World"
    local req
    assert(uv.tcp_connect(client, addr.ip, addr.port, expect(function (err)
      assert(not err, err)
      -- uv_write() on a TCP handle that is still connecting returns UV_EPIPE
      -- on Windows (the handle does not become UV_HANDLE_WRITABLE until the
      -- connection is established), so the write must be issued from the
      -- connect callback rather than right after uv.tcp_connect().
      req = assert(uv.write(client, data, expect(function (err)
        assert(not err, err)
        -- Only valid from within the write callback
        local nwritten = uv.write_nwritten(req)
        assert(nwritten == #data, "expected " .. #data .. ", got " .. tostring(nwritten))
        assert(req:nwritten() == nwritten)
        uv.close(client)
        uv.close(server)
      end)))
    end)))
  end, "1.53.0")
end)
