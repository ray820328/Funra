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

static int _expand_buckets(rdict *d, rdict_size capacity);

rdict *rdict_create(rdict_size init_capacity, void *data_ext) {
    rdict* d = rnew_data(rdict);

    d->entry = NULL;

    d->size = 0;
    d->capacity = 0;
    d->buckets = 0;
    d->bucket_capacity = rdict_bucket_capacity_default;
    d->scale_factor = rdict_scale_factor_default;
    d->data_ext = data_ext;

    d->hash_func = NULL;
    d->copy_key_func = NULL;
    d->copy_value_func = NULL;
    d->compare_key_func = NULL;
    d->free_key_func = NULL;
    d->free_value_func = NULL;
    d->expand_failed_func = expand_failed_func_default;

    _expand_buckets(d, init_capacity ? init_capacity : rdict_init_capacity_default);

    return d;
}

static int _expand_buckets(rdict *d, rdict_size capacity) {
    if (d == NULL) {
        return rdict_code_error;
    }

    //初始化所有entry对象空间
    rdict_size bucket_count = capacity / d->bucket_capacity + 1;
    rdict_size dest_capacity = bucket_count * d->bucket_capacity;
    rdict_entry **new_entry = NULL;
    new_entry = fn_raycmalloc(dest_capacity, sizeof(rdict_entry));// (d->entry)));
    if (new_entry == NULL) {
        return rdict_code_error;
    }

    rdict_entry **entry = rdict_get_buckets(d);
    if (entry == NULL) {
        d->capacity = dest_capacity;// *(d->entry));
        d->size = 0;
        d->buckets = bucket_count;
        rdict_set_buckets(d, new_entry);

        return rdict_code_ok;
    }

    rayprintf(RLOG_DEBUG, "expand map, size vs capacity = %"rdict_size_format" : %"rdict_size_format" -> %"rdict_size_format, 
        d->size, d->capacity, dest_capacity);
    if (d->size * 1.0 / d->capacity < rdict_used_factor_default) { //使用率太小，换hash算法
        rayprintf(RLOG_WARN, "used ratio under the score of %1.3f", d->size * 1.0 / d->capacity);
    }

    rdict_entry *bucket_next = *d->entry;
    rdict_entry *new_bucket_cur = NULL;
    uint64_t bucket_pos = 0;
    for (rdict_size i = 0; i < d->buckets; ++i) {
        if (bucket_next->key.ptr == NULL) {
            ++bucket_next;
            continue; //桶内元素保持连续
        }

        bucket_pos = d->hash_func(bucket_next->key.ptr) % bucket_count * d->bucket_capacity;
        new_bucket_cur = new_entry[bucket_pos];
        if (likely(new_bucket_cur->key.ptr)) {
            while (++new_bucket_cur) {
                if (new_bucket_cur - new_entry[bucket_pos] >= d->bucket_capacity - 1) {
                    return _expand_buckets(d, capacity * 2);//递归扩容
                }
            }

            memcpy(new_bucket_cur, bucket_next, sizeof(rdict_entry));
        }else{
            new_entry[bucket_pos] = memcpy(new_bucket_cur, bucket_next, sizeof(rdict_entry));
        }

        ++bucket_next;
    }

    //最后才能释放原始表
    if (d->entry) {
        rayfree(d->entry);
        d->entry = new_entry;
    }

    return rdict_code_ok;
}

int rdict_expand(rdict *d, rdict_size capacity) {
    if (d->expand_failed_func) d->expand_failed_func = NULL;

    rdict_entry **new_d;
    rdict_size real_capacity = (capacity + 1) / 2 * 2;

    if (real_capacity > rdict_size_max || real_capacity < rdict_size_min) {
        rayprintf(RLOG_ERROR, "invalid size.");
        return rdict_code_error;
    }

    return _expand_buckets(d, real_capacity);
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