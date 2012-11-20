#include "lua-bind.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

char * find_config_path(void) {
    char base[128];
    char ret[256];

    char *xdg_config_dir = getenv("XDG_CONFIG_HOME");
    if ( xdg_config_dir ) {
        strncpy(base, xdg_config_dir, 127);
        goto APPEND;
    }

    char *home_dir = getenv("HOME");
    if ( home_dir ) {
        snprintf(base, 128, "%s/.config", home_dir);
        goto APPEND;
    }

    struct passwd pw;
    struct passwd *pw_ret;
    memset(&pw, 0, sizeof(struct passwd));

    char pw_buf[1024];
    if ( getpwuid_r(getuid(), &pw, pw_buf, 1024, &pw_ret) )
        err(1, "Couldn't get password entry for current user");

    if ( pw_ret->pw_dir ) {
        snprintf(base, 128, "%s/.config", pw_ret->pw_dir);
        goto APPEND;
    }

    errx(1, "Couldn't figure out home directory for current user");

APPEND:
    fprintf(stderr, "got base dir '%s'\n", base);
    snprintf(ret, 256, "%s/%s", base, "nimbleink");

    return strdup(ret);
}

int main(int argc, char **argv) {
    lua_State *L = luab_newstate();

#ifdef LOOP_GTK
    //gtk_init(&argc, &argv);
#endif

    luab_setup(L, argc, argv);

    char *config_file = find_config_path();
    luab_dofile(L, config_file);
    free(config_file);

    exit(0);
}

