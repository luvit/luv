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

#include "luv.h"

static void luv_push_timespec_table(lua_State* L, const uv_timespec_t* t) {
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, t->tv_sec);
  lua_setfield(L, -2, "sec");
  lua_pushinteger(L, t->tv_nsec);
  lua_setfield(L, -2, "nsec");
}

static void luv_push_stats_table(lua_State* L, const uv_stat_t* s) {
  lua_createtable(L, 0, 23);
  lua_pushinteger(L, s->st_dev);
  lua_setfield(L, -2, "dev");
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
  lua_pushinteger(L, s->st_ino);
  lua_setfield(L, -2, "ino");
  lua_pushinteger(L, s->st_size);
  lua_setfield(L, -2, "size");
  lua_pushinteger(L, s->st_blksize);
  lua_setfield(L, -2, "blksize");
  lua_pushinteger(L, s->st_blocks);
  lua_setfield(L, -2, "blocks");
  lua_pushinteger(L, s->st_flags);
  lua_setfield(L, -2, "flags");
  lua_pushinteger(L, s->st_gen);
  lua_setfield(L, -2, "gen");
  luv_push_timespec_table(L, &s->st_atim);
  lua_setfield(L, -2, "atim");
  luv_push_timespec_table(L, &s->st_mtim);
  lua_setfield(L, -2, "mtim");
  luv_push_timespec_table(L, &s->st_ctim);
  lua_setfield(L, -2, "ctim");
  luv_push_timespec_table(L, &s->st_birthtim);
  lua_setfield(L, -2, "birthtim");
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
#ifdef S_ISSOCK
  lua_pushboolean(L, S_ISSOCK(s->st_mode));
  lua_setfield(L, -2, "is_socket");
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
  if (req->result < 0) {
    return luv_error(L, req->result);
  }

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
      lua_pushboolean(L, 1);
      return 1;

    case UV_FS_OPEN:
    case UV_FS_SENDFILE:
    case UV_FS_WRITE:
      lua_pushinteger(L, req->result);
      return 1;

    case UV_FS_STAT:
    case UV_FS_LSTAT:
    case UV_FS_FSTAT:
      luv_push_stats_table(L, &req->statbuf);
      return 1;

    case UV_FS_READLINK:
      lua_pushstring(L, (char*)req->ptr);
      return 1;

    case UV_FS_READ:
      lua_pushlstring(L, req->ptr, req->result);
      return 1;

    case UV_FS_SCANDIR:
      luv_find(req->data);
      return 1;

    default:
      lua_pushnil(L);
      lua_pushfstring(L, "UNKNOWN FS TYPE %d\n", req->fs_type);
      return 2;
  }

}

static void luv_fs_cb(uv_fs_t* req) {
  lua_State* L = luv_find(req->data);
  int nargs = push_fs_result(L, req);
  if (req->fs_type != UV_FS_SCANDIR) {
    uv_fs_req_cleanup(req);
  }
  luv_resume(L, nargs);
}

static int fs_req(lua_State* L) {
  luv_create_fs(L);
  return 1;
}

#define FS_CALL(func, ...) {                                \
  int ret, is_main;                                         \
  is_main = lua_pushthread(L) == 1;                         \
  lua_pop(L, 1);                                            \
  ret = uv_fs_##func(__VA_ARGS__, is_main ? NULL : luv_fs_cb);  \
  if (ret < 0) {                                            \
    uv_fs_req_cleanup(req);                                 \
    return luv_error(L, ret);                               \
  }                                                         \
  if (is_main) {                                            \
    int nargs = push_fs_result(L, req);                     \
    uv_fs_req_cleanup(req);                                 \
    return nargs;                                           \
  }                                                         \
  return luv_wait(L, req->data, 0);                         \
}

static int luv_fs_close(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  FS_CALL(close, loop, req, file);
}

static int luv_fs_open(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  int flags = luv_string_to_flags(L, luaL_checkstring(L, 4));
  int mode = luaL_checkinteger(L, 5);
  FS_CALL(open, loop, req, path, flags, mode);
}

static int luv_fs_read(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  int64_t len = luaL_checkinteger(L, 4);
  int64_t offset = luaL_checkinteger(L, 5);
  uv_buf_t buf = uv_buf_init(malloc(len), len);
  req->ptr = buf.base;
  FS_CALL(read, loop, req, file, &buf, 1, offset);
}

static int luv_fs_unlink(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  FS_CALL(unlink, loop, req, path);
}

static int luv_fs_write(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  uv_buf_t buf;
  int64_t offset;
  buf.base = (char*)luaL_checklstring(L, 4, &buf.len);
  req->ptr = buf.base;
  offset = luaL_checkinteger(L, 5);
  FS_CALL(write, loop, req, file, &buf, 1, offset);
}

static int luv_fs_mkdir(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  int mode = luaL_checkinteger(L, 4);
  FS_CALL(mkdir, loop, req, path, mode);
}

