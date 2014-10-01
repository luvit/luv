local uv = require('luv')
local p = require('lib/utils').prettyPrint

local timer = uv.new_timer()
p("timer", timer)
local handles = uv.walk()
p("handles", handles)


local r = 1024

local function on_timeout()
  print("on_timeout", timer)
  local current = uv.timer_get_repeat(timer)
  if current == 1 then
    uv.timer_stop(timer)
  end
  uv.timer_set_repeat(timer, r)
  r = r / 2
end

uv.timer_start(timer, on_timeout, 200, 0)

print("Starting blocking loop")
uv.run()
print("Event loop emptied")

for i, handle in ipairs(uv.walk()) do
  print("Closing...", handle)
  uv.close(handle, function ()
    print("Closed", handle)
  end)
end

print("Run the loop again to wait for closes")
uv.run()
print("Done")
