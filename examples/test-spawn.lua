local p = require('lib/utils').prettyPrint

local uv = require('luv')

local handle, stdin, stdout, stderr, pid = uv.spawn("sleep", {"100"})


--local handle, stdin, stdout, stderr, pid = uv.spawn("curl", {"-i", "http://luvit.io"})
--local handle, stdin, stdout, stderr, pid = uv.spawn("ls", {"-l"}, {cwd="/home/tim"})
--local handle, stdin, stdout, stderr, pid = uv.spawn("touch", {"me"})

p(handle, {stdin=stdin,stdout=stdout,stderr=stderr, pid=pid})
function handle:onexit(code, signal)
  p("EXIT", {code=code,signal=signal})
  uv.close(handle)
  uv.close(stdin)
end
function stdout:ondata(chunk)
  p("STDOUT.DATA", chunk)
end
function stderr:ondata(chunk)
  p("STDERR.DATA", chunk)
end
function stdout:onend()
  p("STDOUT.END")
  uv.close(stdout)
end
function stderr:onend()
  p("STDERR.END")
  uv.close(stderr)
end
uv.read_start(stderr)
uv.read_start(stdout)

uv.process_kill(handle, "SIGTERM")
--uv.kill(pid, "SIGHUP")

repeat
  print("\ntick.")
until uv.run('once') == 0

print("done")

