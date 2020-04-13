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
#ifdef _WIN32
#include <process.h>
#endif

static int luv_guess_handle(lua_State* L) {
  uv_file file = luaL_checkinteger(L, 1);
  switch (uv_guess_handle(file)) {
#define XX(uc, lc) case UV_##uc: lua_pushstring(L, #lc); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    case UV_FILE: lua_pushstring(L, "file"); break;
    default: return 0;
  }
  return 1;
}

static int luv_version(lua_State* L) {
 lua_pushinteger(L, uv_version());
 return 1;
}

static int luv_version_string(lua_State* L) {
 lua_pushstring(L, uv_version_string());
 return 1;
}

// requires the value at idx to be a string or number
static void luv_prep_buf(lua_State *L, int idx, uv_buf_t *pbuf) {
  size_t len;
  // note: if the value is a number, lua_tolstring converts the stack value to a string
  pbuf->base = (char*)lua_tolstring(L, idx, &len);
  pbuf->len = len;
}

// - number of buffers is stored in *count
// - if refs is non-NULL, then *refs is set to a heap-allocated, LUA_NOREF-terminated array
//   of ref integers (refs are to each string in the bufs)
// returns: heap-allocated array of uv_buf_t
static uv_buf_t* luv_prep_bufs(lua_State* L, int index, size_t *count, int **refs) {
  uv_buf_t *bufs;
  size_t i;
  *count = lua_rawlen(L, index);
  bufs = (uv_buf_t*)malloc(sizeof(uv_buf_t) * *count);
  int *refs_array = NULL;
  if (refs)
    refs_array = (int*)malloc(sizeof(int) * (*count + 1));
  for (i = 0; i < *count; ++i) {
    lua_rawgeti(L, index, i + 1);
    if (!lua_isstring(L, -1)) {
      luaL_argerror(L, index, lua_pushfstring(L, "expected table of strings, found %s in the table", luaL_typename(L, -1)));
      return NULL;
    }
    luv_prep_buf(L, -1, &bufs[i]);
    if (refs) {
      // push the string again to ref it, will be popped by luaL_ref
      lua_pushvalue(L, -1);
      refs_array[i] = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_pop(L, 1);
  }
  if (refs) {
    // refs array is LUA_NOREF-terminated
    refs_array[*count] = LUA_NOREF;
    *refs = refs_array;
  }
  return bufs;
}

// Sets up a uv_bufs_t array to pass to write/send libuv functions that take a uv_buf_t*
// - count: set to length of the returned uv_buf_t array
// - req_data: refs to the strings used are stored in req_data->data/req_data->data_ref
// returns: heap-allocated array of uv_buf_t
static uv_buf_t* luv_check_bufs(lua_State* L, int index, size_t* count, luv_req_t* req_data) {
  uv_buf_t* bufs = NULL;
  if (lua_istable(L, index)) {
    int* refs = NULL;
    bufs = luv_prep_bufs(L, index, count, &refs);
    req_data->data = refs;
    req_data->data_ref = LUV_REQ_MULTIREF;
  }
  else if (lua_isstring(L, index)) {
    *count = 1;
    bufs = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    luv_prep_buf(L, index, bufs);
    lua_pushvalue(L, index);
    req_data->data_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  else {
    luaL_argerror(L, index, lua_pushfstring(L, "data must be string or table of strings, got %s", luaL_typename(L, index)));
  }
  return bufs;
}

// Like luv_check_bufs but does not ref the buf strings.
// Only meant to be used for functions like luv_udp_try_send.
static uv_buf_t* luv_check_bufs_noref(lua_State* L, int index, size_t* count) {
  uv_buf_t* bufs = NULL;
  if (lua_istable(L, index)) {
    bufs = luv_prep_bufs(L, index, count, NULL);
  }
  else if (lua_isstring(L, index)) {
    *count = 1;
    bufs = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    luv_prep_buf(L, index, bufs);
  }
  else {
    luaL_argerror(L, index, lua_pushfstring(L, "data must be string or table of strings, got %s", luaL_typename(L, index)));
  }
  return bufs;
}

static int luv_get_process_title(lua_State* L) {
  char title[MAX_TITLE_LENGTH];
  int ret = uv_get_process_title(title, MAX_TITLE_LENGTH);
  if (ret < 0) return luv_error(L, ret);
  lua_pushstring(L, title);
  return 1;
}

static int luv_set_process_title(lua_State* L) {
  const char* title = luaL_checkstring(L, 1);
  int ret = uv_set_process_title(title);
  return luv_result(L, ret);
}

static int luv_resident_set_memory(lua_State* L) {
  size_t rss;
  int ret = uv_resident_set_memory(&rss);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, rss);
  return 1;
}

static int luv_uptime(lua_State* L) {
  double uptime;
  int ret = uv_uptime(&uptime);
  if (ret < 0) return luv_error(L, ret);
  lua_pushnumber(L, uptime);
  return 1;
}

static void luv_push_timeval_table(lua_State* L, const uv_timeval_t* t) {
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, t->tv_sec);
  lua_setfield(L, -2, "sec");
  lua_pushinteger(L, t->tv_usec);
  lua_setfield(L, -2, "usec");
}

