local uv = require('luv')

local hare_id = uv.new_thread()
local tortoise_id = uv.new_thread()
local step = 10

uv.thread_create(hare_id,function(step,...) 
    local ffi = require'ffi'
    local uv = require('luv')
    local sleep 
    if ffi.os=='Windows' then
        ffi.cdef "void Sleep(int ms);"
        sleep = ffi.C.Sleep
    else 
        ffi.cdef "unsigned int usleep(unsigned int seconds);"
        sleep = ffi.C.usleep
    end
    while (step>0) do 
        step = step - 1
        uv.sleep(math.random(1000))
        print("Hare ran another step")
    end
    print("Hare done running!")
end, step,true,'abcd','false')

uv.thread_create(tortoise_id,function(step,...)
    local uv = require('luv')
    while (step>0) do 
        step = step - 1
        uv.sleep(math.random(100))
        print("Tortoise ran another step")
    end
    print("Tortoise done running!")
end,step,'abcd','false')

print(hare_id==hare_id,uv.thread_equal(hare_id,hare_id))
print(tortoise_id==hare_id,uv.thread_equal(tortoise_id,hare_id))

uv.thread_join(hare_id)
uv.thread_join(tortoise_id)
