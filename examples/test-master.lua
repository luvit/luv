local p = require('lib/utils').prettyPrint
local uv = require('luv')

local server = uv.new_tcp()
uv.tcp_bind(server, "127.0.0.1", 1337)
print("Master process bound to TCP port 1337 on 127.0.0.1")

local function spawnChild()
  local pipe = uv.new_pipe(true)
  local child, pid = uv.spawn(uv.execpath(), {"examples/test-child.lua"},{
    stdio= {nil,1,2,pipe},
    env= {"PIPE_FD=3"}
  })
  p("Spawned child", pid, "and sending handle", server)
  uv.write2(pipe, "123", server)
  uv.shutdown(pipe)
end

-- Spawn a child process for each CPU core
for i = 1, #uv.cpu_info() do
  spawnChild()
end

uv.run("default")

