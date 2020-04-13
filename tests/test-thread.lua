return require('lib/tap')(function (test)

  test("test thread create", function(print, p, expect, uv)
    local delay = 100
    uv.update_time()
    local before = uv.now()
    local thread = uv.new_thread(function(delay)
      local uv = require('luv')
      local t1 = uv.thread_self()
      uv.sleep(delay)
      local t2 = uv.thread_self()
      assert(t1:equal(t2))
      assert(tostring(t1)==tostring(t2))
      _G.print('  In', uv.thread_self())
    end,delay)
    uv.thread_join(thread)
    uv.update_time()
    local elapsed = uv.now() - before
    p({
      delay = delay,
      elapsed = elapsed
    })
    assert(elapsed >= delay, "elapsed should be at least delay ")
  end)

  test("test thread create with arguments", function(print, p, expect, uv)
    uv.update_time()
    local before = uv.now()
    local args = {500, 'string', nil, false, 5, "helloworld"}
    local unpack = unpack or table.unpack
    uv.new_thread(function(num,s,null,bool,five,hw)
      assert(type(num) == "number")
      assert(type(s) == "string")
      assert(null == nil)
      assert(bool == false)
      assert(five == 5)
      assert(hw == 'helloworld')
      require('luv').sleep(100)
    end, unpack(args)):join()
    uv.update_time()
    local elapsed = uv.now() - before
    assert(elapsed >= 100, "elapsed should be at least delay ")
  end)

  test("test thread sleep msecs in main thread", function(print, p, expect, uv)
    local delay = 100
    uv.update_time()
    local before = uv.now()
    print('Runing', uv.thread_self())
    uv.sleep(delay)
    print('Runing', uv.thread_self())
    uv.update_time()
    local elapsed = uv.now() - before
    p({
      delay = delay,
      elapsed = elapsed
    })
    assert(elapsed >= delay, "elapsed should be at least delay ")
  end)

  test("test thread create with options table", function(print, p, expect, uv)
    local delay = 100
    uv.update_time()
    local before = uv.now()
    local args = {delay, 'string', nil, false, 5, "helloworld"}
    local unpack = unpack or table.unpack
    uv.new_thread({stack_size=0}, function(delay,s,null,bool,five,hw)
      assert(type(delay) == "number")
      assert(type(s) == "string")
      assert(null == nil)
      assert(bool == false)
      assert(five == 5)
      assert(hw == 'helloworld')
      require('luv').sleep(delay)
    end, unpack(args)):join()
    uv.update_time()
    local elapsed = uv.now() - before
    p({
      delay = delay,
      elapsed = elapsed
    })
    assert(elapsed >= 100, "elapsed should be at least delay ")
  end, "1.26.0")

end)
