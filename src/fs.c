/*
 *  Copyright 2014 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

static void luv_push_stats_table(lua_State* L, uv_statbuf_t* s) {
  lua_newtable(L);
  lua_pushinteger(L, s->st_dev);
  lua_setfield(L, -2, "dev");
  lua_pushinteger(L, s->st_ino);
  lua_setfield(L, -2, "ino");
  lua_pushinteger(L, s->st_mode);
  lua_setfield(L, -2, "mode");
  lua_pushinteger(L, s->st_nlink);
  lua_setfield(L, -2, "nlink");
  lua_pushinteger(L, s->st_uid);
  lua_setfield(L, -2, "uid");
  lua_pushinteger(L, s->st_gid);
  lua_setfield(L, -2, "gid");
  lua_pushinteger(L, s->st_rdev);
  lua_setfield(L, -2, "rdev");
  lua_pushinteger(L, s->st_size);
  lua_setfield(L, -2, "size");
  lua_pushinteger(L, s->st_atime);
  lua_setfield(L, -2, "atime");
  lua_pushinteger(L, s->st_mtime);
  lua_setfield(L, -2, "mtime");
  lua_pushinteger(L, s->st_ctime);
  lua_setfield(L, -2, "ctime");
  lua_pushboolean(L, S_ISREG(s->st_mode));
  lua_setfield(L, -2, "is_file");
  lua_pushboolean(L, S_ISDIR(s->st_mode));
  lua_setfield(L, -2, "is_directory");
  lua_pushboolean(L, S_ISCHR(s->st_mode));
  lua_setfield(L, -2, "is_character_device");
  lua_pushboolean(L, S_ISBLK(s->st_mode));
  lua_setfield(L, -2, "is_block_device");
  lua_pushboolean(L, S_ISFIFO(s->st_mode));
  lua_setfield(L, -2, "is_fifo");
  lua_pushboolean(L, S_ISLNK(s->st_mode));
  lua_setfield(L, -2, "is_symbolic_link");
  lua_pushboolean(L, S_ISSOCK(s->st_mode));
  lua_setfield(L, -2, "is_socket");
#ifdef __POSIX__
  lua_pushinteger(L, s->st_blksize);
  lua_setfield(L, -2, "blksize");
  lua_pushinteger(L, s->st_blocks);
  lua_setfield(L, -2, "blocks");
#endif
}

static int luv_string_to_flags(lua_State* L, const char* string) {
  if (strcmp(string, "r") == 0) return O_RDONLY;
  if (strcmp(string, "r+") == 0) return O_RDWR;
  if (strcmp(string, "w") == 0) return O_CREAT | O_TRUNC | O_WRONLY;
  if (strcmp(string, "w+") == 0) return O_CREAT | O_TRUNC | O_RDWR;
  if (strcmp(string, "a") == 0) return O_APPEND | O_CREAT | O_WRONLY;
  if (strcmp(string, "a+") == 0) return O_APPEND | O_CREAT | O_RDWR;
  return luaL_error(L, "Unknown file open flag '%s'", string);
}

/* Processes a result and pushes the data onto the stack
   returns the number of items pushed */
static int push_fs_result(lua_State* L, uv_fs_t* req) {

  switch (req->fs_type) {
    case UV_FS_CLOSE:
    case UV_FS_RENAME:
    case UV_FS_UNLINK:
    case UV_FS_RMDIR:
    case UV_FS_MKDIR:
    case UV_FS_FTRUNCATE:
    case UV_FS_FSYNC:
    case UV_FS_FDATASYNC:
    case UV_FS_LINK:
    case UV_FS_SYMLINK:
    case UV_FS_CHMOD:
    case UV_FS_FCHMOD:
    case UV_FS_CHOWN:
    case UV_FS_FCHOWN:
    case UV_FS_UTIME:
    case UV_FS_FUTIME:
      return 0;

    case UV_FS_OPEN:
    case UV_FS_SENDFILE:
    case UV_FS_WRITE:
      lua_pushinteger(L, req->result);
      return 1;

    case UV_FS_STAT:
    case UV_FS_LSTAT:
    case UV_FS_FSTAT:
      luv_push_stats_table(L, (uv_statbuf_t*)req->ptr);
      return 1;

    case UV_FS_READLINK:
      lua_pushstring(L, (char*)req->ptr);
      return 1;

    case UV_FS_READ:
      lua_pushlstring(L, req->ptr, req->result);
      return 1;

    case UV_FS_READDIR:
      {
        int i;
        char* namebuf = (char*)req->ptr;
        int nnames = req->result;

        lua_createtable(L, nnames, 0);
        for (i = 0; i < nnames; i++) {
          lua_pushstring(L, namebuf);
          lua_rawseti(L, -2, i + 1);
          namebuf += strlen(namebuf);
          assert(*namebuf == '\0');
          namebuf += 1;
        }
      }
      return 1;

    default:
      fprintf(stderr, "UNKNOWN FS TYPE %d\n", req->fs_type);
      return 0;
  }

}

