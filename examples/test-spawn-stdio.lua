local p = require('lib/utils').prettyPrint
local uv = require('luv')
local loop = assert(uv.new_loop())

local stdout = uv.new_pipe(loop, false)
local stderr = uv.new_pipe(loop, false)
local stdin = uv.new_pipe(loop, false)

local handle, pid = uv.spawn(loop, "cat", {}, {
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

function stdout:onread(err, chunk)
  if err then error(err) end
  if (chunk) then
    p("stdout data", chunk)
  else
    p("stdout end")
  end
end

function stderr:onread(err, chunk)
  if err then error(err) end
  if (chunk) then
    p("stderr data", chunk)
  else
    p("stderr end")
  end
end

uv.read_start(stdout)
uv.read_start(stderr)
uv.write(uv.write_req(), stdin, "Hello World")
uv.shutdown(uv.shutdown_req(), stdin, function ()
  uv.close(stdin)
end)

uv.run(loop)
