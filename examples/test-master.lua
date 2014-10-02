local p = require('lib/utils').prettyPrint
local uv = require('luv')

local execpath = assert(uv.execpath())
local cpu_count = # assert(uv.cpu_info())

local server = uv.new_tcp()
assert(uv.tcp_bind(server, "::1", 1337))
print("Master process bound to TCP port 1337 on ::1")


local function onexit(self, status, signal)
  p("Child exited", {self=self,status=status,signal=signal})
end

local function spawnChild()
  local pipe = uv.new_pipe(true)
  local child, pid = assert(uv.spawn(execpath, {
    args = {"examples/test-child.lua"},
    stdio = {nil,1,2,pipe},
    env= {"PIPE_FD=3"}
  }, onexit))
  p("Spawned child", pid, "and sending handle", server)
  assert(uv.write2(pipe, "123", server))
  assert(uv.shutdown(pipe))
end

-- Spawn a child process for each CPU core
for i = 1, cpu_count do
  spawnChild()
end

uv.run()

