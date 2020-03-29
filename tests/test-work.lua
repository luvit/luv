return require('lib/tap')(function (test)
  test("test threadpool", function(print,p,expect,_uv)
    p('Please be patient, the test cost a lots of time')
    local count = 1000 --for memleaks dected
    local step = 0
    local ctx
    ctx = _uv.new_work(
        function(n,s) --work,in threadpool
            local uv = require('luv')
            local t = uv.thread_self()
            uv.sleep(10)
            return n, n*n, t, s
        end,
        function(n,r,id, s)
            assert(n*n==r)
            assert(#s==4096)
            if step < count then
                _uv.queue_work(ctx,n,s)
                step = step + 1
                if (step % 100==0) then
                    p(string.format('run %d%%', math.floor(step*100/count)))
                end
            else
              ctx = nil
            end
        end    --after work, in loop thread
    )
    local ls = string.rep('-',4096)

    _uv.queue_work(ctx,2,ls)
    _uv.queue_work(ctx,4,ls)
    _uv.queue_work(ctx,6,ls)
    _uv.queue_work(ctx,-2,ls)
    _uv.queue_work(ctx,-11,ls)
    _uv.queue_work(ctx,2,ls)
    _uv.queue_work(ctx,4,ls)
    _uv.queue_work(ctx,6,ls)
    _uv.queue_work(ctx,-2,ls)
    _uv.queue_work(ctx,-11,ls)
    _uv.queue_work(ctx,2,ls)
    _uv.queue_work(ctx,4,ls)
    _uv.queue_work(ctx,6,ls)
    _uv.queue_work(ctx,-2,ls)
    _uv.queue_work(ctx,-11,ls)
    _uv.queue_work(ctx,2,ls)
    _uv.queue_work(ctx,4,ls)
    _uv.queue_work(ctx,6,ls)
    _uv.queue_work(ctx,-2,ls)
    _uv.queue_work(ctx,-11,ls)
  end)

  test("test threadpool with async", function(print,p,expect,_uv)
    local ctx, async
    async = _uv.new_async(expect(function (a,b,c)
      p('in async notify callback')
      p(a,b,c)
      assert(a=='a')
      assert(b==true)
      assert(c==250)
    end))

    ctx = _uv.new_work(
      function(n, s, a)         --work,in threadpool
          local uv = require('luv')
          local t = tostring(uv.thread_self())
          if a then
            assert(uv.async_send(a,'a',true,250)==0)
          end
          uv.sleep(10)
          return n, n*n, t, s
      end,
      function(n,r,id, s)       --after work, in loop thread
          p(n, r, id, s)
          assert(n*n==r)
          if async then
            _uv.close(async)
          end
          print(id, 'finish', s)
      end
    )
    _uv.queue_work(ctx,2,'hello',async)
  end)
end)
