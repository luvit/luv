-- Determine some platform information used for tests
-- Adapted from: https://github.com/keplerproject/luarocks/blob/master/src/luarocks/cfg.lua
local platform = {}
local system = io.popen("uname -s"):read("*l")
local proc = io.popen("uname -m"):read("*l")

if proc:match("i[%d]86") then
  platform.proc = "x86"
elseif proc:match("amd64") or proc:match("x86_64") then
  platform.proc = "x86_64"
else
  platform.proc = nil
end

if system == "Darwin" then
  platform.unix = true
  platform.macosx = true
elseif system == "Linux" then
  platform.unix = true
  platform.linux = true
elseif system and system:match("^CYGWIN") then
  platform.unix = true
  platform.cygwin = true
elseif system and system:match("^Windows") then
  platform.windows = true
elseif system and system:match("^MINGW") then
  platform.windows = true
  platform.mingw = true
else
  -- Assume unix is all else fails
  platform.unix = true
end

return platform
