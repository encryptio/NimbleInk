#include "reference.h"
#include "inklog.h"
#define INKLOG_MODULE "reference"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <assert.h>

struct ref_queued_decrement {
    void *obj;
    decr_fn decr;
};

static struct ref_queued_decrement *ref_decr_queue = NULL;
static int ref_decr_queue_alloced = 0;
static int ref_decr_queue_used = 0;

static void ref_expand_queue(int n) {
    if ( ref_decr_queue_used + n >= ref_decr_queue_alloced ) {
        ref_decr_queue_alloced *= 2;
        if ( ref_decr_queue_alloced == 0 )
            ref_decr_queue_alloced = 8;

#if DEBUG_GC
        inklog(LOG_DEBUG, "realloc queue to %d elements", ref_decr_queue_alloced);
#endif

        if ( (ref_decr_queue = realloc(ref_decr_queue, ref_decr_queue_alloced*sizeof(struct ref_queued_decrement))) == NULL )
            err(1, "Couldn't allocate space for release pool");
    }
}

void ref_queue_decr(void *obj, decr_fn decr) {
    if ( !obj ) {
        inklog(LOG_CRIT, "Internal error: tried to queue ref decrement on NULL");
        exit(1);
    }

    if ( !decr ) {
        inklog(LOG_CRIT, "Internal error: tried to queue ref decrement with a NULL fn");
        exit(1);
    }

    ref_expand_queue(1);
    ref_decr_queue[ref_decr_queue_used].obj  = obj;
    ref_decr_queue[ref_decr_queue_used].decr = decr;
    ref_decr_queue_used++;
}

void ref_release_pool(void) {
    if ( !ref_decr_queue_used )
        return;

#if DEBUG_GC
    inklog(LOG_DEBUG, "releasing %d objects", ref_decr_queue_used);
    int thought = ref_decr_queue_used;

    int released = 0;
#endif

    // release in order that they were queued
    // also be careful to handle calls to ref_queue_decr while this is happening
    while ( ref_decr_queue_used ) {
        void *obj    = ref_decr_queue[0].obj;
        decr_fn decr = ref_decr_queue[0].decr;
        ref_decr_queue_used--;

        memmove(ref_decr_queue, ref_decr_queue+1, sizeof(struct ref_queued_decrement) * ref_decr_queue_used);

#if DEBUG_GC
        inklog(LOG_DEBUG, "calling decr=%p over %p", decr, obj);
        inklog(LOG_DEBUG, "    refcount=%d\n", *((int*)obj));
#endif

        assert( *((int*)obj) > 0 );

        decr(obj);

#if DEBUG_GC
        released++;
#endif
    }

#if DEBUG_GC
    if ( released != thought )
        inklog(LOG_DEBUG, "finished releasing, actually released %d objects", released);
#endif
}
