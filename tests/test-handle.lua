return require('lib/tap')(function (test)

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
