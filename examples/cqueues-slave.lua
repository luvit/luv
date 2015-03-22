--[[
Demonstrates using cqueues with a luv mainloop
]]

local cqueues = require "cqueues"
local uv = require "luv"

local cq = cqueues.new()

do
	local timer = uv.new_timer()
	local function reset_timer()
		local timeout = cq:timeout()
		if timeout then
			uv.timer_set_repeat(timer, timeout * 1000)
			uv.timer_again(timer)
		end
	end	
	local function onready()
		assert(cq:step(0))
		reset_timer()
	end
	uv.timer_start(timer, 0, 0, onready)
	uv.poll_start(uv.new_poll(cq:pollfd()), cq:events(), onready)
end

cq:wrap(function()
	while true do
		cqueues.sleep(1)
		print("HELLO FROM CQUEUES")
	end
end)

uv.timer_start(uv.new_timer(), 1000, 1000, function()
	print("HELLO FROM LUV")
end)

uv.run()
