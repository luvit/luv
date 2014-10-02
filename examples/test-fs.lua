local p = require('lib/utils').prettyPrint
local uv = require('luv')

local function read_file_sync(path)
  local fd

  local function cleanup(...)
    assert(uv.fs_close(fd))
    return assert(...)
  end

  fd = assert(uv.fs_open(path, "r", tonumber("664", 8)))
  local stat, err = uv.fs_fstat(fd)
  if not stat then return cleanup(nil, err) end

  return cleanup(uv.fs_read(fd, stat.size, 0))
end

local function test_sync()
  print("Calling scandir")
  local req = assert(uv.fs_scandir("examples"))
  p("req", req)
  repeat
    local ent, err = uv.fs_scandir_next(req)
    assert(not err, err)
    if ent then
      ent.data = read_file_sync("examples/" .. ent.name)
    end
    p("ent", ent)
  until not ent

end

local function read_file(path, callback)
  local function onfd(err, fd)
    local function ondata(err, data)
      uv.fs_close(fd, function (err)
        callback(err, data)
      end)
    end

    if err then return callback(err) end
    uv.fs_fstat(fd, function (err, stat)
      if err then return ondata(err) end
      uv.fs_read(fd, stat.size, 0, ondata)
    end)
  end


  uv.fs_open(path, "r", tonumber(644), onfd)

end

local function test(callback)
  print("Calling scandir")
  uv.fs_scandir("examples", function (err, req)
    p("on scandir", {err=err,req=req})

    if err then return callback(err) end

    local onfile, onent

    function onfile(err, file)
      if err then return callback(err) end
      p{data=file}
      next()
    end

    function next()
      local ent, err = uv.fs_scandir_next(req)
      if err then return callback(err) end
      if not ent then return callback() end
      p(ent)
      read_file("examples/" .. ent.name, onfile)
    end

    next()

  end)

end

-- Test the sync I/O
test_sync()

-- Test the async I/O
test(p)


uv.run()
