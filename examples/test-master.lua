local p = require('lib/utils').prettyPrint
local uv = require('luv')
local loop = assert(uv.new_loop())

local execpath = "/usr/local/bin/luajit" -- uv.execpath()
local cpu_count = 4 -- #uv.cpu_info()

local server = uv.new_tcp(loop)
uv.tcp_bind(server, "127.0.0.1", 1337)
print("Master process bound to TCP port 1337 on 127.0.0.1")

local function spawnChild()
  local pipe = uv.new_pipe(loop, true)
  local child, pid = uv.spawn(loop, execpath, {"examples/test-child.lua"},{
    stdio= {nil,1,2,pipe},
    env= {"PIPE_FD=3"}
  })
  p("Spawned child", pid, "and sending handle", server)
  uv.write2(uv.write_req(), pipe, "123", server)
  uv.shutdown(uv.shutdown_req(), pipe)
end

-- Spawn a child process for each CPU core
for i = 1, cpu_count do
  spawnChild()
end

uv.run(loop)

