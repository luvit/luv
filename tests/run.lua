-- Run this from the parent directory as
--
--     luajit tests/run.lua
--
-- or if you want to run a single test, pass it in
--
--     luajit tests/run.lua fs

local tap = require("lib/tap")
local p = require('lib/utils').prettyPrint
local uv = require("luv")

tap(false) -- Disable autorun

local req = uv.fs_scandir("tests")

repeat
  local ent = uv.fs_scandir_next(req)
  p("ent", ent)
  if not ent then
    -- run the tests!
    tap(true)
  end
  local match = string.match(ent.name, "^(test%-.*).lua$")
  if ent.type == "file" and match then
    local path = "tests/" .. match
    require("tests/" .. match)
  end
until not ent


