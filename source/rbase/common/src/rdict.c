/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "stdlib.h"
#include "string.h"

#include "rcommon.h"
#include "rtime.h"
#include "rlog.h"
#include "rdict.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif //__GNUC__

rattribute_unused(static volatile bool rdict_inited = false);

static void expand_failed_func_default(void *data_ext) {
    rassert(false, "default action");
}

rdict *rdict_create(rdict_size init_capacity, void *data_ext) {
    rdict* d = rnew_data(rdict);

    d->entry = NULL;

    d->size = 0;
    d->capacity = 0;
    d->init_capacity = init_capacity ? init_capacity : rdict_init_capacity_default;
    d->scale_factor = rdict_scale_factor_default;
    d->data_ext = data_ext;

    d->hash_func = NULL;
    d->copy_key_func = NULL;
    d->copy_value_func = NULL;
    d->compare_key_func = NULL;
    d->free_key_func = NULL;
    d->free_value_func = NULL;
    d->expand_failed_func = expand_failed_func_default;

    return 0;
}

int rdict_expand(rdict *d, rdict_size capacity) {
    if (d->expand_failed_func) d->expand_failed_func = NULL;

    rdict_entry **new_d;
    rdict_size real_capacity = capacity;

    if (real_capacity > rdict_size_max || real_capacity < rdict_size_min)
        rayprintf(RLOG_ERROR, "invalid size.");
        return 1;

    n.size = realsize;
    n.sizemask = realsize - 1;
    if (malloc_failed) {
        n.table = ztrycalloc(realsize * sizeof(dictEntry*));
        *malloc_failed = n.table == NULL;
        if (*malloc_failed)
            return DICT_ERR;
    }
    else
        n.table = zcalloc(realsize * sizeof(dictEntry*));

    n.used = 0;

    /* Is this the first initialization? If so it's not really a rehashing
     * we just set the first hash table so that it can accept keys. */
    if (d->ht[0].table == NULL) {
        d->ht[0] = n;
        return 0;
    }

    /* Prepare a second hash table for incremental rehashing */
    d->ht[1] = n;
    d->rehashidx = 0;

    return 0;
}

int rdict_add(rdict *d, void *key, void *val) {

    return 0;
}

int rdict_remove(rdict *d, const void *key) {

    return 0;
}

void rdict_release(rdict *d) {

}

rdict_entry * rdict_find(rdict *d, const void *key) {

    return 0;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__