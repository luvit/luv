local utils = require('utils')
local p = utils.prettyPrint
utils.stdout = io.stdout

local luv = require('luv')

p("luv", require('luv'))

local function setTimeout(timeout, callback)
  local timer = luv.newTimer()
  function timer:ontimeout()
    p("ontimeout", self)
    timer:stop()
    timer:close()
    callback(self)
  end
  function timer:onclose()
    p("ontimerclose", self)
  end
  timer:start(timeout, 0)
  return timer
end

local function clearTimeout(timer)
  timer:stop()
  timer:close()
end

local function setInterval(interval, callback)
  local timer = luv.newTimer()
  function timer:ontimeout()
    p("interval", self)
    callback(self)
  end
  function timer:onclose()
    p("onintervalclose", self)
  end
  timer:start(interval, interval)
  return timer
end

local clearInterval = clearTimeout

local i = setInterval(300, function()
  print("interval...")
end)

setTimeout(1000, function()
  clearInterval(i)
end)


repeat
  print("\ntick.")
until luv.runOnce() == 0

print("done")

