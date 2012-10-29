local utils = require('utils')
local p = utils.prettyPrint
local print = utils.print
utils.stdout = io.stdout
local luv = require('luv')

local function createServer(host, port, onConnection)
  local server = luv.newTcp()
  server:bind(host, port)
  function server:onconnection()
    local client = luv.newTcp()
    server:accept(client)
    onConnection(client)
  end
  server:listen()
  return server
end

local server = createServer("127.0.0.1", 8080, function (client)
  local function onend()
    chunk = "HTTP/1.0 200 OK\r\nConnection: Close\r\nContent-Type: text/html\r\nContent-Length: 14\r\n\r\nHello World\n\r\n"
--    client:write(chunk, function ()
--    end)
    client:write(chunk)
    client:shutdown(function ()
      client:close()
    end)
  end
  function client:ondata(chunk)
    onend()
  end
--  function client:onclose()
--  end
  client:readStart()
end)

repeat
until luv.runOnce() == 0
