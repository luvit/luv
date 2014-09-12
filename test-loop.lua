local uv = require('luv')

local loop = uv.new_loop()
print("loop", loop)
print("alive?", uv.loop_alive(loop))
uv.walk(loop, function () end)
uv.run(loop)
uv.loop_close(loop)
