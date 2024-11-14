-- This is a standalone test because it specifically requires luv to
-- be initially require()d from a thread/coroutine.
-- Test for issue #503 (PR #734).

local thread = coroutine.create(function ()
	local uv = require "luv"
	coroutine.yield();
end);

-- Resume (start) thread, which will load luv
-- and then it will yield
coroutine.resume(thread);

-- thread where luv was initially loaded is now suspended

return require('lib/tap')(function (test)

  if _VERSION == "Lua 5.1" then
    -- Lua 5.1 and LuaJIT do not provide an API to determine the main
    -- thread. Therefore it is inherently unsafe to require("luv") in
    -- a coroutine.
    test("callback should be in main thread", function(print,p,expect,uv)
      print("Skipping! This test is expected to fail on Lua 5.1 and LuaJIT.");
    end);
  else
    test("callback should be in main thread", function(print,p,expect,uv)
        -- Now, in the main thread, load luv and create a timer
        local t = uv.new_timer();
        t:start(200, 0, expect(function ()
          -- If luv calls this callback in the non-main (suspended) thread,
          -- it violates a requirement specified in the Lua manual to only
          -- call functions on active threads.

          local our_thread, is_main = coroutine.running();

          -- Basic assertion that we are not running in the suspended thread
          assert(our_thread ~= thread)

          -- How coroutine.running() reports "main thread" varies between
          -- different Lua versions. This should handle them all.
          assert(our_thread == nil or is_main == true)

          -- If we were called in the wrong thread, this may segfault:
          -- coroutine.resume(thread)
          t:close();
        end));
        uv.run("default");
    end);
  end
end);

