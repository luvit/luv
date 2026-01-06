--- @class (exact) Doc.Type.Fun.Attrs
---                   name,   type,   desc
--- @field [integer] [string, Doc.Type, string?]

--- @class (exact) Doc.Type.Fun
--- @field kind 'function'
---                   name,   type,   desc
--- @field args [string, Doc.Type, string?][]

--- @class (exact) Doc.Type.Table.Attrs
--- @field extends? string
---                   name,   type,   default, desc
--- @field [integer] [string, Doc.Type, string?, string?]

--- @class (exact) Doc.Type.Table
--- @field kind 'table'
--- @field extends? string
---                   name,   type,   default, desc
--- @field fields [string, Doc.Type, string?, string?][]

--- @class (exact) Doc.Type.Union
--- @field kind 'union'
--- @field [1] Doc.Type[]

--- @class (exact) Doc.Type.Dict
--- @field kind 'dict'
--- @field key string key
--- @field value Doc.Type value

--- @alias Doc.Type
--- | string
--- | Doc.Type.Fun
--- | Doc.Type.Table
--- | Doc.Type.Dict
--- | Doc.Type.Union

--- @class (exact) Doc.Func.Param
--- @field name string
--- @field type Doc.Type
--- @field desc? string
--- @field default? string

--- @alias Doc.Func.Return [Doc.Type, string]

--- @class (exact) Doc.Func
--- @field name string
--- @field desc? string
--- @field deprecated? string
--- @field since? string
--- @field params? Doc.Func.Param[]
--- @field returns? string|Doc.Func.Return[]
--- @field returns_sync? string|Doc.Func.Return[]
--- @field returns_async? string|Doc.Func.Return[]
--- @field returns_doc? string
--- @field returns_sync_doc? string
--- @field notes? string[]
--- @field warnings? string[]
--- @field example? string
--- @field method_form? string
--- @field see? string

--- @class (exact) Doc
--- @field title? string
--- @field desc? string
--- @field id? string
--- @field class? string
--- @field sections? Doc[]
--- @field funcs? Doc.Func[]
--- @field constants? [string,string][]
--- @field aliases? table<string,[string,string][]>

--- @param ... Doc.Type
--- @return Doc.Type.Union
local function union(...)
  return { kind = 'union', { ... } }
end

--- @param ty Doc.Type
--- @return Doc.Type.Union
local function opt(ty)
  if type(ty) == 'table' and ty.kind == 'union' then
    table.insert(ty[1], 'nil')
    return ty
  end
  return union(ty, 'nil')
end

local opt_str = opt('string')
local opt_int = opt('integer')
local opt_bool = opt('boolean')

--- @param t? Doc.Type.Table.Attrs
--- @return Doc.Type.Table
local function table(t)
  t = t or {}
  local extends = t.extends
  t.extends = nil
  return {
    kind = 'table',
    extends = extends,
    fields = t,
  }
end

--- @param key Doc.Type
--- @param value Doc.Type
--- @return Doc.Type.Dict
local function dict(key, value)
  return {
    kind = 'dict',
    key = key,
    value = value,
  }
end

--- @param extends string
--- @return Doc.Type.Table
local function cls(extends)
  return table({ extends = extends })
end

--- @param ty Doc.Type
--- @param name string
--- @return [Doc.Type, string][]
local function ret_or_fail(ty, name)
  return {
    { opt(ty), name },
    { opt_str, 'err' },
    { opt('uv.error_name'), 'err_name' },
  }
end

--- @param t Doc.Type.Fun.Attrs
--- @return Doc.Type.Fun
local function fun(t)
  return {
    kind = 'function',
    args = t,
  }
end

--- @param args? [string, Doc.Type, string?][]
--- @param optional? boolean
--- @param desc? string
--- @return Doc.Func.Param
local function cb(args, optional, desc)
  --- @type Doc.Type
  local ty = fun(args or {})
  if optional then
    ty = opt(ty)
  end
  return {
    name = 'callback',
    type = ty,
    desc = desc,
  }
end

--- @param args? [string, Doc.Type, string?][]
--- @param optional? boolean
--- @param desc? string
--- @return Doc.Func.Param
local function cb_err(args, optional, desc)
  return cb({
    { 'err', opt_str },
    (_G.table.unpack or unpack)(args or {}),
  }, optional, desc)
end

--- @param args? [string, Doc.Type, string?][]
--- @return Doc.Func.Param
local function async_cb(args)
  args = args or { { 'success', opt_bool } }
  return cb_err(args, true, '(async if provided, sync if `nil`)')
end

local success_ret = ret_or_fail('0', 'success')

local error_names = {
  { 'E2BIG', 'argument list too long.' },
  { 'EACCES', 'permission denied.' },
  { 'EADDRINUSE', 'address already in use.' },
  { 'EADDRNOTAVAIL', 'address not available.' },
  { 'EAFNOSUPPORT', 'address family not supported.' },
  { 'EAGAIN', 'resource temporarily unavailable.' },
  { 'EAI_ADDRFAMILY', 'address family not supported.' },
  { 'EAI_AGAIN', 'temporary failure.' },
  { 'EAI_BADFLAGS', 'bad ai_flags value.' },
  { 'EAI_BADHINTS', 'invalid value for hints.' },
  { 'EAI_CANCELED', 'request canceled.' },
  { 'EAI_FAIL', 'permanent failure.' },
  { 'EAI_FAMILY', 'ai_family not supported.' },
  { 'EAI_MEMORY', 'out of memory.' },
  { 'EAI_NODATA', 'no address.' },
  { 'EAI_NONAME', 'unknown node or service.' },
  { 'EAI_OVERFLOW', 'argument buffer overflow.' },
  { 'EAI_PROTOCOL', 'resolved protocol is unknown.' },
  { 'EAI_SERVICE', 'service not available for socket type.' },
  { 'EAI_SOCKTYPE', 'socket type not supported.' },
  { 'EALREADY', 'connection already in progress.' },
  { 'EBADF', 'bad file descriptor.' },
  { 'EBUSY', 'resource busy or locked.' },
  { 'ECANCELED', 'operation canceled.' },
  { 'ECHARSET', 'invalid Unicode character.' },
  { 'ECONNABORTED', 'software caused connection abort.' },
  { 'ECONNREFUSED', 'connection refused.' },
  { 'ECONNRESET', 'connection reset by peer.' },
  { 'EDESTADDRREQ', 'destination address required.' },
  { 'EEXIST', 'file already exists.' },
  { 'EFAULT', 'bad address in system call argument.' },
  { 'EFBIG', 'file too large.' },
  { 'EHOSTUNREACH', 'host is unreachable.' },
  { 'EINTR', 'interrupted system call.' },
  { 'EINVAL', 'invalid argument.' },
  { 'EIO', 'i/o error.' },
  { 'EISCONN', 'socket is already connected.' },
  { 'EISDIR', 'illegal operation on a directory.' },
  { 'ELOOP', 'too many symbolic links encountered.' },
  { 'EMFILE', 'too many open files.' },
  { 'EMSGSIZE', 'message too long.' },
  { 'ENAMETOOLONG', 'name too long.' },
  { 'ENETDOWN', 'network is down.' },
  { 'ENETUNREACH', 'network is unreachable.' },
  { 'ENFILE', 'file table overflow.' },
  { 'ENOBUFS', 'no buffer space available.' },
  { 'ENODEV', 'no such device.' },
  { 'ENOENT', 'no such file or directory.' },
  { 'ENOMEM', 'not enough memory.' },
  { 'ENONET', 'machine is not on the network.' },
  { 'ENOPROTOOPT', 'protocol not available.' },
  { 'ENOSPC', 'no space left on device.' },
  { 'ENOSYS', 'function not implemented.' },
  { 'ENOTCONN', 'socket is not connected.' },
  { 'ENOTDIR', 'not a directory.' },
  { 'ENOTEMPTY', 'directory not empty.' },
  { 'ENOTSOCK', 'socket operation on non-socket.' },
  { 'ENOTSUP', 'operation not supported on socket.' },
  { 'EOVERFLOW', 'value too large for defined data type.' },
  { 'EPERM', 'operation not permitted.' },
  { 'EPIPE', 'broken pipe.' },
  { 'EPROTO', 'protocol error.' },
  { 'EPROTONOSUPPORT', 'protocol not supported.' },
  { 'EPROTOTYPE', 'protocol wrong type for socket.' },
  { 'ERANGE', 'result too large.' },
  { 'EROFS', 'read-only file system.' },
  { 'ESHUTDOWN', 'cannot send after transport endpoint shutdown.' },
  { 'ESPIPE', 'invalid seek.' },
  { 'ESRCH', 'no such process.' },
  { 'ETIMEDOUT', 'connection timed out.' },
  { 'ETXTBSY', 'text file is busy.' },
  { 'EXDEV', 'cross-device link not permitted.' },
  { 'UNKNOWN', 'unknown error.' },
  { 'EOF', 'end of file.' },
  { 'ENXIO', 'no such device or address.' },
  { 'EMLINK', 'too many links.' },
  { 'ENOTTY', 'inappropriate ioctl for device.' },
  { 'EFTYPE', 'inappropriate file type or format.' },
  { 'EILSEQ', 'illegal byte sequence.' },
  { 'ESOCKTNOSUPPORT', 'socket type not supported.' },
}

--- @type table<string,[string,string][]>
local constants = {
  address_families = {
    { 'AF_UNIX', 'unix' },
    { 'AF_INET', 'inet' },
    { 'AF_INET6', 'inet6' },
    { 'AF_IPX', 'ipx' },
    { 'AF_NETLINK', 'netlink' },
    { 'AF_X25', 'x25' },
    { 'AF_AX25', 'as25' },
    { 'AF_ATMPVC', 'atmpvc' },
    { 'AF_APPLETALK', 'appletalk' },
    { 'AF_PACKET', 'packet' },
  },
  signals = {
    { 'SIGHUP', 'sighup' },
    { 'SIGINT', 'sigint' },
    { 'SIGQUIT', 'sigquit' },
    { 'SIGILL', 'sigill' },
    { 'SIGTRAP', 'sigtrap' },
    { 'SIGABRT', 'sigabrt' },
    { 'SIGIOT', 'sigiot' },
    { 'SIGBUS', 'sigbus' },
    { 'SIGFPE', 'sigfpe' },
    { 'SIGKILL', 'sigkill' },
    { 'SIGUSR1', 'sigusr1' },
    { 'SIGSEGV', 'sigsegv' },
    { 'SIGUSR2', 'sigusr2' },
    { 'SIGPIPE', 'sigpipe' },
    { 'SIGALRM', 'sigalrm' },
    { 'SIGTERM', 'sigterm' },
    { 'SIGCHLD', 'sigchld' },
    { 'SIGSTKFLT', 'sigstkflt' },
    { 'SIGCONT', 'sigcont' },
    { 'SIGSTOP', 'sigstop' },
    { 'SIGTSTP', 'sigtstp' },
    { 'SIGBREAK', 'sigbreak' },
    { 'SIGTTIN', 'sigttin' },
    { 'SIGTTOU', 'sigttou' },
    { 'SIGURG', 'sigurg' },
    { 'SIGXCPU', 'sigxcpu' },
    { 'SIGXFSZ', 'sigxfsz' },
    { 'SIGVTALRM', 'sigvtalrm' },
    { 'SIGPROF', 'sigprof' },
    { 'SIGWINCH', 'sigwinch' },
    { 'SIGIO', 'sigio' },
    { 'SIGPOLL', 'sigpoll' },
    { 'SIGLOST', 'siglost' },
    { 'SIGPWR', 'sigpwr' },
    { 'SIGSYS', 'sigsys' },
  },
  socket_types = {
    { 'SOCK_STREAM', 'stream' },
    { 'SOCK_DGRAM', 'dgram' },
    { 'SOCK_SEQPACKET', 'seqpacket' },
    { 'SOCK_RAW', 'raw' },
    { 'SOCK_RDM', 'rdm' },
  },
  tty_modes = {
    { 'TTY_MODE_NORMAL', 'normal' },
    { 'TTY_MODE_RAW', 'raw' },
    { 'TTY_MODE_IO', 'io' },
    { 'TTY_MODE_RAW_VT', 'raw_vt' },
  },
  fs_utime = {
    { 'FS_UTIME_NOW', 'now' },
    { 'FS_UTIME_OMIT', 'omit' },
  },
}

--- @type table<string,Doc.Type>
local types = {
  -- Request types
  uv_req_t = cls('userdata'),
  uv_connect_t = cls('uv_req_t'),
  uv_fs_t = cls('uv_req_t'),
  uv_getaddrinfo_t = cls('uv_req_t'),
  uv_getnameinfo_t = cls('uv_req_t'),
  uv_shutdown_t = cls('uv_req_t'),
  uv_udp_send_t = cls('uv_req_t'),
  uv_work_t = cls('uv_req_t'),
  uv_write_t = cls('uv_req_t'),

  -- Handle types
  uv_handle_t = cls('userdata'),
  uv_async_t = cls('uv_handle_t'),
  uv_check_t = cls('uv_handle_t'),
  uv_fs_event_t = cls('uv_handle_t'),
  uv_fs_poll_t = cls('uv_handle_t'),
  uv_idle_t = cls('uv_handle_t'),
  uv_poll_t = cls('uv_handle_t'),
  uv_prepare_t = cls('uv_handle_t'),
  uv_process_t = cls('uv_handle_t'),
  uv_signal_t = cls('uv_handle_t'),
  uv_timer_t = cls('uv_handle_t'),
  uv_udp_t = cls('uv_handle_t'),

  -- Stream types
  uv_stream_t = cls('uv_handle_t'),
  uv_pipe_t = cls('uv_stream_t'),
  uv_tty_t = cls('uv_stream_t'),
  uv_tcp_t = cls('uv_stream_t'),

  luv_dir_t = cls('userdata'),
  luv_work_ctx_t = cls('userdata'),
  luv_thread_t = cls('userdata'),
  luv_sem_t = cls('userdata'),

  threadargs = union('number', 'boolean', 'string', 'userdata'),

  buffer = union('string', 'string[]'),

  address = table({
    { 'addr', 'string' },
    { 'family', 'string' },
    { 'port', opt_int },
    { 'socktype', 'string' },
    { 'protocol', 'string' },
    { 'canonname', opt_str },
  }),

  socketinfo = table({
    { 'ip', 'string' },
    { 'family', 'string' },
    { 'port', 'integer' },
  }),

  ['fs_stat.result.time'] = table({
    { 'sec', 'integer' },
    { 'nsec', 'integer' },
  }),

  ['fs_stat.result'] = table({
    { 'dev', 'integer' },
    { 'mode', 'integer' },
    { 'nlink', 'integer' },
    { 'uid', 'integer' },
    { 'gid', 'integer' },
    { 'rdev', 'integer' },
    { 'ino', 'integer' },
    { 'size', 'integer' },
    { 'blksize', 'integer' },
    { 'blocks', 'integer' },
    { 'flags', 'integer' },
    { 'gen', 'integer' },
    { 'atime', 'fs_stat.result.time' },
    { 'mtime', 'fs_stat.result.time' },
    { 'ctime', 'fs_stat.result.time' },
    { 'birthtime', 'fs_stat.result.time' },
    { 'type', 'string' },
  }),

  ['fs_statfs.result'] = table({
    { 'type', 'integer' },
    { 'bsize', 'integer' },
    { 'blocks', 'integer' },
    { 'bfree', 'integer' },
    { 'bavail', 'integer' },
    { 'files', 'integer' },
    { 'ffree', 'integer' },
  }),

  ['getrusage.result.time'] = table({
    { 'sec', 'integer' },
    { 'usec', 'integer' },
  }),

  ['getrusage.result'] = table({
    { 'utime', 'getrusage.result.time', nil, '(user CPU time used)' },
    { 'stime', 'getrusage.result.time', nil, '(system CPU time used)' },
    { 'maxrss', 'integer', nil, '(maximum resident set size)' },
    { 'ixrss', 'integer', nil, '(integral shared memory size)' },
    { 'idrss', 'integer', nil, '(integral unshared data size)' },
    { 'isrss', 'integer', nil, '(integral unshared stack size)' },
    { 'minflt', 'integer', nil, '(page reclaims (soft page faults))' },
    { 'majflt', 'integer', nil, '(page faults (hard page faults))' },
    { 'nswap', 'integer', nil, '(swaps)' },
    { 'inblock', 'integer', nil, '(block input operations)' },
    { 'oublock', 'integer', nil, '(block output operations)' },
    { 'msgsnd', 'integer', nil, '(IPC messages sent)' },
    { 'msgrcv', 'integer', nil, '(IPC messages received)' },
    { 'nsignals', 'integer', nil, '(signals received)' },
    { 'nvcsw', 'integer', nil, '(voluntary context switches)' },
    { 'nivcsw', 'integer', nil, '(involuntary context switches)' },
  }),
}

