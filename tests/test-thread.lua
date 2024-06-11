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
    local args = {500, 'string', nil, false, 5, "helloworld", {111, 222}}
    local unpack = unpack or table.unpack
    uv.new_thread(function(num,s,null,bool,five,hw,t)
      assert(type(num) == "number")
      assert(type(s) == "string")
      assert(null == nil)
      assert(bool == false)
      assert(five == 5)
      assert(hw == 'helloworld')
      assert(#t == 2 and t[1] == 111 and t[2] == 222)
      require('luv').sleep(100)
    end, unpack(args)):join()
    uv.update_time()
    local elapsed = uv.now() - before
    assert(elapsed >= 100, "elapsed should be at least delay ")
  end)

  test("test thread create table arguments", function(print, p, expect, uv)
    uv.update_time()
    local before = uv.now()

    local recursive = { 500 }
    recursive.foo = recursive

    local same = { 1, 2 }
    same = setmetatable(same, same)

    local Class = {}
    Class.__index = Class
    function Class:add(x)
      self.x = self.x + x
    end

    local args = {
      { 500, hw = "helloworld", nest = { 1 } },
      setmetatable({}, {}),
      same,
      setmetatable({ false }, { __index = {
        add = function(x)
          return x + 1
        end,
      } }),
      recursive,
      setmetatable({ x = 10 }, Class),
    }

    local unpack = unpack or table.unpack
    uv.new_thread(function(t, empty, same, meta, rec, cls)
      assert(type(t) == "table")
      assert(t[1] == 500 and t.hw == "helloworld" and t.nest[1] == 1 and getmetatable(t) == nil)
      assert(#empty == 0 and #getmetatable(empty) == 0)
      assert(same == getmetatable(same))
      assert(meta[1] == false)
      assert(meta.add(10) == 11)
      assert(rec[1] == 500 and rec.foo == rec)
      assert(getmetatable(cls) == getmetatable(cls).__index)
      cls:add(1)
      assert(cls.x == 11)
      require("luv").sleep(100)
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

  test("test thread arguments limit", function(print, p, expect, uv)
    local args = {}
    args[1] = uv.new_async(expect(function (n)
      assert(n==9)
      args[1]:close()
    end))
    for i=2, 10 do
      args[i] = i
    end
    local unpack = unpack or table.unpack
    uv.new_thread(function(...)
      local arg = {...}
      assert(#arg == 9)
      arg[1]:send(#arg)
    end, unpack(args)):join()
    assert(#args==10)
  end)

  test("test avoid thread be released before it done", function(print, p, expect, uv)
    uv.new_thread(function(...)
        io.write(table.concat({...}, " ") .. ' from thread\n')
      end, 'hello', 'world')
    collectgarbage('collect')
    collectgarbage('collect')
    collectgarbage('collect')
  end)

  test("thread_getcpu", function(print, p, expect, uv)
    local cpu, err = uv.thread_getcpu()
    if not cpu then
      print(err, "skipping")
      return
    end
    -- starts at 1 to match the tables used by getaffinity/setaffinity
    assert(cpu >= 1)
  end, "1.45.0")

  test("getaffinity, setaffinity", function(print, p, expect, uv)
    local mask_size, err = uv.cpumask_size()
    if not mask_size then
      print(err, "skipping")
      return
    end
    uv.new_thread(function(cpumask_size)
      local _uv = require('luv')
      local thread = _uv.thread_self()
      local affinity = assert(thread:getaffinity())
      assert(#affinity == cpumask_size)

      -- set every cpu's affinity to false except the current cpu
      local cur_cpu = _uv.thread_getcpu()
      -- even though this table may not be a full array-like table,
      -- this still works because `setaffinity` will treat any missing
      -- CPU numbers up to cpumask_size as having a setting of `false`
      local affinity_to_set = {
        [cur_cpu] = true,
      }
      local prev_affinity = assert(thread:setaffinity(affinity_to_set, true))
      -- the returned affinity should match the original affinity
      assert(#prev_affinity == #affinity)
      for i=1,#affinity do
        assert(prev_affinity[i] == affinity[i])
      end

      local new_affinity = thread:getaffinity()
      assert(#new_affinity == cpumask_size)
      for i=1,#new_affinity do
        local expected_setting = i == cur_cpu
        assert(new_affinity[i] == expected_setting)
      end
    end, mask_size):join()
  end, "1.45.0")

  test("getpriority, setpriority", function(_, p, _, uv)
    assert(type(uv.constants.THREAD_PRIORITY_HIGHEST)=='number')
    assert(type(uv.constants.THREAD_PRIORITY_ABOVE_NORMAL)=='number')
    assert(type(uv.constants.THREAD_PRIORITY_NORMAL)=='number')
    assert(type(uv.constants.THREAD_PRIORITY_BELOW_NORMAL)=='number')
    assert(type(uv.constants.THREAD_PRIORITY_LOWEST)=='number')

    local thread = uv.new_thread(function()
      local _uv = require('luv')
      local self = _uv.thread_self()
      local priority = assert(self:getpriority())
      print('priority in thread', priority)
    end)

    local priority = assert(thread:getpriority())
    print('default priority', priority)

    assert(thread:setpriority(uv.constants.THREAD_PRIORITY_LOWEST))
    priority = assert(thread:getpriority())
    print('priority after change', priority)
    thread:join()
  end, "1.48.0")
end)
