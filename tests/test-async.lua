return require('lib/tap')(function (test)

  test("test pass async between threads", function(p, p, expect, uv)
    local async
    async = uv.new_async(expect(function (s, b, i, n, u)
      p('in async notify callback')
      p(s, b, i, n, u)
      assert(s=='a', 'bad string')
      assert(b==true, 'bad boolean')
      assert(i==250, 'bad integer')
      assert(n==3.14, 'bad number')
      assert(type(u)=='userdata', 'bad userdata')
      uv.close(async)
    end))
    uv.new_thread(function(asy)
      assert(type(asy)=='userdata')
      assert(asy:send('a', true, 250, 3.14, io.stderr)==0)
      require('luv').sleep(10)
    end, async):join()
  end)

  test("test async multiple send", function(p, p, expect, uv)
    local async
    async = uv.new_async(expect(function (v)
      p('in async notify callback')
      assert(v=='ok')
      async:close()
    end))
    uv.new_thread(function(asy)
      assert(type(asy)=='userdata')
      assert(asy:send('not ok')==0) -- will be ignored but its ok
      assert(asy:send('ok')==0)
      require('luv').sleep(10)
    end, async):join()
  end)

  test("test async queue send", function(p, p, expect, uv)
    local async
    async = uv.new_async(expect(function (v)
      p('in async notify callback')
      if v == 'close' then
        async:close()
      else
        assert(v=='ok')
      end
    end, 3), 3)
    uv.new_thread(function(asy)
      local uv = require('luv')
      assert(type(asy)=='userdata')
      assert(asy:send('ok')==0)
      assert(asy:send('ok')==0)
      assert(asy:send('close')==0)
      assert(select(3, asy:send('not ok'))=='ENOSPC')
      uv.sleep(10)
    end, async):join()
  end)

  test("test async send from same thread", function(p, p, expect, uv)
    local async
    async = uv.new_async(expect(function (v)
      p('in async notify callback')
      assert(v=='ok')
      async:close()
    end))
    assert(async:send('not ok')==0) -- will be ignored but its ok
    assert(async:send('ok')==0)
    uv.run()
  end)

  test("test async send during callback", function(p, p, expect, uv)
    local async
    async = uv.new_async(expect(function (d, v)
      p('in async notify callback', d, v)
      assert(v=='ok')
      if d > 0 then
        uv.sleep(d)
      else
        async:close()
      end
    end, 2))
    local t = uv.new_thread(function(asy)
      local uv = require('luv')
      assert(type(asy)=='userdata')
      assert(asy:send(100, 'ok')==0)
      uv.sleep(10) -- let async callback starts
      assert(asy:send(0, 'ok')==0)
      uv.sleep(10)
    end, async)
    uv.run()
    t:join()
  end)

  test("test pass back async between threads", function(p, p, expect, uv)
    local async
    async = uv.new_async(expect(function (asy)
      async:close()
      p('in async notify callback')
      assert(type(asy)=='userdata')
      assert(debug.getmetatable(asy))
      assert(asy:send('Hi\0', true, 250)==0) -- only working inside callback
      local timer = uv.new_timer()
      timer:start(10, 0, expect(function()
        timer:close()
        p('timeout')
        assert(not debug.getmetatable(asy)) -- outside callback the userdata loose its metatable
      end))
    end))
    local t = uv.new_thread(function(asy)
      local uv = require('luv')
      assert(type(asy)=='userdata', 'bad aync type')
      local as
      as = uv.new_async(function (s, b, i)
        as:close()
        assert(s=='Hi\0', 'bad string')
        assert(b==true, 'bad boolean')
        assert(i==250, 'bad integer')
      end)
      assert(asy:send(as)==0)
      uv.run()
    end, async)
    uv.run()
    t:join()
  end)

end)
