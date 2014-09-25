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

local function testPoll(loop)
  local poll = uv.new_poll(loop, 0)
  function poll:onpoll(err, evt)
    p("onpoll", err, evt)
    assert(self == poll)
    uv.poll_stop(poll)
  end
  uv.poll_start(poll, "r")
  p("Press Enter to test poll handler")
end

-- Sanity check for uv_timer_t
local function testTimer(loop)
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
    end
  end
  uv.timer_start(timer, 200, 100)
end

local function testSignal(loop, callback)
  local signal = uv.new_signal(loop)
  function signal:onsignal(name)
    p("onsignal", name)
    assert(self == signal)
    uv.signal_stop(signal)
    if callback then callback() end
  end
  uv.signal_start(signal, "SIGINT")
  p("Press Control+C to test signal handler")
end

local function testProcess(loop, callback)
  -- local process = assert(uv.spawn(loop, "sleep", {
  --   args = {1},
  -- }))
  -- local process = assert(uv.spawn(loop, "id", {
  --   uid = 1000,
  --   gid = 1000,
  --   stdio = {nil,2,2}
  -- }))
  -- local process = assert(uv.spawn(loop, "pwd", {
  --   -- cwd = "/home/tim",
  --   stdio = {nil,2,2}
  -- }))
  local process = assert(uv.spawn(loop, "env", {
    args = {"env", "MERGED=true"},
    env = {"NAME=tim", "AGE=32"},
    stdio = {nil,2,2}
  }))
  function process:onexit(exit_status, term_signal)
    p("onexit", exit_status, term_signal)
    assert(self == process)
    if callback then callback() end
  end
  uv.disable_stdio_inheritance()
end

local function testStream(loop)
  local sreq = uv.shutdown_req()
  local wreq = uv.write_req()
  p(sreq, wreq)
end

local tests = {
  testPrepare,
  testCheck,
  testIdle,
  testAsync,
  testPoll,
  testTimer,
  testProcess,
  testStream,
  testSignal,
}


coroutine.wrap(function ()
  local loop = uv.new_loop()
  collectgarbage()
  logHandles(loop)

  local function onDone()
    closeLoop(loop)
    collectgarbage()
    logHandles(loop, true)
  end

  for i=1,#tests-1 do
    tests[i](loop)
    collectgarbage()
    logHandles(loop)
  end

  tests[#tests](loop, onDone)
  collectgarbage()
  logHandles(loop, true)

  print("blocking")
  uv.run(loop)
  print("done blocking")

  collectgarbage()
  logHandles(loop, true)

  uv.loop_close(loop)
end)()