static int luv_getrusage(lua_State* L) {
  uv_rusage_t rusage;
  int ret = uv_getrusage(&rusage);
  if (ret < 0) return luv_error(L, ret);
  lua_createtable(L, 0, 16);
  // user CPU time used
  luv_push_timeval_table(L, &rusage.ru_utime);
  lua_setfield(L, -2, "utime");
  // system CPU time used
  luv_push_timeval_table(L, &rusage.ru_stime);
  lua_setfield(L, -2, "stime");
  // maximum resident set size
  lua_pushinteger(L, rusage.ru_maxrss);
  lua_setfield(L, -2, "maxrss");
  // integral shared memory size
  lua_pushinteger(L, rusage.ru_ixrss);
  lua_setfield(L, -2, "ixrss");
  // integral unshared data size
  lua_pushinteger(L, rusage.ru_idrss);
  lua_setfield(L, -2, "idrss");
  // integral unshared stack size
  lua_pushinteger(L, rusage.ru_isrss);
  lua_setfield(L, -2, "isrss");
  // page reclaims (soft page faults)
  lua_pushinteger(L, rusage.ru_minflt);
  lua_setfield(L, -2, "minflt");
  // page faults (hard page faults)
  lua_pushinteger(L, rusage.ru_majflt);
  lua_setfield(L, -2, "majflt");
  // swaps
  lua_pushinteger(L, rusage.ru_nswap);
  lua_setfield(L, -2, "nswap");
  // block input operations
  lua_pushinteger(L, rusage.ru_inblock);
  lua_setfield(L, -2, "inblock");
  // block output operations
  lua_pushinteger(L, rusage.ru_oublock);
  lua_setfield(L, -2, "oublock");
  // IPC messages sent
  lua_pushinteger(L, rusage.ru_msgsnd);
  lua_setfield(L, -2, "msgsnd");
  // IPC messages received
  lua_pushinteger(L, rusage.ru_msgrcv);
  lua_setfield(L, -2, "msgrcv");
  // signals received
  lua_pushinteger(L, rusage.ru_nsignals);
  lua_setfield(L, -2, "nsignals");
  // voluntary context switches
  lua_pushinteger(L, rusage.ru_nvcsw);
  lua_setfield(L, -2, "nvcsw");
  // involuntary context switches
  lua_pushinteger(L, rusage.ru_nivcsw);
  lua_setfield(L, -2, "nivcsw");
  return 1;
}

