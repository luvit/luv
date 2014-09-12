local uv = require('luv')

local loop = uv.new_loop()
print("loop", loop)
print("alive?", uv.loop_alive(loop))

local timer = uv.new_timer(loop)
print("timer", timer)

uv.walk(loop, function (handle)
  print("Walk handle", handle)
  print ("handle == timer", handle == timer)
end)
uv.run(loop)
uv.loop_close(loop)