/* Pushes a formatted error string onto the stack */
static void push_fs_error(lua_State* L, uv_fs_t* req) {
  uv_err_t err;
  memset(&err, 0, sizeof err);
  err.code = (uv_err_code)req->errorno;
  if (req->path) {
    lua_pushfstring(L, "%s: %s \"%s\"", uv_err_name(err), uv_strerror(err), req->path);
  }
  else {
    lua_pushfstring(L, "%s: %s", uv_err_name(err), uv_strerror(err));
  }
}

static void on_fs(uv_fs_t *req) {
  int argc;
  /* Get the lua state */
  luv_callback_t* callback = (luv_callback_t*)req->data;
  lua_State* L = callback->L;

  /* Get the callback and push on the lua stack */
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, callback->ref);
  free(callback);

  if (req->result == -1) {
    push_fs_error(L, req);
    argc = 1;
  }
  else {
    lua_pushnil(L);
    argc = 1 + push_fs_result(L, req);
  }

  /* Cleanup the req */
  uv_fs_req_cleanup(req);
  free(req);

  luv_call(L, argc, 0);
}

#define FS_CALL(func, index, ...)                                              \
  luv_callback_t* callback;                                                    \
  uv_fs_t* req = malloc(sizeof(*req));                                         \
  if (lua_isnone(L, index)) {                                                  \
    int argc;                                                                  \
    if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, NULL) < 0) {         \
      lua_pushnil(L);                                                          \
      push_fs_error(L, req);                                                   \
      uv_fs_req_cleanup(req);                                                  \
      free(req);                                                               \
      return 2;                                                                \
    }                                                                          \
    argc = push_fs_result(L, req);                                             \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    if (argc) {                                                                \
      return argc;                                                             \
    }                                                                          \
    lua_pushboolean(L, 1);                                                     \
    return 1;                                                                  \
  }                                                                            \
  luaL_checktype(L, index, LUA_TFUNCTION);                                     \
  callback = malloc(sizeof(*callback));                                        \
  callback->L = L;                                                             \
  lua_pushvalue(L, index);                                                     \
  callback->ref = luaL_ref(L, LUA_REGISTRYINDEX);                              \
  req->data = (void*)callback;                                                 \
  if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, on_fs) < 0) {          \
    push_fs_error(L, req);                                                     \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    return lua_error(L);                                                       \
  }                                                                            \
  return 0;                                                                    \

/* HACK: Hacked version that patches req->ptr to hold buffer
   TODO: get this into libuv itself, there is no reason it couldn't store this
   for us. */
#define FS_CALL2(func, index, ...)                                             \
{                                                                              \
  luv_callback_t* callback;                                                    \
  uv_fs_t* req = malloc(sizeof(*req));                                         \
  if (lua_isnone(L, index)) {                                                  \
    int argc;                                                                  \
    if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, NULL) < 0) {         \
      push_fs_error(L, req);                                                   \
      uv_fs_req_cleanup(req);                                                  \
      free(req);                                                               \
      return lua_error(L);                                                     \
    }                                                                          \
    req->ptr = buffer;                                              /* HACK */ \
    argc = push_fs_result(L, req);                                             \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    return argc;                                                               \
  }                                                                            \
  luaL_checktype(L, index, LUA_TFUNCTION);                                     \
  callback = malloc(sizeof(*callback));                                        \
  callback->L = L;                                                             \
  lua_pushvalue(L, index);                                                     \
  callback->ref = luaL_ref(L, LUA_REGISTRYINDEX);                              \
  req->data = (void*)callback;                                                 \
  if (uv_fs_##func(uv_default_loop(), req, __VA_ARGS__, on_fs) < 0) {          \
    push_fs_error(L, req);                                                     \
    uv_fs_req_cleanup(req);                                                    \
    free(req);                                                                 \
    return lua_error(L);                                                       \
  }                                                                            \
  req->ptr = buffer;                                                /* HACK */ \
  return 0;                                                                    \
}                                                                              \