static int luv_cpu_info(lua_State* L) {
  uv_cpu_info_t* cpu_infos;
  int count, i;
  int ret = uv_cpu_info(&cpu_infos, &count);
  if (ret < 0) return luv_error(L, ret);
  lua_newtable(L);

  for (i = 0; i < count; i++) {
    lua_newtable(L);
    lua_pushstring(L, cpu_infos[i].model);
    lua_setfield(L, -2, "model");
    lua_pushnumber(L, cpu_infos[i].speed);
    lua_setfield(L, -2, "speed");
    lua_newtable(L);
    lua_pushnumber(L, cpu_infos[i].cpu_times.user);
    lua_setfield(L, -2, "user");
    lua_pushnumber(L, cpu_infos[i].cpu_times.nice);
    lua_setfield(L, -2, "nice");
    lua_pushnumber(L, cpu_infos[i].cpu_times.sys);
    lua_setfield(L, -2, "sys");
    lua_pushnumber(L, cpu_infos[i].cpu_times.idle);
    lua_setfield(L, -2, "idle");
    lua_pushnumber(L, cpu_infos[i].cpu_times.irq);
    lua_setfield(L, -2, "irq");
    lua_setfield(L, -2, "times");
    lua_rawseti(L, -2, i + 1);
  }

  uv_free_cpu_info(cpu_infos, count);
  return 1;
}

static int luv_interface_addresses(lua_State* L) {
  uv_interface_address_t* interfaces;
  int count, i;
  char ip[INET6_ADDRSTRLEN];
  char netmask[INET6_ADDRSTRLEN];

  uv_interface_addresses(&interfaces, &count);

  lua_newtable(L);

  for (i = 0; i < count; i++) {
    lua_getfield(L, -1, interfaces[i].name);
    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);
      lua_pushvalue(L, -1);
      lua_setfield(L, -3, interfaces[i].name);
    }
    lua_newtable(L);
    lua_pushboolean(L, interfaces[i].is_internal);
    lua_setfield(L, -2, "internal");

    lua_pushlstring(L, interfaces[i].phys_addr, sizeof(interfaces[i].phys_addr));
    lua_setfield(L, -2, "mac");

    if (interfaces[i].address.address4.sin_family == AF_INET) {
      uv_ip4_name(&interfaces[i].address.address4, ip, sizeof(ip));
      uv_ip4_name(&interfaces[i].netmask.netmask4, netmask, sizeof(netmask));
    } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
      uv_ip6_name(&interfaces[i].address.address6, ip, sizeof(ip));
      uv_ip6_name(&interfaces[i].netmask.netmask6, netmask, sizeof(netmask));
    } else {
      strncpy(ip, "<unknown sa family>", INET6_ADDRSTRLEN);
      strncpy(netmask, "<unknown sa family>", INET6_ADDRSTRLEN);
    }
    lua_pushstring(L, ip);
    lua_setfield(L, -2, "ip");
    lua_pushstring(L, netmask);
    lua_setfield(L, -2, "netmask");

    lua_pushstring(L, luv_af_num_to_string(interfaces[i].address.address4.sin_family));
    lua_setfield(L, -2, "family");
    lua_rawseti(L, -2, lua_rawlen (L, -2) + 1);
    lua_pop(L, 1);
  }
  uv_free_interface_addresses(interfaces, count);
  return 1;
}

static int luv_loadavg(lua_State* L) {
  double avg[3];
  uv_loadavg(avg);
  lua_pushnumber(L, avg[0]);
  lua_pushnumber(L, avg[1]);
  lua_pushnumber(L, avg[2]);
  return 3;
}

static int luv_exepath(lua_State* L) {
  size_t size = 2*PATH_MAX;
  char exe_path[2*PATH_MAX];
  int ret = uv_exepath(exe_path, &size);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, exe_path, size);
  return 1;
}

static int luv_cwd(lua_State* L) {
  size_t size = 2*PATH_MAX;
  char path[2*PATH_MAX];
  int ret = uv_cwd(path, &size);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, path, size);
  return 1;
}

