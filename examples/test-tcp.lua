local p = require('lib/utils').prettyPrint

local port = os.getenv("IP") or "0.0.0.0"

local uv = require('luv')

local function create_server(host, port, on_connection, on_listening)
  uv.tcp_bind(host, port, function(bound, address, server) 
      if bound then
          p(1, server)
          function server:onconnection()
              local client = uv.new_tcp()
              uv.accept(server,client)
              on_connection(client,server)
          end
          uv.listen(server)
          on_listening(server)
      else
          print("Couldn't bind?",server)
          for n,v in pairs(address) do
              print('',n,v)
          end
      end
  end, (function(address)
  for n,v in pairs(address) do
      print(n,v)
  end
  return true
  end))
end


create_server(port, 0, (function (client)
    p("new client", client, uv.tcp_getsockname(client), uv.tcp_getpeername(client))
    function client:ondata(chunk)
        p("ondata", chunk)
        uv.write(client, chunk, function ()
            p("written", chunk)
        end)
    end
    function client:onend()
        p("onend")
        uv.shutdown(client, function ()
            p("onshutdown")
            uv.close(client)
        end)
    end
    function client:onclose()
        p("client onclose")
    end
    uv.read_start(client)
end), 
(function(server)
    function server:onclose()
        p("server closed")
    end
    local address = uv.tcp_getsockname(server)
    p("server", server, address)

    local client = uv.new_tcp()
    uv.tcp_connect(client, address.address, address.port, function ()
        uv.read_start(client)
        p("writing from client")
        function client:ondata(chunk)
            p("received at client", chunk)
            uv.shutdown(client, function ()
                p("client done shutting down")
            end)
        end
        function client:onend()
            p("client received end")
            uv.close(client)
        end
        function client:onclose()
            p("client closed")
            uv.close(server)
        end
        uv.write(client, "Hello")
        uv.write(client, "World", function ()
            p("written from client")
        end)

    end)
end))

repeat
  print("\nTick..")
until uv.run('once') == 0

print("done")
