#include <stdlib.h>
#include <string.h> 
#include <err.h>
#include "loop.h"

struct key_pair {
    uint32_t ch;
    const char *name;
};

static struct key_pair pairs_by_ch[] = {
    {LKEY_UP,     "up"},
    {LKEY_DOWN,   "down"},
    {LKEY_LEFT,   "left"},
    {LKEY_RIGHT,  "right"},
    {LKEY_ESCAPE, "escape"},
    {LKEY_F1,     "F1"},
    {LKEY_F2,     "F2"},
    {LKEY_F3,     "F3"},
    {LKEY_F4,     "F4"},
    {LKEY_F5,     "F5"},
    {LKEY_F6,     "F6"},
    {LKEY_F7,     "F7"},
    {LKEY_F8,     "F8"},
    {LKEY_F9,     "F9"},
    {LKEY_F10,    "F10"},
    {LKEY_F11,    "F11"},
    {LKEY_F12,    "F12"},
    {LKEY_F13,    "F13"},
    {LKEY_F14,    "F14"},
    {LKEY_F15,    "F15"},
    {'\b',        "backspace"},
    {' ',         "space"},
    {'\t',        "tab"},
    {'\n',        "enter"},
    {'`',         "backtick"},
    {'~',         "tilde"},
    {'!',         "exclamation"},
    {'@',         "at"},
    {'#',         "hash"},
    {'$',         "dollar"},
    {'%',         "percent"},
    {'^',         "carat"},
    {'&',         "ampersand"}
};

#define KEYPAIR_COUNT (sizeof(pairs_by_ch)/sizeof(pairs_by_ch[0]))

static struct key_pair *pairs_by_name;

static int compare_by_ch(const void *a, const void *b) {
    uint32_t a_ch = ((struct key_pair *) a)->ch;
    uint32_t b_ch = ((struct key_pair *) b)->ch;
    if ( a_ch > b_ch ) return 1;
    if ( a_ch < b_ch ) return -1;
    return 0;
}

static int compare_by_name(const void *a, const void *b) {
    const char *a_name = ((struct key_pair *) a)->name;
    const char *b_name = ((struct key_pair *) b)->name;
    return strcmp(a_name, b_name);
}

void loop_setup_keypairs(void) {
    qsort(pairs_by_ch, KEYPAIR_COUNT, sizeof(struct key_pair), compare_by_ch);

    if ( (pairs_by_name = malloc(sizeof(struct key_pair) * KEYPAIR_COUNT)) == NULL )
        err(1, "Couldn't allocate space for pairs_by_name key list");
    memcpy(pairs_by_name, pairs_by_ch, sizeof(struct key_pair) * KEYPAIR_COUNT);

    qsort(pairs_by_name, KEYPAIR_COUNT, sizeof(struct key_pair), compare_by_name);
}

const char * loop_keyname(uint32_t ch) {
    struct key_pair query;
    query.ch = ch;

    struct key_pair *res = bsearch(&query, pairs_by_ch, KEYPAIR_COUNT, sizeof(struct key_pair), compare_by_ch);

    if ( !res )
        return NULL;
    return res->name;
}

uint32_t loop_keycode(const char *name) {
    struct key_pair query;
    query.name = name;

    struct key_pair *res = bsearch(&query, pairs_by_ch, KEYPAIR_COUNT, sizeof(struct key_pair), compare_by_name);

    if ( !res )
        return 0;
    return res->ch;
}

