local p = require('utils').prettyPrint

local uv = require('luv')

local ls = uv.spawn("ls")
p("child", ls)
function ls:onexit(code, signal)
  p{code=code,signal=signal}
end

repeat
  print("\ntick.")
until uv.run('once') == 0

print("done")

