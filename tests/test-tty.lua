-- come from https://github.com/libuv/libuv/blob/v1.x/test/test-tty.c
local success, ffi = pcall(require, 'ffi')
if not success then
  print('Skipped test-tty: LuaJIT FFI not found')
  return
end

if not (ffi.os == "Linux" or ffi.os == "OSX") then
  print('Skipped test-tty: Not on Linux or macOS')
  return
end

return require('lib/tap')(function (test)

  test("tty normal", function (print, p, expect, uv)
    local stdin = uv.new_tty(0, true)
    local stdout = uv.new_tty(1, false)

    assert(uv.is_readable(stdin))
    assert(uv.is_writable(stdout))

    uv.close(stdin)
    uv.close(stdout)
  end)

  test("tty pty", function (print, p, expect, uv)
    ffi.cdef[[
        struct winsize {
          unsigned short ws_row;
          unsigned short ws_col;
          unsigned short ws_xpixel;   /* unused */
          unsigned short ws_ypixel;   /* unused */
        };
        int openpty(int *amaster, int *aslave, char *name,
          const void* termp, const struct winsize* winp);
        ]]

    local master_fd = ffi.new('int[1]')
    local slave_fd = ffi.new('int[1]')
    local winp = ffi.new('struct winsize[1]')

    local r, util = pcall(ffi.load, 'util')
    if r then
      r = util.openpty(master_fd, slave_fd, nil, nil, winp);
    else
      r = ffi.C.openpty(master_fd, slave_fd, nil, nil, winp);
    end
    assert(tonumber(r)==0)

    local master_tty = uv.new_tty(tonumber(master_fd[0]), false)
    local slave_tty = uv.new_tty(tonumber(slave_fd[0]), false)

    assert(uv.is_readable(master_tty))
    assert(uv.is_writable(master_tty))
    assert(uv.is_readable(slave_tty))
    assert(uv.is_writable(slave_tty))

    master_tty:close()
    slave_tty:close()
  end)

  test("tty device", function (print, p, expect, uv)
    ffi.cdef[[
        typedef unsigned mode_t;
        int open(const char *pathname, int flags, mode_t mode);
        ]]

    local ttyin_fd = ffi.C.open("/dev/tty", uv.constants.O_RDONLY, 0);
    if tonumber(ttyin_fd) == -1 and ffi.errno() == 6 then
      print("Skip, open /dev/tty fail")
      return
    end

    assert(tonumber(ttyin_fd) >= 0, ffi.errno())

    local ttyout_fd = ffi.C.open("/dev/tty", uv.constants.O_WRONLY, 0);
    assert(tonumber(ttyout_fd) >= 0, ffi.errno())

    assert('tty' == uv.guess_handle(ttyin_fd));
    assert('tty' == uv.guess_handle(ttyout_fd));

    local tty_in = uv.new_tty(ttyin_fd, true)
    assert(uv.is_readable(tty_in))
    assert(not uv.is_writable(tty_in))

    local tty_out = uv.new_tty(ttyout_fd, false)
    assert(not uv.is_readable(tty_out))
    assert(uv.is_writable(tty_out))

    local width, height = uv.tty_get_winsize(tty_out)
    print(string.format("width=%d height=%d\n", width, height));

    assert(width >= 0)
    assert(height >= 0)

    assert(uv.tty_set_mode(tty_in, "raw"))
    assert(uv.tty_set_mode(tty_in, "normal"))

    assert(uv.tty_reset_mode())
    assert(uv.tty_reset_mode())
    assert(uv.tty_reset_mode())

    uv.close(tty_in)
    uv.close(tty_out)
  end)
end)
