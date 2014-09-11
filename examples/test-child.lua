local p = require('lib/utils').prettyPrint
local uv = require('luv')

-- The parent is going to pass us the server handle over a pipe
-- This will be our local file descriptor at PIPE_FD
local pipe = uv.new_pipe(true)
uv.pipe_open(pipe, tonumber(os.getenv("PIPE_FD")))


-- Configure the server handle
local server = uv.new_tcp()
function server:onconnection()
  local client = uv.new_tcp()
  uv.accept(server, client)
  p("New TCP", client, "on", server)
  p{client=client}
  uv.write(client, "BYE!\n");
  uv.shutdown(client, function ()
    uv.close(client)
  end)
end

-- Read the server handle from the parent
function pipe:ondata(data, pending)
  if pending then
    uv.accept(pipe, server)
    uv.listen(server)
    p("Received server handle from parent process", server)
  end
end
uv.read2_start(pipe)

-- Start the event loop!
uv.run("default")
