local uv = require('luv')

coroutine.wrap(function()
   local thread = coroutine.running()
   local timer = uv.new_timer()
   local res = 'a'
   function timer:ontimeout()
      print('in timeout callback')
      res = 'b'
      coroutine.resume(thread)
   end
   uv.timer_start(timer, 1000, 0)
   print('yielding')
   coroutine.yield()
   print('result = ' .. a)
end)()

uv.run('default')
