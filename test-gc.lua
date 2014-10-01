local p = require('lib/utils').prettyPrint
local uv = require('luv')

local loop = uv.new_loop()

-- Helper to log all handles in a loop
local function logHandles(verbose)
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
local function closeLoop()
  print(loop, "Closing all handles in loop")
  for i, handle in ipairs(uv.walk(loop)) do
    print(handle, "closing...")
    uv.close(handle)
    print(handle, "closed.")
  end
end


local function test()
  local timer = uv.new_timer(loop)
  logHandles()
  function timer:ontimeout()
    collectgarbage()
    print("Timeout")
    uv.timer_stop(timer)
    collectgarbage()
    closeLoop()
    collectgarbage()
    logHandles()
  end
  uv.timer_start(timer, 200, 0)
  collectgarbage()
  p{
    timer=timer,
    loop=loop,
  }
end
test()
logHandles()
collectgarbage()
uv.run(loop)
collectgarbage()
uv.loop_close(loop)

