local uv = require('luv')
local loop = uv.new_loop()
coroutine.wrap(function ()

  print("loop", loop)
  print("alive?", uv.loop_alive(loop))

  local timer = uv.new_timer(loop)
  print("timer", timer, timer.type)
  timer.a = 1
  timer.b = true
  timer.c = "Hwllo"

  for k,v in pairs(timer) do
    print(k, v)
  end

  uv.walk(loop, function (handle)
    print("Walk handle", handle)
    print ("handle == timer", handle == timer, handle.type)
  end)
  print("Closing")
  uv.close(timer)
  print("Closed")
end)()
print("blocking")
uv.run(loop)
uv.loop_close(loop)
print("done")
