local utils = require('utils')
local p = utils.prettyPrint
local print = utils.print
utils.stdout = io.stdout

local uv = require('direct')

p("uv", uv)

local loop = uv.default_loop()
p("loop", loop)
print("Starting event loop")
p("now", uv.now(loop))
uv.run(loop)
print("done...")