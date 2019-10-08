-- Run this from the parent directory as
--
--     luajit tests/run.lua
--

local tap = require("lib/tap")
local uv = require("luv")

local req = uv.fs_scandir("tests")

while true do
  local name = uv.fs_scandir_next(req)
  if not name then break end
  local match = string.match(name, "^test%-(.*).lua$")
  if match then
    local path = "tests/test-" .. match
    tap(match)
    require(path)
  end
end

-- run the tests!
tap(true)
