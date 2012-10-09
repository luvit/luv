local utils = require('utils')
local p = utils.prettyPrint
local print = utils.print
utils.stdout = io.stdout

local serverPort = os.getenv("IP") or "0.0.0.0"

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

local server = createServer(serverPort, 0, function (client)
  p("new client", client, client:getsockname(), client:getpeername())
  function client:ondata(chunk)
    p("ondata", chunk)
    client:write(chunk, function ()
      p("written", chunk)
    end)
  end
  function client:onend()
    p("onend")
    client:shutdown(function ()
      p("onshutdown")
      client:close()
    end)
  end
  function client:onclose()
    p("client onclose")
  end
  client:readStart()
end)
function server:onclose()
  p("server closed")
end
local serverAddress = server:getsockname()
p("server", server, serverAddress)

local client = luv.newTcp()
client:connect(serverAddress.address, serverAddress.port, function ()
  client:readStart()
  p("writing from client")
  function client:ondata(chunk)
    p("received at client", chunk)
    client:shutdown(function ()
      p("client done shutting down")
    end)
  end
  function client:onend()
    p("client received end")
    client:close()
  end
  function client:onclose()
    p("client closed")
    server:close()
  end
  client:write("Hello")
  client:write("World", function ()
    p("written from client")
  end)

end)
repeat
  print("\nTick..")
until luv.runOnce() == 0

print("done")

