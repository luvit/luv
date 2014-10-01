local uv = require('luv')
local dump = require('lib/utils').dump

local function protect(...)
  local text = table.concat({...}, "\t")
  text = "  " .. string.gsub(text, "\n", "\n  ")
  print(text)
end

local function pprotect(...)
  local n = select('#', ...)
  local arguments = { ... }

  for i = 1, n do
    arguments[i] = dump(arguments[i])
  end

  protect(table.concat(arguments, "\t"))
end

local function tap(suite)

  local passed = 0

  local tests = {}
  suite(function (name, fn)
    tests[#tests + 1] = {
      name = name,
      fn = fn
    }
  end)

  if #tests < 1 then
    error("No tests specified!")
  end

  print("1.." .. #tests)
  for i = 1, #tests do
    local test = tests[i]
    local pass, err = xpcall(function ()
      local expected = 0
      local function expect(fn, count)
        expected = expected + (count or 1)
        return function (...)
          expected = expected - 1
          local ret = fn(...)
          collectgarbage()
          return ret
        end
      end
      test.fn(protect, pprotect, expect, uv)
      collectgarbage()
      uv.run()
      collectgarbage()
      if expected > 0 then
        error("Missing " .. expected .. " expected call" .. (expected == 1 and "" or "s"))
      elseif expected < 0 then
        error("Found " .. -expected .. " unexpected call" .. (expected == -1 and "" or "s"))
      end
      collectgarbage()
      uv.walk(function (handle)
        error("Unclosed handle " .. tostring(handle))
      end)
      collectgarbage()
    end, debug.traceback)
    if pass then
      print("ok " .. i .. " " .. test.name)
      passed = passed + 1
    else
      protect(err)
      print("not ok " .. i .. " " .. test.name)
    end
  end

  return passed, #tests - passed, #tests

end


--[[
-- Sample Usage

local passed, failed, total = tap(function (test)

  test("add 1 to 2", function(print)
    print("Adding 1 to 2")
    assert(1 + 2 == 3)
  end)

  test("close handle", function (print, p, expect, uv)
    local handle = uv.new_timer()
    uv.close(handle, expect(function (self)
      assert(self == handle)
    end))
  end)

  test("simulate failure", function ()
    error("Oopsie!")
  end)

end)
]]

return tap
