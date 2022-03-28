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

#define rdict_size unsigned long
#define rdict_size_max LONG_MAX
#define rdict_size_min 0L
#define rdict_size_format "ul"
#define rdict_bucket_capacity_default 16
#define rdict_init_capacity_default 16
#define rdict_scale_factor_default 0.75
#define rdict_used_factor_default 0.75

typedef enum rdict_code {
    rdict_code_ok = 0,
    rdict_code_error = 1,
} rdict_code;

typedef struct rdict_entry {
    union {
        void *ptr;
        uint64_t u64;
        int64_t s64;
        float f;
        double d;
    } key;

    union {
        void *ptr;
        uint64_t u64;
        int64_t s64;
        float f;
        double d;
    } value;

    struct rdict_entry *next;
} rdict_entry;

typedef struct rdict {
    rdict_entry **entry;
    rdict_size size;
    rdict_size capacity;
    rdict_size buckets;
    rdict_size bucket_capacity;
    float scale_factor;
    void* data_ext;

    uint64_t (*hash_func)(const void *key);
    void *(*copy_key_func)(void *data_ext, const void *key);
    void *(*copy_value_func)(void *data_ext, const void *obj);
    int (*compare_key_func)(void *data_ext, const void *key1, const void *key2);
    void (*free_key_func)(void *data_ext, void *key);
    void (*free_value_func)(void *data_ext, void *obj);
    void (*expand_failed_func)(void *data_ext);
} rdict;

typedef struct rdict_iterator {
    rdict *d;
    long index;
    int table, safe;
    rdict_entry *entry, *next;
    /* unsafe iterator fingerprint for misuse detection. */
    long long fingerprint;
} rdict_iterator;

typedef void (rdict_scan_func)(void *data_ext, const rdict_entry *de);
typedef void (rdict_scan_bucket_func)(void *data_ext, rdict_entry **bucketref);

/* ------------------------------- Macros ------------------------------------*/
#define rdict_free_key(d, entry) \
    if ((d)->free_key_func) \
        (d)->free_key_func((d)->data_ext, (entry)->key.ptr); \
    if ((entry)->key.ptr) \
        (entry)->key.ptr = NULL

#define rdict_free_value(d, entry) \
    if ((d)->free_value_func) \
        (d)->free_value_func((d)->data_ext, (entry)->value.ptr); \
    if ((entry)->value.ptr) \
        (entry)->value.ptr = NULL

#define rdict_set_key(d, entry, _key_) do { \
    if ((d)->copy_key_func) \
        (entry)->key.ptr = (d)->copy_key_func((d)->data_ext, _key_); \
    else \
        (entry)->key.ptr = (_key_); \
} while(0)

#define rdict_set_type_value(d, entry, _value_, _type_) \
    static int rdict_set_value##_type_(d, entry, _value_) { \
        if ((d)->copy_value_func) \
            (entry)->value.##_type_ = (d)->copy_value_func((d)->data_ext, _value_); \
        else \
            (entry)->value.##_type_ = (_value_); \
        return 0; \
    }


#define rdict_set_value(d, entry, _value_) do { \
    if ((d)->copy_value_func) \
        (entry)->value.ptr = (d)->copy_value_func((d)->data_ext, _value_); \
    else \
        (entry)->value.ptr = (_value_); \
} while(0)

#define rdict_setSignedIntegerVal(entry, _val_) \
    do { (entry)->v.value.s64 = _val_; } while(0)

#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.value.u64 = _val_; } while(0)

#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.value.d = _val_; } while(0)

#define rdict_compare_keys(d, key1, key2) \
    (((d)->compare_key_func) ? \
        (d)->compare_key_func((d)->data_ext, key1, key2) : \
        (key1) == (key2))


#define rdict_get_buckets(d) ((d)->entry)
#define rdict_set_buckets(d, buckets) (d)->entry = (buckets)
#define rdict_hash_key(d, key) (d)->hash_func(key)
#define rdict_get_key(he) ((he)->key)
#define rdict_get_value(he) ((he)->v.value)
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)
#define dictGetDoubleVal(he) ((he)->v.d)
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)
#define dictIsRehashing(d) ((d)->rehashidx != -1)
#define dictPauseRehashing(d) (d)->pauserehash++
#define dictResumeRehashing(d) (d)->pauserehash--

/* If our unsigned long type can store a 64 bit number, use a 64 bit PRNG. */
#if ULONG_MAX >= 0xffffffffffffffff
#define randomULong() ((unsigned long) genrand64_int64())
#else
#define randomULong() random()
#endif

/* API */
rdict *rdict_create(rdict_size init_capacity, void *data_ext);
int rdict_expand(rdict *d, rdict_size capacity);
//int dictTryExpand(rdict *d, rdict_size capacity);
int rdict_add(rdict *d, void *key, void *val);
//dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);
//dictEntry *dictAddOrFind(dict *d, void *key);
//int dictReplace(dict *d, void *key, void *val);
int rdict_remove(rdict *d, const void *key);
//dictEntry *dictUnlink(dict *ht, const void *key);
//void dictFreeUnlinkedEntry(dict *d, dictEntry *he);
void rdict_release(rdict *d);
rdict_entry * rdict_find(rdict *d, const void *key);
//void *dictFetchValue(dict *d, const void *key);
//int dictResize(dict *d);
//dictIterator *dictGetIterator(dict *d);
//dictIterator *dictGetSafeIterator(dict *d);
//dictEntry *dictNext(dictIterator *iter);
//void dictReleaseIterator(dictIterator *iter);
//dictEntry *dictGetRandomKey(dict *d);
//dictEntry *dictGetFairRandomKey(dict *d);
//unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);
//void dictGetStats(char *buf, size_t bufsize, dict *d);
//uint64_t dictGenHashFunction(const void *key, int len);
//uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);
//void dictEmpty(dict *d, void(callback)(void*));
//void dictEnableResize(void);
//void dictDisableResize(void);
//int dictRehash(dict *d, int n);
//int dictRehashMilliseconds(dict *d, int ms);
//void dictSetHashFunctionSeed(uint8_t *seed);
//uint8_t *dictGetHashFunctionSeed(void);
//unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);
//uint64_t dictGetHash(dict *d, const void *key);
//dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, uint64_t hash);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_DICT_H