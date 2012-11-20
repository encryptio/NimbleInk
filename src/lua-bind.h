#ifndef __LUABIND_H__
#define __LUABIND_H__

#include "lua.h"

void luab_setpath(lua_State *L, const char *path);
void luab_setup(lua_State *L, int argc, char **argv);
lua_State * luab_newstate(void);
int luab_dofile(lua_State *L, const char *path);

#endif