static int luv_fs_open(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int flags = luv_string_to_flags(L, luaL_checkstring(L, 2));
  int mode = luaL_checkint(L, 3);
  FS_CALL(open, 4, path, flags, mode);
}

static int luv_fs_close(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(close, 2, file);
}

static int luv_fs_read(lua_State* L) {
  char* buffer;
  uv_file file = luaL_checkint(L, 1);
  size_t length = luaL_checkint(L, 2);
  off_t offset = -1;
  if (!lua_isnil(L, 3)) {
    offset = luaL_checkint(L, 3);
  }
  buffer = malloc(length + 1);
  FS_CALL2(read, 4, file, buffer, length, offset);
}

static int luv_fs_write(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  size_t length;
  off_t offset = -1;
  const char* string = luaL_checklstring(L, 2, &length);
  char* buffer = malloc(length);
  memcpy(buffer, string, length);
  if (!lua_isnil(L, 3)) {
    offset = luaL_checkint(L, 3);
  }
  FS_CALL2(write, 4, file, buffer, length, offset);
}

static int luv_fs_unlink(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(unlink, 2, path);
}

static int luv_fs_mkdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  FS_CALL(mkdir, 3, path, mode);
}

static int luv_fs_rmdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(rmdir, 2, path);
}

static int luv_fs_readdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(readdir, 2, path, 0);
}

static int luv_fs_stat(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(stat, 2, path);
}

static int luv_fs_fstat(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(fstat, 2, file);
}

static int luv_fs_rename(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  FS_CALL(rename, 3, path, new_path);
}

static int luv_fs_fsync(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(fsync, 2, file);
}

static int luv_fs_fdatasync(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  FS_CALL(fdatasync, 2, file);
}

static int luv_fs_ftruncate(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  off_t offset = luaL_checkint(L, 2);
  FS_CALL(ftruncate, 3, file, offset);
}

static int luv_fs_sendfile(lua_State* L) {
  uv_file out_fd = luaL_checkint(L, 1);
  uv_file in_fd = luaL_checkint(L, 2);
  off_t in_offset = luaL_checkint(L, 3);
  size_t length = luaL_checkint(L, 4);
  FS_CALL(sendfile, 5, out_fd, in_fd, in_offset, length);
}

static int luv_fs_chmod(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  FS_CALL(chmod, 3, path, mode);
}

static int luv_fs_utime(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);
  FS_CALL(utime, 4, path, atime, mtime);
}

static int luv_fs_futime(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  double atime = luaL_checknumber(L, 2);
  double mtime = luaL_checknumber(L, 3);
  FS_CALL(futime, 4, file, atime, mtime);
}

static int luv_fs_lstat(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(lstat, 2, path);
}

static int luv_fs_link(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  FS_CALL(link, 3, path, new_path);
}

static int luv_fs_symlink(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  int flags = luv_string_to_flags(L, luaL_checkstring(L, 3));
  FS_CALL(symlink, 4, path, new_path, flags);
}

static int luv_fs_readlink(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FS_CALL(readlink, 2, path);
}

static int luv_fs_fchmod(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  int mode = strtoul(luaL_checkstring(L, 2), NULL, 8);
  FS_CALL(fchmod, 3, file, mode);
}

static int luv_fs_chown(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);
  FS_CALL(chown, 4, path, uid, gid);
}

static int luv_fs_fchown(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  int uid = luaL_checkint(L, 2);
  int gid = luaL_checkint(L, 3);
  FS_CALL(fchown, 4, file, uid, gid);
}
