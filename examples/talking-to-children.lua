local p = require('lib/utils').prettyPrint
local uv = require('luv')

local stdout = uv.new_pipe(false)
local stderr = uv.new_pipe( false)
local stdin = uv.new_pipe(false)


local function onexit(self, code, signal)
  p("exit", {self=self, code=code,signal=signal})
end

local function onclose(self)
  p("close", {self=self})
end

local function onread(self, err, chunk)
  assert(not err, err)
  if (chunk) then
    p("data", {self=self,data=chunk})
  else
    p("end", {self=self})
  end
end

local function onclose(self)
  p("onclose", {self=self})
end

local function onshutdown(self)
  uv.close(self, onclose)
end

local handle, pid = uv.spawn("cat", {
  stdio = {stdin, stdout, stderr}
}, onexit)

p{
  handle=handle,
  pid=pid
}

uv.read_start(stdout, onread)
uv.read_start(stderr, onread)
uv.write(stdin, "Hello World")
uv.shutdown(stdin, onshutdown)

uv.run(loop)

-- Close everything
uv.walk(function (handle)
  uv.close(handle, onclose)
end)

uv.run(loop)