static int luv_chdir(lua_State* L) {
  int ret = uv_chdir(luaL_checkstring(L, 1));
  return luv_result(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 9, 0)
static int luv_os_tmpdir(lua_State* L) {
  size_t size = 2*PATH_MAX;
  char tmpdir[2*PATH_MAX];
  int ret = uv_os_tmpdir(tmpdir, &size);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, tmpdir, size);
  return 1;
}

static int luv_os_homedir(lua_State* L) {
  size_t size = 2*PATH_MAX;
  char homedir[2*PATH_MAX];
  int ret = uv_os_homedir(homedir, &size);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, homedir, size);
  return 1;
}

static int luv_os_get_passwd(lua_State* L) {
  uv_passwd_t pwd;
  int ret = uv_os_get_passwd(&pwd);
  if (ret < 0) return luv_error(L, ret);
  lua_newtable(L);
  if (pwd.username) {
    lua_pushstring(L, pwd.username);
    lua_setfield(L, -2, "username");
  }
  if (pwd.uid >= 0) {
    lua_pushinteger(L, pwd.uid);
    lua_setfield(L, -2, "uid");
  }
  if (pwd.gid >= 0) {
    lua_pushinteger(L, pwd.gid);
    lua_setfield(L, -2, "gid");
  }
  if (pwd.shell) {
    lua_pushstring(L, pwd.shell);
    lua_setfield(L, -2, "shell");
  }
  if (pwd.homedir) {
    lua_pushstring(L, pwd.homedir);
    lua_setfield(L, -2, "homedir");
  }
  uv_os_free_passwd(&pwd);
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 29, 0)
static int luv_get_constrained_memory(lua_State* L) {
  lua_pushnumber(L, uv_get_constrained_memory());
  return 1;
}
#endif

static int luv_get_total_memory(lua_State* L) {
  lua_pushnumber(L, uv_get_total_memory());
  return 1;
}

static int luv_get_free_memory(lua_State* L) {
  lua_pushnumber(L, uv_get_free_memory());
  return 1;
}

static int luv_hrtime(lua_State* L) {
  lua_pushnumber(L, uv_hrtime());
  return 1;
}

static int luv_getpid(lua_State* L){
  int pid = getpid();
  lua_pushinteger(L, pid);
  return 1;
}

#ifndef _WIN32
static int luv_getuid(lua_State* L){
  int uid = getuid();
  lua_pushinteger(L, uid);
  return 1;
}

static int luv_getgid(lua_State* L){
  int gid = getgid();
  lua_pushinteger(L, gid);
  return 1;
}

static int luv_setuid(lua_State* L){
  int uid = luaL_checkinteger(L, 1);
  int r = setuid(uid);
  if (-1 == r) {
    luaL_error(L, "Error setting UID");
  }
  return 0;
}

static int luv_setgid(lua_State* L){
  int gid = luaL_checkinteger(L, 1);
  int r = setgid(gid);
  if (-1 == r) {
    luaL_error(L, "Error setting GID");
  }
  return 0;
}

#if LUV_UV_VERSION_GEQ(1, 8, 0)
static int luv_print_all_handles(lua_State* L){
  luv_ctx_t* ctx = luv_context(L);
  uv_print_all_handles(ctx->loop, stderr);
  return 0;
}

static int luv_print_active_handles(lua_State* L){
  luv_ctx_t* ctx = luv_context(L);
  uv_print_active_handles(ctx->loop, stderr);
  return 0;
}
#endif
#endif

#if LUV_UV_VERSION_GEQ(1, 12, 0)
static int luv_os_getenv(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  size_t size = luaL_optinteger(L, 2, LUAL_BUFFERSIZE);
  char *buff = malloc(size);
  int ret = uv_os_getenv(name, buff, &size);
  if (ret == 0) {
    lua_pushlstring(L, buff, size);
    ret = 1;
  } else
    ret = luv_error(L, ret);
  free(buff);
  return ret;
}

