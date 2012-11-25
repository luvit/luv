local p = require('utils').prettyPrint

local uv = require('luv')

local function set_timeout(timeout, callback)
  local timer = uv.new_timer()
  function timer:ontimeout()
    p("ontimeout", self)
    uv.timer_stop(timer)
    uv.close(timer)
    callback(self)
  end
  function timer:onclose()
    p("ontimerclose", self)
  end
  uv.timer_start(timer, timeout, 0)
  return timer
end

local function clear_timeout(timer)
  uv.timer_stop(timer)
  uv.close(timer)
end

local function set_interval(interval, callback)
  local timer = uv.new_timer()
  function timer:ontimeout()
    p("interval", self)
    callback(self)
  end
  function timer:onclose()
    p("onintervalclose", self)
  end
  uv.timer_start(timer, interval, interval)
  return timer
end

local clear_interval = clear_timeout

local i = set_interval(300, function()
  print("interval...")
end)

set_timeout(1000, function()
  clear_interval(i)
end)


repeat
  print("\ntick.")
until uv.run_once() == 0

print("done")

