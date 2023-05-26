local uv = require('luv')

local test = uv.new_pipe(false)
uv.close(test)
