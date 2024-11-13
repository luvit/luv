local uv = require('uv')

local function setTimeout(callback, ms)
  local timer = uv.new_timer()
  timer:start(ms, 0, function()
    uv.walk(function(...)
      print('uv.walk() in Main Lua', ...)
    end)
    timer:stop()
    timer:close()
    callback()
  end)
  return timer
end

setTimeout(function()
  print('Main Lua done')
end, 1000)

local delay = 100
uv.update_time()
local before = uv.now()
local thread = uv.new_thread(function(delay)
  local uv = require('uv')
  local t1 = uv.thread_self()
  uv.sleep(delay)
  local t2 = uv.thread_self()
  assert(t1:equal(t2))
  assert(tostring(t1)==tostring(t2))
  _G.print('Runing', uv.thread_self())

  assert(_THREAD)

  local function setTimeout(callback, ms)
    local timer = uv.new_timer()
    timer:start(ms, 0, function()
      uv.walk(function(...)
        print('in thread Lua', ...)
      end)
      timer:stop()
      timer:close()
      callback()
    end)
    return timer
  end

  setTimeout(function()
    print('thread Lua done')
  end, 1000)
  uv.run()
end,delay)
print('launch thread:', thread)

uv.thread_join(thread)
-- uv.update_time()
-- local elapsed = uv.now() - before
-- assert(elapsed >= delay, "elapsed should be at least delay ")

uv.run()
print('DONE')
