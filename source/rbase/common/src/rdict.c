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

static rdict_entry* _find_bucket(rdict* d, void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity);
static rdict_entry* _find_bucket_raw(rdict_hash_func_type hash_func, rdict_entry* d_entry_start, void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity);

static void expand_failed_func_default(void* data_ext) {
    rassert(false, "default action");
}

static uint64_t hash_func_default(const void* key) {
    return key;
}

static void set_key_func_default(void* data_ext, rdict_entry* entry, const void* key) {
    entry->key.ptr = key;
}

static void set_value_func_default(void* data_ext, rdict_entry* entry, const void* obj) {
    entry->value.ptr = obj;
}

static int compare_key_func_default(void* data_ext, const void* key1, const void* key2) {
    if (key1 == key2)
        return 1;
    return 0;
}

rdict *rdict_create(rdict_size_t init_capacity, rdict_size_t bucket_capacity, void* data_ext) {
    rdict* d = rnew_data(rdict);

    d->entry = NULL;
    d->entry_null = NULL;

    d->size = 0;
    d->scale_factor = rdict_scale_factor_default;
    d->capacity = init_capacity > 0 ? init_capacity : rdict_init_capacity_default;
    d->buckets = 0;
    d->bucket_capacity = bucket_capacity <= 0 ? rdict_bucket_capacity_default : bucket_capacity;
    d->data_ext = data_ext;

    d->hash_func = hash_func_default;
    d->set_key_func = set_key_func_default;
    d->set_value_func = set_value_func_default;
    d->copy_key_func = NULL;
    d->copy_value_func = NULL;
    d->compare_key_func = compare_key_func_default;
    d->free_key_func = NULL;
    d->free_value_func = NULL;
    d->expand_failed_func = expand_failed_func_default;

    rdict_expand(d, d->capacity);

    return d;
}

static int _expand_buckets(rdict* d, rdict_size_t capacity) {
    if (d == NULL) {
        return rdict_code_error;
    }

    //初始化所有entry对象空间
    rdict_size_t bucket_count = capacity % d->bucket_capacity == 0 ? capacity / d->bucket_capacity : capacity / d->bucket_capacity + 1;
    rdict_size_t dest_capacity = bucket_count * d->bucket_capacity;
    rdict_entry *new_entry = NULL;
    new_entry = fn_raycmalloc(dest_capacity, rdict_entry);// (d->entry)));
    if (new_entry == NULL) {
        rlog(RLOG_ERROR, "invalid malloc.");
        return rdict_code_error;
    }
    //for (size_t i = 0; i < dest_capacity; i++) {
    //    rdict_init_entry(&new_entry[i], 0, 0);
    //}

    rdict_entry *entry = rdict_get_buckets(d);
    if (entry == NULL) {
        d->capacity = dest_capacity;// *(d->entry));
        d->size = 0;
        d->buckets = bucket_count;
        rdict_set_buckets(d, new_entry);

        return rdict_code_ok;
    }

    rlog(RLOG_DEBUG, "expand map, size : capacity = %"rdict_size_t_format" : %"rdict_size_t_format" -> %"rdict_size_t_format"\n", 
        d->size, d->capacity, dest_capacity);
    if (d->size * 1.0 / d->capacity < rdict_used_factor_default) { //使用率太小，换hash算法
        rlog(RLOG_WARN, "used ratio under the score of %1.3f\n", d->size * 1.0 / d->capacity);
    }

    rdict_entry* entry_next = NULL;
    rdict_entry* entry_start = entry_next = rdict_get_buckets(d);
    rdict_entry* new_bucket_cur = NULL;
    rdict_size_t inc = 0;
    for (rdict_size_t i = 0; i < d->capacity; ) {
        entry_next = entry_start + i;

        if (entry_next->key.ptr == NULL) {
            i = (i / d->bucket_capacity + 1) * d->bucket_capacity;
            continue; //桶内元素保持连续
        }

        new_bucket_cur = _find_bucket_raw(d->hash_func, new_entry, entry_next->key.ptr, bucket_count, d->bucket_capacity);
        for (inc = 0; new_bucket_cur && new_bucket_cur->key.ptr; ++new_bucket_cur, ++inc) {
            if (inc >= d->bucket_capacity - 1) {
                rayfree(new_entry);//释放当前申请的块
                new_entry = NULL;
                return rdict_expand(d, capacity * 2);//递归扩容
            }
        }
        memcpy(new_bucket_cur, entry_next, sizeof(rdict_entry));

        i++;
    }

    //最后才能释放原始表
    if (d->entry) {
        rayfree(d->entry);
        d->entry = new_entry;
        d->capacity = dest_capacity;
        d->buckets = bucket_count;
    }

    return rdict_code_ok;
}

