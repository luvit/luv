return require('lib/tap')(function (test)

  test("get type", function (print, p, expect, uv)
    local req = uv.fs_stat('.', expect(function(err)
      assert(not err, err)
    end))

    local typename, typeid = req:get_type()
    assert(typename == "fs")

    local typename_, typeid_ = uv.req_get_type(req)
    assert(typename == typename_)
    assert(typeid == typeid_)
  end, "1.19.0")

end)
