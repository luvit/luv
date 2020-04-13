local TEST_PORT = 9123

return require('lib/tap')(function (test)
  test("basic udp server and client (ipv4)", function (print, p, expect, uv)
    local server = uv.new_udp()
    assert(uv.udp_bind(server, "0.0.0.0", TEST_PORT))
    assert(uv.udp_recv_start(server, expect(function (err, data, addr, flags)
      p("server on recv", server, data, addr, flags)
      assert(not err, err)
      assert(data == "PING")
      uv.close(server, expect(function()
        p("server on close", server)
      end))
    end)))
    p{server=server}

    local client = uv.new_udp()
    local req = assert(uv.udp_send(client, "PING", "127.0.0.1", TEST_PORT, expect(function (err)
      p("client on send", client, err)
      assert(not err, err)
      uv.close(client, expect(function()
        p("client on close", client)
      end))
    end)))
    p{client=client,req=req}
  end)

  test("basic udp send from table", function (print, p, expect, uv)
    local sendData = {"P", "I", "NG"}
    local server = uv.new_udp()
    assert(uv.udp_bind(server, "0.0.0.0", TEST_PORT))
    assert(uv.udp_recv_start(server, expect(function (err, data, addr, flags)
      p("server on recv", server, data, addr, flags)
      assert(not err, err)
      assert(data == table.concat(sendData))
      uv.close(server, expect(function()
        p("server on close", server)
      end))
    end)))
    p{server=server}

    local client = uv.new_udp()
    local req = assert(uv.udp_send(client, sendData, "127.0.0.1", TEST_PORT, expect(function (err)
      p("client on send", client, err)
      assert(not err, err)
      uv.close(client, expect(function()
        p("client on close", client)
      end))
    end)))
    p{client=client,req=req}
  end)

  test("basic udp server and client (ipv6)", function (print, p, expect, uv)
    local server = uv.new_udp()
    local _, err = uv.udp_bind(server, "::1", TEST_PORT)
    if err then
      p("ipv6 unavailable", err)
      uv.close(server)
      return
    end
    assert(uv.udp_recv_start(server, expect(function (err, data, addr, flags)
      p("server on recv", server, data, addr, flags)
      assert(not err, err)
      assert(data == "PING")
      uv.close(server, expect(function()
        p("server on close", server)
      end))
    end)))
    p{server=server}

    local client = uv.new_udp()
    local req = assert(uv.udp_send(client, "PING", "::1", TEST_PORT, expect(function (err)
      p("client on send", client, err)
      assert(not err, err)
      uv.close(client, expect(function()
        p("client on close", client)
      end))
    end)))
    p{client=client,req=req}
  end)

  test("udp send args", function(print, p, expect, uv)
    local udp = uv.new_udp()

    local _, err = pcall(function() uv.udp_send(udp, "PING", 5, 5) end)
    print(assert(err))
    _, err = pcall(function() uv.udp_send(udp, "PING", "host", "port") end)
    print(assert(err))
    _, err = pcall(function() uv.udp_send(udp, "PING", "host", nil) end)
    print(assert(err))
    _, err = pcall(function() uv.udp_send(udp, "PING", nil, 5) end)
    print(assert(err))

    uv.close(udp)
  end, "1.27.0")

  test("udp connect", function(print, p, expect, uv)
    local server = uv.new_udp()
    local client = uv.new_udp()

    assert(uv.udp_bind(server, "0.0.0.0", TEST_PORT))
    local numRecvs = 0
    assert(uv.udp_recv_start(server, function (err, data, addr, flags)
      p("server on recv", server, data, addr, flags)
      assert(not err, err)
      -- nil data signifies nothing more to read
      if data ~= nil then
        assert(data == "PING", data)
        numRecvs = numRecvs + 1
        if numRecvs == 4 then
          uv.close(server, expect(function()
            p("server on close", server)
          end))
          uv.close(client, expect(function()
            p("client on close", client)
          end))
        end
      end
    end))
    p{server=server}

    assert(uv.udp_connect(client, "127.0.0.1", TEST_PORT))
    local _, err = uv.udp_connect(client, "8.8.8.8", TEST_PORT)
    assert(err and err:sub(1,7) == "EISCONN", err)

    local addr = assert(uv.udp_getpeername(client))
    p(addr)
    assert(addr.ip == "127.0.0.1")
    assert(addr.port == TEST_PORT)

    -- To send messages in connected UDP sockets addr must be NULL
    _, err = uv.udp_try_send(client, "PING", "127.0.0.1", TEST_PORT)
    assert(err and err:sub(1,7) == "EISCONN", err)

    local r = assert(uv.udp_try_send(client, "PING", nil, nil))
    assert(r == 4)

    _, err = uv.udp_try_send(client, "PING", "8.8.8.8", TEST_PORT)
    assert(err and err:sub(1,7) == "EISCONN", err)

    assert(uv.udp_connect(client, nil, nil))
    _, err = uv.udp_connect(client, nil, nil)
    assert(err and err:sub(1,8) == "ENOTCONN", err)

    _, err = uv.udp_getpeername(client)
    assert(err and err:sub(1,8) == "ENOTCONN", err)

    r = uv.udp_try_send(client, "PING", "127.0.0.1", TEST_PORT)
    assert(r == 4)
    _, err = uv.udp_try_send(client, "PING", nil, nil)
    assert(err and err:sub(1,12) == "EDESTADDRREQ", err)

    assert(uv.udp_connect(client, "127.0.0.1", TEST_PORT))
    _, err = uv.udp_send(client, "PING", "127.0.0.1", TEST_PORT, function()
      error("this send should fail")
    end)
    assert(err and err:sub(1,7) == "EISCONN", err)

    assert(uv.udp_send(client, "PING", nil, nil, expect(function(err)
      assert(not err, err)
      uv.udp_connect(client, nil, nil)
      _, err = uv.udp_send(client, "PING", nil, nil, function()
        error("this send should fail")
      end)
      assert(err and err:sub(1,12) == "EDESTADDRREQ", err)

      uv.udp_send(client, "PING", "127.0.0.1", TEST_PORT, expect(function(err)
        assert(not err, err)
      end))
    end)))
  end, "1.27.0")
end)
