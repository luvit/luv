return require('lib/tap')(function (test) 
  test("test threadpool", function(print,p,expect,_uv)
    local ctx = _uv.new_work(
        function(n) --work,in threadpool
            local uv = require('luv')
            local t = uv.thread_self()
            uv.sleep(100)
            return n,n*n
        end,
        function(n,r) assert(n*n==r) end    --after work, in loop thread
    )
    _uv.queue_work(ctx,2)
    _uv.queue_work(ctx,4)
    _uv.queue_work(ctx,6)
    _uv.queue_work(ctx,-2)
    _uv.queue_work(ctx,-11)
  end)

end)
