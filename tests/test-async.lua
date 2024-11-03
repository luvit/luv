return require('lib/tap')(function (test)

  test("test pass async between threads", function(p, p, expect, uv)
    local async
    async = uv.new_async(expect(function (a,b,c)
      p('in async notify callback')
      p(a,b,c)
      assert(a=='a')
      assert(b==true)
      assert(c==250)
      uv.close(async)
    end))
    uv.new_thread(function(asy)
      local uv = require'luv'
      assert(type(asy)=='userdata')
      assert(uv.async_send(asy,'a',true,250)==0)
      uv.run()
    end, async):join()
  end)

  test("test pass back async between threads", function(p, p, expect, uv)
    local rasync
    local async
    async = uv.new_async(expect(function (a)
      uv.close(async)
      p('in async notify callback')
      p(a)
      assert(type(a)=='userdata')
      assert(uv.async_send(a,'a',true,250)==0)
      rasync = a
    end))
    local t = uv.new_thread(function(asy)
      local uv = require'luv'
      assert(type(asy)=='userdata', 'bad aync type')
      local as
      as = uv.new_async(function (a,b,c)
        uv.close(as)
        assert(a=='a', 'bad string')
        assert(b==true, 'bad boolean')
        assert(c==250, 'bad number')
      end)
      assert(uv.async_send(asy,as)==0)
      uv.run()
    end, async)
    uv.run()
    t:join()
    assert(rasync, 'thread async not received')
  end)

end)
