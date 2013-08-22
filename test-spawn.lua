local p = require('utils').prettyPrint

local uv = require('luv')

--local ls = uv.spawn("touch", {"me"}, {cwd="/home/tim"})
local ls = uv.spawn("touch", {"me"})
p("child", ls)
function ls:onexit(code, signal)
  print("EXIT")
  p{code=code,signal=signal}
end

repeat
  print("\ntick.")
until uv.run('once') == 0

print("done")

