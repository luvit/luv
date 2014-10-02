local p = require('lib/utils').prettyPrint
local uv = require('luv')

local function read_file_sync(path)
  local fd

  local function cleanup(...)
    assert(uv.fs_close(fd))
    return assert(...)
  end

  local fd = assert(uv.fs_open(path, "r", tonumber("664", 8)))
  local stat, err = uv.fs_fstat(fd)
  if not stat then return cleanup(nil, err) end

  return cleanup(uv.fs_read(fd, stat.size, 0))
end

local function test()
  print("Calling scandir")
  local req = assert(uv.fs_scandir("examples"))
  p("req", req)
  repeat
    local ent, err = uv.fs_scandir_next(req)
    assert(not err, err)
    p("ent", ent)
  until not ent

end

test()

uv.run()
