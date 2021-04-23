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

  -- return a test function reusable for ipv4 and ipv6
  local function multicast_join_test(bind_addr, multicast_addr, interface_addr)
    return function(print, p, expect, uv)
      local uvVersionGEQ = require('lib/utils').uvVersionGEQ

      local server = assert(uv.new_udp())
      assert(uv.udp_bind(server, bind_addr, TEST_PORT))
      local _, err, errname = uv.udp_set_membership(server, multicast_addr, interface_addr, "join")
      if errname == "ENODEV" then
        print("no multicast route, skipping")
        server:close()
        return
      elseif errname == "EADDRNOTAVAIL" and multicast_addr == "ff02::1" then
        -- OSX, BSDs, and some other platforms need %lo in their multicast/interface addr
        -- so try that instead
        multicast_addr = "ff02::1%lo0"
        interface_addr = "::1%lo0"
        assert(uv.udp_set_membership(server, multicast_addr, interface_addr, "join"))
      else
        assert(not err, err)
      end

      local client = assert(uv.new_udp())

      local recv_cb_called = 0
      local function recv_cb(err, data, addr, flags)
        assert(not err, err)
        p(data, addr)

        -- empty callback can happen, just return early
        if data == nil and addr == nil then
          return
        end

        assert(addr)
        assert(data == "PING")

        recv_cb_called = recv_cb_called + 1
        if recv_cb_called == 2 then
          -- note: because of this conditional close, the test will fail with an unclosed handle if recv_cb_called
          -- doesn't hit 2, so we don't need to expect(recv_cb) or assert recv_cb_called == 2
          server:close()
        else
          -- udp_set_source_membership added in 1.32.0
          if uvVersionGEQ("1.32.0") then
            local source_addr = addr.ip
            assert(server:set_membership(multicast_addr, interface_addr, "leave"))
            _, err, errname = server:set_source_membership(multicast_addr, interface_addr, source_addr, "join")
            if errname == "ENOSYS" then
              -- not all systems support set_source_membership, so rejoin the previous group and continue on
              assert(server:set_membership(multicast_addr, interface_addr, "join"))
            else
              assert(not err, err)
            end
          end
          assert(client:send("PING", multicast_addr, TEST_PORT, expect(function(err)
            assert(not err, err)
            client:close()
          end)))
        end
      end

      server:recv_start(recv_cb)

      assert(client:send("PING", multicast_addr, TEST_PORT, expect(function(err)
        -- EPERM here likely means that a firewall has denied the send, which
        -- can happen in some build/CI environments, e.g. the Fedora build system.
        -- Reproducible on Linux with iptables by doing:
        --  iptables --policy OUTPUT DROP
        --  iptables -A OUTPUT -s 127.0.0.1 -j ACCEPT
        -- and for ipv6:
        --  ip6tables --policy OUTPUT DROP
        --  ip6tables -A OUTPUT -s ::1 -j ACCEPT
        if err == "EPERM" then
          print("send to multicast ip was likely denied by firewall, skipping")
          client:close()
          server:close()
          return
        end
        assert(not err, err)
      end)))
    end
  end

  -- TODO This might be overkill, but the multicast
  -- tests seem to rely on external interfaces being
  -- available on some platforms. So, we use this to skip
  -- the tests when there are no relevant exernal interfaces
  -- available. Note: The Libuv multicast join test does use this
  -- same check for skipping the ipv6 test; we just expanded it to
  -- the ipv4 test as well.
  local function has_external_interface(uv, family)
    local addresses = assert(uv.interface_addresses())
    for _, vals in pairs(addresses) do
      for _, info in ipairs(vals) do
        if (not family or info.family == family) and not info.internal then
          return true
        end
      end
    end
    return false
  end

  test("udp multicast join ipv4", function(print, p, expect, uv)
    if not has_external_interface(uv, "inet") then
      print("no external ipv4 interface, skipping")
      return
    end
    local testfn = multicast_join_test("0.0.0.0", "239.255.0.1", nil)
    return testfn(print, p, expect, uv)
  end)

  test("udp multicast join ipv6", function(print, p, expect, uv)
    if not has_external_interface(uv, "inet6") then
      print("no external ipv6 interface, skipping")
      return
    end
    local testfn = multicast_join_test("::", "ff02::1", nil)
    return testfn(print, p, expect, uv)
  end)

  test("udp recvmmsg", function(print, p, expect, uv)
    local NUM_SENDS = 8
    local NUM_MSGS_PER_ALLOC = 4

    local recver = uv.new_udp({mmsgs = NUM_MSGS_PER_ALLOC})
    assert(recver:bind("0.0.0.0", TEST_PORT))

    local sender = uv.new_udp()

    local msgs_recved = 0
    local recv_cb = function(err, data, addr, flags)
      assert(not err, err)
      p(data, addr)

      -- empty callback can happen, just return early
      if data == nil and addr == nil then
        return
      end

      assert(addr)
      assert(data == "PING")

      msgs_recved = msgs_recved + 1
      if msgs_recved == NUM_SENDS then
        sender:close()
        recver:close()
      end
    end

    assert(recver:recv_start(recv_cb))

    for i=1,NUM_SENDS do
      assert(sender:try_send("PING", "127.0.0.1", TEST_PORT))
    end
  end, "1.39.0")
end)
