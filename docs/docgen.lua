--- This module is responsible for generating documentation and metadata for
--- the luv project. It processes documentation sections from docs.lua,
--- and writes the output to markdown (docs.md) and Lua files (meta.lua).

local docs = dofile('docs/docs.lua')
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

--- Enumerates key-value pairs of a table, ordered by key.
--- @generic T: table, K, V
--- @param t T Dict-like table
--- @return fun(table: table<K, V>, index?: K):K, V # |for-in| iterator over sorted keys and their values
--- @return T
local function spairs(t)
  --- @cast t table<any,any>

  -- collect the keys
  local keys = {}
  for k in pairs(t) do
    table.insert(keys, k)
  end
  table.sort(keys)

  -- Return the iterator function.
  local i = 0
  return function()
    i = i + 1
    if keys[i] then
      return keys[i], t[keys[i]]
    end
  end,
    t
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

--- @type table<Doc.Type, string>
local gtypes = {}

--- @param nm string
--- @param ty Doc.Type
local function add_gtype(nm, ty)
  if gtypes[ty] then
    error('Type already exists: ' .. nm)
  end
  gtypes[ty] = nm
  types[nm] = ty
end

--- @param ns string
--- @param nm? string
--- @param ty Doc.Type
local function gen_type(ns, nm, ty)
  ty = remove_nil(ty)

  if type(ty) ~= 'table' or gtypes[ty] then
    return
  end

  local ty_nm = nm and ('%s.%s'):format(ns, nm) or ns

  if ty.kind == 'function' then
    if #ty.args > 2 then
      add_gtype(ty_nm, ty)
    end
    for _, f in ipairs(ty.args) do
      local arg_nm, arg_ty = f[1], f[2]
      gen_type(ty_nm, arg_nm, arg_ty)
    end
  elseif ty.kind == 'union' then
    local nonstrtys = 0
    for _, uty in ipairs(ty[1]) do
      if type(uty) == 'table' and not types[uty] then
        nonstrtys = nonstrtys + 1
      end
    end

    for i, uty in ipairs(ty[1]) do
      gen_type(ty_nm, nonstrtys > 1 and '_' .. i or nil, uty)
    end
  elseif ty.kind == 'dict' then
    gen_type(ty_nm, types[ty_nm] and 'value' or nil, ty.value)
  elseif ty.kind == 'table' then
    if #ty.fields > 2 or (#ty.fields > 0 and ty.fields[1][4]) then
      add_gtype(ty_nm, ty)
    end
    for _, f in ipairs(ty.fields) do
      local field_nm, field_ty = f[1], f[2]
      gen_type(ty_nm, field_nm, field_ty)
    end
  end
end