int rdict_expand(rdict* d, rdict_size_t capacity) {
    if (d->expand_failed_func) d->expand_failed_func = NULL;

    rdict_size_t real_capacity = capacity;
    if (real_capacity < rdict_hill_expand_capacity) {//小于峰值，按倍数扩容
        real_capacity *= rdict_expand_factor;
    }
    else {
        real_capacity += rdict_hill_add_capacity;
    }
    real_capacity = real_capacity / d->scale_factor;//按比率扩大

    if (real_capacity > rdict_size_max || real_capacity < rdict_size_min) {
        rlog(RLOG_ERROR, "invalid size: "rdict_size_t_format, capacity);
        return rdict_code_error;
    }

    return _expand_buckets(d, real_capacity);
}

int rdict_add(rdict* d, void* key, void* val) {
    if (d == NULL) {
        return rdict_code_error;
    }
    if (!key) {
        if (!d->entry_null) {
            d->entry_null = rnew_data(rdict_entry);
            d->size += 1;
            rdict_set_key(d, d->entry_null, key);
        } else {
            rdict_free_value(d, d->entry_null);
        }
        rdict_set_value(d, d->entry_null, val);

        return rdict_code_ok;
    }

    rdict_entry *entry_cur = NULL;
    rdict_entry *entry_start = entry_cur  = _find_bucket(d, key, d->buckets, d->bucket_capacity);

    while (entry_cur->key.ptr && !rdict_is_key_equal(d, entry_cur->key.ptr, key)) { //桶内元素长度受限
        //rlog(RLOG_DEBUG, "go next %"PRId64" -> %"PRId64"\n", entry_cur, entry_cur + 1);
        ++ entry_cur;
    }

    if (entry_cur >= (entry_start + d->bucket_capacity - 1)) {//扩容
        rlog(RLOG_INFO, "not enough space for adding, need expand.\n");
        int code_expand = rdict_expand(d, d->capacity);
        if (code_expand != rdict_code_ok) {
            return code_expand;
        }
        return rdict_add(d, key, val);
    }

    if (!entry_cur->key.ptr) {//key不存在，则添加
        d->size += 1;
        rdict_set_key(d, entry_cur, key);
    }
    else {
        rdict_free_value(d, entry_cur);
    }

    rdict_set_value(d, entry_cur, val); //支持 NULL 元素

    //rlog(RLOG_DEBUG, "add at: %"PRId64" -> %"PRId64" -> %"PRId64"\n", key, entry_start, entry_cur);

    return rdict_code_ok;
}

int rdict_remove(rdict* d, const void* key) {
    if (d == NULL) {
        return rdict_code_error;
    }
    if (!key) {
        if (d->entry_null) {
            rdict_free_key(d, d->entry_null);
            rdict_free_value(d, d->entry_null);
            rfree_data(rdict_entry, d->entry_null);

            d->entry_null = NULL;
            d->size -= 1;
            rassert(d->size >= 0, "size must > 0");
        }

        return rdict_code_ok;
    }

    rdict_entry* entry_cur = NULL;
    rdict_entry* entry_start = entry_cur = _find_bucket(d, key, d->buckets, d->bucket_capacity);
    rdict_entry* entry_end = entry_start + (d->bucket_capacity - 1);

    //if (key == 0x7f) {
    //    rlog(RLOG_DEBUG, "rdict_remove key = %lld\n", key);
    //}

    while (entry_cur->key.ptr && !rdict_is_key_equal(d, entry_cur->key.ptr, key)) {
        if (entry_cur == entry_end) { //桶内元素长度受限
            return rdict_code_not_exist;
        }
        ++entry_cur;
    }
    //rlog(RLOG_DEBUG, "remove at bucket: %"PRId64" -> %"PRId64", %"PRId64"\n", key, entry_start, entry_cur);

    if (!entry_cur->key.ptr) {
        //uint64_t hash = rdict_hash_key(d, key);
        //uint64_t bucket_pos = (hash % d->buckets) * d->bucket_capacity;
        return rdict_code_not_exist;
    }

    rdict_free_key(d, entry_cur);
    rdict_free_value(d, entry_cur);
    rdict_init_entry(entry_cur, NULL, NULL);
    d->size -= 1;

    //元素前移
    for (entry_start = entry_end; entry_start > entry_cur; entry_start--) {
        if (entry_start->key.ptr) {
            memcpy(entry_cur, entry_start, sizeof(rdict_entry));
            memset(entry_start, 0, sizeof(rdict_entry));
            break;
        }
    }

    return rdict_code_ok;
}

