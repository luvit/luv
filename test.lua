local utils = require('utils')
local p = utils.prettyPrint
local print = utils.print
utils.stdout = io.stdout

local luv = require('luv')

p("luv", require('luv'))

local server = luv.newTcp()

p("server", server)
local tcp_m = getmetatable(server).__index
p("tcp_m", tcp_m)
local stream_m = getmetatable(tcp_m).__index
p("stream_m", stream_m)
local handle_m = getmetatable(stream_m).__index
p("handle_m", handle_m)


local function createServer(host, port, onConnection)
  local server = luv.newTcp()
  server:bind(host, port)
  server:listen(function ()
    local client = luv.newTcp()
    server:accept(client)
    onConnection(client)
  end)
  return server
end

createServer("0.0.0.0", 8080, function (socket)
  p("new client", socket)
  socket:readStart()
end)

repeat
  print(".")
until luv.runOnce() == 0

print("done")

