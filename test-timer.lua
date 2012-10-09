local utils = require('utils')
local p = utils.prettyPrint
utils.stdout = io.stdout

local luv = require('luv')

p("luv", require('luv'))

local timer = luv.newTimer()

p("timer", timer)
local timer_m = getmetatable(timer).__index
p("timer_m", timer_m)
local handle_m = getmetatable(timer_m).__index
p("handle_m", handle_m)

timer:close()
repeat
  print("tick.")
until luv.runOnce() == 0

print("done")

