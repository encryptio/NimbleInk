#ifndef __LOOP_H__
#define __LOOP_H__

#include "reference.h"

#include <stdbool.h>
#include <inttypes.h>

struct loop_window; // left undefined, defined in loop-*.c

// bitmask fields for loop_fn_key_event's "mods"
#define LMOD_SHIFT (1 << 0)
#define LMOD_CTRL  (1 << 1)
#define LMOD_ALT   (1 << 2)
#define LMOD_CMD   (1 << 3)
#define LMOD_LOGO  (1 << 4)

// constants for loop_fn_key_event's "ch"
#define LKEY_UP       256
#define LKEY_DOWN     257
#define LKEY_LEFT     258
#define LKEY_RIGHT    259
#define LKEY_ESCAPE   260
#define LKEY_F1       261
#define LKEY_F2       262
#define LKEY_F3       263
#define LKEY_F4       264
#define LKEY_F5       265
#define LKEY_F6       266
#define LKEY_F7       267
#define LKEY_F8       268
#define LKEY_F9       269
#define LKEY_F10      270
#define LKEY_F11      271
#define LKEY_F12      272
#define LKEY_F13      273
#define LKEY_F14      274
#define LKEY_F15      275
#define LKEY_INSERT   276
#define LKEY_HOME     277
#define LKEY_END      278
#define LKEY_PAGEUP   279
#define LKEY_PAGEDOWN 280

typedef bool (*loop_fn_want_immediate)(void *pt, struct loop_window *win);
typedef void (*loop_fn_key_event)(void *pt, struct loop_window *win, bool is_repeat, uint32_t ch, uint8_t mods);

// MUST be called before any other loop_* functions are called
void loop_initialize(void);
void loop_until_quit(void);
void loop_set_quit(void);

struct loop_window *loop_window_open(
        char *path,
        void *pt,
        loop_fn_want_immediate want_immediate,
        loop_fn_key_event keyevt); // GC'd, closes when destroyed

void loop_window_incr(void *l);
void loop_window_decr(void *l);

static inline void loop_window_decr_q(struct loop_window *l) {
    ref_queue_decr((void*)(l), loop_window_decr);
}

struct zipper * loop_window_get_zipper(struct loop_window *win);
struct glview * loop_window_get_glview(struct loop_window *win);
void loop_window_set_fullscreen(struct loop_window *win, bool fullscreen);
bool loop_window_get_fullscreen(struct loop_window *win);

// event helpers
const char * loop_keyname(uint32_t ch);
uint32_t loop_keycode(const char *name);

#endif
