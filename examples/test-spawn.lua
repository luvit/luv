local p = require('lib/utils').prettyPrint

local uv = require('luv')

local loop = assert(uv.new_loop())

local handle, pid = uv.spawn(loop, "sleep", {
  args = {"100"}
})

p{handle=handle, pid=pid}
function handle:onexit(code, signal)
  p("EXIT", {code=code,signal=signal})
  uv.close(handle)
end

uv.kill(pid, "SIGTERM")

repeat
  print("\ntick.")
until uv.run(loop, 'once') == 0

print("done")