void rdict_clear(rdict* d) {
    if (!d) {
        return;
    }

    if (d->entry_null) {
        rdict_free_key(d, d->entry_null);
        rdict_free_value(d, d->entry_null);
        rfree_data(rdict_entry, d->entry_null);
        d->entry_null = NULL;
    }

    rdict_iterator it = rdict_it(d);
    for (rdict_entry *de = NULL; de = rdict_next(&it); ) {
        rdict_free_key(d, de);
        rdict_free_value(d, de);
    }

    d->size = 0;
    d->buckets = 0;
    if (d->entry) {
       memset(d->entry, 0, sizeof(rdict_entry) * d->capacity);
    }
}

void rdict_release(rdict* d) {
    rdict_clear(d);

    rfree_data(rdict, d);
}

rdict_entry * rdict_find(rdict* d, const void* key) {
    if (!key) {
        return d->entry_null;
    }

    rdict_entry* entry = _find_bucket(d, key, d->buckets, d->bucket_capacity);

    while (entry->key.ptr) {
        if (rdict_is_key_equal(d, entry->key.ptr, key)) {
            return entry;
        }
        ++entry;
    }

    return NULL;
}

rdict_iterator* rdict_it_heap(rdict* d) {
    rdict_iterator* it = rnew_data(rdict_iterator);

    if (!d || !d->entry) {
        return it;
    }

    it->d = d;
    it->entry = it->next = d->entry;

    return it;
}

rdict_entry* rdict_next(rdict_iterator* it) {
    if (unlikely(it->next == it->d->entry_null)) {
        it->next = it->entry;
        
        if (it->d->entry_null) {
            return it->d->entry_null;
        }
    }
    
    if (likely(it->next < it->entry + it->d->capacity)) {
        if (it->next->key.ptr) {
            return it->next++;
        }
        else {
            it->next = it->entry + ((it->next - it->entry) / it->d->bucket_capacity + 1) *  it->d->bucket_capacity;
            for (; it->next && (it->next < it->entry + it->d->capacity - 1);) {
                if (it->next->key.ptr) {
                    return it->next++;
                }
                it->next = it->entry + ((it->next - it->entry) / it->d->bucket_capacity + 1) *  it->d->bucket_capacity;
            }
        }
    }

    return NULL;
}

static rdict_entry* _find_bucket(rdict* d, void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity) {
    uint64_t hash = rdict_hash_key(d, key);
    uint64_t bucket_pos = (hash % bucket_count) * bucket_capacity;
    return rdict_get_entry(d, bucket_pos);
}

static rdict_entry* _find_bucket_raw(rdict_hash_func_type hash_func, rdict_entry* d_entry_start, void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity) {
    uint64_t hash = hash_func(key);
    uint64_t bucket_pos = (hash % bucket_count) * bucket_capacity;
    return d_entry_start == NULL ? NULL : d_entry_start + bucket_pos;
}



uint64_t rhash_func_murmur(const char *key)
{
    const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
    const size_t len = strlen(key);
    const unsigned char* p = (const unsigned char*)key;
    const unsigned char *end = p + (len & ~(uint64_t)0x7);
    uint64_t h = (len * m);

    while (p != end) {
        uint64_t k;
        memcpy(&k, p, sizeof(k));

        k *= m;
        k ^= k >> 47u;
        k *= m;

        h ^= k;
        h *= m;
        p += 8;
    }

    switch (len & 7u) {
    case 7: h ^= (uint64_t)p[6] << 48ul; // fall through
    case 6: h ^= (uint64_t)p[5] << 40ul; // fall through
    case 5: h ^= (uint64_t)p[4] << 32ul; // fall through
    case 4: h ^= (uint64_t)p[3] << 24ul; // fall through
    case 3: h ^= (uint64_t)p[2] << 16ul; // fall through
    case 2: h ^= (uint64_t)p[1] << 8ul;  // fall through
    case 1: h ^= (uint64_t)p[0];         // fall through
        h *= m;
    default:
        break;
    };

    h ^= h >> 47u;
    h *= m;
    h ^= h >> 47u;

    return h; // (uint32_t)h;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__