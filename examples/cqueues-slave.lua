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
      -- libuv takes milliseconds as an integer,
      -- while cqueues gives timeouts as a floating point number
      -- use `math.ceil` as we'd rather wake up late than early
      timer:set_repeat(math.ceil(timeout * 1000))
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