--- @type Doc
local doc = {
  title = 'LibUV in Lua',
  desc = [[
    The [luv][] project provides access to the multi-platform support library
    [libuv][] in Lua code. It was primarily developed for the [luvit][] project as
    the built-in `uv` module, but can be used in other Lua environments.

    More information about the core libuv library can be found at the original
    [libuv documentation page][].
  ]],
  sections = {
    { -- lvl 3 sections
      sections = {
        {
          title = 'TCP Echo Server Example',
          desc = [[
            Here is a small example showing a TCP echo server:

            ```lua
            local uv = require("luv") -- "luv" when stand-alone, "uv" in luvi apps

            local server = uv.new_tcp()
            server:bind("127.0.0.1", 1337)
            server:listen(128, function (err)
              assert(not err, err)
              local client = uv.new_tcp()
              server:accept(client)
              client:read_start(function (err, chunk)
                assert(not err, err)
                if chunk then
                  client:write(chunk)
                else
                  client:shutdown()
                  client:close()
                end
              end)
            end)
            print("TCP server listening at 127.0.0.1 port 1337")
            uv.run() -- an explicit run call is necessary outside of luvit
            ```
          ]],
        },
        {
          title = 'Module Layout',
          desc = [[
            The luv library contains a single Lua module referred to hereafter as `uv` for
            simplicity. This module consists mostly of functions with names corresponding to
            their original libuv versions. For example, the libuv function `uv_tcp_bind` has
            a luv version at `uv.tcp_bind`. Currently, only two non-function fields exists:
            `uv.constants` and `uv.errno`, which are tables.
          ]],
        },
        {
          title = 'Functions vs Methods',
          desc = [[
            In addition to having simple functions, luv provides an optional method-style
            API. For example, `uv.tcp_bind(server, host, port)` can alternatively be called
            as `server:bind(host, port)`. Note that the first argument `server` becomes the
            object and `tcp_` is removed from the function name. Method forms are
            documented below where they exist.
          ]],
        },
        {
          title = 'Synchronous vs Asynchronous Functions',
          desc = [[
            Functions that accept a callback are asynchronous. These functions may
            immediately return results to the caller to indicate their initial status, but
            their final execution is deferred until at least the next libuv loop iteration.
            After completion, their callbacks are executed with any results passed to it.

            Functions that do not accept a callback are synchronous. These functions
            immediately return their results to the caller.

            Some (generally FS and DNS) functions can behave either synchronously or
            asynchronously. If a callback is provided to these functions, they behave
            asynchronously; if no callback is provided, they behave synchronously.
          ]],
        },
        {
          title = 'Pseudo-Types',
          desc = [[
            Some unique types are defined. These are not actual types in Lua, but they are
            used here to facilitate documenting consistent behavior:
            - `fail`: an assertable `nil, string, string` tuple (see [Error Handling][])
            - `callable`: a `function`; or a `table` or `userdata` with a `__call`
              metamethod
            - `buffer`: a `string` or a sequential `table` of `string`s
            - `threadargs`: variable arguments (`...`) of type `nil`, `boolean`, `number`,
              `string`, or `userdata`, numbers of argument limited to 9.
          ]],
        },
      },
    },
    {
      title = 'Contents',
      desc = [[
        This documentation is mostly a retelling of the [libuv API documentation][]
        within the context of luv's Lua API. Low-level implementation details and
        unexposed C functions and types are not documented here except for when they
        are relevant to behavior seen in the Lua module.

        - [Constants][]
        - [Error Handling][]
        - [Version Checking][]
        - [`uv_loop_t`][] — Event loop
        - [`uv_req_t`][] — Base request
        - [`uv_handle_t`][] — Base handle
          - [`uv_timer_t`][] — Timer handle
          - [`uv_prepare_t`][] — Prepare handle
          - [`uv_check_t`][] — Check handle
          - [`uv_idle_t`][] — Idle handle
          - [`uv_async_t`][] — Async handle
          - [`uv_poll_t`][] — Poll handle
          - [`uv_signal_t`][] — Signal handle
          - [`uv_process_t`][] — Process handle
          - [`uv_stream_t`][] — Stream handle
            - [`uv_tcp_t`][] — TCP handle
            - [`uv_pipe_t`][] — Pipe handle
            - [`uv_tty_t`][] — TTY handle
          - [`uv_udp_t`][] — UDP handle
          - [`uv_fs_event_t`][] — FS Event handle
          - [`uv_fs_poll_t`][] — FS Poll handle
        - [File system operations][]
        - [Thread pool work scheduling][]
        - [DNS utility functions][]
        - [Threading and synchronization utilities][]
        - [Miscellaneous utilities][]
        - [Metrics operations][]
      ]],
    },
    {
      title = 'Constants',
      id = 'constants',
      desc = [[
        As a Lua library, luv supports and encourages the use of lowercase strings to
        represent options. For example:
        ```lua
        -- signal start with string input
        uv.signal_start("sigterm", function(signame)
          print(signame) -- string output: "sigterm"
        end)
        ```

        However, luv also superficially exposes libuv constants in a Lua table at
        `uv.constants` where its keys are uppercase constant names and their associated
        values are integers defined internally by libuv. The values from this table may
        be supported as function arguments, but their use may not change the output
        type. For example:

        ```lua
        -- signal start with integer input
        uv.signal_start(uv.constants.SIGTERM, function(signame)
          print(signame) -- string output: "sigterm"
        end)
        ```

        The uppercase constants defined in `uv.constants` that have associated
        lowercase option strings are listed below.
      ]],
      sections = {
        {
          title = 'Address Families',
          constants = constants.address_families,
        },
        {
          title = 'Signals',
          constants = constants.signals,
        },
        {
          title = 'Socket Types',
          constants = constants.socket_types,
        },
        {
          title = 'TTY Modes',
          constants = constants.tty_modes,
        },
        {
          title = 'FS Modification Times',
          constants = constants.fs_utime,
        },
      },
    },
    {
      title = 'Error Handling',
      id = 'error-handling',
      desc = [[
        In libuv, errors are represented by negative numbered constants. While these
        constants are made available in the `uv.errno` table, they are not returned by
        luv functions and the libuv functions used to handle them are not exposed.
        Instead, if an internal error is encountered, the failing luv function will
        return to the caller an assertable `nil, err, name` tuple:

        - `nil` idiomatically indicates failure
        - `err` is a string with the format `{name}: {message}`
          - `{name}` is the error name provided internally by `uv_err_name`
          - `{message}` is a human-readable message provided internally by `uv_strerror`
        - `name` is the same string used to construct `err`

        This tuple is referred to below as the `fail` pseudo-type.

        When a function is called successfully, it will return either a value that is
        relevant to the operation of the function, or the integer `0` to indicate
        success, or sometimes nothing at all. These cases are documented below.

        Below is a list of known error names and error strings. See libuv's
        [error constants][] page for an original source.
      ]],
      aliases = { error_name = error_names },
    },
    {
      title = 'Version Checking',
      id = 'version-checking',
      funcs = {
        {
          name = 'version',
          desc = [[
            Returns the libuv version packed into a single integer. 8 bits are used for each
            component, with the patch number stored in the 8 least significant bits. For
            example, this would be 0x010203 in libuv 1.2.3.
          ]],
          returns = 'integer',
        },
        {
          name = 'version_string',
          desc = [[
            Returns the libuv version number as a string. For example, this would be "1.2.3"
            in libuv 1.2.3. For non-release versions, the version suffix is included.
          ]],
          returns = 'string',
        },
      },
    },
    {
      title = '`uv_loop_t` - Event loop',
      id = 'uv_loop_t--event-loop',
      desc = [[
        The event loop is the central part of libuv's functionality. It takes care of
        polling for I/O and scheduling callbacks to be run based on different sources of
        events.

        In luv, there is an implicit uv loop for every Lua state that loads the library.
        You can use this library in an multi-threaded environment as long as each thread
        has it's own Lua state with its corresponding own uv loop. This loop is not
        directly exposed to users in the Lua module.
      ]],
      funcs = {
        {
          name = 'loop_close',
          desc = [[
            Closes all internal loop resources. In normal execution, the loop will
            automatically be closed when it is garbage collected by Lua, so it is not
            necessary to explicitly call `loop_close()`. Call this function only after the
            loop has finished executing and all open handles and requests have been closed,
            or it will return `EBUSY`.
          ]],
          returns = success_ret,
        },
        {
          name = 'run',
          desc = [[
            This function runs the event loop. It will act differently depending on the
            specified mode:

              - `"default"`: Runs the event loop until there are no more active and
              referenced handles or requests. Returns `true` if `uv.stop()` was called and
              there are still active handles or requests. Returns `false` in all other
              cases.

              - `"once"`: Poll for I/O once. Note that this function blocks if there are no
              pending callbacks. Returns `false` when done (no active handles or requests
              left), or `true` if more callbacks are expected (meaning you should run the
              event loop again sometime in the future).

              - `"nowait"`: Poll for I/O once but don't block if there are no pending
              callbacks. Returns `false` if done (no active handles or requests left),
              or `true` if more callbacks are expected (meaning you should run the event
              loop again sometime in the future).

          ]],
          notes = {
            [[
              Luvit will implicitly call `uv.run()` after loading user code, but if
              you use the luv bindings directly, you need to call this after registering
              your initial set of event callbacks to start the event loop.
            ]],
          },
          params = {
            { name = 'mode', type = opt_str, default = '"default"' },
          },
          -- If loop is already running then returns (nil, string)
          returns = ret_or_fail('boolean', 'running'),
        },
        {
          name = 'loop_configure',
          desc = [[
            Set additional loop options. You should normally call this before the first call
            to uv_run() unless mentioned otherwise.

            Supported options:

              - `"block_signal"`: Block a signal when polling for new events. The second argument
              to loop_configure() is the signal name (as a lowercase string) or the signal number.
              This operation is currently only implemented for `"sigprof"` signals, to suppress
              unnecessary wakeups when using a sampling profiler. Requesting other signals will
              fail with `EINVAL`.
              - `"metrics_idle_time"`: Accumulate the amount of idle time the event loop spends
              in the event provider. This option is necessary to use `metrics_idle_time()`.

            An example of a valid call to this function is:

            ```lua
            uv.loop_configure("block_signal", "sigprof")
            ```
          ]],
          notes = {
            [[
              Be prepared to handle the `ENOSYS` error; it means the loop option is
              not supported by the platform.
            ]],
          },
          params = {
            { name = 'option', type = 'string' },
            { name = '...', type = 'any', desc = 'depends on `option`' },
          },
          returns = success_ret,
        },
        {
          name = 'loop_mode',
          desc = [[
            If the loop is running, returns a string indicating the mode in use. If the loop
            is not running, `nil` is returned instead.
          ]],
          returns = { { opt_str } },
        },
        {
          name = 'loop_alive',
          desc = [[
            Returns `true` if there are referenced active handles, active requests, or
            closing handles in the loop; otherwise, `false`.
          ]],
          returns = ret_or_fail('boolean', 'alive'),
        },
        {
          name = 'stop',
          desc = [[
            Stop the event loop, causing `uv.run()` to end as soon as possible. This
            will happen not sooner than the next loop iteration. If this function was called
            before blocking for I/O, the loop won't block for I/O on this iteration.
          ]],
        },
        {
          name = 'backend_fd',
          desc = [[
            Get backend file descriptor. Only kqueue, epoll, and event ports are supported.

            This can be used in conjunction with `uv.run("nowait")` to poll in one thread
            and run the event loop's callbacks in another
          ]],
          notes = {
            [[
              Embedding a kqueue fd in another kqueue pollset doesn't work on all
              platforms. It's not an error to add the fd but it never generates events.
            ]],
          },
          returns = { { opt_int } },
        },
        {
          name = 'backend_timeout',
          desc = [[
            Get the poll timeout. The return value is in milliseconds, or -1 for no timeout.
          ]],
          returns = 'integer',
        },
        {
          name = 'now',
          desc = [[
            Returns the current timestamp in milliseconds. The timestamp is cached at the
            start of the event loop tick, see `uv.update_time()` for details and rationale.

            The timestamp increases monotonically from some arbitrary point in time. Don't
            make assumptions about the starting point, you will only get disappointed.

          ]],
          notes = {
            'Use `uv.hrtime()` if you need sub-millisecond granularity.',
          },
          returns = 'integer',
        },
        {
          name = 'update_time',
          desc = [[
            Update the event loop's concept of "now". Libuv caches the current time at the
            start of the event loop tick in order to reduce the number of time-related
            system calls.

            You won't normally need to call this function unless you have callbacks that
            block the event loop for longer periods of time, where "longer" is somewhat
            subjective but probably on the order of a millisecond or more.
          ]],
        },
        {
          name = 'walk',
          desc = [[
            Walk the list of handles: `callback` will be executed with each handle.
          ]],
          example = [[
            ```lua
            -- Example usage of uv.walk to close all handles that aren't already closing.
            uv.walk(function (handle)
              if not handle:is_closing() then
                handle:close()
              end
            end)
            ```
          ]],
          params = {
            cb({ { 'handle', 'uv_handle_t' } }),
          },
        },
      },
    },
    {
      title = '`uv_req_t` - Base request',
      id = 'uv_req_t--base-request',
      class = 'uv_req_t',
      desc = '`uv_req_t` is the base type for all libuv request types.',
      funcs = {
        {
          name = 'cancel',
          method_form = 'req:cancel()',
          desc = [[
            Cancel a pending request. Fails if the request is executing or has finished
            executing. Only cancellation of `uv_fs_t`, `uv_getaddrinfo_t`,
            `uv_getnameinfo_t` and `uv_work_t` requests is currently supported.
          ]],
          params = {
            { name = 'req', type = 'uv_req_t' },
          },
          returns = success_ret,
        },
        {
          name = 'req_get_type',
          method_form = 'req:get_type()',
          desc = [[
            Returns the name of the struct for a given request (e.g. `"fs"` for `uv_fs_t`)
            and the libuv enum integer for the request's type (`uv_req_type`).
          ]],
          params = {
            { name = 'req', type = 'uv_req_t' },
          },
          returns = {
            { 'string', 'type' },
            { 'integer', 'enum' },
          },
        },
      },
    },
    {
      title = '`uv_handle_t` - Base handle',
      id = 'uv_handle_t--base-handle',
      class = 'uv_handle_t',
      desc = [[
        `uv_handle_t` is the base type for all libuv handle types. All API functions
        defined here work with any handle type.
      ]],
      funcs = {
        {
          name = 'is_active',
          method_form = 'handle:is_active()',
          desc = [[
            Returns `true` if the handle is active, `false` if it's inactive. What "active”
            means depends on the type of handle:

              - A [`uv_async_t`][] handle is always active and cannot be deactivated, except
              by closing it with `uv.close()`.

              - A [`uv_pipe_t`][], [`uv_tcp_t`][], [`uv_udp_t`][], etc. handle - basically
              any handle that deals with I/O - is active when it is doing something that
              involves I/O, like reading, writing, connecting, accepting new connections,
              etc.

              - A [`uv_check_t`][], [`uv_idle_t`][], [`uv_timer_t`][], etc. handle is active
              when it has been started with a call to `uv.check_start()`, `uv.idle_start()`,
              `uv.timer_start()` etc. until it has been stopped with a call to its
              respective stop function.
          ]],
          params = {
            { name = 'handle', type = 'uv_handle_t' },
          },
          returns = ret_or_fail('boolean', 'active'),
        },
        {
          name = 'is_closing',
          method_form = 'handle:is_closing()',
          desc = [[
            Returns `true` if the handle is closing or closed, `false` otherwise.
          ]],
          notes = {
            [[
              This function should only be used between the initialization of the
              handle and the arrival of the close callback.
            ]],
          },
          params = {
            { name = 'handle', type = 'uv_handle_t' },
          },
          returns = ret_or_fail('boolean', 'closing'),
        },
        {
          name = 'close',
          method_form = 'handle:close([callback])',
          desc = [[
            Request handle to be closed. `callback` will be called asynchronously after this
            call. This MUST be called on each handle before memory is released.

            Handles that wrap file descriptors are closed immediately but `callback` will
            still be deferred to the next iteration of the event loop. It gives you a chance
            to free up any resources associated with the handle.

            In-progress requests, like `uv_connect_t` or `uv_write_t`, are cancelled and
            have their callbacks called asynchronously with `ECANCELED`.
          ]],
          params = {
            { name = 'handle', type = 'uv_handle_t' },
            cb(nil, true),
          },
        },
        {
          name = 'ref',
          method_form = 'handle:ref()',
          desc = [[
            Reference the given handle. References are idempotent, that is, if a handle is
            already referenced calling this function again will have no effect.
          ]],
          see = 'Reference counting',
          params = {
            { name = 'handle', type = 'uv_handle_t' },
          },
        },
        {
          name = 'unref',
          method_form = 'handle:unref()',
          desc = [[
            Un-reference the given handle. References are idempotent, that is, if a handle
            is not referenced calling this function again will have no effect.
          ]],
          see = 'Reference counting',
          params = {
            { name = 'handle', type = 'uv_handle_t' },
          },
        },
        {
          name = 'has_ref',
          method_form = 'handle:has_ref()',
          desc = 'Returns `true` if the handle referenced, `false` if not.',
          see = 'Reference counting',
          params = {
            { name = 'handle', type = 'uv_handle_t' },
          },
          returns = ret_or_fail('boolean', 'has_ref'),
        },
        {
          name = 'send_buffer_size',
          method_form = 'handle:send_buffer_size([size])',
          desc = [[
            Gets or sets the size of the send buffer that the operating system uses for the
            socket.

            If `size` is omitted (or `0`), this will return the current send buffer size; otherwise, this will use `size` to set the new send buffer size.

            This function works for TCP, pipe and UDP handles on Unix and for TCP and UDP
            handles on Windows.
          ]],
          notes = {
            [[
              Linux will set double the size and return double the size of the
              original set value.
            ]],
          },
          params = {
            { name = 'handle', type = 'uv_handle_t' },
            { name = 'size', type = opt_int, default = '0' },
          },
          returns = ret_or_fail('integer', 'success'),
          returns_doc = [[
            - `integer` or `fail` (if `size` is `nil` or `0`)
            - `0` or `fail` (if `size` is not `nil` and not `0`)
          ]],
        },
        {
          name = 'recv_buffer_size',
          method_form = 'handle:recv_buffer_size([size])',
          desc = [[
            Gets or sets the size of the receive buffer that the operating system uses for
            the socket.

            If `size` is omitted (or `0`), this will return the current send buffer size; otherwise, this will use `size` to set the new send buffer size.

            This function works for TCP, pipe and UDP handles on Unix and for TCP and UDP
            handles on Windows.
          ]],
          notes = {
            [[
              Linux will set double the size and return double the size of the
              original set value.
            ]],
          },
          params = {
            { name = 'handle', type = 'uv_handle_t' },
            { name = 'size', type = opt_int, default = '0' },
          },
          returns = ret_or_fail('integer', 'success'),
          returns_doc = [[
            - `integer` or `fail` (if `size` is `nil` or `0`)
            - `0` or `fail` (if `size` is not `nil` and not `0`)
          ]],
        },
        {
          name = 'fileno',
          method_form = 'handle:fileno()',
          desc = [[
            Gets the platform dependent file descriptor equivalent.

            The following handles are supported: TCP, pipes, TTY, UDP and poll. Passing any
            other handle type will fail with `EINVAL`.

            If a handle doesn't have an attached file descriptor yet or the handle itself
            has been closed, this function will return `EBADF`.
          ]],
          warnings = {
            [[
              Be very careful when using this function. libuv assumes it's in
              control of the file descriptor so any change to it may lead to malfunction.
            ]],
          },
          params = {
            { name = 'handle', type = 'uv_handle_t' },
          },
          returns = ret_or_fail('integer', 'fileno'),
        },
        {
          name = 'handle_get_type',
          method_form = 'handle:get_type()',
          desc = [[
            Returns the name of the struct for a given handle (e.g. `"pipe"` for `uv_pipe_t`)
            and the libuv enum integer for the handle's type (`uv_handle_type`).
          ]],
          params = {
            { name = 'handle', type = 'uv_handle_t' },
          },
          returns = {
            { 'string', 'type' },
            { 'integer', 'enum' },
          },
        },
      },
    },
    {
      title = 'Reference counting',
      id = 'reference-counting',
      desc = [[
        The libuv event loop (if run in the default mode) will run until there are no
        active and referenced handles left. The user can force the loop to exit early by
        unreferencing handles which are active, for example by calling `uv.unref()`
        after calling `uv.timer_start()`.

        A handle can be referenced or unreferenced, the refcounting scheme doesn't use a
        counter, so both operations are idempotent.

        All handles are referenced when active by default, see `uv.is_active()` for a
        more detailed explanation on what being active involves.
      ]],
    },
    {
      title = '`uv_timer_t` - Timer handle',
      id = 'uv_timer_t--timer-handle',
      class = 'uv_timer_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Timer handles are used to schedule callbacks to be called in the future.
      ]],
      funcs = {
        {
          name = 'new_timer',
          desc = [[
            Creates and initializes a new `uv_timer_t`. Returns the Lua userdata wrapping
            it.
          ]],
          example = [[
            ```lua
            -- Creating a simple setTimeout wrapper
            local function setTimeout(timeout, callback)
              local timer = uv.new_timer()
              timer:start(timeout, 0, function ()
                timer:stop()
                timer:close()
                callback()
              end)
              return timer
            end

            -- Creating a simple setInterval wrapper
            local function setInterval(interval, callback)
              local timer = uv.new_timer()
              timer:start(interval, interval, function ()
                callback()
              end)
              return timer
            end

            -- And clearInterval
            local function clearInterval(timer)
              timer:stop()
              timer:close()
            end
            ```
          ]],
          returns = ret_or_fail('uv_timer_t', 'timer'),
        },
        {
          name = 'timer_start',
          method_form = 'timer:start(timeout, repeat, callback)',
          desc = [[
            Start the timer. `timeout` and `repeat` are in milliseconds.

            If `timeout` is zero, the callback fires on the next event loop iteration. If
            `repeat` is non-zero, the callback fires first after `timeout` milliseconds and
            then repeatedly after `repeat` milliseconds.
          ]],
          params = {
            { name = 'timer', type = 'uv_timer_t' },
            { name = 'timeout', type = 'integer' },
            { name = 'repeat', type = 'integer' },
            cb(),
          },
          returns = success_ret,
        },
        {
          name = 'timer_stop',
          method_form = 'timer:stop()',
          desc = 'Stop the timer, the callback will not be called anymore.',
          params = {
            { name = 'timer', type = 'uv_timer_t' },
          },
          returns = success_ret,
        },
        {
          name = 'timer_again',
          method_form = 'timer:again()',
          desc = [[
            Stop the timer, and if it is repeating restart it using the repeat value as the
            timeout. If the timer has never been started before it raises `EINVAL`.
          ]],
          params = {
            { name = 'timer', type = 'uv_timer_t' },
          },
          returns = success_ret,
        },
        {
          name = 'timer_set_repeat',
          method_form = 'timer:set_repeat(repeat)',
          desc = [[
            Set the repeat interval value in milliseconds. The timer will be scheduled to
            run on the given interval, regardless of the callback execution duration, and
            will follow normal timer semantics in the case of a time-slice overrun.

            For example, if a 50 ms repeating timer first runs for 17 ms, it will be
            scheduled to run again 33 ms later. If other tasks consume more than the 33 ms
            following the first timer callback, then the callback will run as soon as
            possible.

          ]],
          params = {
            { name = 'timer', type = 'uv_timer_t' },
            { name = 'repeat', type = 'integer' },
          },
        },
        {
          name = 'timer_get_repeat',
          method_form = 'timer:get_repeat()',
          desc = 'Get the timer repeat value.',
          params = {
            { name = 'timer', type = 'uv_timer_t' },
          },
          returns = { { 'integer', 'repeat' } },
        },
        {
          name = 'timer_get_due_in',
          method_form = 'timer:get_due_in()',
          desc = [[
            Get the timer due value or 0 if it has expired. The time is relative to `uv.now()`.
          ]],
          since = '1.40.0',
          params = {
            { name = 'timer', type = 'uv_timer_t' },
          },
          returns = { { 'integer', 'due_in' } },
        },
      },
    },
    {
      title = '`uv_prepare_t` - Prepare handle',
      id = 'uv_prepare_t--prepare-handle',
      class = 'uv_prepare_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Prepare handles will run the given callback once per loop iteration, right
        before polling for I/O.

        ```lua
        local prepare = uv.new_prepare()
        prepare:start(function()
          print("Before I/O polling")
        end)
        ```
      ]],
      funcs = {
        {
          name = 'new_prepare',
          desc = [[
            Creates and initializes a new `uv_prepare_t`. Returns the Lua userdata wrapping
            it.
          ]],
          returns = 'uv_prepare_t',
        },
        {
          name = 'prepare_start',
          method_form = 'prepare:start(callback)',
          desc = 'Start the handle with the given callback.',
          params = {
            { name = 'prepare', type = 'uv_prepare_t' },
            cb(),
          },
          returns = success_ret,
        },
        {
          name = 'prepare_stop',
          method_form = 'prepare:stop()',
          desc = 'Stop the handle, the callback will no longer be called.',
          params = {
            { name = 'prepare', type = 'uv_prepare_t' },
          },
          returns = success_ret,
        },
      },
    },
    {
      title = '`uv_check_t` - Check handle',
      id = 'uv_check_t--check-handle',
      class = 'uv_check_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Check handles will run the given callback once per loop iteration, right after
        polling for I/O.

        ```lua
        local check = uv.new_check()
        check:start(function()
          print("After I/O polling")
        end)
        ```
      ]],
      funcs = {
        {
          name = 'new_check',
          desc = [[
            Creates and initializes a new `uv_check_t`. Returns the Lua userdata wrapping
            it.
          ]],
          returns = 'uv_check_t',
        },
        {
          name = 'check_start',
          method_form = 'check:start(callback)',
          desc = 'Start the handle with the given callback.',
          params = {
            { name = 'check', type = 'uv_check_t' },
            cb(),
          },
          returns = success_ret,
        },
        {
          name = 'check_stop',
          method_form = 'check:stop()',
          desc = 'Stop the handle, the callback will no longer be called.',
          params = {
            { name = 'check', type = 'uv_check_t' },
          },
          returns = success_ret,
        },
      },
    },
    {
      title = '`uv_idle_t` - Idle handle',
      id = 'uv_idle_t--idle-handle',
      class = 'uv_idle_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Idle handles will run the given callback once per loop iteration, right before
        the [`uv_prepare_t`][] handles.

        **Note**: The notable difference with prepare handles is that when there are
        active idle handles, the loop will perform a zero timeout poll instead of
        blocking for I/O.

        **Warning**: Despite the name, idle handles will get their callbacks called on
        every loop iteration, not when the loop is actually "idle".

        ```lua
        local idle = uv.new_idle()
        idle:start(function()
          print("Before I/O polling, no blocking")
        end)
        ```
      ]],
      funcs = {
        {
          name = 'new_idle',
          desc = [[
            Creates and initializes a new `uv_idle_t`. Returns the Lua userdata wrapping
            it.
          ]],
          returns = 'uv_idle_t',
        },
        {
          name = 'idle_start',
          method_form = 'idle:start(callback)',
          desc = 'Start the handle with the given callback.',
          params = {
            { name = 'idle', type = 'uv_idle_t' },
            cb(),
          },
          returns = success_ret,
        },
        {
          name = 'idle_stop',
          method_form = 'idle:stop()',
          desc = 'Stop the handle, the callback will no longer be called.',
          params = {
            { name = 'idle', type = 'uv_idle_t' },
          },
          returns = success_ret,
        },
      },
    },
    {
      title = '`uv_async_t` - Async handle',
      id = 'uv_async_t--async-handle',
      class = 'uv_async_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Async handles allow the user to "wakeup" the event loop and get a callback
        called from another thread.

        ```lua
        local async
        async = uv.new_async(function()
          print("async operation ran")
          async:close()
        end)

        async:send()
        ```
      ]],
      funcs = {
        {
          name = 'new_async',
          desc = [[
            Creates and initializes a new `uv_async_t`. Returns the Lua userdata wrapping
            it.
          ]],
          notes = {
            [[
            Unlike other handle initialization functions, this immediately starts
            the handle.
            ]],
          },
          params = {
            {
              name = 'callback',
              type = fun({
                { '...', 'threadargs', 'passed to/from `uv.async_send(async, ...)`' },
              }),
            },
          },
          returns = ret_or_fail('uv_async_t', 'async'),
        },
        {
          name = 'async_send',
          method_form = 'async:send(...)',
          desc = "Wakeup the event loop and call the async handle's callback.",
          notes = {
            [[
              It's safe to call this function from any thread. The callback will be
              called on the loop thread.
            ]],
          },
          warnings = {
            [[
              libuv will coalesce calls to `uv.async_send(async)`, that is, not
              every call to it will yield an execution of the callback. For example: if
              `uv.async_send()` is called 5 times in a row before the callback is called, the
              callback will only be called once. If `uv.async_send()` is called again after
              the callback was called, it will be called again.
            ]],
          },
          params = {
            { name = 'async', type = 'uv_async_t' },
            { name = '...', type = 'threadargs' },
          },
          returns = success_ret,
        },
      },
    },
    {
      title = '`uv_poll_t` - Poll handle',
      id = 'uv_poll_t--poll-handle',
      class = 'uv_poll_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Poll handles are used to watch file descriptors for readability and writability,
        similar to the purpose of [poll(2)](http://linux.die.net/man/2/poll).

        The purpose of poll handles is to enable integrating external libraries that
        rely on the event loop to signal it about the socket status changes, like c-ares
        or libssh2. Using `uv_poll_t` for any other purpose is not recommended;
        `uv_tcp_t`, `uv_udp_t`, etc. provide an implementation that is faster and more
        scalable than what can be achieved with `uv_poll_t`, especially on Windows.

        It is possible that poll handles occasionally signal that a file descriptor is
        readable or writable even when it isn't. The user should therefore always be
        prepared to handle EAGAIN or equivalent when it attempts to read from or write
        to the fd.

        It is not okay to have multiple active poll handles for the same socket, this
        can cause libuv to busyloop or otherwise malfunction.

        The user should not close a file descriptor while it is being polled by an
        active poll handle. This can cause the handle to report an error, but it might
        also start polling another socket. However the fd can be safely closed
        immediately after a call to `uv.poll_stop()` or `uv.close()`.

        **Note**: On windows only sockets can be polled with poll handles. On Unix any
        file descriptor that would be accepted by poll(2) can be used.
      ]],
      funcs = {
        {
          name = 'new_poll',
          desc = [[
            Initialize the handle using a file descriptor.

            The file descriptor is set to non-blocking mode.
          ]],
          params = {
            { name = 'fd', type = 'integer' },
          },
          returns = ret_or_fail('uv_poll_t', 'poll'),
        },
        {
          name = 'new_socket_poll',
          desc = [[
            Initialize the handle using a socket descriptor. On Unix this is identical to
            `uv.new_poll()`. On windows it takes a SOCKET handle.

            The socket is set to non-blocking mode.
          ]],
          params = {
            { name = 'fd', type = 'integer' },
          },
          returns = ret_or_fail('uv_poll_t', 'poll'),
        },
        {
          name = 'poll_start',
          method_form = 'poll:start(events, callback)',
          desc = [[
            Starts polling the file descriptor. `events` are: `"r"`, `"w"`, `"rw"`, `"d"`,
            `"rd"`, `"wd"`, `"rwd"`, `"p"`, `"rp"`, `"wp"`, `"rwp"`, `"dp"`, `"rdp"`,
            `"wdp"`, or `"rwdp"` where `r` is `READABLE`, `w` is `WRITABLE`, `d` is
            `DISCONNECT`, and `p` is `PRIORITIZED`. As soon as an event is detected
            the callback will be called with status set to 0, and the detected events set on
            the events field.

            The user should not close the socket while the handle is active. If the user
            does that anyway, the callback may be called reporting an error status, but this
            is not guaranteed.

          ]],
          notes = {
            [[
              Calling `uv.poll_start()` on a handle that is already active is fine.
              Doing so will update the events mask that is being watched for.
            ]],
          },
          params = {
            { name = 'poll', type = 'uv_poll_t' },
            { name = 'events', type = opt_str, default = '"rw"' },
            {
              name = 'callback',
              type = fun({
                { 'err', opt_str },
                { 'events', opt_str },
              }),
            },
          },
          returns = success_ret,
        },
        {
          name = 'poll_stop',
          method_form = 'poll:stop()',
          desc = [[
            Stop polling the file descriptor, the callback will no longer be called.
          ]],
          params = {
            { name = 'poll', type = 'uv_poll_t' },
          },
          returns = success_ret,
        },
      },
    },
    {
      title = '`uv_signal_t` - Signal handle',
      id = 'uv_signal_t--signal-handle',
      class = 'uv_signal_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Signal handles implement Unix style signal handling on a per-event loop bases.

        **Windows Notes:**

        Reception of some signals is emulated on Windows:
          - SIGINT is normally delivered when the user presses CTRL+C. However, like on
          Unix, it is not generated when terminal raw mode is enabled.
          - SIGBREAK is delivered when the user pressed CTRL + BREAK.
          - SIGHUP is generated when the user closes the console window. On SIGHUP the
          program is given approximately 10 seconds to perform cleanup. After that
          Windows will unconditionally terminate it.
          - SIGWINCH is raised whenever libuv detects that the console has been resized.
          SIGWINCH is emulated by libuv when the program uses a uv_tty_t handle to write
          to the console. SIGWINCH may not always be delivered in a timely manner; libuv
          will only detect size changes when the cursor is being moved. When a readable
          [`uv_tty_t`][] handle is used in raw mode, resizing the console buffer will
          also trigger a SIGWINCH signal.
          - Watchers for other signals can be successfully created, but these signals
          are never received. These signals are: SIGILL, SIGABRT, SIGFPE, SIGSEGV,
          SIGTERM and SIGKILL.
          - Calls to raise() or abort() to programmatically raise a signal are not
          detected by libuv; these will not trigger a signal watcher.

        **Unix Notes:**

          - SIGKILL and SIGSTOP are impossible to catch.
          - Handling SIGBUS, SIGFPE, SIGILL or SIGSEGV via libuv results into undefined
          behavior.
          - SIGABRT will not be caught by libuv if generated by abort(), e.g. through
          assert().
          - On Linux SIGRT0 and SIGRT1 (signals 32 and 33) are used by the NPTL pthreads
          library to manage threads. Installing watchers for those signals will lead to
          unpredictable behavior and is strongly discouraged. Future versions of libuv
          may simply reject them.

        ```lua
        -- Create a new signal handler
        local signal = uv.new_signal()
        -- Define a handler function
        uv.signal_start(signal, "sigint", function(signame)
          print("got " .. signame .. ", shutting down")
          os.exit(1)
        end)
        ```
      ]],
      funcs = {
        {
          name = 'new_signal',
          desc = [[
            Creates and initializes a new `uv_signal_t`. Returns the Lua userdata wrapping
            it.
          ]],
          returns = ret_or_fail('uv_signal_t', 'signal'),
        },
        {
          name = 'signal_start',
          method_form = 'signal:start(signame, callback)',
          desc = [[
            Start the handle with the given callback, watching for the given signal.

            See [Constants][] for supported `signame` input and output values.
          ]],
          params = {
            { name = 'signal', type = 'uv_signal_t' },
            { name = 'signame', type = 'string|integer' },
            {
              name = 'callback',
              type = fun({ { 'signame', 'string' } }),
            },
          },
          returns = success_ret,
        },
        {
          name = 'signal_start_oneshot',
          method_form = 'signal:start_oneshot(signame, callback)',
          desc = [[
            Same functionality as `uv.signal_start()` but the signal handler is reset the moment the signal is received.

            See [Constants][] for supported `signame` input and output values.
          ]],
          params = {
            { name = 'signal', type = 'uv_signal_t' },
            { name = 'signame', type = 'string|integer' },
            {
              name = 'callback',
              type = fun({ { 'signame', 'string' } }),
            },
          },
          returns = success_ret,
        },
        {
          name = 'signal_stop',
          method_form = 'signal:stop()',
          desc = 'Stop the handle, the callback will no longer be called.',
          params = {
            { name = 'signal', type = 'uv_signal_t' },
          },
          returns = success_ret,
        },
      },
    },
    {
      title = '`uv_process_t` - Process handle',
      id = 'uv_process_t--process-handle',
      class = 'uv_process_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Process handles will spawn a new process and allow the user to control it and
        establish communication channels with it using streams.
      ]],
      funcs = {
        {
          name = 'disable_stdio_inheritance',
          desc = [[
            Disables inheritance for file descriptors / handles that this process inherited
            from its parent. The effect is that child processes spawned by this process
            don't accidentally inherit these handles.

            It is recommended to call this function as early in your program as possible,
            before the inherited file descriptors can be closed or duplicated.
          ]],
          notes = {
            [[
              This function works on a best-effort basis: there is no guarantee that
              libuv can discover all file descriptors that were inherited. In general it does
              a better job on Windows than it does on Unix.
            ]],
          },
        },
        {
          name = 'spawn',
          desc = [[
            Initializes the process handle and starts the process. If the process is
            successfully spawned, this function will return the handle and pid of the child
            process.

            Possible reasons for failing to spawn would include (but not be limited to) the
            file to execute not existing, not having permissions to use the setuid or setgid
            specified, or not having enough memory to allocate for the new process.

            ```lua
            local stdin = uv.new_pipe()
            local stdout = uv.new_pipe()
            local stderr = uv.new_pipe()

            print("stdin", stdin)
            print("stdout", stdout)
            print("stderr", stderr)

            local handle, pid = uv.spawn("cat", {
              stdio = {stdin, stdout, stderr}
            }, function(code, signal) -- on exit
              print("exit code", code)
              print("exit signal", signal)
            end)

            print("process opened", handle, pid)

            uv.read_start(stdout, function(err, data)
              assert(not err, err)
              if data then
                print("stdout chunk", stdout, data)
              else
                print("stdout end", stdout)
              end
            end)

            uv.read_start(stderr, function(err, data)
              assert(not err, err)
              if data then
                print("stderr chunk", stderr, data)
              else
                print("stderr end", stderr)
              end
            end)

            uv.write(stdin, "Hello World")

            uv.shutdown(stdin, function()
              print("stdin shutdown", stdin)
              uv.close(handle, function()
                print("process closed", handle, pid)
              end)
            end)
            ```

            When the child process exits, `on_exit` is called with an exit code and signal.
          ]],
          params = {
            { name = 'path', type = 'string' },
            {
              name = 'options',
              type = table({
                {
                  'args',
                  opt('string[]'),
                  nil,
                  [[
                    Command line arguments as a list of strings. The first
                    string should *not* be the path to the program, since that is already
                    provided via `path`. On Windows, this uses CreateProcess which concatenates
                    the arguments into a string. This can cause some strange errors
                    (see `options.verbatim` below for Windows).
                  ]],
                },
                {
                  'stdio',
                  opt(dict('integer', opt(union('integer', 'uv_stream_t')))),
                  nil,
                  [[
                    Set the file descriptors that will be made available to
                    the child process. The convention is that the first entries are stdin, stdout,
                    and stderr.

                    The entries can take many shapes.
                    - If `integer`, then the child process inherits that same zero-indexed
                      fd from the parent process.
                    - If `uv_stream_t` handles are passed in, those are used as a read-write pipe
                      or inherited stream depending if the stream has a valid fd.
                    - If `nil`, means to ignore that fd in the child process.

                    **Note**: On Windows, file descriptors after the third are
                    available to the child process only if the child processes uses the MSVCRT
                    runtime.
                  ]],
                },
                {
                  'env',
                  opt('string[]'),
                  nil,
                  [[
                    Set environment variables for the new process.
                    Each entry should be a string in the form of `NAME=VALUE`.
                  ]]
                },
                {
                  'cwd',
                  opt_str,
                  nil,
                  'Set the current working directory for the sub-process.',
                },
                {
                  'uid',
                  opt_str,
                  nil,
                  "Set the child process' user id.",
                },
                {
                  'gid',
                  opt_str,
                  nil,
                  "Set the child process' group id.",
                },
                {
                  'verbatim',
                  opt_bool,
                  nil,
                  [[
                    If true, do not wrap any arguments in quotes, or
                    perform any other escaping, when converting the argument list into a command
                    line string. This option is only meaningful on Windows systems. On Unix it is
                    silently ignored.
                  ]],
                },
                {
                  'detached',
                  opt_bool,
                  nil,
                  [[
                    If true, spawn the child process in a detached state -
                    this will make it a process group leader, and will effectively enable the
                    child to keep running after the parent exits. Note that the child process
                    will still keep the parent's event loop alive unless the parent process calls
                    `uv.unref()` on the child's process handle.
                  ]],
                },
                {
                  'hide',
                  opt_bool,
                  nil,
                  [[
                    If true, hide the subprocess console window that would
                    normally be created. This option is only meaningful on Windows systems. On
                    Unix it is silently ignored.
                  ]],
                },
              }),
            },
            {
              name = 'on_exit',
              type = fun({ { 'code', 'integer' }, { 'signal', 'integer' } }),
            },
          },
          returns = {
            { opt('uv_process_t'), 'handle' },
            { union('integer', 'string'), 'pid or err' },
            { opt('uv.error_name'), 'err_name' },
          },
        },
        {
          name = 'process_kill',
          method_form = 'process:kill(signame)',
          desc = [[
            Sends the specified signal to the given process handle. Check the documentation
            on `uv_signal_t` for signal support, specially on Windows.

            See [Constants][] for supported `signame` input values.
          ]],
          params = {
            { name = 'process', type = 'uv_process_t' },
            { name = 'signame', type = opt(union('string', 'integer')), default = 'sigterm' },
          },
          returns = success_ret,
        },
        {
          name = 'kill',
          desc = [[
            Sends the specified signal to the given PID. Check the documentation on
            `uv_signal_t` for signal support, specially on Windows.

            See [Constants][] for supported `signame` input values.
          ]],
          params = {
            { name = 'pid', type = 'integer' },
            { name = 'signame', type = opt(union('string', 'integer')), default = 'sigterm' },
          },
          returns = success_ret,
        },
        {
          name = 'process_get_pid',
          method_form = 'process:get_pid()',
          desc = "Returns the handle's pid.",
          params = {
            { name = 'process', type = 'uv_process_t' },
          },
          returns = 'integer',
        },
      },
    },
    {
      title = '`uv_stream_t` - Stream handle',
      id = 'uv_stream_t--stream-handle',
      class = 'uv_stream_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        Stream handles provide an abstraction of a duplex communication channel.
        [`uv_stream_t`][] is an abstract type, libuv provides 3 stream implementations
        in the form of [`uv_tcp_t`][], [`uv_pipe_t`][] and [`uv_tty_t`][].
      ]],
      funcs = {
        {
          name = 'shutdown',
          method_form = 'stream:shutdown([callback])',
          desc = [[
            Shutdown the outgoing (write) side of a duplex stream. It waits for pending
            write requests to complete. The callback is called after shutdown is complete.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            cb_err({}, true),
          },
          returns = ret_or_fail('uv_shutdown_t', 'shutdown'),
        },
        {
          name = 'listen',
          method_form = 'stream:listen(backlog, callback)',
          desc = [[
            Start listening for incoming connections. `backlog` indicates the number of
            connections the kernel might queue, same as `listen(2)`. When a new incoming
            connection is received the callback is called.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            { name = 'backlog', type = 'integer' },
            cb_err(),
          },
          returns = success_ret,
        },
        {
          name = 'accept',
          method_form = 'stream:accept(client_stream)',
          desc = [[
            This call is used in conjunction with `uv.listen()` to accept incoming
            connections. Call this function after receiving a callback to accept the
            connection.

            When the connection callback is called it is guaranteed that this function
            will complete successfully the first time. If you attempt to use it more than
            once, it may fail. It is suggested to only call this function once per
            connection call.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            { name = 'client_stream', type = 'uv_stream_t' },
          },
          returns = success_ret,
          example = [[
            ```lua
            server:listen(128, function (err)
              local client = uv.new_tcp()
              server:accept(client)
            end)
            ```
          ]],
        },
        {
          name = 'read_start',
          method_form = 'stream:read_start(callback)',
          desc = [[
            Read data from an incoming stream. The callback will be made several times until
            there is no more data to read or `uv.read_stop()` is called. When we've reached
            EOF, `data` will be `nil`.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            {
              name = 'callback',
              type = fun({
                { 'err', opt_str },
                { 'data', opt_str },
              }),
            },
          },
          returns = success_ret,
          example = [[
            ```lua
            stream:read_start(function (err, chunk)
              if err then
                -- handle read error
              elseif chunk then
                -- handle data
              else
                -- handle disconnect
              end
            end)
            ```
          ]],
        },
        {
          name = 'read_stop',
          method_form = 'stream:read_stop()',
          desc = [[
            Stop reading data from the stream. The read callback will no longer be called.

            This function is idempotent and may be safely called on a stopped stream.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
          },
          returns = success_ret,
        },
        {
          name = 'write',
          method_form = 'stream:write(data, [callback])',
          desc = [[
            Write data to stream.

            `data` can either be a Lua string or a table of strings. If a table is passed
            in, the C backend will use writev to send all strings in a single system call.

            The optional `callback` is for knowing when the write is complete.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            { name = 'data', type = 'buffer' },
            cb_err({}, true),
          },
          returns = ret_or_fail('uv_write_t', 'write'),
        },
        {
          name = 'write2',
          method_form = 'stream:write2(data, send_handle, [callback])',
          desc = [[
            Extended write function for sending handles over a pipe. The pipe must be
            initialized with `ipc` option `true`.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            { name = 'data', type = 'buffer' },
            { name = 'send_handle', type = 'uv_stream_t' },
            cb_err({}, true),
          },
          returns = ret_or_fail('uv_write_t', 'write'),
          notes = {
            [[
              `send_handle` must be a TCP socket or pipe, which is a server or a
              connection (listening or connected state). Bound sockets or pipes will be
              assumed to be servers.
            ]],
          },
        },
        {
          name = 'try_write',
          method_form = 'stream:try_write(data)',
          desc = [[
            Same as `uv.write()`, but won't queue a write request if it can't be completed
            immediately.

            Will return number of bytes written (can be less than the supplied buffer size).
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            { name = 'data', type = 'buffer' },
          },
          returns = ret_or_fail('integer', 'bytes_written'),
        },
        {
          name = 'try_write2',
          method_form = 'stream:try_write2(data, send_handle)',
          desc = [[
            Like `uv.write2()`, but with the properties of `uv.try_write()`. Not supported on Windows, where it returns `UV_EAGAIN`.

            Will return number of bytes written (can be less than the supplied buffer size).
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            { name = 'data', type = 'buffer' },
            { name = 'send_handle', type = 'uv_stream_t' },
          },
          returns = ret_or_fail('integer', 'bytes_written'),
        },
        {
          name = 'is_readable',
          method_form = 'stream:is_readable()',
          desc = 'Returns `true` if the stream is readable, `false` otherwise.',
          params = {
            { name = 'stream', type = 'uv_stream_t' },
          },
          returns = 'boolean',
        },
        {
          name = 'is_writable',
          method_form = 'stream:is_writable()',
          desc = 'Returns `true` if the stream is writable, `false` otherwise.',
          params = {
            { name = 'stream', type = 'uv_stream_t' },
          },
          returns = 'boolean',
        },
        {
          name = 'stream_set_blocking',
          method_form = 'stream:set_blocking(blocking)',
          desc = [[
            Enable or disable blocking mode for a stream.

            When blocking mode is enabled all writes complete synchronously. The interface
            remains unchanged otherwise, e.g. completion or failure of the operation will
            still be reported through a callback which is made asynchronously.
          ]],
          params = {
            { name = 'stream', type = 'uv_stream_t' },
            { name = 'blocking', type = 'boolean' },
          },
          returns = success_ret,
          warnings = {
            [[
              Relying too much on this API is not recommended. It is likely to
              change significantly in the future. Currently this only works on Windows and
              only for `uv_pipe_t` handles. Also libuv currently makes no ordering guarantee
              when the blocking mode is changed after write requests have already been
              submitted. Therefore it is recommended to set the blocking mode immediately
              after opening or creating the stream.
            ]],
          },
        },
        {
          name = 'stream_get_write_queue_size',
          method_form = 'stream:get_write_queue_size()',
          desc = "Returns the stream's write queue size.",
          params = {
            { name = 'stream', type = 'uv_stream_t' },
          },
          returns = 'integer',
        },
      },
    },
    {
      title = '`uv_tcp_t` - TCP handle',
      id = 'uv_tcp_t--tcp-handle',
      class = 'uv_tcp_t',
      desc = [[
        > [`uv_handle_t`][] and [`uv_stream_t`][] functions also apply.

        TCP handles are used to represent both TCP streams and servers.
      ]],
      funcs = {
        {
          name = 'new_tcp',
          desc = [[
            Creates and initializes a new `uv_tcp_t`. Returns the Lua userdata wrapping it.

            If set, `flags` must be a valid address family. See [Constants][] for supported
            address family input values.
          ]],
          params = {
            { name = 'flags', type = opt(union('string', 'integer')) },
          },
          returns = ret_or_fail('uv_tcp_t', 'tcp'),
        },
        {
          name = 'tcp_open',
          method_form = 'tcp:open(sock)',
          desc = 'Open an existing file descriptor or SOCKET as a TCP handle.',
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
            { name = 'sock', type = 'integer' },
          },
          returns = success_ret,
          notes = {
            [[
              The passed file descriptor or SOCKET is not checked for its type, but it's required that it represents a valid stream socket.
            ]],
          },
        },
        {
          name = 'tcp_nodelay',
          method_form = 'tcp:nodelay(enable)',
          desc = "Enable / disable Nagle's algorithm.",
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
            { name = 'enable', type = 'boolean' },
          },
          returns = success_ret,
        },
        {
          name = 'tcp_keepalive',
          method_form = 'tcp:keepalive(enable, [delay])',
          desc = [[
            Enable / disable TCP keep-alive. `delay` is the initial delay in seconds,
            ignored when enable is `false`.
          ]],
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
            { name = 'enable', type = 'boolean' },
            { name = 'delay', type = opt_int },
          },
          returns = success_ret,
        },
        {
          name = 'tcp_simultaneous_accepts',
          method_form = 'tcp:simultaneous_accepts(enable)',
          desc = [[
            Enable / disable simultaneous asynchronous accept requests that are queued by
            the operating system when listening for new TCP connections.

            This setting is used to tune a TCP server for the desired performance. Having
            simultaneous accepts can significantly improve the rate of accepting connections
            (which is why it is enabled by default) but may lead to uneven load distribution
            in multi-process setups.
          ]],
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
            { name = 'enable', type = 'boolean' },
          },
          returns = success_ret,
        },
        {
          name = 'tcp_bind',
          method_form = 'tcp:bind(host, port, [flags])',
          desc = [[
            Bind the handle to an host and port. `host` should be an IP address and
            not a domain name. Any `flags` are set with a table with field `ipv6only`
            equal to `true` or `false`.

            When the port is already taken, you can expect to see an `EADDRINUSE` error
            from either `uv.tcp_bind()`, `uv.listen()` or `uv.tcp_connect()`. That is, a
            successful call to this function does not guarantee that the call to `uv.listen()`
            or `uv.tcp_connect()` will succeed as well.

            Use a port of `0` to let the OS assign an ephemeral port.  You can look it up
            later using `uv.tcp_getsockname()`.
          ]],
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
            { name = 'host', type = 'string' },
            { name = 'port', type = 'integer' },
            {
              name = 'flags',
              type = opt(table({
                { 'ipv6only', 'boolean' },
              })),
            },
          },
          returns = success_ret,
        },
        {
          name = 'tcp_getpeername',
          method_form = 'tcp:getpeername()',
          desc = [[
            Get the address of the peer connected to the handle.

            See [Constants][] for supported address `family` output values.
          ]],
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
          },
          returns = ret_or_fail('socketinfo', 'address'),
        },
        {
          name = 'tcp_getsockname',
          method_form = 'tcp:getsockname()',
          desc = [[
            Get the current address to which the handle is bound.

            See [Constants][] for supported address `family` output values.
          ]],
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
          },
          returns = ret_or_fail('socketinfo', 'address'),
        },
        {
          name = 'tcp_connect',
          method_form = 'tcp:connect(host, port, callback)',
          desc = 'Establish an IPv4 or IPv6 TCP connection.',
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
            { name = 'host', type = 'string' },
            { name = 'port', type = 'integer' },
            cb_err(),
          },
          returns = ret_or_fail('uv_connect_t', 'connect'),
          example = [[
            ```lua
            local client = uv.new_tcp()
            client:connect("127.0.0.1", 8080, function (err)
              -- check error and carry on.
            end)
            ```
          ]],
        },
        {
          name = 'tcp_write_queue_size',
          method_form = 'tcp:write_queue_size()',
          deprecated = 'Please use `uv.stream_get_write_queue_size()` instead.',
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
          },
        },
        {
          name = 'tcp_close_reset',
          method_form = 'tcp:close_reset([callback])',
          desc = [[
            Resets a TCP connection by sending a RST packet. This is accomplished by setting
            the SO_LINGER socket option with a linger interval of zero and then calling
            `uv.close()`. Due to some platform inconsistencies, mixing of `uv.shutdown()`
            and `uv.tcp_close_reset()` calls is not allowed.
          ]],
          params = {
            { name = 'tcp', type = 'uv_tcp_t' },
            cb(nil, true),
          },
          returns = success_ret,
        },
        {
          name = 'socketpair',
          desc = [[
            Create a pair of connected sockets with the specified properties. The resulting handles can be passed to `uv.tcp_open`, used with `uv.spawn`, or for any other purpose.

            See [Constants][] for supported `socktype` input values.

            When `protocol` is set to 0 or nil, it will be automatically chosen based on the socket's domain and type. When `protocol` is specified as a string, it will be looked up using the `getprotobyname(3)` function (examples: `"ip"`, `"icmp"`, `"tcp"`, `"udp"`, etc).

            Flags:
             - `nonblock`: Opens the specified socket handle for `OVERLAPPED` or `FIONBIO`/`O_NONBLOCK` I/O usage. This is recommended for handles that will be used by libuv, and not usually recommended otherwise.

            Equivalent to `socketpair(2)` with a domain of `AF_UNIX`.
          ]],
          params = {
            { name = 'socktype', type = opt(union('string', 'integer')), default = 'stream' },
            { name = 'protocol', type = opt(union('string', 'integer')), default = '0' },
            {
              name = 'flags1',
              type = opt(table({ { 'nonblock', 'boolean', 'false' } })),
            },
            {
              name = 'flags2',
              type = opt(table({ { 'nonblock', 'boolean', 'false' } })),
            },
          },
          returns = {
            { opt('[integer, integer]'), 'fds' },
            { opt_str, 'err' },
            { opt('uv.error_name'), 'err_name' },
          },
          example = [[
            ```lua
            -- Simple read/write with tcp
            local fds = uv.socketpair(nil, nil, {nonblock=true}, {nonblock=true})

            local sock1 = uv.new_tcp()
            sock1:open(fds[1])

            local sock2 = uv.new_tcp()
            sock2:open(fds[2])

            sock1:write("hello")
            sock2:read_start(function(err, chunk)
              assert(not err, err)
              print(chunk)
            end)
            ```
          ]],
        },
      },
    },
    {
      title = '`uv_pipe_t` - Pipe handle',
      id = 'uv_pipe_t--pipe-handle',
      class = 'uv_pipe_t',
      desc = [[
        > [`uv_handle_t`][] and [`uv_stream_t`][] functions also apply.

        Pipe handles provide an abstraction over local domain sockets on Unix and named pipes on Windows.

        ```lua
        local pipe = uv.new_pipe(false)

        pipe:bind('/tmp/sock.test')

        pipe:listen(128, function()
          local client = uv.new_pipe(false)
          pipe:accept(client)
          client:write("hello!\n")
          client:close()
        end)
        ```
      ]],
      funcs = {
        {
          name = 'new_pipe',
          desc = [[
            Creates and initializes a new `uv_pipe_t`. Returns the Lua userdata wrapping
            it. The `ipc` argument is a boolean to indicate if this pipe will be used for
            handle passing between processes.
          ]],
          params = {
            { name = 'ipc', type = opt_bool, default = 'false' },
          },
          returns = ret_or_fail('uv_pipe_t', 'pipe'),
        },
        {
          name = 'pipe_open',
          method_form = 'pipe:open(fd)',
          desc = [[
            Open an existing file descriptor or [`uv_handle_t`][] as a pipe.
          ]],
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
            { name = 'fd', type = 'integer' },
          },
          returns = success_ret,
          notes = {
            'The file descriptor is set to non-blocking mode.',
          },
        },
        {
          name = 'pipe_bind',
          method_form = 'pipe:bind(name)',
          desc = 'Bind the pipe to a file path (Unix) or a name (Windows).',
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
            { name = 'name', type = 'string' },
          },
          returns = success_ret,
          notes = {
            [[
              Paths on Unix get truncated to sizeof(sockaddr_un.sun_path) bytes,
              typically between 92 and 108 bytes.
            ]],
          },
        },
        {
          name = 'pipe_connect',
          method_form = 'pipe:connect(name, [callback])',
          desc = 'Connect to the Unix domain socket or the named pipe.',
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
            { name = 'name', type = 'string' },
            cb_err({}, true),
          },
          returns = ret_or_fail('uv_connect_t', 'connect'),
          notes = {
            [[
              Paths on Unix get truncated to sizeof(sockaddr_un.sun_path) bytes,
              typically between 92 and 108 bytes.
            ]],
          },
        },
        {
          name = 'pipe_getsockname',
          method_form = 'pipe:getsockname()',
          desc = 'Get the name of the Unix domain socket or the named pipe.',
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
          },
          returns = ret_or_fail('string', 'name'),
        },
        {
          name = 'pipe_getpeername',
          method_form = 'pipe:getpeername()',
          desc = [[
            Get the name of the Unix domain socket or the named pipe to which the handle is
            connected.
          ]],
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
          },
          returns = ret_or_fail('string', 'name'),
        },
        {
          name = 'pipe_pending_instances',
          method_form = 'pipe:pending_instances(count)',
          desc = [[
            Set the number of pending pipe instance handles when the pipe server is waiting
            for connections.
          ]],
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
            { name = 'count', type = 'integer' },
          },
          notes = {
            'This setting applies to Windows only.',
          },
        },
        {
          name = 'pipe_pending_count',
          method_form = 'pipe:pending_count()',
          desc = 'Returns the pending pipe count for the named pipe.',
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
          },
          returns = 'integer',
        },
        {
          name = 'pipe_pending_type',
          method_form = 'pipe:pending_type()',
          desc = [[
            Used to receive handles over IPC pipes.

            First - call `uv.pipe_pending_count()`, if it's > 0 then initialize a handle of
            the given type, returned by `uv.pipe_pending_type()` and call
            `uv.accept(pipe, handle)`.
          ]],
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
          },
          returns = 'string',
        },
        {
          name = 'pipe_chmod',
          method_form = 'pipe:chmod(flags)',
          desc = [[
            Alters pipe permissions, allowing it to be accessed from processes run by different users.
            Makes the pipe writable or readable by all users. `flags` are: `"r"`, `"w"`, `"rw"`, or `"wr"`
            where `r` is `READABLE` and `w` is `WRITABLE`. This function is blocking.
          ]],
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
            { name = 'flags', type = 'string' },
          },
          returns = success_ret,
        },
        {
          name = 'pipe',
          desc = [[
            Create a pair of connected pipe handles. Data may be written to the `write` fd and read from the `read` fd. The resulting handles can be passed to `pipe_open`, used with `spawn`, or for any other purpose.

            Flags:
             - `nonblock`: Opens the specified socket handle for `OVERLAPPED` or `FIONBIO`/`O_NONBLOCK` I/O usage. This is recommended for handles that will be used by libuv, and not usually recommended otherwise.

            Equivalent to `pipe(2)` with the `O_CLOEXEC` flag set.
          ]],
          params = {
            {
              name = 'read_flags',
              type = opt(table({ { 'nonblock', 'boolean', 'false' } })),
            },
            {
              name = 'write_flags',
              type = opt(table({ { 'nonblock', 'boolean', 'false' } })),
            },
          },
          returns = ret_or_fail(
            table({
              { 'read', 'integer', nil, '(file descriptor)' },
              { 'write', 'integer', nil, '(file descriptor)' },
            }),
            'fds'
          ),
          example = [[
            ```lua
            -- Simple read/write with pipe_open
            local fds = uv.pipe({nonblock=true}, {nonblock=true})

            local read_pipe = uv.new_pipe()
            read_pipe:open(fds.read)

            local write_pipe = uv.new_pipe()
            write_pipe:open(fds.write)

            write_pipe:write("hello")
            read_pipe:read_start(function(err, chunk)
              assert(not err, err)
              print(chunk)
            end)
            ```
          ]],
        },
        {
          name = 'pipe_bind2',
          method_form = 'pipe:bind2(name, [flags])',
          desc = [[
            Bind the pipe to a file path (Unix) or a name (Windows).

            `Flags`:

            - If `type(flags)` is `number`, it must be `0` or `uv.constants.PIPE_NO_TRUNCATE`.
            - If `type(flags)` is `table`, it must be `{}` or `{ no_truncate = true|false }`.
            - If `type(flags)` is `nil`, it use default value `0`.
            - Returns `EINVAL` for unsupported flags without performing the bind operation.

            Supports Linux abstract namespace sockets. namelen must include the leading '\0' byte but not the trailing nul byte.
          ]],
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
            { name = 'name', type = 'string' },
            { name = 'flags', type = opt(union('integer', 'table')), default = '0' },
          },
          returns = success_ret,
          notes = {
            [[
              1. Paths on Unix get truncated to sizeof(sockaddr_un.sun_path) bytes,
              typically between 92 and 108 bytes.
              2. New in version 1.46.0.
            ]],
          },
        },
        {
          name = 'pipe_connect2',
          method_form = 'pipe:connect2(name, [flags], [callback])',
          desc = [[
            Connect to the Unix domain socket or the named pipe.

            `Flags`:

            - If `type(flags)` is `number`, it must be `0` or `uv.constants.PIPE_NO_TRUNCATE`.
            - If `type(flags)` is `table`, it must be `{}` or `{ no_truncate = true|false }`.
            - If `type(flags)` is `nil`, it use default value `0`.
            - Returns `EINVAL` for unsupported flags without performing the bind operation.

            Supports Linux abstract namespace sockets. namelen must include the leading nul byte but not the trailing nul byte.
          ]],
          params = {
            { name = 'pipe', type = 'uv_pipe_t' },
            { name = 'name', type = 'string' },
            { name = 'flags', type = opt(union('integer', 'table')), default = '0' },
            cb_err({}, true),
          },
          returns = ret_or_fail('uv_connect_t', 'connect'),
          notes = {
            [[
              1. Paths on Unix get truncated to sizeof(sockaddr_un.sun_path) bytes,
              typically between 92 and 108 bytes.
              2. New in version 1.46.0.
            ]],
          },
        },
      },
    },
    {
      title = '`uv_tty_t` - TTY handle',
      id = 'uv_tty_t--tty-handle',
      class = 'uv_tty_t',
      desc = [[
        > [`uv_handle_t`][] and [`uv_stream_t`][] functions also apply.

        TTY handles represent a stream for the console.

        ```lua
        -- Simple echo program
        local stdin = uv.new_tty(0, true)
        local stdout = uv.new_tty(1, false)

        stdin:read_start(function (err, data)
          assert(not err, err)
          if data then
            stdout:write(data)
          else
            stdin:close()
            stdout:close()
          end
        end)
        ```
      ]],
      funcs = {
        {
          name = 'new_tty',
          desc = [[
            Initialize a new TTY stream with the given file descriptor. Usually the file
            descriptor will be:

             - 0 - stdin
             - 1 - stdout
             - 2 - stderr

            On Unix this function will determine the path of the fd of the terminal using
            ttyname_r(3), open it, and use it if the passed file descriptor refers to a TTY.
            This lets libuv put the tty in non-blocking mode without affecting other
            processes that share the tty.

            This function is not thread safe on systems that don’t support ioctl TIOCGPTN or TIOCPTYGNAME, for instance OpenBSD and Solaris.
          ]],
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'readable', type = 'boolean' },
          },
          returns = ret_or_fail('uv_tty_t', 'tty'),
          notes = {
            'If reopening the TTY fails, libuv falls back to blocking writes.',
          },
        },
        {
          name = 'tty_set_mode',
          method_form = 'tty:set_mode(mode)',
          desc = [[
            Set the TTY using the specified terminal mode.

            See [Constants][] for supported TTY mode input values.
          ]],
          params = {
            { name = 'tty', type = 'uv_tty_t' },
            { name = 'mode', type = 'string|integer' },
          },
          returns = success_ret,
        },
        {
          name = 'tty_reset_mode',
          desc = [[
            To be called when the program exits. Resets TTY settings to default values for
            the next process to take over.

            This function is async signal-safe on Unix platforms but can fail with error
            code `EBUSY` if you call it when execution is inside `uv.tty_set_mode()`.
          ]],
          returns = success_ret,
        },
        {
          name = 'tty_get_winsize',
          method_form = 'tty:get_winsize()',
          desc = 'Gets the current Window width and height.',
          params = {
            { name = 'tty', type = 'uv_tty_t' },
          },
          returns_doc = '`integer, integer` or `fail`',
          returns = {
            { opt_int, 'width' },
            { union('integer', 'string'), 'height or err' },
            { opt('uv.error_name'), 'err_name' },
          },
        },
        {
          name = 'tty_set_vterm_state',
          desc = [[
            Controls whether console virtual terminal sequences are processed by libuv or
            console. Useful in particular for enabling ConEmu support of ANSI X3.64 and
            Xterm 256 colors. Otherwise Windows10 consoles are usually detected
            automatically. State should be one of: `"supported"` or `"unsupported"`.

            This function is only meaningful on Windows systems. On Unix it is silently
            ignored.
          ]],
          params = {
            { name = 'state', type = 'string' },
          },
        },
        {
          name = 'tty_get_vterm_state',
          desc = [[
            Get the current state of whether console virtual terminal sequences are handled
            by libuv or the console. The return value is `"supported"` or `"unsupported"`.

            This function is not implemented on Unix, where it returns `ENOTSUP`.
          ]],
          returns = ret_or_fail('string', 'state'),
        },
      },
    },
    {
      title = '`uv_udp_t` - UDP handle',
      id = 'uv_udp_t--udp-handle',
      class = 'uv_udp_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        UDP handles encapsulate UDP communication for both clients and servers.
      ]],
      funcs = {
        {
          name = 'new_udp',
          desc = [[
            Creates and initializes a new `uv_udp_t`. Returns the Lua userdata wrapping
            it. The actual socket is created lazily.

            See [Constants][] for supported address `family` input values.

            When specified, `mmsgs` determines the number of messages able to be received
            at one time via `recvmmsg(2)` (the allocated buffer will be sized to be able
            to fit the specified number of max size dgrams). Only has an effect on
            platforms that support `recvmmsg(2)`.

            **Note:** For backwards compatibility reasons, `flags` can also be a string or
            integer. When it is a string, it will be treated like the `family` key above.
            When it is an integer, it will be used directly as the `flags` parameter when
            calling `uv_udp_init_ex`.
          ]],
          params = {
            {
              name = 'flags',
              type = opt(table({
                { 'family', opt_str },
                { 'mmsgs', opt_int, '1' },
              })),
            },
          },
          returns = ret_or_fail('uv_udp_t', 'udp'),
        },
        {
          name = 'udp_get_send_queue_size',
          method_form = 'udp:get_send_queue_size()',
          desc = "Returns the handle's send queue size.",
          params = {
            { name = 'udp', type = 'uv_udp_t' },
          },
          returns = 'integer',
        },
        {
          name = 'udp_get_send_queue_count',
          method_form = 'udp:get_send_queue_count()',
          desc = "Returns the handle's send queue count.",
          params = {
            { name = 'udp', type = 'uv_udp_t' },
          },
          returns = 'integer',
        },
        {
          name = 'udp_open',
          method_form = 'udp:open(fd)',
          desc = [[
            Opens an existing file descriptor or Windows SOCKET as a UDP handle.

            Unix only: The only requirement of the sock argument is that it follows the
            datagram contract (works in unconnected mode, supports sendmsg()/recvmsg(),
            etc). In other words, other datagram-type sockets like raw sockets or netlink
            sockets can also be passed to this function.

            The file descriptor is set to non-blocking mode.

            Note: The passed file descriptor or SOCKET is not checked for its type, but
            it's required that it represents a valid datagram socket.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'fd', type = 'integer' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_bind',
          method_form = 'udp:bind(host, port, [flags])',
          desc = [[
            Bind the UDP handle to an IP address and port. Any `flags` are set with a table
            with fields `reuseaddr` or `ipv6only` equal to `true` or `false`.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'host', type = 'string' },
            { name = 'port', type = 'number' },
            {
              name = 'flags',
              type = opt(table({
                { 'ipv6only', opt_bool },
                { 'reuseaddr', opt_bool },
              })),
            },
          },
          returns = success_ret,
        },
        {
          name = 'udp_getsockname',
          method_form = 'udp:getsockname()',
          desc = 'Get the local IP and port of the UDP handle.',
          params = {
            { name = 'udp', type = 'uv_udp_t' },
          },
          returns = ret_or_fail('socketinfo', 'address'),
        },
        {
          name = 'udp_getpeername',
          method_form = 'udp:getpeername()',
          desc = [[
            Get the remote IP and port of the UDP handle on connected UDP handles.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
          },
          returns = ret_or_fail('socketinfo', 'address'),
        },
        {
          name = 'udp_set_membership',
          method_form = 'udp:set_membership(multicast_addr, interface_addr, membership)',
          desc = [[
            Set membership for a multicast address. `multicast_addr` is multicast address to
            set membership for. `interface_addr` is interface address. `membership` can be
            the string `"leave"` or `"join"`.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'multicast_addr', type = 'string' },
            { name = 'interface_addr', type = opt_str },
            { name = 'membership', type = 'string' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_set_source_membership',
          method_form = 'udp:set_source_membership(multicast_addr, interface_addr, source_addr, membership)',
          desc = [[
            Set membership for a source-specific multicast group. `multicast_addr` is multicast
            address to set membership for. `interface_addr` is interface address. `source_addr`
            is source address. `membership` can be the string `"leave"` or `"join"`.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'multicast_addr', type = 'string' },
            { name = 'interface_addr', type = opt_str },
            { name = 'source_addr', type = 'string' },
            { name = 'membership', type = 'string' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_set_multicast_loop',
          method_form = 'udp:set_multicast_loop(on)',
          desc = [[
            Set IP multicast loop flag. Makes multicast packets loop back to local
            sockets.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'on', type = 'boolean' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_set_multicast_ttl',
          method_form = 'udp:set_multicast_ttl(ttl)',
          desc = [[
            Set the multicast ttl.

            `ttl` is an integer 1 through 255.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'ttl', type = 'integer' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_set_multicast_interface',
          method_form = 'udp:set_multicast_interface(interface_addr)',
          desc = 'Set the multicast interface to send or receive data on.',
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'interface_addr', type = 'string' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_set_broadcast',
          method_form = 'udp:set_broadcast(on)',
          desc = 'Set broadcast on or off.',
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'on', type = 'boolean' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_set_ttl',
          method_form = 'udp:set_ttl(ttl)',
          desc = [[
            Set the time to live.

            `ttl` is an integer 1 through 255.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'ttl', type = 'integer' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_send',
          method_form = 'udp:send(data, host, port, callback)',
          desc = [[
            Send data over the UDP socket. If the socket has not previously been bound
            with `uv.udp_bind()` it will be bound to `0.0.0.0` (the "all interfaces" IPv4
            address) and a random port number.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'data', type = 'buffer' },
            { name = 'host', type = 'string' },
            { name = 'port', type = 'integer' },
            cb_err(),
          },
          returns = ret_or_fail('uv_udp_send_t', 'send'),
        },
        {
          name = 'udp_try_send',
          method_form = 'udp:try_send(data, host, port)',
          desc = [[
            Same as `uv.udp_send()`, but won't queue a send request if it can't be
            completed immediately.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'data', type = 'buffer' },
            { name = 'host', type = 'string' },
            { name = 'port', type = 'integer' },
          },
          returns = ret_or_fail('integer', 'bytes_sent'),
        },
        {
          name = 'udp_try_send2',
          method_form = 'udp:try_send2(messages, flags)',
          desc = [[
            Like `uv.udp_try_send()`, but can send multiple datagrams.
            Lightweight abstraction around `sendmmsg(2)`, with a `sendmsg(2)` fallback loop
            for platforms that do not support the former. The `udp` handle must be fully
            initialized, either from a `uv.udp_bind` call, another call that will bind
            automatically (`udp_send`, `udp_try_send`, etc), or from `uv.udp_connect`.

            `messages` should be an array-like table, where `addr` must be specified
            if the `udp` has not been connected via `udp_connect`. Otherwise, `addr`
            must be `nil`.

            `flags` is reserved for future extension and must currently be `nil` or `0` or
            `{}`.

            Returns the number of messages sent successfully. An error will only be returned
            if the first datagram failed to be sent.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            {
              name = 'messages',
              type = dict(
                'integer',
                table({
                  { 'data', 'buffer' },
                  { 'addr', table({ { 'ip', 'string' }, { 'port', 'integer' } }) },
                })
              ),
            },
            { name = 'flags', type = opt(union('0', table())) },
            { name = 'port', type = 'integer' },
          },
          returns = ret_or_fail('integer', 'messages_sent'),
          example = [[
            ```lua
            -- If client:connect(...) was not called
            local addr = { ip = "127.0.0.1", port = 1234 }
            client:try_send2({
              { data = "Message 1", addr = addr },
              { data = "Message 2", addr = addr },
            })

            -- If client:connect(...) was called
            client:try_send2({
              { data = "Message 1" },
              { data = "Message 2" },
            })
            ```
          ]],
        },
        {
          name = 'udp_recv_start',
          method_form = 'udp:recv_start(callback)',
          desc = [[
            Prepare for receiving data. If the socket has not previously been bound with
            `uv.udp_bind()` it is bound to `0.0.0.0` (the "all interfaces" IPv4 address)
            and a random port number.

            See [Constants][] for supported address `family` output values.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            cb_err({
              { 'data', opt_str },
              {
                'addr',
                opt(table({
                  { 'ip', 'string' },
                  { 'port', 'integer' },
                  { 'family', 'string' },
                })),
              },
              {
                'flags',
                table({
                  { 'partial', opt_bool },
                  { 'mmsg_chunk', opt_bool },
                }),
              },
            }),
          },
          returns = success_ret,
        },
        {
          name = 'udp_recv_stop',
          method_form = 'udp:recv_stop()',
          desc = 'Stop listening for incoming datagrams.',
          params = {
            { name = 'udp', type = 'uv_udp_t' },
          },
          returns = success_ret,
        },
        {
          name = 'udp_connect',
          method_form = 'udp:connect(host, port)',
          desc = [[
            Associate the UDP handle to a remote address and port, so every message sent by
            this handle is automatically sent to that destination. Calling this function
            with a NULL addr disconnects the handle. Trying to call `uv.udp_connect()` on an
            already connected handle will result in an `EISCONN` error. Trying to disconnect
            a handle that is not connected will return an `ENOTCONN` error.
          ]],
          params = {
            { name = 'udp', type = 'uv_udp_t' },
            { name = 'host', type = 'string' },
            { name = 'port', type = 'integer' },
          },
          returns = success_ret,
        },
      },
    },
    {
      title = '`uv_fs_event_t` - FS Event handle',
      id = 'uv_fs_event_t--fs-event-handle',
      class = 'uv_fs_event_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        FS Event handles allow the user to monitor a given path for changes, for
        example, if the file was renamed or there was a generic change in it. This
        handle uses the best backend for the job on each platform.
      ]],
      funcs = {
        {
          name = 'new_fs_event',
          desc = [[
            Creates and initializes a new `uv_fs_event_t`. Returns the Lua userdata wrapping
            it.
          ]],
          returns = ret_or_fail('uv_fs_event_t', 'fs_event'),
        },
        {
          name = 'fs_event_start',
          method_form = 'fs_event:start(path, flags, callback)',
          desc = [[
            Start the handle with the given callback, which will watch the specified path
            for changes.
          ]],
          params = {
            { name = 'fs_event', type = 'uv_fs_event_t' },
            { name = 'path', type = 'string' },
            {
              name = 'flags',
              type = table({
                { 'watch_entry', opt_bool, 'false' },
                { 'stat', opt_bool, 'false' },
                { 'recursive', opt_bool, 'false' },
              }),
            },
            cb_err({
              { 'filename', 'string' },
              { 'events', table({ { 'change', opt_bool }, { 'rename', opt_bool } }) },
            }),
          },
          returns = success_ret,
        },
        {
          name = 'fs_event_stop',
          method_form = 'fs_event:stop()',
          desc = 'Stop the handle, the callback will no longer be called.',
          params = {
            { name = 'fs_event', type = 'uv_fs_event_t' },
          },
          returns = success_ret,
        },
        {
          name = 'fs_event_getpath',
          method_form = 'fs_event:getpath()',
          desc = 'Get the path being monitored by the handle.',
          params = {
            { name = 'fs_event', type = 'uv_fs_event_t' },
          },
          returns = ret_or_fail('string', 'path'),
        },
      },
    },
    {
      title = '`uv_fs_poll_t` - FS Poll handle',
      id = 'uv_fs_poll_t--fs-poll-handle',
      class = 'uv_fs_poll_t',
      desc = [[
        > [`uv_handle_t`][] functions also apply.

        FS Poll handles allow the user to monitor a given path for changes. Unlike
        `uv_fs_event_t`, fs poll handles use `stat` to detect when a file has changed so
        they can work on file systems where fs event handles can't.
      ]],
      funcs = {
        {
          name = 'new_fs_poll',
          desc = [[
            Creates and initializes a new `uv_fs_poll_t`. Returns the Lua userdata wrapping
            it.
          ]],
          returns = ret_or_fail('uv_fs_poll_t', 'fs_poll'),
        },
        {
          name = 'fs_poll_start',
          method_form = 'fs_poll:start(path, interval, callback)',
          desc = [[
            Check the file at `path` for changes every `interval` milliseconds.

            **Note:** For maximum portability, use multi-second intervals. Sub-second
            intervals will not detect all changes on many file systems.
          ]],
          params = {
            { name = 'fs_poll', type = 'uv_fs_poll_t' },
            { name = 'path', type = 'string' },
            { name = 'interval', type = 'integer' },
            cb_err({
              { 'prev', opt('table'), '(see `uv.fs_stat`)' },
              { 'curr', opt('table'), '(see `uv.fs_stat`)' },
            }),
          },
          returns = success_ret,
        },
        {
          name = 'fs_poll_stop',
          method_form = 'fs_poll:stop()',
          desc = 'Stop the handle, the callback will no longer be called.',
          params = {
            { name = 'fs_poll', type = 'uv_fs_poll_t' },
          },
          returns = success_ret,
        },
        {
          name = 'fs_poll_getpath',
          method_form = 'fs_poll:getpath()',
          desc = 'Get the path being monitored by the handle.',
          params = {
            { name = 'fs_poll', type = 'uv_fs_poll_t' },
          },
          returns = ret_or_fail('string', 'path'),
        },
      },
    },
    {
      title = 'File system operations',
      id = 'file-system-operations',
      desc = [[
        Most file system functions can operate synchronously or asynchronously. When a synchronous version is called (by omitting a callback), the function will
        immediately return the results of the FS call. When an asynchronous version is
        called (by providing a callback), the function will immediately return a
        `uv_fs_t userdata` and asynchronously execute its callback; if an error is encountered, the first and only argument passed to the callback will be the `err` error string; if the operation completes successfully, the first argument will be `nil` and the remaining arguments will be the results of the FS call.

        Synchronous and asynchronous versions of `readFile` (with naive error handling)
        are implemented below as an example:

        ```lua
        local function readFileSync(path)
          local fd = assert(uv.fs_open(path, "r", 438))
          local stat = assert(uv.fs_fstat(fd))
          local data = assert(uv.fs_read(fd, stat.size, 0))
          assert(uv.fs_close(fd))
          return data
        end

        local data = readFileSync("main.lua")
        print("synchronous read", data)
        ```

        ```lua
        local function readFile(path, callback)
          uv.fs_open(path, "r", 438, function(err, fd)
            assert(not err, err)
            uv.fs_fstat(fd, function(err, stat)
              assert(not err, err)
              uv.fs_read(fd, stat.size, 0, function(err, data)
                assert(not err, err)
                uv.fs_close(fd, function(err)
                  assert(not err, err)
                  return callback(data)
                end)
              end)
            end)
          end)
        end

        readFile("main.lua", function(data)
          print("asynchronous read", data)
        end)
        ```
        ]],
      funcs = {
        {
          name = 'fs_close',
          desc = 'Equivalent to `close(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_open',
          desc = [[
              Equivalent to `open(2)`. Access `flags` may be an integer or one of: `"r"`,
              `"rs"`, `"sr"`, `"r+"`, `"rs+"`, `"sr+"`, `"w"`, `"wx"`, `"xw"`, `"w+"`,
              `"wx+"`, `"xw+"`, `"a"`, `"ax"`, `"xa"`, `"a+"`, `"ax+"`, or "`xa+`".
            ]],
          params = {
            { name = 'path', type = 'string' },
            { name = 'flags', type = 'string|integer' },
            {
              name = 'mode',
              type = 'integer',
              desc = "(octal `chmod(1)` mode, e.g. `tonumber('644', 8)`)",
            },
            async_cb({ { 'fd', opt_int } }),
          },
          returns_sync = ret_or_fail('integer', 'fd'),
          returns_async = 'uv_fs_t',
          notes = {
            [[
                On Windows, libuv uses `CreateFileW` and thus the file is always
                opened in binary mode. Because of this, the `O_BINARY` and `O_TEXT` flags are
                not supported.
              ]],
          },
        },
        {
          name = 'fs_read',
          desc = [[
              Equivalent to `preadv(2)`. Returns any data. An empty string indicates EOF.

              If `offset` is nil or omitted, it will default to `-1`, which indicates 'use and update the current file offset.'

              **Note:** When `offset` is >= 0, the current file offset will not be updated by the read.
            ]],
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'size', type = 'integer' },
            { name = 'offset', type = opt_int },
            async_cb({ { 'data', opt_str } }),
          },
          returns_sync = ret_or_fail('string', 'data'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_unlink',
          desc = 'Equivalent to `unlink(2)`.',
          params = {
            { name = 'path', type = 'string' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_write',
          desc = [[
              Equivalent to `pwritev(2)`. Returns the number of bytes written.

              If `offset` is nil or omitted, it will default to `-1`, which indicates 'use and update the current file offset.'

              **Note:** When `offset` is >= 0, the current file offset will not be updated by the write.
            ]],
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'data', type = 'buffer' },
            { name = 'offset', type = opt_int },
            async_cb({ { 'bytes', opt_int } }),
          },
          returns_sync = ret_or_fail('integer', 'bytes_written'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_mkdir',
          desc = 'Equivalent to `mkdir(2)`.',
          params = {
            { name = 'path', type = 'string' },
            {
              name = 'mode',
              type = 'integer',
              desc = "(octal `chmod(1)` mode, e.g. `tonumber('755', 8)`)",
            },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_mkdtemp',
          desc = 'Equivalent to `mkdtemp(3)`.',
          params = {
            { name = 'template', type = 'string' },
            async_cb({ { 'path', opt_str } }),
          },
          returns_sync = ret_or_fail('string', 'path'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_mkstemp',
          desc = [[
              Equivalent to `mkstemp(3)`. Returns a temporary file handle and filename.
            ]],
          params = {
            { name = 'template', type = 'string' },
            async_cb({ { 'fd', opt_int }, { 'path', opt_str } }),
          },
          -- returns_sync = ret_or_fail('integer, string', 'fd, path'),
          returns_sync = {
            { opt_int, 'fd' },
            { 'string', 'path or err' },
            { opt('uv.error_name'), 'err_name' },
          },
          returns_sync_doc = '`integer, string` or `fail`',
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_rmdir',
          desc = 'Equivalent to `rmdir(2)`.',
          params = {
            { name = 'path', type = 'string' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_scandir',
          desc = [[
              Equivalent to `scandir(3)`, with a slightly different API. Returns a handle that
              the user can pass to `uv.fs_scandir_next()`.

              **Note:** This function can be used synchronously or asynchronously. The request
              userdata is always synchronously returned regardless of whether a callback is
              provided and the same userdata is passed to the callback if it is provided.
            ]],
          params = {
            { name = 'path', type = 'string' },
            cb_err({ { 'success', opt('uv_fs_t') } }, true),
          },
          returns = ret_or_fail('uv_fs_t', 'handle'),
        },
        {
          name = 'fs_scandir_next',
          desc = [[
              Called on a `uv_fs_t` returned by `uv.fs_scandir()` to get the next directory
              entry data as a `name, type` pair. When there are no more entries, `nil` is
              returned.

              **Note:** This function only has a synchronous version. See `uv.fs_opendir` and
              its related functions for an asynchronous version.
            ]],
          params = {
            { name = 'fs', type = 'uv_fs_t' },
          },
          returns = {
            { opt_str, 'name' },
            { 'string', 'type or err' },
            { opt('uv.error_name'), 'err_name' },
          },
          returns_doc = '`string, string` or `nil` or `fail`',
        },
        -- fs_stat.result
        {
          name = 'fs_stat',
          desc = 'Equivalent to `stat(2)`.',
          params = {
            { name = 'path', type = 'string' },
            async_cb({ { 'stat', opt('fs_stat.result') } }),
          },
          returns_sync = ret_or_fail('fs_stat.result', 'stat'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_fstat',
          desc = 'Equivalent to `fstat(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            async_cb({ { 'stat', opt('fs_stat.result') } }),
          },
          returns_sync = ret_or_fail('fs_stat.result', 'stat'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_lstat',
          desc = 'Equivalent to `lstat(2)`.',
          params = {
            { name = 'path', type = 'string' },
            async_cb({ { 'stat', opt('fs_stat.result') } }),
          },
          returns_sync = ret_or_fail('fs_stat.result', 'stat'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_rename',
          desc = 'Equivalent to `rename(2)`.',
          params = {
            { name = 'path', type = 'string' },
            { name = 'new_path', type = 'string' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_fsync',
          desc = 'Equivalent to `fsync(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_fdatasync',
          desc = 'Equivalent to `fdatasync(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_ftruncate',
          desc = 'Equivalent to `ftruncate(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'offset', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_sendfile',
          desc = [[
              Limited equivalent to `sendfile(2)`. Returns the number of bytes written.
            ]],
          params = {
            { name = 'out_fd', type = 'integer' },
            { name = 'in_fd', type = 'integer' },
            { name = 'in_offset', type = 'integer' },
            { name = 'size', type = 'integer' },
            async_cb({ { 'bytes', opt_int } }),
          },
          returns_sync = ret_or_fail('integer', 'bytes'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_access',
          desc = [[
              Equivalent to `access(2)` on Unix. Windows uses `GetFileAttributesW()`. Access
              `mode` can be an integer or a string containing `"R"` or `"W"` or `"X"`.
              Returns `true` or `false` indicating access permission.
            ]],
          params = {
            { name = 'path', type = 'string' },
            {
              name = 'mode',
              type = 'string',
              desc = "(a combination of the `'r'`, `'w'` and `'x'` characters denoting the symbolic mode as per `chmod(1)`)",
            },
            async_cb({ { 'permission', opt_bool } }),
          },
          returns_sync = ret_or_fail('boolean', 'permission'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_chmod',
          desc = 'Equivalent to `chmod(2)`.',
          params = {
            { name = 'path', type = 'string' },
            {
              name = 'mode',
              type = 'integer',
              desc = "(octal `chmod(1)` mode, e.g. `tonumber('644', 8)`)",
            },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_fchmod',
          desc = 'Equivalent to `fchmod(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'mode', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_utime',
          desc = [[
              Equivalent to `utime(2)`.

              See [Constants][] for supported FS Modification Time constants.

              Passing `"now"` or `uv.constants.FS_UTIME_NOW` as the atime or mtime sets the timestamp to the
              current time.

              Passing `nil`, `"omit"`, or `uv.constants.FS_UTIME_OMIT` as the atime or mtime leaves the timestamp
              untouched.
            ]],
          params = {
            { name = 'path', type = 'string' },
            { name = 'atime', type = opt(union('number', 'string')) },
            { name = 'mtime', type = opt(union('number', 'string')) },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_futime',
          desc = [[
              Equivalent to `futimes(3)`.

              See [Constants][] for supported FS Modification Time constants.

              Passing `"now"` or `uv.constants.FS_UTIME_NOW` as the atime or mtime sets the timestamp to the
              current time.

              Passing `nil`, `"omit"`, or `uv.constants.FS_UTIME_OMIT` as the atime or mtime leaves the timestamp
              untouched.
            ]],
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'atime', type = opt(union('number', 'string')) },
            { name = 'mtime', type = opt(union('number', 'string')) },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_lutime',
          desc = [[
              Equivalent to `lutimes(3)`.

              See [Constants][] for supported FS Modification Time constants.

              Passing `"now"` or `uv.constants.FS_UTIME_NOW` as the atime or mtime sets the timestamp to the
              current time.

              Passing `nil`, `"omit"`, or `uv.constants.FS_UTIME_OMIT` as the atime or mtime leaves the timestamp
              untouched.
            ]],
          params = {
            { name = 'path', type = 'string' },
            { name = 'atime', type = opt(union('number', 'string')) },
            { name = 'mtime', type = opt(union('number', 'string')) },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_link',
          desc = 'Equivalent to `link(2)`.',
          params = {
            { name = 'path', type = 'string' },
            { name = 'new_path', type = 'string' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_symlink',
          desc = 'Equivalent to `symlink(2)`. If the `flags` parameter is omitted, then the 3rd parameter will be treated as the `callback`.',
          params = {
            { name = 'path', type = 'string' },
            { name = 'new_path', type = 'string' },
            {
              name = 'flags',
              type = opt(union(
                'integer',
                table({
                  {
                    'dir',
                    opt_bool,
                  },
                  { 'junction', opt_bool },
                })
              )),
            },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_readlink',
          desc = 'Equivalent to `readlink(2)`.',
          params = {
            { name = 'path', type = 'string' },
            async_cb({ { 'path', opt_str } }),
          },
          returns_sync = ret_or_fail('string', 'path'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_realpath',
          desc = 'Equivalent to `realpath(3)`.',
          params = {
            { name = 'path', type = 'string' },
            async_cb({ { 'path', opt_str } }),
          },
          returns_sync = ret_or_fail('string', 'path'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_chown',
          desc = 'Equivalent to `chown(2)`.',
          params = {
            { name = 'path', type = 'string' },
            { name = 'uid', type = 'integer' },
            { name = 'gid', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_fchown',
          desc = 'Equivalent to `fchown(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'uid', type = 'integer' },
            { name = 'gid', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_lchown',
          desc = 'Equivalent to `lchown(2)`.',
          params = {
            { name = 'fd', type = 'integer' },
            { name = 'uid', type = 'integer' },
            { name = 'gid', type = 'integer' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_copyfile',
          desc = [[
              Copies a file from path to new_path. If the `flags` parameter is omitted, then the 3rd parameter will be treated as the `callback`.
            ]],
          params = {
            { name = 'path', type = 'string' },
            { name = 'new_path', type = 'string' },
            {
              name = 'flags',
              type = opt(union(
                'integer',
                table({
                  { 'excl', opt_bool },
                  { 'ficlone', opt_bool },
                  { 'ficlone_force', opt_bool },
                })
              )),
            },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_opendir',
          desc = [[
              Opens path as a directory stream. Returns a handle that the user can pass to
              `uv.fs_readdir()`. The `entries` parameter defines the maximum number of entries
              that should be returned by each call to `uv.fs_readdir()`.
            ]],
          params = {
            { name = 'path', type = 'string' },
            async_cb({ { 'dir', opt('luv_dir_t') } }),
            { name = 'entries', type = opt_int },
          },
          returns_sync = ret_or_fail('luv_dir_t', 'dir'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_readdir',
          method_form = 'dir:readdir([callback])',
          desc = [[
              Iterates over the directory stream `luv_dir_t` returned by a successful
              `uv.fs_opendir()` call. A table of data tables is returned where the number
              of entries `n` is equal to or less than the `entries` parameter used in
              the associated `uv.fs_opendir()` call.
            ]],
          params = {
            { name = 'dir', type = 'luv_dir_t' },
            async_cb({
              {
                'entries',
                opt(dict('integer', table({ { 'name', 'string' }, { 'type', 'string' } }))),
              },
            }),
          },
          returns_sync = ret_or_fail(
            dict('integer', table({ { 'name', 'string' }, { 'type', 'string ' } })),
            'entries'
          ),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_closedir',
          desc = 'Closes a directory stream returned by a successful `uv.fs_opendir()` call.',
          method_form = 'dir:closedir([callback])',
          params = {
            { name = 'dir', type = 'luv_dir_t' },
            async_cb(),
          },
          returns_sync = ret_or_fail('boolean', 'success'),
          returns_async = 'uv_fs_t',
        },
        {
          name = 'fs_statfs',
          desc = 'Equivalent to `statfs(2)`.',
          params = {
            { name = 'path', type = 'string' },
            async_cb({ { 'stat', opt('fs_statfs.result') } }),
          },
          returns_sync = ret_or_fail('fs_statfs.result', 'stat'),
          returns_async = 'uv_fs_t',
        },
      },
    },
    {
      title = 'Thread pool work scheduling',
      id = 'thread-pool-work-scheduling',
      desc = [[
        Libuv provides a threadpool which can be used to run user code and get notified
        in the loop thread. This threadpool is internally used to run all file system
        operations, as well as `getaddrinfo` and `getnameinfo` requests.

        ```lua
        local function work_callback(a, b)
          return a + b
        end

        local function after_work_callback(c)
          print("The result is: " .. c)
        end

        local work = uv.new_work(work_callback, after_work_callback)

        work:queue(1, 2)

        -- output: "The result is: 3"
        ```

      ]],
      funcs = {
        {
          name = 'new_work',
          desc = [[
            Creates and initializes a new `luv_work_ctx_t` (not `uv_work_t`).
            `work_callback` is a Lua function or a string containing Lua code or bytecode dumped from a function.
            Returns the Lua userdata wrapping it.
          ]],
          params = {
            {
              name = 'work_callback',
              type = union(
                'string',
                fun({
                  { '...', 'threadargs', 'passed to/from `uv.queue_work(work_ctx, ...)`' },
                })
              ),
            },
            {
              name = 'after_work_callback',
              type = fun({
                { '...', 'threadargs', 'returned from `work_callback`' },
              }),
            },
          },
          returns = 'luv_work_ctx_t',
        },
        {
          name = 'queue_work',
          method_form = 'work_ctx:queue(...)',
          desc = [[
            Queues a work request which will run `work_callback` in a new Lua state in a
            thread from the threadpool with any additional arguments from `...`. Values
            returned from `work_callback` are passed to `after_work_callback`, which is
            called in the main loop thread.
          ]],
          params = {
            { name = 'work_ctx', type = 'luv_work_ctx_t' },
            { name = '...', type = 'threadargs' },
          },
          returns = ret_or_fail('boolean', 'success'),
        },
      },
    },
    {
      title = 'DNS utility functions',
      id = 'dns-utility-functions',
      funcs = {
        {
          name = 'getaddrinfo',
          desc = [[
            Equivalent to `getaddrinfo(3)`. Either `node` or `service` may be `nil` but not
            both.

            See [Constants][] for supported address `family` input and output values.

            See [Constants][] for supported `socktype` input and output values.

            When `protocol` is set to 0 or nil, it will be automatically chosen based on the
            socket's domain and type. When `protocol` is specified as a string, it will be
            looked up using the `getprotobyname(3)` function. Examples: `"ip"`, `"icmp"`,
            `"tcp"`, `"udp"`, etc.
          ]],
          params = {
            { name = 'host', type = opt_str },
            { name = 'service', type = opt_str },
            {
              name = 'hints',
              type = opt(table({
                { 'family', opt(union('string', 'integer')) },
                { 'socktype', opt(union('string', 'integer')) },
                { 'protocol', opt(union('string', 'integer')) },
                { 'addrconfig', opt_bool },
                { 'v4mapped', opt_bool },
                { 'all', opt_bool },
                { 'numerichost', opt_bool },
                { 'passive', opt_bool },
                { 'numericserv', opt_bool },
                { 'canonname', opt_bool },
              })),
            },
            async_cb({ { 'addresses', opt(dict('integer', 'address')) } }),
          },
          returns_sync = ret_or_fail(dict('integer', 'address'), 'addresses'),
          returns_async = ret_or_fail('uv_getaddrinfo_t', 'addrinfo'),
        },
        {
          name = 'getnameinfo',
          desc = [[
            Equivalent to `getnameinfo(3)`.

            See [Constants][] for supported address `family` input values.
          ]],
          params = {
            {
              name = 'address',
              type = table({
                { 'ip', opt_str },
                { 'port', opt_int },
                { 'family', opt(union('string', 'integer')) },
              }),
            },
            async_cb({ { 'host', opt_str }, { 'service', opt_str } }),
          },
          returns_sync = {
            { opt_str, 'host' },
            { 'string', 'service or err' },
            { opt('uv.error_name'), 'err_name' },
          },
          returns_sync_doc = '`string, string` or `fail`',
          returns_async = ret_or_fail('uv_getnameinfo_t', 'nameinfo'),
        },
      },
    },
    {
      title = 'Threading and synchronization utilities',
      id = 'threading-and-synchronization-utilities',
      desc = [[
        Libuv provides cross-platform implementations for multiple threading and
         synchronization primitives. The API largely follows the pthreads API.
      ]],
      funcs = {
        {
          name = 'new_thread',
          desc = [[
            Creates and initializes a `luv_thread_t` (not `uv_thread_t`). Returns the Lua
            userdata wrapping it and asynchronously executes `entry`, which can be either
            a Lua function or a string containing Lua code or bytecode dumped from a function. Additional arguments `...`
            are passed to the `entry` function and an optional `options` table may be
            provided. Currently accepted `option` fields are `stack_size`.
          ]],
          params = {
            {
              name = 'options',
              type = opt(table({
                { 'stack_size', opt_int },
              })),
            },
            { name = 'entry', type = 'function|string' },
            { name = '...', type = 'threadargs', desc = 'passed to `entry`' },
          },
          returns = ret_or_fail('luv_thread_t', 'thread'),
          notes = {
            'unsafe, please make sure the thread end of life before Lua state close.',
          },
        },
        {
          name = 'thread_equal',
          method_form = 'thread:equal(other_thread)',
          desc = [[
            Returns a boolean indicating whether two threads are the same. This function is
            equivalent to the `__eq` metamethod.
          ]],
          params = {
            { name = 'thread', type = 'luv_thread_t' },
            { name = 'other_thread', type = 'luv_thread_t' },
          },
          returns = 'boolean',
        },
        {
          name = 'thread_setaffinity',
          method_form = 'thread:setaffinity(affinity, [get_old_affinity])',
          desc = [[
            Sets the specified thread's affinity setting.

            `affinity` must be a table where each of the keys are a CPU number and the
            values are booleans that represent whether the `thread` should be eligible to
            run on that CPU. If the length of the `affinity` table is not greater than or
            equal to `uv.cpumask_size()`, any CPU numbers missing from the table will have
            their affinity set to `false`. If setting the affinity of more than
            `uv.cpumask_size()` CPUs is desired, `affinity` must be an array-like table
            with no gaps, since `#affinity` will be used as the `cpumask_size` if it is
            greater than `uv.cpumask_size()`.

            If `get_old_affinity` is `true`, the previous affinity settings for the `thread`
            will be returned. Otherwise, `true` is returned after a successful call.

            **Note:** Thread affinity setting is not atomic on Windows. Unsupported on macOS.
          ]],
          params = {
            { name = 'thread', type = 'luv_thread_t' },
            { name = 'affinity', type = dict('integer', 'boolean') },
            { name = 'get_old_affinity', type = opt_bool },
          },
          -- TODO: can also return boolean
          returns = ret_or_fail(dict('integer', 'boolean'), 'affinity'),
        },
        {
          name = 'thread_getaffinity',
          method_form = 'thread:getaffinity([mask_size])',
          desc = [[
            Gets the specified thread's affinity setting.

            If `mask_size` is provided, it must be greater than or equal to
            `uv.cpumask_size()`. If the `mask_size` parameter is omitted, then the return
            of `uv.cpumask_size()` will be used. Returns an array-like table where each of
            the keys correspond to a CPU number and the values are booleans that represent
            whether the `thread` is eligible to run on that CPU.

            **Note:** Thread affinity getting is not atomic on Windows. Unsupported on macOS.
          ]],
          params = {
            { name = 'thread', type = 'luv_thread_t' },
            { name = 'mask_size', type = opt_int },
          },
          returns = ret_or_fail(dict('integer', 'boolean'), 'affinity'),
        },
        {
          name = 'thread_getcpu',
          desc = [[
            Gets the CPU number on which the calling thread is running.

            **Note:** The first CPU will be returned as the number 1, not 0. This allows for
            the number to correspond with the table keys used in `uv.thread_getaffinity` and
            `uv.thread_setaffinity`.
          ]],
          returns = ret_or_fail('integer', 'cpu'),
        },
        {
          name = 'thread_setpriority',
          method_form = 'thread:setpriority(priority)',
          desc = [[
            Sets the specified thread's scheduling priority setting. It requires elevated
            privilege to set specific priorities on some platforms.

            The priority can be set to the following constants.

            - uv.constants.THREAD_PRIORITY_HIGHEST
            - uv.constants.THREAD_PRIORITY_ABOVE_NORMAL
            - uv.constants.THREAD_PRIORITY_NORMAL
            - uv.constants.THREAD_PRIORITY_BELOW_NORMAL
            - uv.constants.THREAD_PRIORITY_LOWEST
          ]],
          params = {
            { name = 'thread', type = 'luv_thread_t' },
            { name = 'priority', type = 'integer' },
          },
          returns = ret_or_fail('boolean', 'success'),
        },
        {
          name = 'thread_getpriority',
          method_form = 'thread:getpriority()',
          desc = [[
            Gets the  thread's priority setting.

            Retrieves the scheduling priority of the specified thread. The returned priority
            value is platform dependent.

            For Linux, when schedule policy is SCHED_OTHER (default), priority is 0.
          ]],
          params = {
            { name = 'thread', type = 'luv_thread_t' },
          },
          returns = ret_or_fail('integer', 'priority'),
        },
        {
          name = 'thread_self',
          desc = 'Returns the handle for the thread in which this is called.',
          returns = 'luv_thread_t',
        },
        {
          name = 'thread_join',
          method_form = 'thread:join()',
          desc = 'Waits for the `thread` to finish executing its entry function.',
          params = {
            { name = 'thread', type = 'luv_thread_t' },
          },
          returns = ret_or_fail('boolean', 'success'),
        },
        {
          name = 'thread_detach',
          method_form = 'thread:detach()',
          desc = [[
            Detaches a thread. Detached threads automatically release their resources upon
            termination, eliminating the need for the application to call `uv.thread_join`.
          ]],
          params = {
            { name = 'thread', type = 'luv_thread_t' },
          },
          returns = ret_or_fail('boolean', 'success'),
        },
        {
          name = 'thread_setname',
          desc = [[
            Sets the name of the current thread. Different platforms define different limits
            on the max number of characters a thread name can be: Linux, IBM i (16), macOS
            (64), Windows (32767), and NetBSD (32), etc. The name will be truncated
            if `name` is larger than the limit of the platform.
          ]],
          params = {
            { name = 'name', type = 'string' },
          },
          returns = success_ret,
        },
        {
          name = 'thread_getname',
          method_form = 'thread:getname()',
          desc = 'Gets the name of the thread specified by `thread`.',
          params = {
            { name = 'thread', type = 'luv_thread_t' },
          },
          returns = ret_or_fail('string', 'name'),
        },
        {
          name = 'sleep',
          desc = [[
            Pauses the thread in which this is called for a number of milliseconds.
          ]],
          params = {
            { name = 'msec', type = 'integer' },
          },
        },
        {
          name = 'new_sem',
          desc = [[
            Creates a new semaphore with the specified initial value. A semaphore is safe to
            share across threads. It represents an unsigned integer value that can incremented
            and decremented atomically but any attempt to make it negative will "wait" until
            the value can be decremented by another thread incrementing it.

            The initial value must be a non-negative integer.
          ]],
          params = {
            { name = 'value', type = opt_int },
          },
          returns = ret_or_fail('luv_sem_t', 'sem'),
          notes = {
            [[A semaphore must be shared between threads, any `uv.sem_wait()` on a single thread that blocks will deadlock.]],
          },
        },
        {
          name = 'sem_post',
          method_form = 'sem:post()',
          desc = [[
            Increments (unlocks) a semaphore, if the semaphore's value consequently becomes
            greater than zero then another thread blocked in a sem_wait call will be woken
            and proceed to decrement the semaphore.
          ]],
          params = {
            { name = 'sem', type = 'luv_sem_t' },
          },
        },
        {
          name = 'sem_wait',
          method_form = 'sem:wait()',
          desc = [[
              Decrements (locks) a semaphore, if the semaphore's value is greater than zero
              then the value is decremented and the call returns immediately. If the semaphore's
              value is zero then the call blocks until the semaphore's value rises above zero or
              the call is interrupted by a signal.
          ]],
          params = {
            { name = 'sem', type = 'luv_sem_t' },
          },
        },
        {
          name = 'sem_trywait',
          method_form = 'sem:trywait()',
          desc = [[
              The same as `uv.sem_wait()` but returns immediately if the semaphore is not available.

              If the semaphore's value was decremented then `true` is returned, otherwise the semaphore
              has a value of zero and `false` is returned.
          ]],
          params = {
            { name = 'sem', type = 'luv_sem_t' },
          },
          returns = 'boolean',
        },
      },
    },
    {
      title = 'Miscellaneous utilities',
      id = 'miscellaneous-utilities',
      funcs = {
        {
          name = 'exepath',
          desc = 'Returns the executable path.',
          returns = ret_or_fail('string', 'path'),
        },
        {
          name = 'cwd',
          desc = 'Returns the current working directory.',
          returns = ret_or_fail('string', 'path'),
        },
        {
          name = 'chdir',
          desc = 'Sets the current working directory with the string `cwd`.',
          params = {
            { name = 'cwd', type = 'string' },
          },
          returns = success_ret,
        },
        {
          name = 'get_process_title',
          desc = 'Returns the title of the current process.',
          returns = ret_or_fail('string', 'title'),
        },
        {
          name = 'set_process_title',
          desc = 'Sets the title of the current process with the string `title`.',
          params = {
            { name = 'title', type = 'string' },
          },
          returns = success_ret,
        },
        {
          name = 'get_total_memory',
          desc = 'Returns the current total system memory in bytes.',
          returns = 'number',
        },
        {
          name = 'get_free_memory',
          desc = 'Returns the current free system memory in bytes.',
          returns = 'number',
        },
        {
          name = 'get_constrained_memory',
          desc = [[
            Gets the amount of memory available to the process in bytes based on limits
            imposed by the OS. If there is no such constraint, or the constraint is unknown,
            0 is returned. Note that it is not unusual for this value to be less than or
            greater than the total system memory.
          ]],
          returns = 'number',
        },
        {
          name = 'get_available_memory',
          desc = [[
            Gets the amount of free memory that is still available to the process (in
            bytes). This differs from `uv.get_free_memory()` in that it takes into account
            any limits imposed by the OS. If there is no such constraint, or the constraint
            is unknown, the amount returned will be identical to `uv.get_free_memory()`.
          ]],
          returns = 'number',
        },
        {
          name = 'resident_set_memory',
          desc = 'Returns the resident set size (RSS) for the current process.',
          returns = ret_or_fail('integer', 'rss'),
        },
        {
          name = 'getrusage',
          desc = 'Returns the resource usage.',
          returns = ret_or_fail('getrusage.result', 'rusage'),
        },
        {
          name = 'getrusage_thread',
          desc = [[
            Gets the resource usage measures for the calling thread.

            **Note** Not supported on all platforms. May return `ENOTSUP`.
            On macOS and Windows not all fields are set (the unsupported fields are filled
            with zeroes).
          ]],
          returns = ret_or_fail('getrusage.result', 'rusage'),
        },
        {
          name = 'available_parallelism',
          desc = [[
            Returns an estimate of the default amount of parallelism a program should use. Always returns a non-zero value.

            On Linux, inspects the calling thread’s CPU affinity mask to determine if it has been pinned to specific CPUs.

            On Windows, the available parallelism may be underreported on systems with more than 64 logical CPUs.

            On other platforms, reports the number of CPUs that the operating system considers to be online.
          ]],
          returns = 'integer',
        },
        {
          name = 'cpu_info',
          desc = [[
            Returns information about the CPU(s) on the system as a table of tables for each
            CPU found.
          ]],
          returns = ret_or_fail(
            dict(
              'integer',
              table({
                { 'model', 'string' },
                { 'speed', 'integer' },
                {
                  'times',
                  table({
                    { 'user', 'integer' },
                    { 'nice', 'integer' },
                    { 'sys', 'integer' },
                    { 'idle', 'integer' },
                    { 'irq', 'integer' },
                  }),
                },
              })
            ),
            'cpu_info'
          ),
        },
        {
          name = 'cpumask_size',
          desc = [[
            Returns the maximum size of the mask used for process/thread affinities, or
            `ENOTSUP` if affinities are not supported on the current platform.
          ]],
          returns = ret_or_fail('integer', 'size'),
        },
        {
          name = 'getpid',
          deprecated = 'Please use `uv.os_getpid()` instead.',
          returns = 'integer',
        },
        {
          name = 'getuid',
          desc = 'Returns the user ID of the process.',
          returns = 'integer',
          notes = {
            'This is not a libuv function and is not supported on Windows.',
          },
        },
        {
          name = 'getgid',
          desc = 'Returns the group ID of the process.',
          returns = 'integer',
          notes = {
            'This is not a libuv function and is not supported on Windows.',
          },
        },
        {
          name = 'setuid',
          desc = 'Sets the user ID of the process with the integer `id`.',
          params = {
            { name = 'id', type = 'integer' },
          },
          notes = {
            'This is not a libuv function and is not supported on Windows.',
          },
        },
        {
          name = 'setgid',
          desc = 'Sets the group ID of the process with the integer `id`.',
          params = {
            { name = 'id', type = 'integer' },
          },
          notes = {
            'This is not a libuv function and is not supported on Windows.',
          },
        },
        {
          name = 'hrtime',
          desc = [[
            Returns a current high-resolution time in nanoseconds as a number. This is
            relative to an arbitrary time in the past. It is not related to the time of day
            and therefore not subject to clock drift. The primary use is for measuring
            time between intervals.
          ]],
          returns = 'integer',
        },
        {
          name = 'clock_gettime',
          desc = [[
            Obtain the current system time from a high-resolution real-time or monotonic
            clock source. `clock_id` can be the string `"monotonic"` or `"realtime"`.

            The real-time clock counts from the UNIX epoch (1970-01-01) and is subject
            to time adjustments; it can jump back in time.

            The monotonic clock counts from an arbitrary point in the past and never
            jumps back in time.
          ]],
          params = {
            { name = 'clock_id', type = 'string' },
          },
          returns = ret_or_fail(table({ { 'sec', 'integer' }, { 'nsec', 'integer' } }), 'time'),
        },
        {
          name = 'uptime',
          desc = 'Returns the current system uptime in seconds.',
          returns = ret_or_fail('number', 'uptime'),
        },
        {
          name = 'print_all_handles',
          desc = [[
            Prints all handles associated with the main loop to stderr. The format is
            `[flags] handle-type handle-address`. Flags are `R` for referenced, `A` for
            active and `I` for internal.
          ]],
          notes = {
            'This is not available on Windows.',
          },
          warnings = {
            [[
              This function is meant for ad hoc debugging, there are no API/ABI
              stability guarantees.
            ]],
          },
        },
        {
          name = 'print_active_handles',
          desc = 'The same as `uv.print_all_handles()` except only active handles are printed.',
          notes = {
            'This is not available on Windows.',
          },
          warnings = {
            [[
              This function is meant for ad hoc debugging, there are no API/ABI
              stability guarantees.
            ]],
          },
        },
        {
          name = 'guess_handle',
          desc = [[
            Used to detect what type of stream should be used with a given file
            descriptor `fd`. Usually this will be used during initialization to guess the
            type of the stdio streams.
          ]],
          params = {
            { name = 'fd', type = 'integer' },
          },
          returns = 'string',
        },
        {
          name = 'gettimeofday',
          desc = [[
            Cross-platform implementation of `gettimeofday(2)`. Returns the seconds and
            microseconds of a unix time as a pair.
          ]],
          -- returns = ret_or_fail('integer, integer', 'second and microseconds'),
          returns = {
            { opt_int, 'seconds' },
            { union('integer', 'string'), 'microseconds or err' },
            { opt('uv.error_name'), 'err_name' },
          },
          returns_doc = '`integer, integer` or `fail`',
        },
        {
          name = 'interface_addresses',
          desc = [[
            Returns address information about the network interfaces on the system in a
            table. Each table key is the name of the interface while each associated value
            is an array of address information where fields are `ip`, `family`, `netmask`,
            `internal`, and `mac`.

            See [Constants][] for supported address `family` output values.
          ]],
          returns = {
            {
              dict(
                'string',
                table({
                  { 'ip', 'string' },
                  { 'family', 'string' },
                  { 'netmask', 'string' },
                  { 'internal', 'boolean' },
                  { 'mac', 'string' },
                })
              ),
              'addresses',
            },
          },
        },
        {
          name = 'if_indextoname',
          desc = 'IPv6-capable implementation of `if_indextoname(3)`.',
          params = {
            { name = 'ifindex', type = 'integer' },
          },
          returns = ret_or_fail('string', 'name'),
        },
        {
          name = 'if_indextoiid',
          desc = [[
            Retrieves a network interface identifier suitable for use in an IPv6 scoped
            address. On Windows, returns the numeric `ifindex` as a string. On all other
            platforms, `uv.if_indextoname()` is used.
          ]],
          params = {
            { name = 'ifindex', type = 'integer' },
          },
          returns = ret_or_fail('string', 'iid'),
        },
        {
          name = 'loadavg',
          desc = 'Returns the load average as a triad. Not supported on Windows.',
          returns = 'number, number, number',
        },
        {
          name = 'os_uname',
          desc = 'Returns system information.',
          returns = {
            {
              table({
                { 'sysname', 'string' },
                { 'release', 'string' },
                { 'version', 'string' },
                { 'machine', 'string' },
              }),
              'info',
            },
          },
        },
        {
          name = 'os_gethostname',
          desc = 'Returns the hostname.',
          returns = 'string',
        },
        {
          name = 'os_getenv',
          desc = [[
            Returns the environment variable specified by `name` as string. The internal
            buffer size can be set by defining `size`. If omitted, `LUAL_BUFFERSIZE` is
            used. If the environment variable exceeds the storage available in the internal
            buffer, `ENOBUFS` is returned. If no matching environment variable exists,
            `ENOENT` is returned.
          ]],
          params = {
            { name = 'name', type = 'string' },
            { name = 'size', type = opt_int, default = 'LUA_BUFFERSIZE' },
          },
          returns = ret_or_fail('string', 'value'),
          warnings = { 'This function is not thread-safe.' },
        },
        {
          name = 'os_setenv',
          desc = 'Sets the environmental variable specified by `name` with the string `value`.',
          params = {
            { name = 'name', type = 'string' },
            { name = 'value', type = 'string' },
          },
          returns = ret_or_fail('boolean', 'success'),
          warnings = {
            'This function is not thread-safe.',
          },
        },
        {
          name = 'os_unsetenv',
          desc = 'Unsets the environmental variable specified by `name`.',
          params = {
            { name = 'name', type = 'string' },
          },
          returns = ret_or_fail('boolean', 'success'),
          warnings = { 'This function is not thread-safe.' },
        },
        {
          name = 'os_environ',
          desc = [[
            Returns all environmental variables as a dynamic table of names associated with
            their corresponding values.
          ]],
          returns = 'table',
          warnings = { 'This function is not thread-safe.' },
        },
        {
          name = 'os_homedir',
          desc = 'Returns the home directory.',
          returns = ret_or_fail('string', 'path'),
          warnings = { 'This function is not thread-safe.' },
        },
        {
          name = 'os_tmpdir',
          desc = 'Returns a temporary directory.',
          returns = ret_or_fail('string', 'path'),
          warnings = { 'This function is not thread-safe.' },
        },
        {
          name = 'os_get_passwd',
          desc = [[
            Gets a subset of the password file entry for the current effective uid (not the
            real uid). On Windows, `uid`, `gid`, and `shell` are set to `nil`.
          ]],
          returns = ret_or_fail(
            table({
              { 'username', 'string' },
              { 'uid', 'integer?', nil, "(nil on Windows)" },
              { 'gid', 'integer?', nil, "(nil on Windows)" },
              { 'shell', 'string?', nil, "(nil on Windows)"},
              { 'homedir', 'string' },
            }),
            'passwd'
          )
        },
        {
          name = 'os_getpid',
          desc = 'Returns the current process ID.',
          returns = 'number',
        },
        {
          name = 'os_getppid',
          desc = 'Returns the parent process ID.',
          returns = 'number',
        },
        {
          name = 'os_getpriority',
          desc = 'Returns the scheduling priority of the process specified by `pid`.',
          params = {
            { name = 'pid', type = 'integer' },
          },
          returns = ret_or_fail('integer', 'priority'),
        },
        {
          name = 'os_setpriority',
          desc = [[
            Sets the scheduling priority of the process specified by `pid`. The `priority`
            range is between -20 (high priority) and 19 (low priority).
          ]],
          params = {
            { name = 'pid', type = 'integer' },
            { name = 'priority', type = 'integer' },
          },
          returns = ret_or_fail('boolean', 'success'),
        },
        {
          name = 'random',
          desc = [[
            Fills a string of length `len` with cryptographically strong random bytes
            acquired from the system CSPRNG. `flags` is reserved for future extension
            and must currently be `nil` or `0` or `{}`.

            Short reads are not possible. When less than `len` random bytes are available,
            a non-zero error value is returned or passed to the callback. If the callback
            is omitted, this function is completed synchronously.

            The synchronous version may block indefinitely when not enough entropy is
            available. The asynchronous version may not ever finish when the system is
            low on entropy.
          ]],
          params = {
            { name = 'len', type = 'integer' },
            { name = 'flags', type = opt(union('0', table())) },
            async_cb({ { 'bytes', opt_str } }),
          },
          returns_sync = ret_or_fail('string', 'bytes'),
          returns_async = success_ret,
        },
        {
          name = 'translate_sys_error',
          desc = [[
            Returns the libuv error message and error name (both in string form, see [`err` and `name` in Error Handling](#error-handling)) equivalent to the given platform dependent error code: POSIX error codes on Unix (the ones stored in errno), and Win32 error codes on Windows (those returned by GetLastError() or WSAGetLastError()).
          ]],
          params = {
            { name = 'errcode', type = 'integer' },
          },
          returns = {
            { opt_str, 'message' },
            { opt_str, 'name' },
          },
          returns_doc = '`string, string` or `nil`',
        },
      },
    },
    {
      title = 'Metrics operations',
      id = 'metrics-operations',
      funcs = {
        {
          name = 'metrics_idle_time',
          desc = [[
            Retrieve the amount of time the event loop has been idle in the kernel’s event
            provider (e.g. `epoll_wait`). The call is thread safe.

            The return value is the accumulated time spent idle in the kernel’s event
            provider starting from when the [`uv_loop_t`][] was configured to collect the idle time.

            **Note:** The event loop will not begin accumulating the event provider’s idle
            time until calling `loop_configure` with `"metrics_idle_time"`.
          ]],
          returns = 'number',
        },
        {
          name = 'metrics_info',
          desc = [[
            Get the metrics table from current set of event loop metrics. It is recommended
            to retrieve these metrics in a `prepare` callback (see `uv.new_prepare`,
            `uv.prepare_start`) in order to make sure there are no inconsistencies with the
            metrics counters.
          ]],
          returns = {
            {
              table({
                { 'loop_count', 'number' },
                { 'events', 'integer' },
                { 'events_waiting', 'number' },
              }),
              'info',
            },
          },
        },
      },
    },
    {
      title = 'String manipulation functions',
      desc = [[
        These string utilities are needed internally for dealing with Windows, and are exported to allow clients to work uniformly with this data when the libuv API is not complete.

        **Notes**:

        1. New in luv version 1.49.0.
        2. See [the WTF-8 spec](https://simonsapin.github.io/wtf-8/) for information about WTF-8.
        3. Luv uses Lua-style strings, which means that all inputs and return values (UTF-8 or UTF-16 strings) do not include a NUL terminator.
      ]],
      funcs = {
        {
          name = 'utf16_length_as_wtf8',
          desc = 'Get the length (in bytes) of a UTF-16 (or UCS-2) string `utf16` value after converting it to WTF-8.',
          params = {
            { name = 'utf16', type = 'string' },
          },
          returns = 'integer',
        },
        {
          name = 'utf16_to_wtf8',
          desc = [[
            Convert UTF-16 (or UCS-2) string `utf16` to WTF-8 string. The endianness of the UTF-16 (or UCS-2) string is assumed to be the same as the native endianness of the platform.
          ]],
          params = {
            { name = 'utf16', type = 'string' },
          },
          returns = 'string',
        },
        {
          name = 'wtf8_length_as_utf16',
          desc = [[
            Get the length (in UTF-16 code units) of a WTF-8 `wtf8` value after converting it to UTF-16 (or UCS-2). Note: The number of bytes needed for a UTF-16 (or UCS-2) string is `<number of code units> * 2`.
          ]],
          params = {
            { name = 'wtf8', type = 'string' },
          },
          returns = 'integer',
        },
        {
          name = 'wtf8_to_utf16',
          desc = [[
            Convert WTF-8 string in `wtf8` to UTF-16 (or UCS-2) string. The endianness of the UTF-16 (or UCS-2) string will be the same as the native endianness of the platform.
          ]],
          params = {
            { name = 'wtf8', type = 'string' },
          },
          returns = 'string',
        },
      },
    },
    { -- links
      desc = [[
        ---

        [luv]: https://github.com/luvit/luv
        [luvit]: https://github.com/luvit/luvit
        [libuv]: https://github.com/libuv/libuv
        [libuv documentation page]: http://docs.libuv.org/
        [libuv API documentation]: http://docs.libuv.org/en/v1.x/api.html
        [error constants]: https://docs.libuv.org/en/v1.x/errors.html#error-constants
      ]],
    },
  },
}

return { doc, types }
