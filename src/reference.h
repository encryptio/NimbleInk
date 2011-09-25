#ifndef __REFERENCE_H__
#define __REFERENCE_H__

// refcounting garbage collecting
//
// this code implements a release pool, to allow returning of values
// with effective refcount 0

typedef void (*decr_fn)(void *obj);
typedef void (*incr_fn)(void *obj);

void ref_queue_decr(void *obj, decr_fn decr);
void ref_release_pool(void);

#endif
