--come from https://github.com/luvit/luv/issues/599

-- run `lua manual-test-exit.lua || echo $?`
-- it shoud print `5`

local uv = require('luv')

local function setTimeout(callback, ms)
  local timer = uv.new_timer()
  timer:start(ms, 0, function()
    timer:stop()
    timer:close()
    callback()
  end)
  return timer
end

setTimeout(function()
  os.exit(5, true)
end, 1000)

uv.run()
