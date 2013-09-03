uv = require('luv')
local utils = require('lib/utils')
local stdin
if uv.guess_handle(0) ~= "TTY" or
   uv.guess_handle(1) ~= "TTY" then
  error "stdio must be a tty"
end
local stdin = uv.new_tty(0, true)
local stdout = uv.new_tty(1)

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
  uv.write(stdout, prompt .. ' ')
end

displayPrompt '>'

function stdin:ondata(line)
  local prompt = evaluateLine(line)
  displayPrompt(prompt)
end

function stdin:onend()
  uv.close(stdin)
end

uv.read_start(stdin)

uv.run("default")

print("")
