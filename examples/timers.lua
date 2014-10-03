local p = require('lib/utils').prettyPrint
local uv = require('luv')

local function set_timeout(timeout, callback)
  local timer = uv.new_timer()
  local function ontimeout(self)
    p("ontimeout", self)
    uv.timer_stop(self)
    uv.close(self)
    callback(self)
  end
  uv.timer_start(timer, timeout, 0, ontimeout)
  return timer
end

local function clear_timeout(timer)
  uv.timer_stop(timer)
  uv.close(timer)
end

local function set_interval(interval, callback)
  local timer = uv.new_timer()
  local function ontimeout(self)
    p("interval", self)
    callback(self)
  end
  uv.timer_start(timer, interval, interval, ontimeout)
  return timer
end

local clear_interval = clear_timeout

local i = set_interval(300, function()
  print("interval...")
end)

set_timeout(1000, function()
  clear_interval(i)
end)


local handle = uv.new_timer()
local delay = 1024
local function ontimeout()
  p("tick", delay)
  delay = delay / 2
  if delay >= 1 then
    uv.timer_set_repeat(handle, delay)
    uv.timer_again(handle)
  else
    uv.timer_stop(handle)
    uv.close(handle)
    p("done")
  end
end
uv.timer_start(handle, delay, 0, ontimeout)


repeat
  print("\ntick.")
until uv.run('once') == 0

print("done")

uv.loop_close()

