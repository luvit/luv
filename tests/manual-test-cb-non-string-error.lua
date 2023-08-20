return require('lib/tap')(function (test)
  test("timer callback with errors", function(print, p, expect, uv)
    local Error = {}
    Error.__index = Error

    function Error.new(msg)
        local o = setmetatable({}, Error)
        o.msg = assert(msg)
        return o
    end

    function Error:__tostring()
        return assert(tostring(self.msg))
    end

    local timer = uv.new_timer()
    timer:start(10, 0, function()
      timer:stop()
      timer:close()
      local e = Error.new('Error in timeout callback')
      error(e)
    end)
  end)
end)
