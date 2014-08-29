local p = require('lib/utils').prettyPrint
local uv = require('luv')

local stdout = uv.new_pipe()
local stderr = uv.new_pipe()
local stdin = uv.new_pipe()

local handle, pid = uv.spawn("cat", {}, {
  stdio = {stdin, stdout, stderr}
})

p{
  handle=handle,
  pid=pid
}

function handle:onexit(code, signal)
  p("exit", {code=code,signal=signal})
end

function handle:onclose()
  p("handle close")
end

function stdout:ondata(chunk)
  p("stdout data", chunk)
end

function stdout:onend()
 p("stdout end")
end

function stderr:ondata(chunk)
  p("stderr data", chunk)
end

function stderr:onend()
  p("stderr end")
end


uv.read_start(stdout)
uv.read_start(stderr)
uv.write(stdin, "Hello World")
uv.shutdown(stdin, function ()
  uv.close(stdin)
end)

uv.run("default")
