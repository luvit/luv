local p = require('lib/utils').prettyPrint
local uv = require('luv')

-- Helper to log all handles in a loop
local function logHandles(loop, verbose)
  local handles = uv.walk(loop)
  if not verbose then
    p(loop, uv.walk(loop))
    return
  end
  local data = {}
  for i, handle in ipairs(uv.walk(loop)) do
    local props = {}
    for key, value in pairs(handle) do
      props[key] = value
    end
    data[handle] = props
  end
  p(loop, data)
end

-- Close all handles for a given loop
local function closeLoop(loop)
  print(loop, "Closing all handles in loop")
  for i, handle in ipairs(uv.walk(loop)) do
    print(handle, "closing...")
    uv.close(handle)
    print(handle, "closed.")
  end
end

-- Sanity check for uv_prepare_t
local function testPrepare(loop, callback)
  local prepare = uv.new_prepare(loop)
  function prepare:onprepare()
    print("onprepare")
    assert(self == prepare)
    uv.prepare_stop(prepare)
    if callback then callback() end
  end
  uv.prepare_start(prepare);
end

-- Sanity check for uv_check_t
local function testCheck(loop, callback)
  local check = uv.new_check(loop)
  function check:oncheck()
    print("oncheck")
    assert(self == check)
    uv.check_stop(check)
    if callback then callback() end
  end
  uv.check_start(check)
end

-- Sanity check for uv_idle_t
local function testIdle(loop, callback)
  local idle = uv.new_idle(loop)
  function idle:onidle()
    print("onidle")
    assert(self == idle)
    uv.idle_stop(idle)
    if callback then callback() end
  end
  uv.idle_start(idle)
end

local function testAsync(loop)
  local async = uv.new_async(loop)
  function async:onasync()
    print("onasync")
    assert(self == async)
  end
  uv.async_send(async)
end

-- Sanity check for uv_timer_t
local function testTimer(loop, callback)
  local timer = uv.new_timer(loop)
  function timer:ontimeout()
    print("ontimeout 1")
    assert(self == timer)
    uv.timer_stop(timer)
    assert(uv.timer_get_repeat(timer) == 100)
    uv.timer_set_repeat(timer, 200)
    uv.timer_again(timer)
    function timer:ontimeout()
      print("ontimeout 2")
      assert(self == timer)
      assert(uv.timer_get_repeat(timer) == 200)
      uv.timer_stop(self)
      if callback then callback() end
    end
  end
  uv.timer_start(timer, 200, 100)
end


coroutine.wrap(function ()
  local loop = uv.new_loop()
  collectgarbage()
  logHandles(loop)
  testPrepare(loop)
  collectgarbage()
  logHandles(loop)
  testCheck(loop)
  collectgarbage()
  logHandles(loop)
  testIdle(loop)
  collectgarbage()
  logHandles(loop)
  testAsync(loop)
  collectgarbage()
  logHandles(loop)
  testTimer(loop, function()
    closeLoop(loop)
    collectgarbage()
    logHandles(loop)
  end)
  collectgarbage()
  logHandles(loop)
  print("blocking")
  collectgarbage()
  logHandles(loop, true)
  uv.run(loop)
  collectgarbage()
  print("done")
  collectgarbage()
  logHandles(loop)
  uv.loop_close(loop)
  collectgarbage()
end)()



