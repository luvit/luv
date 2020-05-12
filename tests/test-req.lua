return require('lib/tap')(function (test)

  test("cancel", function (print, p, expect, uv)
    -- might need to try a few times since cancel can fail with EBUSY
    for _=1,5 do
      local req = assert(uv.fs_stat('.', expect(function(err)
        if err then
          assert(err:match("^ECANCELED"))
        else
          assert(not err, err)
        end
      end)))

      local _, cancel_err = req:cancel()
      if cancel_err == nil then
        break
      end
    end
  end)

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
