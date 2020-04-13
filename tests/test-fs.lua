return require('lib/tap')(function (test)

  test("read a file sync", function (print, p, expect, uv)
    local fd = assert(uv.fs_open('README.md', 'r', tonumber('644', 8)))
    p{fd=fd}
    local stat = assert(uv.fs_fstat(fd))
    p{stat=stat}
    local chunk = assert(uv.fs_read(fd, stat.size, 0))
    assert(#chunk == stat.size)
    assert(uv.fs_close(fd))
  end)

  test("read a file async", function (print, p, expect, uv)
    uv.fs_open('README.md', 'r', tonumber('644', 8), expect(function (err, fd)
      assert(not err, err)
      p{fd=fd}
      uv.fs_fstat(fd, expect(function (err, stat)
        assert(not err, err)
        p{stat=stat}
        uv.fs_read(fd, stat.size, 0, expect(function (err, chunk)
          assert(not err, err)
          p{chunk=#chunk}
          assert(#chunk == stat.size)
          uv.fs_close(fd, expect(function (err)
            assert(not err, err)
          end))
        end))
      end))
    end))
  end)

  test("fs.write", function (print, p, expect, uv)
    local path = "_test_"
    local fd = assert(uv.fs_open(path, "w", 438))
    uv.fs_write(fd, "Hello World\n", -1)
    uv.fs_write(fd, {"with\n", "more\n", "lines\n"}, -1)
    uv.fs_close(fd)
    uv.fs_unlink(path)
  end)

  -- collect garbage after uv.fs_write but before the write callback
  -- is called in order to potentially garbage collect the strings that
  -- are being sent. See https://github.com/luvit/luv/issues/397
  test("fs.write data refs", function (print, p, expect, uv)
    local path = "_test_"
    local fd = assert(uv.fs_open(path, "w+", tonumber("0666", 8)))
    do
      -- the number here gets coerced into a string
      local t = {"with", 600, "lines"}
      uv.fs_write(fd, t, -1, function()
        local expectedContents = table.concat(t)
        local stat = assert(uv.fs_fstat(fd))
        assert(stat.size == #expectedContents)
        local chunk = assert(uv.fs_read(fd, stat.size, 0))
        assert(chunk == expectedContents)
        assert(uv.fs_close(fd))
        assert(uv.fs_unlink(path))
      end)
    end
    local count = collectgarbage("count")
    collectgarbage("collect")
    assert(count - collectgarbage("count") > 0)
  end)

  test("fs.stat sync", function (print, p, expect, uv)
    local stat = assert(uv.fs_stat("README.md"))
    assert(stat.size)
  end)

  test("fs.stat async", function (print, p, expect, uv)
    assert(uv.fs_stat("README.md", expect(function (err, stat)
      assert(not err, err)
      assert(stat.size)
    end)))
  end)

  test("fs.stat sync error", function (print, p, expect, uv)
    local stat, err, code = uv.fs_stat("BAD_FILE!")
    p{err=err,code=code,stat=stat}
    assert(not stat)
    assert(err)
    assert(code == "ENOENT")
  end)

  test("fs.stat async error", function (print, p, expect, uv)
    assert(uv.fs_stat("BAD_FILE@", expect(function (err, stat)
      p{err=err,stat=stat}
      assert(err)
      assert(not stat)
    end)))
  end)

  test("fs.scandir", function (print, p, expect, uv)
    local req = uv.fs_scandir('.')
    local function iter()
      return uv.fs_scandir_next(req)
    end
    for name, ftype in iter do
      p{name=name, ftype=ftype}
      assert(name)
      -- ftype is not available in all filesystems; for example it's
      -- provided for HFS+ (OSX), NTFS (Windows) but not for ext4 (Linux).
    end
  end)

  test("fs.realpath", function (print, p, expect, uv)
    p(assert(uv.fs_realpath('.')))
    assert(uv.fs_realpath('.', expect(function (err, path)
      assert(not err, err)
      p(path)
    end)))
  end, "1.8.0")

  test("fs.copyfile", function (print, p, expect, uv)
    local path = "_test_"
    local path2 = "_test2_"
    local fd = assert(uv.fs_open(path, "w", 438))
    uv.fs_write(fd, "Hello World\n", -1)
    uv.fs_close(fd)
    assert(uv.fs_copyfile(path, path2))
    assert(uv.fs_unlink(path))
    assert(uv.fs_unlink(path2))
  end, "1.14.0")

  test("fs.{open,read,close}dir object sync #1", function(print, p, expect, uv)
    local dir = assert(uv.fs_opendir('.'))
    repeat
      local dirent = dir:readdir()
      if dirent then
        assert(#dirent==1)
        p(dirent)
      end
    until not dirent
    assert(dir:closedir()==true)
  end, "1.28.0")

  test("fs.{open,read,close}dir object sync #2", function(print, p, expect, uv)
    local dir = assert(uv.fs_opendir('.'))
    repeat
      local dirent = dir:readdir()
      if dirent then
        assert(#dirent==1)
        p(dirent)
      end
    until not dirent
    dir:closedir(function(err, state)
      assert(err==nil)
      assert(state==true)
      assert(tostring(dir):match("^uv_dir_t"))
      print(dir, 'closed')
    end)
  end, "1.28.0")

  test("fs.{open,read,close}dir sync one entry", function(print, p, expect, uv)
    local dir = assert(uv.fs_opendir('.'))
    repeat
      local dirent = uv.fs_readdir(dir)
      if dirent then
        assert(#dirent==1)
        p(dirent)
      end
    until not dirent
    assert(uv.fs_closedir(dir)==true)
  end, "1.28.0")

  test("fs.{open,read,close}dir sync more entry", function(print, p, expect, uv)
    local dir = assert(uv.fs_opendir('.', nil, 50))
    repeat
      local dirent = uv.fs_readdir(dir)
      if dirent then p(dirent) end
    until not dirent
    assert(uv.fs_closedir(dir)==true)
  end, "1.28.0")

  test("fs.{open,read,close}dir with more entry", function(print, p, expect, uv)
    local function opendir_cb(err, dir)
      assert(not err)
      local function readdir_cb(err, dirs)
        assert(not err)
        if dirs then
          p(dirs)
          uv.fs_readdir(dir, readdir_cb)
        else
          assert(uv.fs_closedir(dir)==true)
        end
      end

      uv.fs_readdir(dir, readdir_cb)
    end
    assert(uv.fs_opendir('.', opendir_cb, 50))
  end, "1.28.0")

  test("fs.statfs sync", function (print, p, expect, uv)
    local stat = assert(uv.fs_statfs("."))
    p(stat)
    assert(stat.bavail>0)
  end, "1.31.0")

  test("fs.statfs async", function (print, p, expect, uv)
    assert(uv.fs_statfs(".", expect(function (err, stat)
      assert(not err, err)
      p(stat)
      assert(stat.bavail>0)
    end)))
  end, "1.31.0")

  test("fs.statfs sync error", function (print, p, expect, uv)
    local stat, err, code = uv.fs_statfs("BAD_FILE!")
    p{err=err,code=code,stat=stat}
    assert(not stat)
    assert(err)
    assert(code == "ENOENT")
  end, "1.31.0")

  test("fs.statfs async error", function (print, p, expect, uv)
    assert(uv.fs_statfs("BAD_FILE@", expect(function (err, stat)
      p{err=err,stat=stat}
      assert(err)
      assert(not stat)
    end)))
  end, "1.31.0")

  test("fs.mkdtemp async", function(print, p, expect, uv)
    local tp = "luvXXXXXX"
    uv.fs_mkdtemp(tp, function(err, path)
      assert(not err)
      assert(path:match("^luv......"))
      assert(uv.fs_rmdir(path))
    end)
  end)

  test("fs.mkdtemp sync", function(print, p, expect, uv)
    local tp = "luvXXXXXX"
    local path, err, code = uv.fs_mkdtemp(tp)
    assert(path:match("^luv......"))
    assert(uv.fs_rmdir(path))
  end)

  test("fs.mkdtemp async error", function(print, p, expect, uv)
    local tp = "luvXXXXXZ"
    uv.fs_mkdtemp(tp, function(err, path)
      -- Will success on MacOS
      if not err then
        assert(path:match("^luv......"))
        assert(uv.fs_rmdir(path))
      else
        assert(err:match("^EINVAL:"))
        assert(path==nil)
      end
    end)
  end)

  test("fs.mkdtemp sync error", function(print, p, expect, uv)
    local tp = "luvXXXXXZ"
    local path, err, code = uv.fs_mkdtemp(tp)
    -- Will success on MacOS
    if not err then
      assert(path:match("^luv......"))
      assert(uv.fs_rmdir(path))
    else
      assert(path==nil)
      assert(err:match("^EINVAL:"))
      assert(code=='EINVAL')
    end
  end)

  test("fs.mkstemp async", function(print, p, expect, uv)
    local tp = "luvXXXXXX"
    uv.fs_mkstemp(tp, function(err, fd, path)
      assert(not err)
      assert(type(fd)=='number')
      assert(path:match("^luv......"))
      assert(uv.fs_close(fd))
      assert(uv.fs_unlink(path))
    end)
  end, "1.34.0")

  test("fs.mkstemp sync", function(print, p, expect, uv)
    local tp = "luvXXXXXX"
    local content = "hello world!"
    local fd, path = uv.fs_mkstemp(tp)
    assert(type(fd)=='number')
    assert(path:match("^luv......"))
    uv.fs_write(fd, content, -1)
    assert(uv.fs_close(fd))

    fd = assert(uv.fs_open(path, "r", 438))
    local stat = assert(uv.fs_fstat(fd))
    local chunk = assert(uv.fs_read(fd, stat.size, 0))
    assert(#chunk == stat.size)
    assert(chunk==content)
    assert(uv.fs_close(fd))
    assert(uv.fs_unlink(path))
  end, "1.34.0")

  test("fs.mkstemp async error", function(print, p, expect, uv)
    local tp = "luvXXXXXZ"
    uv.fs_mkstemp(tp, function(err, path, fd)
      assert(err:match("^EINVAL:"))
      assert(path==nil)
      assert(fd==nil)
    end)
  end, "1.34.0")

  test("fs.mkstemp sync error", function(print, p, expect, uv)
    local tp = "luvXXXXXZ"
    local path, err, code = uv.fs_mkstemp(tp)
    assert(path==nil)
    assert(err:match("^EINVAL:"))
    assert(code=='EINVAL')
  end, "1.34.0")
end)
