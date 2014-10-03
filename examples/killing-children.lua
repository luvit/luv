local p = require('lib/utils').prettyPrint
local uv = require('luv')


local function onexit(self, code, signal)
  p("EXIT", {code=code,signal=signal})
  uv.close(self)
end

local child, pid = uv.spawn("sleep", {
  args = {"100"}
}, onexit)

p{child=child, pid=pid}

-- uv.kill(pid, "SIGTERM")
uv.process_kill(child, "SIGTERM")

repeat
  print("\ntick.")
until uv.run(loop, 'once') == 0

print("done")