static int luv_os_setenv(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  const char* value = luaL_checkstring(L, 2);
  int ret = uv_os_setenv(name, value);
  if (ret == 0)
    lua_pushboolean(L, 1);
  else
    return luv_error(L, ret);
  return 1;
}

static int luv_os_unsetenv(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  int ret = uv_os_unsetenv(name);
  if (ret == 0)
    lua_pushboolean(L, 1);
  else
    return luv_error(L, ret);
  return 1;
}

static int luv_os_gethostname(lua_State* L) {
#if LUV_UV_VERSION_GEQ(1, 26, 0)
  char hostname[UV_MAXHOSTNAMESIZE];
#else
  char hostname[PATH_MAX];
#endif
  size_t size = sizeof(hostname);
  int ret = uv_os_gethostname(hostname, &size);
  if (ret == 0) {
    lua_pushlstring(L, hostname, size);
    ret = 1;
  }
  else
    ret = luv_error(L, ret);
  return ret;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 16, 0)
static int luv_if_indextoname(lua_State* L) {
  /* 40 bytes address, 16 bytes device name, plus reserve. */
  char scoped_addr[128];
  size_t scoped_addr_len = sizeof(scoped_addr);
  unsigned int ifindex = (unsigned int)luaL_checkinteger(L, 1);

  int ret = uv_if_indextoname(ifindex - 1, scoped_addr, &scoped_addr_len);
  if (ret == 0) {
    lua_pushlstring(L, scoped_addr, scoped_addr_len);
    ret = 1;
  }
  else
    ret = luv_error(L, ret);
  return ret;
}

static int luv_if_indextoiid(lua_State* L) {
  char interface_id[UV_IF_NAMESIZE];
  size_t interface_id_len = sizeof(interface_id);
  unsigned int ifindex = (unsigned int)luaL_checkinteger(L, 1);

  int ret = uv_if_indextoiid(ifindex - 1, interface_id, &interface_id_len);
  if (ret == 0) {
    lua_pushlstring(L, interface_id, interface_id_len);
    ret = 1;
  }
  else
    ret = luv_error(L, ret);
  return ret;
}

static int luv_os_getppid(lua_State* L) {
  lua_pushnumber(L, uv_os_getppid());
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 18, 0)
static int luv_os_getpid(lua_State* L) {
  lua_pushnumber(L, uv_os_getpid());
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 23, 0)
static int luv_os_getpriority(lua_State* L) {
  int priority;
  uv_pid_t pid = luaL_checkinteger(L, 1);
  int ret = uv_os_getpriority(pid, &priority);
  if (ret == 0) {
    lua_pushnumber(L, priority);
    ret = 1;
  }
  else {
    ret = luv_error(L, ret);
  }
  return ret;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 23, 0)
static int luv_os_setpriority(lua_State* L) {
  uv_pid_t pid = luaL_checkinteger(L, 1);
  int priority= luaL_checkinteger(L, 2);
  int ret = uv_os_setpriority(pid, priority);
  if (ret == 0) {
    lua_pushboolean(L, 1);
    ret = 1;
  }
  else
    ret = luv_error(L, ret);
  return ret;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 25, 0)
static int luv_os_uname(lua_State* L) {
  uv_utsname_t uname;

  int ret = uv_os_uname(&uname);
  if (ret == 0) {
    lua_newtable(L);
    lua_pushstring(L, uname.sysname);
    lua_setfield(L, -2, "sysname");
    lua_pushstring(L, uname.release);
    lua_setfield(L, -2, "release");
    lua_pushstring(L, uname.version);
    lua_setfield(L, -2, "version");
    lua_pushstring(L, uname.machine);
    lua_setfield(L, -2, "machine");
    ret = 1;
  }
  else
    ret = luv_error(L, ret);
  return ret;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 28, 0)