--- @param section Doc
local function gen_types_for_doc(section)
  for _, funcs in ipairs(section.funcs or {}) do
    for _, p in ipairs(funcs.params or {}) do
      gen_type(funcs.name, p.name, p.type)
    end
    local returns = funcs.returns or funcs.returns_sync
    if type(returns) == 'table' then
      for _, p in ipairs(returns) do
        gen_type(funcs.name, p[2], p[1])
      end
    end
  end

  for _, subsection in ipairs(section.sections or {}) do
    gen_types_for_doc(subsection)
  end
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
  --- @param param Doc.Func.Param
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
  --- @param x string|Doc.Func.Return[]
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

  --- @param func Doc.Func
  --- @return string
  local function sig(func)
    local args = {} --- @type string[]
    for _, param in ipairs(func.params or {}) do
      local nm = param.name
      if isoptional(param.type) then
        nm = '[' .. nm .. ']'
      end
      args[#args + 1] = nm
    end
    return ('`uv.%s(%s)`'):format(func.name, table.concat(args, ', '))
  end

  --- @param out file*
  --- @param func Doc.Func
  --- @param lvl integer
  local function write_func(out, func, lvl)
    out:write(heading(lvl, sig(func)), '\n\n')

    if func.method_form then
      out:write(('> method form `%s`\n\n'):format(func.method_form))
    end

    if func.deprecated then
      out:write('**Deprecated:** ', dedent(func.deprecated), '\n\n')
      return
    end

    if func.params then
      out:write('**Parameters:**\n')
      for _, param in ipairs(func.params) do
        write_param(out, param)
      end
      out:write('\n')
    end

    if func.desc then
      out:write(dedent(func.desc), '\n\n')
    end

    if func.returns_doc then
      write_return_doc(out, func.returns_doc)
    elseif func.returns_sync and func.returns_async then
      if func.returns_sync_doc then
        write_return_doc(out, func.returns_sync_doc, 'sync')
      else
        write_return(out, func.returns_sync, 'sync')
      end
      write_return(out, func.returns_async, 'async')
    else
      write_return(out, func.returns)
    end

    if func.example then
      out:write(dedent(func.example), '\n\n')
    end

    if func.see then
      out:write(('See [%s][].\n\n'):format(func.see))
    end

    for _, note in ipairs(func.notes or {}) do
      local notes = dedent(note)
      out:write('**Note**:')
      out:write(notes:sub(1, 3) == '1. ' and '\n' or ' ')
      out:write(notes, '\n\n')
    end

    for _, warn in ipairs(func.warnings or {}) do
      out:write(('**Warning**: %s\n\n'):format(dedent(warn)))
    end

    if func.since then
      out:write(('**Note**: New in libuv version %s.\n\n'):format(func.since))
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

    for _, alias in spairs(doc.aliases or {}) do
      for _, a in ipairs(alias) do
        out:write(('- `%s`: %s\n'):format(a[1], a[2]))
      end
      out:write('\n')
    end

    for _, func in ipairs(doc.funcs or {}) do
      write_func(out, func, lvl + 1)
    end

    for _, subsection in ipairs(doc.sections or {}) do
      Doc.write(out, subsection, lvl + 1)
    end
  end
end

local Meta = {}

do -- meta
  --- @param x string
  --- @return string
  local function id(x)
    if x == 'repeat' then
      x = 'repeat_'
    end
    return (x:gsub(' ', '_'))
  end

  --- @param func Doc.Func
  --- @param method? {class: string, name: string}
  --- @return string
  local function sig(func, method)
    local args = {} --- @type string[]
    for i, param in ipairs(func.params or {}) do
      if not (func.returns_async and param.name == 'callback' and i == #func.params) and (not method or i > 1) then
        args[#args + 1] = id(param.name)
      end
    end
    if method then
      return ('function %s:%s(%s) end'):format(method.class, method.name, table.concat(args, ', '))
    end
    return ('function uv.%s(%s) end'):format(func.name, table.concat(args, ', '))
  end

  --- @param ty Doc.Type
  --- @param no_gtypes? boolean
  --- @return string
  function Meta.ty(ty, no_gtypes)
    if type(ty) == 'string' then
      if types[ty] then
        ty = 'uv.' .. ty
      end
      return ty
    end

    if not no_gtypes and gtypes[ty] then
      return 'uv.' .. gtypes[ty]
    end

    if ty.kind == 'dict' then
      return ('table<%s, %s>'):format(ty.key, Meta.ty(ty.value))
    elseif ty.kind == 'function' then
      local r = {} --- @type string[]
      for _, arg in pairs(ty.args) do
        local arg_nm, arg_ty = arg[1], arg[2]
        r[#r + 1] = ('%s: %s'):format(arg_nm, Meta.ty(arg_ty))
      end
      return ('fun(%s)'):format(table.concat(r, ', '))
    elseif ty.kind == 'table' then
      if #ty.fields == 0 then
        return '{}'
      end
      local r = {} --- @type string[]
      for _, arg in pairs(ty.fields) do
        local field_nm, field_ty = arg[1], arg[2]
        r[#r + 1] = ('%s: %s'):format(field_nm, Meta.ty(field_ty))
      end
      return ('{ %s }'):format(table.concat(r, ', '))
    elseif ty.kind == 'union' then
      local r = {} --- @type string[]
      local add_optional = false
      for i, uty in ipairs(ty[1]) do
        if i == #ty[1] and uty == 'nil' then
          add_optional = true
        else
          r[#r + 1] = Meta.ty(uty)
        end
      end
      return table.concat(r, '|') .. (add_optional and '?' or '')
    end

    error('unknown type: ' .. ty)
  end

  --- @param out file*
  --- @param x string|Doc.Func.Return[]?
  local function write_return(out, x)
    if type(x) == 'string' then
      out:write('--- @return ', Meta.ty(x), '\n')
    elseif type(x) == 'table' then
      for _, ret in ipairs(x) do
        out:write('--- @return ', Meta.ty(ret[1]))
        if ret[2] then
          out:write(' ', id(ret[2]))
        end
        out:write('\n')
      end
    end
  end

  --- @param out file*
  --- @param func Doc.Func
  --- @param x string|Doc.Func.Return[]
  local function write_async_overload(out, func, x)
    if type(x) == 'string' then
      x = { { x } }
    end

    out:write('--- @overload fun(')
    local args = {} --- @type string[]
    for _, arg in ipairs(func.params) do
      local ty = arg.type
      if arg.name == 'callback' then
        ty = remove_nil(ty)
      end
      args[#args + 1] = ('%s: %s'):format(id(arg.name), Meta.ty(ty))
    end
    out:write(table.concat(args, ', '), '): ')

    local ret = {} --- @type string[]
    for _, r in ipairs(x) do
      ret[#ret + 1] = Meta.ty(r[1])
    end
    out:write(table.concat(ret, ', '), '\n')
  end

  --- @param out file*
  --- @param str string
  local function write_comment(out, str)
    --- @diagnostic disable-next-line: no-unknown
    for line, nl in str:gmatch('([^\r\n]*)([\n\r]?)') do
      if line ~= '' then
        out:write('--- ', line, '\n')
      elseif nl ~= '' then
        out:write('---\n')
      end
    end
  end

  local types_written = {} --- @type table<string,true>

  --- @param out file*
  --- @param nm string
  --- @param ty Doc.Type
  --- @param nonl? boolean
  --- @return boolean
  local function write_type(out, nm, ty, nonl)
    if types_written[nm] then
      return false
    end
    types_written[nm] = true

    if type(ty) == 'string' then
      out:write('--- @alias ', Meta.ty(nm), ' ', Meta.ty(ty), '\n\n')
    end

    if ty.kind == 'dict' then
      if type(ty.value) == 'string' then
        out:write(
          '--- @alias ',
          Meta.ty(nm),
          ' table<',
          Meta.ty(ty.key),
          ',',
          Meta.ty(ty.value),
          '>\n'
        )
      else
        out:write('--- @class ', Meta.ty(nm), '\n')
        out:write('--- @field [', Meta.ty(ty.key), '] ', Meta.ty(ty.value), '\n')
      end
    elseif ty.kind == 'table' then
      out:write('--- @class ', Meta.ty(nm))
      if ty.extends then
        out:write(' : ', Meta.ty(ty.extends))
      end
      out:write('\n')
      for _, arg in pairs(ty.fields) do
        local name, aty, desc = arg[1], arg[2], arg[4]
        if desc then
          out:write('---\n')
          write_comment(out, dedent(desc))
        end
        out:write('--- @field ', name, ' ', Meta.ty(aty), '\n')
      end
    elseif ty.kind == 'union' then
      local tys = ty[1]
      out:write('--- @alias ', Meta.ty(nm), '\n')
      for _, uty in ipairs(tys) do
        out:write('--- | ', Meta.ty(uty), '\n')
      end
    elseif ty.kind == 'function' then
      out:write('--- @alias ', Meta.ty(nm), '\n')
      out:write('--- | ', Meta.ty(ty, true), '\n')
    else
      error('unknown')
    end
    if not nonl then
      out:write('\n')
    end

    return true
  end

  --- @param out file*
  --- @param func Doc.Func
  --- @param method? { class: string, name: string }
  local function write_func(out, func, method)
    for nm, ty in spairs(types) do
      if nm:sub(1, #func.name) == func.name then
        write_type(out, nm, ty)
      end
    end

    if func.deprecated then
      out:write('--- @deprecated ', dedent(func.deprecated), '\n')
    end

    if func.desc then
      write_comment(out, dedent(func.desc))
    end

    if func.example then
      out:write('--- Example\n')
      write_comment(out, dedent(func.example))
    end

    for _, note in ipairs(func.notes or {}) do
      local notes = dedent(note)
      out:write('--- **Note**:\n')
      write_comment(out, notes)
    end

    for _, warn in ipairs(func.warnings or {}) do
      out:write('--- **Warning**:\n')
      write_comment(out, dedent(warn))
    end

    -- if method.see then
    --   out:write(('--- @see %s\n'):format(method.see))
    -- end

    -- if method.since then
    --   out:write(('**Note**: New in libuv version %s.\n\n'):format(method.since))
    -- end

    if func.params then
      for i, param in ipairs(func.params) do
        local is_async_callback_param = func.returns_async and param.name == 'callback'
        if not (is_async_callback_param and i == #func.params) and (not method or i > 1) then
          out:write('--- @param ', id(param.name), ' ', is_async_callback_param and 'nil' or Meta.ty(param.type))
          if param.desc then
            if param.desc:match('\n') then
              write_comment(out, param.desc)
            else
              out:write(' ', param.desc)
            end
          end
          out:write('\n')
        end
      end
    end

    write_return(out, func.returns or func.returns_sync)
    if func.returns_async then
      write_async_overload(out, func, func.returns_async)
    end

    out:write(sig(func, method), '\n\n')
  end

  --- @param out file*
  --- @param doc Doc
  local function write_doc(out, doc)
    if doc.title then
      out:write('--- # ', doc.title, '\n')
    end

    if doc.desc then
      if doc.title then
        out:write('---\n')
      end
      write_comment(out, dedent(doc.desc))
    end

    if doc.class then
      assert(write_type(out, doc.class, types[doc.class], true))
      out:write('local ', doc.class, ' = {}\n')
    end

    for _, constant in ipairs(doc.constants or {}) do
      out:write(("uv.constants.%s = '%s'\n"):format(constant[1], constant[2]))
    end

    for name, alias in spairs(doc.aliases or {}) do
      out:write('\n')
      out:write(('--- @alias uv.%s\n'):format(name))
      for _, a in ipairs(alias) do
        out:write(("--- | '%s' # %s\n"):format(a[1], a[2]))
      end
    end

    if doc.funcs then
      out:write('\n')
      for _, func in ipairs(doc.funcs or {}) do
        write_func(out, func)
        if func.method_form then
          assert(func.params, func.name)
          local name = func.method_form:match('^[%w_]+%:([%w_]+)%(')
          local class = func.params[1].type
          assert(type(class) == 'string')
          if write_type(out, class, types[class], true) then
            out:write('local ', class, ' = {}\n\n')
          end
          write_func(out, func, { class = class, name = name })
        end
      end
    end

    if doc.sections then
      out:write('\n')
      for _, subsection in ipairs(doc.sections) do
        write_doc(out, subsection)
      end
    end

    out:write('\n')
  end

  --- @param out file*
  --- @param doc Doc
  function Meta.write(out, doc)
    out:write('--- @meta\n')
    out:write('--- @class uv\n')
    out:write('local uv = {}\n')
    out:write('uv.constants = {}\n')
    out:write('\n')

    write_doc(out, doc)

    for nm, ty in spairs(types) do
      write_type(out, nm, ty)
    end
  end
end

local function main()
  -- Generate types for large inline types
  gen_types_for_doc(doctop)

  local outdoc = assert(io.open('docs/docs.md', 'w'))
  Doc.write(outdoc, doctop)

  local outmeta = assert(io.open('docs/meta.lua', 'w'))
  Meta.write(outmeta, doctop)
end

main()
