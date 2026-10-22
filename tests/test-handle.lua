return require('lib/tap')(function (test)

  -- Exercise allocation cleanup under the suite's ASan/LSan and Valgrind jobs.
  test("poll constructor initialization failure", function (print, p, expect, uv)
    local handle, err, code = uv.new_poll(-1)
    assert(handle == nil)
    assert(type(err) == "string" and type(code) == "string")
  end, "1.3.0")

  test("socket poll constructor initialization failure", function (print, p, expect, uv)
    local handle, err, code = uv.new_socket_poll(-1)
    assert(handle == nil)
    assert(type(err) == "string" and type(code) == "string")
  end, "1.3.0")

  test("tty constructor initialization failure", function (print, p, expect, uv)
    local handle, err, code = uv.new_tty(-1, false)
    assert(handle == nil)
    assert(type(err) == "string" and type(code) == "string")
  end, "1.5.0")

  test("get type", function (print, p, expect, uv)
    local pipe = assert(uv.new_pipe())

    local typename, typeid = pipe:get_type()
    assert(typename == "pipe")

    local typename_, typeid_ = uv.handle_get_type(pipe)
    assert(typename == typename_)
    assert(typeid == typeid_)

    pipe:close()
  end, "1.19.0")

end)
