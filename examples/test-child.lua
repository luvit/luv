local p = require('lib/utils').prettyPrint
local uv = require('luv')

-- The parent is going to pass us the server handle over a pipe
-- This will be our local file descriptor at PIPE_FD
local pipe = uv.new_pipe(true)
local pipe_fd = tonumber(os.getenv("PIPE_FD"))
assert(uv.pipe_open(pipe, pipe_fd))

-- Configure the server handle
local server = uv.new_tcp()
local function onconnection()
  local client = uv.new_tcp()
  uv.accept(server, client)
  p("New TCP", client, "on", server)
  p{client=client}
  uv.write(client, "BYE!\n");
  uv.shutdown(client, function ()
    uv.close(client)
    uv.close(server)
  end)
end

-- Read the server handle from the parent
local function onread(self, err, data)
  p("onread", {self=self,err=err,data=data})
  assert(not err, err)
  if uv.pipe_pending_count(self) > 0 then
    local pending_type = uv.pipe_pending_type(self)
    p("pending_type", pending_type)
    assert(pending_type == "tcp")
    assert(uv.accept(pipe, server))
    assert(uv.listen(server, 128, onconnection))
    p("Received server handle from parent process", server)
  elseif data then
    p("ondata", data)
  else
    p("onend", data)
  end
  if pending then
  end
end
uv.read_start(pipe, onread)

-- Start the event loop!
uv.run()
