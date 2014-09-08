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

static void luv_process_on_exit(uv_process_t* process, int exit_status, int term_signal) {
  lua_State* L = luv_prepare_event(process->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (luv_get_callback(L, "onexit")) {
    lua_pushinteger(L, exit_status);
    lua_pushinteger(L, term_signal);
    luv_call(L, 3, 0);
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif

}

static int luv_spawn(lua_State* L) {
  const char* command = luaL_checkstring(L, 1);
  char** env = NULL;
  uv_stdio_container_t* stdio = NULL;
  uv_process_options_t options;
  uv_process_t* handle;
  size_t stdioc = 0;
  size_t i;
  char* cwd;
  int r;
  /* process the args list */
  /* +1 for inserted command at front */
  size_t argc = lua_rawlen(L, 2) + 1;
  /* +1 for null terminator at end */
  char** args = malloc((argc + 1) * sizeof(*args));
  args[0] = strdup(command);
  for (i = 1; i < argc; ++i) {
    lua_rawgeti(L, 2, i);
    args[i] = (char*)lua_tostring(L, -1);
    lua_pop(L, 1);
  }
  args[argc] = NULL;

  /* Get the cwd */
  lua_getfield(L, 3, "cwd");
  cwd = (char*)lua_tostring(L, -1);
  lua_pop(L, 1);

  /* Get the env */
  lua_getfield(L, 3, "env");
  if (lua_type(L, -1) == LUA_TTABLE) {
    argc = lua_rawlen(L, -1);
    env = malloc((argc + 1) * sizeof(char*));
    for (i = 0; i < argc; ++i) {
      lua_rawgeti(L, -1, i + 1);
      env[i] = (char*)lua_tostring(L, -1);
      lua_pop(L, 1);
    }
    env[argc] = NULL;
  }
  lua_pop(L, 1);

  /* get the stdio list */
  lua_getfield(L, 3, "stdio");
  if (lua_type(L, -1) == LUA_TTABLE) {
    stdioc = lua_rawlen(L, -1);
    stdio = malloc(stdioc * sizeof(*stdio));
    for (i = 0; i < stdioc; ++i) {
      lua_rawgeti(L, -1, i + 1);
      /* integers are assumed to be file descripters */
      if (lua_type(L, -1) == LUA_TNUMBER) {
        stdio[i].flags = UV_INHERIT_FD;
        stdio[i].data.fd = lua_tointeger(L, -1);
      }
      /* userdata is assumed to be a uv_stream_t instance */
      else if (lua_type(L, -1) == LUA_TUSERDATA) {
        uv_stream_t* stream = luv_get_stream(L, -1);
        if (stream->type == UV_NAMED_PIPE) {
          /* TODO: make this work for pipes with existing FDs */

          stdio[i].flags = UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE;
        }
        else {
          stdio[i].flags = UV_INHERIT_STREAM;
        }
        stdio[i].data.stream = stream;
      }
      else {
        stdio[i].flags = UV_IGNORE;
      }
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  memset(&options, 0, sizeof(uv_process_options_t));
  options.exit_cb = luv_process_on_exit;
  options.file = command;
  options.args = args;
  options.env = env;
  options.cwd = cwd;
  options.flags = 0;
  options.stdio_count = stdioc;
  options.stdio = stdio;

  handle = luv_create_process(L);
  r = uv_spawn(uv_default_loop(), handle, options);
  free(args);
  free(stdio);
  free(env);

  if (r) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "spawn: %s", uv_strerror(err));
  }

  luv_handle_ref(L, handle->data, -1);

  /* Return the Pid */
  lua_pushinteger(L, handle->pid);

  return 2;
}

static int luv_parse_signal(lua_State* L, int slot) {
  if (lua_isnumber(L, slot)) {
    return lua_tonumber(L, slot);
  }
  if (lua_isstring(L, slot)) {
    const char* string = lua_tostring(L, slot);
#ifdef SIGHUP
    if (strcmp(string, "SIGHUP") == 0) return SIGHUP;
#endif
#ifdef SIGINT
    if (strcmp(string, "SIGINT") == 0) return SIGINT;
#endif
#ifdef SIGQUIT
    if (strcmp(string, "SIGQUIT") == 0) return SIGQUIT;
#endif
#ifdef SIGILL
    if (strcmp(string, "SIGILL") == 0) return SIGILL;
#endif
#ifdef SIGTRAP
    if (strcmp(string, "SIGTRAP") == 0) return SIGTRAP;
#endif
#ifdef SIGABRT
    if (strcmp(string, "SIGABRT") == 0) return SIGABRT;
#endif
#ifdef SIGIOT
    if (strcmp(string, "SIGIOT") == 0) return SIGIOT;
#endif
#ifdef SIGBUS
    if (strcmp(string, "SIGBUS") == 0) return SIGBUS;
#endif
#ifdef SIGFPE
    if (strcmp(string, "SIGFPE") == 0) return SIGFPE;
#endif
#ifdef SIGKILL
    if (strcmp(string, "SIGKILL") == 0) return SIGKILL;
#endif
#ifdef SIGUSR1
    if (strcmp(string, "SIGUSR1") == 0) return SIGUSR1;
#endif
#ifdef SIGSEGV
    if (strcmp(string, "SIGSEGV") == 0) return SIGSEGV;
#endif
#ifdef SIGUSR2
    if (strcmp(string, "SIGUSR2") == 0) return SIGUSR2;
#endif
#ifdef SIGPIPE
    if (strcmp(string, "SIGPIPE") == 0) return SIGPIPE;
#endif
#ifdef SIGALRM
    if (strcmp(string, "SIGALRM") == 0) return SIGALRM;
#endif
#ifdef SIGTERM
    if (strcmp(string, "SIGTERM") == 0) return SIGTERM;
#endif
#ifdef SIGCHLD
    if (strcmp(string, "SIGCHLD") == 0) return SIGCHLD;
#endif
#ifdef SIGSTKFLT
    if (strcmp(string, "SIGSTKFLT") == 0) return SIGSTKFLT;
#endif
#ifdef SIGCONT
    if (strcmp(string, "SIGCONT") == 0) return SIGCONT;
#endif
#ifdef SIGSTOP
    if (strcmp(string, "SIGSTOP") == 0) return SIGSTOP;
#endif
#ifdef SIGTSTP
    if (strcmp(string, "SIGTSTP") == 0) return SIGTSTP;
#endif
#ifdef SIGBREAK
    if (strcmp(string, "SIGBREAK") == 0) return SIGBREAK;
#endif
#ifdef SIGTTIN
    if (strcmp(string, "SIGTTIN") == 0) return SIGTTIN;
#endif
#ifdef SIGTTOU
    if (strcmp(string, "SIGTTOU") == 0) return SIGTTOU;
#endif
#ifdef SIGURG
    if (strcmp(string, "SIGURG") == 0) return SIGURG;
#endif
#ifdef SIGXCPU
    if (strcmp(string, "SIGXCPU") == 0) return SIGXCPU;
#endif
#ifdef SIGXFSZ
    if (strcmp(string, "SIGXFSZ") == 0) return SIGXFSZ;
#endif
#ifdef SIGVTALRM
    if (strcmp(string, "SIGVTALRM") == 0) return SIGVTALRM;
#endif
#ifdef SIGPROF
    if (strcmp(string, "SIGPROF") == 0) return SIGPROF;
#endif
#ifdef SIGWINCH
    if (strcmp(string, "SIGWINCH") == 0) return SIGWINCH;
#endif
#ifdef SIGIO
    if (strcmp(string, "SIGIO") == 0) return SIGIO;
#endif
#ifdef SIGPOLL
# if SIGPOLL != SIGIO
    if (strcmp(string, "SIGPOLL") == 0) return SIGPOLL;
# endif
#endif
#ifdef SIGLOST
    if (strcmp(string, "SIGLOST") == 0) return SIGLOST;
#endif
#ifdef SIGPWR
# if SIGPWR != SIGLOST
    if (strcmp(string, "SIGPWR") == 0) return SIGPWR;
# endif
#endif
#ifdef SIGSYS
    if (strcmp(string, "SIGSYS") == 0) return SIGSYS;
#endif
    return luaL_error(L, "Unknown signal '%s'", string);
  }
  return SIGTERM;
}

static int luv_kill(lua_State* L) {
  int pid = luaL_checkint(L, 1);
  int signum = luv_parse_signal(L, 2);
  uv_err_t err = uv_kill(pid, signum);
  if (err.code) {
    return luaL_error(L, "kill: %s", uv_strerror(err));
  }
  return 0;
}

static int luv_process_kill(lua_State* L) {
  uv_process_t* handle = luv_get_process(L, 1);
  int signum = luv_parse_signal(L, 2);
  if (uv_process_kill(handle, signum)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "process_kill: %s", uv_strerror(err));
  }
  return 0;
}
