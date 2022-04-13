/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RAY_DICT_H
#define RAY_DICT_H

#include "string.h"

#include "rcommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define rdict_size_t unsigned long
#define rdict_size_max 0xFFFFFFFFUL
#define rdict_size_min 0L
#define rdict_size_t_format "lu"
#define rdict_bucket_capacity_default 16
#define rdict_init_capacity_default 16
#define rdict_scale_factor_default 0.75
#define rdict_used_factor_default 0.75
#define rdict_expand_factor 2
#define rdict_hill_expand_capacity (10240000 / sizeof(rdict_entry))
#define rdict_hill_add_capacity (1024000 / sizeof(rdict_entry))

typedef enum rdict_code {
    rdict_code_ok = 0,
    rdict_code_error = 1,
    rdict_code_not_exist = 2,
} rdict_code;

/* ------------------------------- Structs ------------------------------------*/

typedef struct rdict_entry {
    union {
        void* ptr;
        uint64_t u64;
        int64_t s64;
        float f;
        double d;
    } key;

    union {
        void* ptr;
        uint64_t u64;
        int64_t s64;
        float f;
        double d;
    } value;

    //struct rdict_entry *next;
} rdict_entry;

typedef uint64_t(*rdict_hash_func_type)(const void* key);
/** 0 - 不相等; !0 - 相等 **/
typedef int (*rdict_compare_key_func_type)(void* data_ext, const void* key1, const void* key2);

typedef struct rdict {
    rdict_entry *entry;
    rdict_entry *entry_null;
    rdict_size_t size;
    rdict_size_t capacity;
    rdict_size_t buckets;
    rdict_size_t bucket_capacity;
    float scale_factor;
    void* data_ext;

    rdict_hash_func_type hash_func;
    void (*set_key_func)(void* data_ext, rdict_entry* entry, const void* key);
    void (*set_value_func)(void* data_ext, rdict_entry* entry, const void* obj);
    void* (*copy_key_func)(void* data_ext, const void* key);
    void* (*copy_value_func)(void* data_ext, const void* obj);
    rdict_compare_key_func_type compare_key_func;
    void (*free_key_func)(void* data_ext, void* key);
    void (*free_value_func)(void* data_ext, void* obj);
    void (*expand_failed_func)(void* data_ext);
} rdict;

typedef struct rdict_iterator {
    rdict* d;
    rdict_entry *entry, *next;
} rdict_iterator;

typedef void (rdict_scan_func)(void* data_ext, const rdict_entry *de);
typedef void (rdict_scan_bucket_func)(void* data_ext, rdict_entry **bucketref);

/* ------------------------------- Macros ------------------------------------*/

#define rdict_is_key_equal(d, key1, key2) (d)->compare_key_func((d)->data_ext, key1, key2)

#define rdict_free_key(d, entry) \
    if (entry) \
        if ((d)->free_key_func) \
            (d)->free_key_func((d)->data_ext, (entry)->key.ptr); \
        if ((entry)->key.ptr) \
            (entry)->key.ptr = NULL

#define rdict_free_value(d, entry) \
    if (entry) \
        if ((d)->free_value_func) \
            (d)->free_value_func((d)->data_ext, (entry)->value.ptr); \
        if ((entry)->value.ptr) \
            (entry)->value.ptr = NULL

#define rdict_set_key(d, entry, _key_) \
    if (entry) \
        if ((d)->copy_key_func) \
            (d)->set_key_func((d)->data_ext, (entry), (d)->copy_key_func((d)->data_ext, (_key_))); \
        else \
            (d)->set_key_func((d)->data_ext, (entry), (_key_))

#define rdict_set_value(d, entry, _value_) \
    if (entry) \
        if ((d)->copy_value_func) \
            (d)->set_value_func((d)->data_ext, (entry), (d)->copy_value_func((d)->data_ext, (_value_))); \
        else \
            (d)->set_value_func((d)->data_ext, (entry), (_value_))

#define rdict_set_type_value(d, entry, _value_, _type_) \
    static int rdict_set_value##_type_(d, entry, _value_) { \
        if (entry) \
            if ((d)->copy_value_func) \
                (entry)->value.##_type_ = (d)->copy_value_func((d)->data_ext, _value_); \
            else \
                (entry)->value.##_type_ = (_value_); \
        return 0; \
    }

#define rdict_set_signed_int_val(entry, _val_) \
    (entry)->value.s64 = (_val_);

#define rdict_set_unsigned_int_val(entry, _val_) \
    (entry)->value.u64 = (_val_);

#define dict_set_double_val(entry, _val_) \
    (entry)->value.d = (_val_);

#define dict_set_float_val(entry, _val_) \
    (entry)->value.f = (_val_);

#define rdict_compare_keys(d, key1, key2) \
    (((d)->compare_key_func) ? (d)->compare_key_func((d)->data_ext, (key1), (key2)) : (key1) == (key2))

#define rdict_get_buckets(d) ((d)->entry ? ((d)->entry) : NULL)
#define rdict_set_buckets(d, buckets) (d)->entry = (buckets)
#define rdict_get_entry(d, pos) ((d)->entry ? (d)->entry + (pos) : NULL)
#define rdict_get_entry_raw(entry, pos) ((entry) ? (entry) + (pos) : NULL)
#define rdict_hash_key(d, key) (d)->hash_func((key))
#define rdict_init_entry(entry, k, v) (entry)->key.ptr = (k); (entry)->value.ptr = (v)
#define rdict_get_key(entry) ((entry)->key.ptr)
#define rdict_get_value(entry) ((entry)->value.ptr)
#define rdict_size(d) ((d)->size)
#define rdict_rehashing(d) ((d)->rehash_id != 0)

//rdict_iterator* rdict_it(rdict* d);
#define rdict_it(d) \
    { \
        (d), (d)->entry, (d)->entry_null \
    }
#define rdict_it_first(it) \
    do { \
        (it)->entry = (it)->d->entry; \
        (it)->next = (it)->d->entry_null; \
    } while(0)

#define rdict_free(d) \
    if (d) { \
        rdict_release(d); \
        d = NULL; \
    }

/* ------------------------------- APIs ------------------------------------*/

uint64_t rhash_func_murmur(const char *key);

rdict* rdict_create(rdict_size_t init_capacity, rdict_size_t bucket_capacity, void* data_ext);
int rdict_expand(rdict* d, rdict_size_t capacity);
int rdict_add(rdict* d, void* key, void* val);
int rdict_remove(rdict* d, const void* key);
/** 只置空数据，不释放entry内存 **/
void rdict_clear(rdict* d);
void rdict_release(rdict* d);
rdict_entry* rdict_find(rdict* d, const void* key);

/* 注意rehash失效 */
rdict_iterator* rdict_it_heap(rdict* d);
rdict_entry* rdict_next(rdict_iterator* it);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_DICT_H