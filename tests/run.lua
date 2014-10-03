-- Run this from the parent directory as
--
--     luajit tests/run.lua
--

local tap = require("lib/tap")
local uv = require("luv")

local req = uv.fs_scandir("tests")

repeat
  local ent = uv.fs_scandir_next(req)
  if not ent then
    -- run the tests!
    tap(true)
  end
  local match = string.match(ent.name, "^test%-(.*).lua$")
  if ent.type == "file" and match then
    local path = "tests/test-" .. match
    tap(match)
    require(path)
  end
until not ent


