local p = require('lib/utils').prettyPrint

local uv = require('luv')

local handle, pid = uv.spawn("sleep", {"100"}, {})

p{handle=handle, pid=pid}
function handle:onexit(code, signal)
  p("EXIT", {code=code,signal=signal})
  uv.close(handle)
end

uv.process_kill(handle, "SIGTERM")
--uv.kill(pid, "SIGHUP")

repeat
  print("\ntick.")
until uv.run('once') == 0

print("done")

