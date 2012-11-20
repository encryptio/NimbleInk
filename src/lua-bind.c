#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <stdbool.h>
#include <assert.h>

lua_State * luab_newstate(void) {
    lua_State *L = luaL_newstate();
    if ( L == NULL )
        err(1, "Couldn't allocate space for new Lua interpereter state");

    luaL_openlibs(L);

    // more aggressive garbage collection
    lua_gc(L, LUA_GCSETPAUSE, 400);
    lua_gc(L, LUA_GCSETSTEPMUL, 400);

    return L;
}

// Takes a dot-separated path as an argument and sets "<path> = top" where top is the element at the top of the stack.
// Eats the top element.
// [-1, +0, -]
void luab_setpath(lua_State *L, const char *path) {
    // stack: ..., Value
    lua_checkstack(L, 2);

    const char *at = path;
    const char *end = strchr(at, '.');

    // no . means it's directly a global
    if ( !end ) {
        lua_setglobal(L, path);
        return;
    }
    
    char part[64];
    strncpy(part, at, end-at);
    part[63] = '\0';
    if ( end-at < 64 )
        part[end-at] = '\0';

    // part is the first section, a subkey of the global table

    lua_getglobal(L, part);
    if ( lua_isnil(L, -1) ) {
        // stack: ..., Value, nil
        lua_pop(L, 1);
        lua_createtable(L, 0, 1);
        lua_setglobal(L, part);
        lua_getglobal(L, part);
        assert(!lua_isnil(L, -1));
    }

    // stack: ..., Value, table

    at = end+1;

    while ( *at ) {
        end = strchr(at, '.');

        if ( end == NULL ) {
            // last section, do the final set

            lua_insert(L, -2);
            // stack: ..., table, Value

            lua_setfield(L, -2, at);
            // stack: ..., table

            lua_pop(L, 1);
            return;
        } else {
            // more sections to go, do a key lookup
            strncpy(part, at, end-at);
            part[63] = '\0';
            if ( end-at < 64 )
                part[end-at] = '\0';

            at = end+1;

            lua_getfield(L, -1, part);

            // stack: ..., Value, table, (subtable|nil)

            if ( lua_isnil(L, -1) ) {
                lua_pop(L, 1);
                lua_createtable(L, 0, 1);
                lua_setfield(L, -2, part);
                lua_getfield(L, -1, part);
            }

            // stack: ..., Value, table, subtable

            lua_insert(L, -2);
            lua_pop(L, 1);
            // stack: ..., Value, subtable
        }
    }

    assert(0);
}

static void set_lua_sysargs(lua_State *L, int argc, char **argv) {
    lua_checkstack(L, 3);

    // make the args table
    lua_createtable(L, argc, 0);
    for (int i = 0; i < argc; i++) {
        lua_pushinteger(L, i+1);
        lua_pushstring(L, argv[i]);
        lua_settable(L, -3);
    }

    luab_setpath(L, "sys.args");
}

void luab_setup(lua_State *L, int argc, char **argv) {
    set_lua_sysargs(L, argc, argv);
}

int luab_dofile(lua_State *L, const char *path) {
    int err;
    if ( (err = luaL_loadfile(L, path)) == 0 )
        err = lua_pcall(L, 0, 0, 0);

    if ( err ) {
        switch (err) {
            case LUA_ERRFILE:
                fprintf(stderr, "Couldn't read %s:\n", path);
                break;

            case LUA_ERRRUN:
                fprintf(stderr, "Runtime error while evaluating %s:\n", path);
                break;

            case LUA_ERRMEM:
                fprintf(stderr, "Out of memory while evaluating %s:\n", path);
                break;

            case LUA_ERRSYNTAX:
                fprintf(stderr, "Syntax error in %s:\n", path);
                break;

            default:
                fprintf(stderr, "Couldn't evaluate %s (code %d):\n", path, err);
        }

        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    return err;
}

