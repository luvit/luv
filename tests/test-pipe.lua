local TEST_PIPENAME = "\\\\?\\pipe\\luv-test"

return require('lib/tap')(function (test)
  test("pipe chmod", function (print, p, expect, uv)
    local pipe = assert(uv.new_pipe())
    assert(pipe:bind(TEST_PIPENAME))
    local _, err, errname = pipe:chmod("r")
    if errname == "EPERM" then
      print("Insufficient privileges to alter pipe fmode, skipping")
      pipe:close()
      return
    end
    assert(not err, err)
    assert(pipe:chmod("w"))
    assert(pipe:chmod("rw"))
    assert(pipe:chmod("wr"))

    local ok = pcall(function() uv.pipe_chmod(pipe, "bad flags") end)
    assert(not ok)

    pipe:close()
  end, "1.16.0")
end)