static int luv_gettimeofday(lua_State* L) {
  uv_timeval64_t tv = { 0 };

  int ret = uv_gettimeofday(&tv);
  if (ret == 0)
  {
#if defined(__LP64__)
    lua_pushinteger(L, tv.tv_sec);
#else
    lua_pushnumber(L, tv.tv_sec);
#endif
    lua_pushinteger(L, tv.tv_usec);
    return 2;
  }
  else
    ret = luv_error(L, ret);
  return ret;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 31, 0)
static int luv_os_environ(lua_State* L) {
  int i, ret, envcount;
  uv_env_item_t* envitems;
  ret = uv_os_environ(&envitems, &envcount);
  if (ret==0) {
    lua_newtable(L);
    for(i=0; i<envcount; i++) {
      lua_pushstring(L, envitems[i].name);
      lua_pushstring(L, envitems[i].value);
      lua_rawset(L, -3);
    }
    uv_os_free_environ(envitems, envcount);
    return 1;
  }
  return luv_error(L, ret);
}
#endif

static int luv_sleep(lua_State* L) {
  unsigned int msec = luaL_checkinteger(L, 1);
#if LUV_UV_VERSION_GEQ(1, 34, 0)
  uv_sleep(msec);
#else
#ifdef _WIN32
  Sleep(msec);
#else
  usleep(msec * 1000);
#endif
#endif
  return 0;
}

#if LUV_UV_VERSION_GEQ(1, 33, 0)
static void luv_random_cb(uv_random_t* req, int status, void* buf, size_t buflen) {
  luv_req_t* data = (luv_req_t*)req->data;
  lua_State* L = data->ctx->L;
  int nargs;

  if (status < 0) {
    luv_status(L, status);
    nargs = 1;
  }
  else {
    lua_pushnil(L);
    lua_pushlstring(L, (const char*)buf, buflen);
    nargs = 2;
  }

  luv_fulfill_req(L, (luv_req_t*)req->data, nargs);
  luv_cleanup_req(L, (luv_req_t*)req->data);
  req->data = NULL;
}

static int luv_random(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  size_t buflen = (size_t)luaL_checkinteger(L, 1);
  // this is duplication of code in LibUV but since we need to try allocating the memory
  // before calling uv_random, we need to do this check ahead-of-time
  if (buflen > 0x7FFFFFFFu) {
    return luv_error(L, UV_E2BIG);
  }

  // flags param can be nil, an integer, or a table
  unsigned int flags = 0;
  if (lua_type(L, 2) == LUA_TNUMBER || lua_isnoneornil(L, 2)) {
    flags = (unsigned int)luaL_optinteger(L, 2, 0);
  }
  else if (lua_type(L, 2) == LUA_TTABLE) {
    // this is for forwards-compatibility: if flags ever get added,
    // we want to be able to take a table
  }
  else {
    return luaL_argerror(L, 2, "expected nil, integer, or table");
  }

  int cb_ref = luv_check_continuation(L, 3);
  int sync = cb_ref == LUA_NOREF;

  void* buf = lua_newuserdata(L, buflen);
  if (sync) {
    // sync version doesn't need anything except buf, buflen, and flags
    int ret = uv_random(NULL, NULL, buf, buflen, flags, NULL);
    if (ret < 0) {
      return luv_error(L, ret);
    }
    lua_pushlstring(L, (const char*)buf, buflen);
    return 1;
  }
  else {
    // ref buffer
    int buf_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    uv_random_t* req = (uv_random_t*)lua_newuserdata(L, sizeof(*req));
    req->data = luv_setup_req(L, ctx, cb_ref);
    ((luv_req_t*)req->data)->req_ref = buf_ref;

    int ret = uv_random(ctx->loop, req, buf, buflen, flags, luv_random_cb);
    if (ret < 0) {
      luv_cleanup_req(L, (luv_req_t*)req->data);
      lua_pop(L, 1);
      return luv_error(L, ret);
    }
    return luv_result(L, ret);
  }
}
#endif
