--- This module is responsible for generating documentation and metadata for
--- the luv project. It processes documentation sections from docs.lua,
--- and writes the output to markdown (docs.md) and Lua files (meta.lua).

local docs = require('docs')
local doctop = docs[1]
local types = docs[2]

--- @param str string
--- @param indent? integer
--- @return string
local function dedent(str, indent)
  local prefix = 99

  str = str:gsub('^\n', ''):gsub('%s+$', '')

  for line in str:gmatch('[^\n]+') do
    local s, e = line:find('^%s*')
    local amount = (e - s) + 1
    if amount < prefix then
      prefix = amount
    end
  end

  local result = {} --- @type string[]
  for line in str:gmatch('([^\n]*)\n?') do
    result[#result + 1] = line:sub(prefix + 1)
  end

  local ret = table.concat(result, '\n')
  ret = ret:gsub('\n+$', '')

  if indent then
    local ind = string.rep(' ', indent)
    ret = ind .. ret:gsub('\n', '\n' .. ind)
    ret = ret:gsub('\n%s+\n', '\n\n')
  end

  return ret
end

--- @param ty Doc.Type
--- @return Doc.Type
local function remove_nil(ty)
  if not (type(ty) == 'table' and ty.kind == 'union') then
    return ty
  end
  local r = { kind = 'union', {} } --- @type Doc.Type.Union
  for _, uty in ipairs(ty[1]) do
    if uty ~= 'nil' then
      table.insert(r[1], uty)
    end
  end
  if #r[1] == 1 then
    return r[1][1]
  end
  return r
end

local Doc = {}

do -- doc
  --- @param lvl integer
  --- @param str string
  --- @return string
  local function heading(lvl, str)
    return string.rep('#', lvl) .. ' ' .. str:gsub(' %- ', ' — ')
  end

  --- @param ty Doc.Type
  --- @return boolean
  local function isoptional(ty)
    if type(ty) == 'string' then
      return ty == 'nil'
    end
    if ty.kind == 'union' then
      for _, uty in ipairs(ty[1]) do
        if isoptional(uty) then
          return true
        end
      end
    end
    return false
  end

  local function pad(lvl)
    return string.rep(' ', lvl * 2)
  end

  --- @param ty Doc.Type.Fun
  --- @param lvl? integer
  --- @param desc? string
  --- @return string
  local function tyfun(ty, lvl, desc)
    local r = {} --- @type string[]
    r[#r + 1] = '`callable`'
    if desc then
      r[#r] = r[#r] .. ' ' .. desc
    end
    for _, arg in ipairs(ty.args) do
      local arg_nm, arg_ty, arg_desc = arg[1], arg[2], arg[3]
      if arg_nm == 'err' then
        arg_ty = 'nil|string'
      end
      r[#r + 1] = ('%s- `%s`: %s'):format(pad(lvl), arg_nm, Doc.ty(arg_ty, lvl + 1))
      if arg_desc then
        r[#r] = r[#r] .. ' ' .. arg_desc
      end
    end

    return table.concat(r, '\n')
  end

  --- @param ty Doc.Type.Table
  --- @param lvl? integer
  --- @param desc? string
  --- @return string
  local function tytbl(ty, lvl, desc)
    local r = {} --- @type string[]
    r[#r + 1] = '`table`'
    if desc then
      r[#r] = r[#r] .. ' ' .. desc
    end
    for _, field in ipairs(ty.fields) do
      local name, aty, default, adesc = field[1], field[2], field[3], field[4]
      r[#r + 1] = ('%s- `%s`: %s'):format(pad(lvl), name, Doc.ty(aty, lvl + 1, adesc))
      if default then
        r[#r] = ('%s (default: `%s`)'):format(r[#r], default)
      end
    end

    return table.concat(r, '\n')
  end

  --- @param ty string
  --- @param lvl? integer
  --- @param desc? string
  --- @return string
  local function tystr(ty, lvl, desc)
    do -- TODO(lewis6991): remove
      if ty == 'uv_handle_t' or ty == 'uv_req_t' or ty == 'uv_stream_t' then
        return '`userdata` for sub-type of `' .. ty .. '`'
      end
      ty = ty:gsub('uv_[a-z_]+', '%0 userdata')
      ty = ty:gsub('%|', '` or `')
    end

    if desc then
      if desc:match('\n') then
        desc = '\n' .. dedent(desc, lvl * 2)
      else
        desc = ' ' .. desc
      end
    end
    return '`' .. ty .. '`' .. (desc or '')
  end

  --- @param ty Doc.Type.Dict
  --- @param lvl? integer
  --- @param desc? string
  --- @return string
  local function tydict(ty, lvl, desc)
    local r = {} --- @type string[]
    r[#r + 1] = '`table`'
    if desc then
      r[#r] = r[#r] .. ' ' .. desc
    end
    -- TODO(lewis6991): remove
    if ty.key == 'integer' then
      ty = {
        kind = 'dict',
        key = '1, 2, 3, ..., n',
        value = ty.value,
      }
    end
    r[#r + 1] = ('%s- `[%s]`: %s'):format(pad(lvl), ty.key, Doc.ty(ty.value, lvl + 1))
    return table.concat(r, '\n')
  end

  --- @param ty Doc.Type.Union
  --- @param lvl? integer
  --- @param desc? string
  --- @return string
  local function tyunion(ty, lvl, desc)
    local tys = ty[1]

    local r = {} --- @type string[]

    local main_ty --- @type integer?
    for i, uty in ipairs(tys) do
      if Doc.ty(uty):match('\n') then
        main_ty = i
      end
      r[#r + 1] = Doc.ty(uty, lvl, i == #tys and desc or nil)
    end

    if main_ty and #tys > 1 then
      local others = {} --- @type string[]
      for i, oty in ipairs(r) do
        if i ~= main_ty then
          others[#others + 1] = oty
        end
      end
      local other_ty_str = table.concat(others, ' or ')
      -- local other_ty_str = table.concat(others, '|')

      return (r[main_ty]:gsub('\n', ' or ' .. other_ty_str .. '\n', 1))
    end

    return table.concat(r, ' or ')
    -- return table.concat(r, '|')
  end

  local types_noinline = {
    threadargs = true,
    buffer = true,
  }

  local ty_dispatch = {
    ['function'] = tyfun,
    ['table'] = tytbl,
    ['dict'] = tydict,
    ['union'] = tyunion,
  }

  --- @param ty Doc.Type
  --- @param lvl? integer
  --- @param desc? string
  --- @return string
  function Doc.ty(ty, lvl, desc)
    -- resolve type
    if types[ty] and not types_noinline[ty] and not types[ty].extends then
      ty = types[ty]
    end
    lvl = lvl or 0
    local f --- @type fun(ty: Doc.Type, lvl: integer, desc: string): string
    if type(ty) == 'string' then
      f = tystr
    else
      f = assert(ty_dispatch[ty.kind])
    end
    return f(ty, lvl, desc)
  end

  --- @param out file*
  --- @param param Doc.Method.Param
  local function write_param(out, param)
    out:write(('- `%s`:'):format(param.name))
    local ty = param.type
    if ty then
      out:write(' ', Doc.ty(ty, 1, param.desc))
    elseif param.desc then
      out:write(' ', param.desc)
    end
    if param.default then
      out:write((' (default: `%s`)'):format(param.default))
    end
    out:write('\n')
  end

  --- @param out file*
  --- @param x string|Doc.Method.Return[]
  --- @param variant? string
  local function write_return(out, x, variant)
    local variant_str = variant and (' (%s version)'):format(variant) or ''
    if type(x) == 'string' then
      out:write(('**Returns%s:** %s\n'):format(variant_str, Doc.ty(x)))
    elseif type(x) == 'table' then
      if x[2] and x[2][2] == 'err' and x[3] and x[3][2] == 'err_name' then
        local sty = x[1][1]
        sty = remove_nil(sty)
        local rty = Doc.ty(sty, nil, 'or `fail`')
        out:write(('**Returns%s:** %s\n\n'):format(variant_str, rty))
        return
      else
        local tys = {} --- @type string[]
        for _, ret in ipairs(x) do
          tys[#tys + 1] = Doc.ty(ret[1])
        end
        out:write(('**Returns%s:** %s\n'):format(variant_str, table.concat(tys, ', ')))
      end
    else
      out:write('**Returns:** Nothing.\n')
    end
    out:write('\n')
  end

  --- @param out file*
  --- @param x string
  --- @param variant? string
  local function write_return_doc(out, x, variant)
    local variant_str = variant and (' (%s version)'):format(variant) or ''
    out:write(('**Returns%s:**'):format(variant_str))
    local r = dedent(x)
    out:write(r:sub(1, 1) == '-' and '\n' or ' ')
    out:write(r, '\n\n')
  end

  --- @param method Doc.Method
  --- @return string
  local function sig(method)
    local args = {} --- @type string[]
    for _, param in ipairs(method.params or {}) do
      local nm = param.name
      if isoptional(param.type) then
        nm = '[' .. nm .. ']'
      end
      args[#args + 1] = nm
    end
    return ('`uv.%s(%s)`'):format(method.name, table.concat(args, ', '))
  end

  --- @param out file*
  --- @param method Doc.Method
  --- @param lvl integer
  local function write_method(out, method, lvl)
    out:write(heading(lvl, sig(method)), '\n\n')

    if method.method_form then
      out:write(('> method form `%s`\n\n'):format(method.method_form))
    end

    if method.deprecated then
      out:write('**Deprecated:** ', dedent(method.deprecated), '\n\n')
      return
    end

    if method.params then
      out:write('**Parameters:**\n')
      for _, param in ipairs(method.params) do
        write_param(out, param)
      end
      out:write('\n')
    end

    if method.desc then
      out:write(dedent(method.desc), '\n\n')
    end

    if method.returns_doc then
      write_return_doc(out, method.returns_doc)
    elseif method.returns_sync and method.returns_async then
      if method.returns_sync_doc then
        write_return_doc(out, method.returns_sync_doc, 'sync')
      else
        write_return(out, method.returns_sync, 'sync')
      end
      write_return(out, method.returns_async, 'async')
    else
      write_return(out, method.returns)
    end

    if method.example then
      out:write(dedent(method.example), '\n\n')
    end

    if method.see then
      out:write(('See [%s][].\n\n'):format(method.see))
    end

    for _, note in ipairs(method.notes or {}) do
      local notes = dedent(note)
      out:write('**Note**:')
      out:write(notes:sub(1, 3) == '1. ' and '\n' or ' ')
      out:write(notes, '\n\n')
    end

    for _, warn in ipairs(method.warnings or {}) do
      out:write(('**Warning**: %s\n\n'):format(dedent(warn)))
    end

    if method.since then
      out:write(('**Note**: New in libuv version %s.\n\n'):format(method.since))
    end
  end

  --- @param out file*
  --- @param doc Doc
  --- @param lvl? integer
  function Doc.write(out, doc, lvl)
    lvl = lvl or 1
    local title = doc.title
    if title then
      out:write(heading(lvl, title))
      out:write('\n\n')
    end
    local id = doc.id
    if id then
      local tag = assert(title):match('^`[a-z_]+`') or title
      out:write(('[%s]: #%s\n\n'):format(tag, id))
    end

    if doc.desc then
      out:write(dedent(doc.desc), '\n\n')
    end

    if doc.constants then
      for _, constant in ipairs(doc.constants) do
        out:write(('- `%s`: "%s"\n'):format(constant[1], constant[2]))
      end
      out:write('\n')
    end

    for _, method in ipairs(doc.methods or {}) do
      write_method(out, method, lvl + 1)
    end

    for _, subsection in ipairs(doc.sections or {}) do
      Doc.write(out, subsection, lvl + 1)
    end
  end
end

local function main()
  local outdoc = assert(io.open('docs.md', 'w'))
  Doc.write(outdoc, doctop)
end

main()
