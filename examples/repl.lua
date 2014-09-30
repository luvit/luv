uv = require('luv')
local utils = require('lib/utils')
local loop = assert(uv.new_loop())

local stdin
-- if uv.guess_handle(0) ~= "TTY" or
--    uv.guess_handle(1) ~= "TTY" then
--   error "stdio must be a tty"
-- end
local stdin = uv.new_tty(loop, 0, true)
local stdout = uv.new_tty(loop, 1, false)

local debug = require('debug')
local c = utils.color

local function gatherResults(success, ...)
  local n = select('#', ...)
  return success, { n = n, ... }
end

local function printResults(results)
  for i = 1, results.n do
    results[i] = utils.dump(results[i])
  end
  print(table.concat(results, '\t'))
end

local buffer = ''

local function evaluateLine(line)
  if line == "<3\n" then
    print("I " .. c("Bred") .. "♥" .. c() .. " you too!")
    return '>'
  end
  local chunk  = buffer .. line
  local f, err = loadstring('return ' .. chunk, 'REPL') -- first we prefix return

  if not f then
    f, err = loadstring(chunk, 'REPL') -- try again without return
  end

  if f then
    buffer = ''
    local success, results = gatherResults(xpcall(f, debug.traceback))

    if success then
      -- successful call
      if results.n > 0 then
        printResults(results)
      end
    else
      -- error
      print(results[1])
    end
  else

    if err:match "'<eof>'$" then
      -- Lua expects some more input; stow it away for next time
      buffer = chunk .. '\n'
      return '>>'
    else
      print(err)
      buffer = ''
    end
  end

  return '>'
end

local function displayPrompt(prompt)
  uv.write(uv.write_req(), stdout, prompt .. ' ')
end

function stdin:onread(err, line)
  if err then error(err) end
  if line then
    local prompt = evaluateLine(line)
    displayPrompt(prompt)
  else
    uv.close(stdin)
  end
end

coroutine.wrap(function()
  displayPrompt '>'
  uv.read_start(stdin)
end)()

uv.run(loop)

print("")
