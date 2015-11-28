--[[
Demonstrates using cqueues with a luv mainloop
]]

local cqueues = require "cqueues"
local uv = require "luv"

local cq = cqueues.new()

do
  local timer = uv.new_timer()
  local function reset_timer()
    local timeout = cq:timeout()
    if timeout then
      timer:set_repeat(timeout * 1000)
      timer:again()
    end
  end
  local function onready()
    assert(cq:step(0))
    reset_timer()
  end
  timer:start(0, 0, onready)
  uv.new_poll(cq:pollfd()):start(cq:events(), onready)
end

cq:wrap(function()
  while true do
    cqueues.sleep(1)
    print("HELLO FROM CQUEUES")
  end
end)

uv.new_timer():start(1000, 1000, function()
  print("HELLO FROM LUV")
end)

uv.run()
