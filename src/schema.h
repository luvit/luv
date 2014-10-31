#ifndef LSCHEMA_H
#define LSCHEMA_H

#include "luv.h"

typedef const struct {
  const char* name;
  int (*checker)(lua_State* L, int index);
} lschema_entry;

static int luv_isfile(lua_State* L, int index);
static int luv_iscontinuation(lua_State* L, int index);
void lschema_check(lua_State* L, lschema_entry schema[]);

#endif
