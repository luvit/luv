local p = require('lib/utils').prettyPrint
local uv = require('luv')
local loop = assert(uv.new_loop())


local function test()
  print("Calling scandir")
  local req = assert(uv.fs_scandir(loop, uv.fs_req(), "examples"))
  p("req", req)
  repeat
    local ent = assert(uv.fs_scandir_next(req))
    p("ent", ent)
  until not ent

end
-- local fd = assert(uv.fs_open(loop, uv.fs_req(), "examples/test-fs.lua", "r", tonumber("644", 8)))
-- p("fd", fd)
-- local chunk = assert(uv.fs_read(loop, uv.fs_req(), fd, 0x1000, 0))
-- p("chunk", chunk)

coroutine.wrap(test)()
