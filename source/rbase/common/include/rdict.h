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
#include "rtools.h"

#ifdef __cplusplus
extern "C" {
#endif

#define rdict_size_t uint32_t
#define rdict_size_max 0xFFFFFFFFUL
#define rdict_size_min 0L
#define rdict_size_t_format "u"
#define rdict_bucket_capacity_default 16
#define rdict_init_capacity_default 16
#define rdict_scale_factor_default 0.75
#define rdict_used_factor_default 0.75
#define rdict_expand_factor 2
#define rdict_hill_expand_capacity (10240000 / sizeof(rdict_entry_t))
#define rdict_hill_add_capacity (1024000 / sizeof(rdict_entry_t))

typedef enum rdict_code_t {
    rdict_code_ok = 0,
    rdict_code_error = 1,
    rdict_code_not_exist = 2,
} rdict_code_t;

/* ------------------------------- Structs ------------------------------------*/

typedef struct rdict_entry_t {
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

    //struct rdict_entry_t *next;
} rdict_entry_t;

typedef uint64_t(*rdict_hash_func_type)(const void* key);
/** 0 - 不相等; !0 - 相等 **/
typedef int (*rdict_compare_key_func_type)(void* data_ext, const void* key1, const void* key2);

typedef struct rdict_t {
    rdict_entry_t *entry;
    rdict_entry_t *entry_null;
    rdict_size_t size;
    rdict_size_t capacity;
    rdict_size_t buckets;
    rdict_size_t bucket_capacity;
    float scale_factor;
    void* data_ext;

    rdict_hash_func_type hash_func;
    void (*set_key_func)(void* data_ext, rdict_entry_t* entry, const void* key);
    void (*set_value_func)(void* data_ext, rdict_entry_t* entry, const void* obj);
    void* (*copy_key_func)(void* data_ext, const void* key);
    void* (*copy_value_func)(void* data_ext, const void* obj);
    rdict_compare_key_func_type compare_key_func;
    void (*free_key_func)(void* data_ext, void* key);
    void (*free_value_func)(void* data_ext, void* obj);
    void (*expand_failed_func)(void* data_ext);
} rdict_t;

typedef struct rdict_iterator_t {
    rdict_t* d;
    rdict_entry_t *entry, *next;
} rdict_iterator_t;

typedef void (rdict_scan_func)(void* data_ext, const rdict_entry_t *de);
typedef void (rdict_scan_bucket_func)(void* data_ext, rdict_entry_t **bucketref);

/* ------------------------------- Macros ------------------------------------*/

#define rdict_is_key_equal(d, key1, key2) ((d)->compare_key_func((d)->data_ext, key1, key2) == rcode_eq)

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

#define rdict_block_declare_type_key_func(K) \
    uint64_t rdict_hash_func_##K(const K##_inner_type key); \
    K##_inner_type rdict_copy_key_func_##K(void* data_ext, const K##_inner_type key); \
    void rdict_free_key_func_##K(void* data_ext, K##_inner_type key); \
    int rdict_compare_key_func_##K(void* data_ext, const K##_inner_type key1, const K##_inner_type key2)

#define rdict_block_declare_type_value_func(V) \
    V##_inner_type rdict_copy_value_func_##V(void* data_ext, const V##_inner_type obj); \
    void rdict_free_value_func_##V(void* data_ext, V##_inner_type obj)

#define rdict_block_define_type_key_func(K) \
uint64_t rdict_hash_func_##K(const K##_inner_type key) { \
    if (K##_hash_func) { \
        return K##_hash_func(key); \
    } \
    return (uint64_t)key; \
} \
K##_inner_type rdict_copy_key_func_##K(void* data_ext, const K##_inner_type key) { \
    if (K##_copy_func) { \
        return K##_copy_func(key); \
    } \
    return (K##_inner_type)key; \
} \
void rdict_free_key_func_##K(void* data_ext, K##_inner_type key) { \
    if (K##_free_func) { \
        K##_free_func(key); \
    } \
} \
int rdict_compare_key_func_##K(void* data_ext, const K##_inner_type key1, const K##_inner_type key2) { \
    if (K##_compare_func) { \
        return K##_compare_func(key1, key2); \
    } \
    if ( key1 == key2) { \
        return 0; \
    } \
    return 1; \
}

#define rdict_block_define_type_value_func(V) \
V##_inner_type rdict_copy_value_func_##V(void* data_ext, const V##_inner_type obj) { \
    if (V##_copy_func) { \
        return V##_copy_func(obj); \
    } \
    return (V##_inner_type)obj; \
} \
void rdict_free_value_func_##V(void* data_ext, V##_inner_type obj) { \
    if (V##_free_func) { \
        V##_free_func(obj); \
    } \
} \


//#define rdict_compare_keys(d, key1, key2) \
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

rdict_t* rdict_create(rdict_size_t init_capacity, rdict_size_t bucket_capacity, void* data_ext);
int rdict_expand(rdict_t* d, rdict_size_t capacity);
int rdict_add(rdict_t* d, void* key, void* val);
int rdict_remove(rdict_t* d, const void* key);
/** 只置空数据，不释放entry内存 **/
void rdict_clear(rdict_t* d);
void rdict_release(rdict_t* d);
rdict_entry_t* rdict_find(rdict_t* d, const void* key);

/* 注意rehash失效 */
rdict_iterator_t* rdict_it_heap(rdict_t* d);
rdict_entry_t* rdict_next(rdict_iterator_t* it);


// rdict_block_declare_type_key_func(rdata_type_int);
// rdict_block_declare_type_key_func(rdata_type_int64);
// rdict_block_declare_type_key_func(rdata_type_uint64);
rdict_block_declare_type_key_func(rdata_type_string);
// rdict_block_declare_type_key_func(rdata_type_ptr);

// rdict_block_declare_type_value_func(rdata_type_int);
// rdict_block_declare_type_value_func(rdata_type_float);
// rdict_block_declare_type_value_func(rdata_type_double);
// rdict_block_declare_type_value_func(rdata_type_int64);
// rdict_block_declare_type_value_func(rdata_type_uint64);
rdict_block_declare_type_value_func(rdata_type_string);
// rdict_block_declare_type_value_func(rdata_type_ptr);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_DICT_H