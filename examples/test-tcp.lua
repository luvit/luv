local p = require('lib/utils').prettyPrint
local port = os.getenv("IP") or "0.0.0.0"
local uv = require('luv')

local function create_server(host, port, on_connection)
  local server = uv.new_tcp()
  p(1, server)
  uv.tcp_bind(server, host, port)
  function server:onconnection()
    local client = uv.new_tcp()
    uv.accept(server, client)
    on_connection(client)
  end
  uv.listen(server, 128)
  return server
end

  local server = create_server(port, 0, function (client)
    p("new client", client, uv.tcp_getsockname(client), uv.tcp_getpeername(client))
    function client:onread(err, chunk)
      p("onread", {err=err,chunk=chunk})
      if err then error(err) end
      if chunk then
        uv.write(uv.write_req(), client, chunk)
      else
        uv.shutdown(uv.shutdown_req(), client)
        uv.close(client)
      end
    end
    uv.read_start(client)
  end)
  local address = uv.tcp_getsockname(server)
  p("server", server, address)

  local client = uv.new_tcp()
  uv.tcp_connect(uv.connect_req(), client, "127.0.0.1", address.port)
  uv.read_start(client)
  p("writing from client")
  function client:onread(err, chunk)
    p("received at client", {err=err,chunk=chunk})
    if err then error(err) end
    if chunk then
      uv.shutdown(uv.shutdown_req(), client)
      p("client done shutting down")
    else
      uv.close(client)
      uv.close(server)
    end
  end
  uv.write(uv.write_req(), client, "Hello")
  uv.write(uv.write_req(), client, "World")
end)()

repeat
  print("\nTick..")
until uv.run('once') == 0

print("done")

uv.loop_close()