static int luv_fs_mkdtemp(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* tpl = luaL_checkstring(L, 3);
  FS_CALL(mkdtemp, loop, req, tpl);
}

static int luv_fs_rmdir(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  FS_CALL(rmdir, loop, req, path);
}

static int luv_fs_scandir(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  int flags = 0; // TODO: find out what these flags are.
  FS_CALL(scandir, loop, req, path, flags);
}

static int luv_fs_scandir_next(lua_State* L) {
  uv_fs_t* req = luv_check_fs(L, 1);
  uv_dirent_t ent;
  int ret = uv_fs_scandir_next(req, &ent);
  const char* type;
  if (ret == UV_EOF) {
    uv_fs_req_cleanup(req);
    return 0;
  }
  if (ret < 0) return luv_error(L, ret);
  lua_createtable(L, 0, 2);
  lua_pushstring(L, ent.name);
  lua_setfield(L, -2, "name");
  switch (ent.type) {
    case UV_DIRENT_UNKNOWN: type = "unknown"; break;
    case UV_DIRENT_FILE: type = "file"; break;
    case UV_DIRENT_DIR: type = "dir"; break;
    case UV_DIRENT_LINK: type = "link"; break;
    case UV_DIRENT_FIFO: type = "fifo"; break;
    case UV_DIRENT_SOCKET: type = "socket"; break;
    case UV_DIRENT_CHAR: type = "char"; break;
    case UV_DIRENT_BLOCK: type = "block"; break;
  }
  lua_pushstring(L, type);
  lua_setfield(L, -2, "type");
  return 1;
}

static int luv_fs_stat(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  FS_CALL(stat, loop, req, path);
}

static int luv_fs_fstat(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  FS_CALL(fstat, loop, req, file);
}

static int luv_fs_lstat(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  FS_CALL(lstat, loop, req, path);
}

static int luv_fs_rename(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  const char* new_path = luaL_checkstring(L, 4);
  FS_CALL(rename, loop, req, path, new_path);
}

static int luv_fs_fsync(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  FS_CALL(fsync, loop, req, file);
}

static int luv_fs_fdatasync(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  FS_CALL(fdatasync, loop, req, file);
}

static int luv_fs_ftruncate(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  int64_t offset = luaL_checkinteger(L, 4);
  FS_CALL(ftruncate, loop, req, file, offset);
}

static int luv_fs_sendfile(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file out_fd = luaL_checkinteger(L, 3);
  uv_file in_fd = luaL_checkinteger(L, 4);
  int64_t in_offset = luaL_checkinteger(L, 5);
  size_t length = luaL_checkinteger(L, 6);
  FS_CALL(sendfile, loop, req, out_fd, in_fd, in_offset, length);
}

static int luv_fs_chmod(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  int mode = luaL_checkinteger(L, 4);
  FS_CALL(chmod, loop, req, path, mode);
}

static int luv_fs_fchmod(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  int mode = luaL_checkinteger(L, 4);
  FS_CALL(fchmod, loop, req, file, mode);
}

static int luv_fs_utime(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  double atime = luaL_checknumber(L, 4);
  double mtime = luaL_checknumber(L, 5);
  FS_CALL(utime, loop, req, path, atime, mtime);
}

static int luv_fs_futime(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  double atime = luaL_checknumber(L, 4);
  double mtime = luaL_checknumber(L, 5);
  FS_CALL(futime, loop, req, file, atime, mtime);
}

static int luv_fs_link(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  const char* new_path = luaL_checkstring(L, 4);
  FS_CALL(link, loop, req, path, new_path);
}

static int luv_fs_symlink(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  const char* new_path = luaL_checkstring(L, 4);
  int flags = 0;
  if (lua_type(L, 5) == LUA_TTABLE) {
    lua_getfield(L, 5, "dir");
    if (lua_toboolean(L, -1)) flags |= UV_FS_SYMLINK_DIR;
    lua_pop(L, 1);
    lua_getfield(L, 5, "junction");
    if (lua_toboolean(L, -1)) flags |= UV_FS_SYMLINK_JUNCTION;
    lua_pop(L, 1);
  }
  FS_CALL(symlink, loop, req, path, new_path, flags);
}

static int luv_fs_readlink(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  FS_CALL(readlink, loop, req, path);
}

static int luv_fs_chown(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  const char* path = luaL_checkstring(L, 3);
  uv_uid_t uid = luaL_checkinteger(L, 4);
  uv_uid_t gid = luaL_checkinteger(L, 5);
  FS_CALL(chown, loop, req, path, uid, gid);
}

static int luv_fs_fchown(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_t* req = luv_check_fs(L, 2);
  uv_file file = luaL_checkinteger(L, 3);
  uv_uid_t uid = luaL_checkinteger(L, 4);
  uv_uid_t gid = luaL_checkinteger(L, 5);
  FS_CALL(fchown, loop, req, file, uid, gid);
}
