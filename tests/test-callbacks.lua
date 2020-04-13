return require('lib/tap')(function (test)

  -- Notes:
  -- * When using a callable table as a callback, it will get itself as the first param when it is called.
  --   This matches what happens when calling the callable table normally.
  -- * expect wraps its argument in a function and returns the function, so expect should not be called on
  --   a callable table directly.

  test("luv_handle_t: function", function (print, p, expect, uv)
    local handle = uv.new_timer()

    local function onclose()
      p("closed", handle)
    end
    local function ontimeout()
      p("timeout", handle)
      uv.close(handle, expect(onclose))
    end

    uv.timer_start(handle, 10, 0, expect(ontimeout))
  end)

  test("luv_handle_t: callable table", function (print, p, expect, uv)
    local handle = uv.new_timer()

    local function onclose(self)
      p("closed", self, handle)
    end
    local onCloseTable = setmetatable({}, {__call=expect(onclose)})

    local function ontimeout(self)
      p("timeout", self, handle)
      uv.close(handle, onCloseTable)
    end
    local onTimeoutTable = setmetatable({}, {__call=expect(ontimeout)})

    uv.timer_start(handle, 10, 0, onTimeoutTable)
  end)

  test("luv_req_t: function", function (print, p, expect, uv)
    local fn = function(err, stat)
      assert(not err)
      assert(stat)
    end
    assert(uv.fs_stat('.', expect(fn)))
  end)

  test("luv_req_t: callable table", function (print, p, expect, uv)
    local callable
    local fn = function(self, err, stat)
      assert(self == callable)
      assert(not err, err)
      assert(stat)
    end
    callable = setmetatable({}, {__call=expect(fn)})
    assert(uv.fs_stat('.', callable))
  end)

end)