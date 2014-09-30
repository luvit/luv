local p = require('lib/utils').prettyPrint
local uv = require('luv')
local loop = assert(uv.new_loop())

-- The parent is going to pass us the server handle over a pipe
-- This will be our local file descriptor at PIPE_FD
local pipe = uv.new_pipe(loop, true)
uv.pipe_open(pipe, tonumber(os.getenv("PIPE_FD")))


-- Configure the server handle
local server = uv.new_tcp(loop)
function server:onconnection()
  local client = uv.new_tcp(loop)
  uv.accept(server, client)
  p("New TCP", client, "on", server)
  p{client=client}
  uv.write(uv.write_req(), client, "BYE!\n");
  uv.shutdown(client, function ()
    uv.close(client)
  end)
end

-- Read the server handle from the parent
function pipe:onread(err, data)
  if err then error(err) end
  if uv.pipe_pending_count(self) then
    local pending_type = uv.pipe_pending_type(self)
    p("pending_type", pending_type)
    uv.accept(pipe, server)
    uv.listen(server)
    p("Received server handle from parent process", server)
  elseif data then
    p("ondata", data)
  else
    p("onend", data)
  end
  if pending then
  end
end
uv.read_start(pipe)

-- Start the event loop!
uv.run(loop)
