/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "stdlib.h"

#include "rstring.h"

#include "rcommon.h"
#include "rtime.h"
#include "rlog.h"
#include "rdict.h"
//#include "rpool.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

rattribute_unused(static volatile bool rdict_inited = false);

static rdict_entry_t* _find_bucket(rdict_t* d, const void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity);
static rdict_entry_t* _find_bucket_raw(rdict_hash_func_type hash_func, rdict_entry_t* d_entry_start, const void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity);
static int _expand_buckets(rdict_t* d, rdict_size_t capacity);

static void expand_failed_func_default(void* data_ext) {
    rassert(false, "default action");
}

static void set_key_func_default(void* data_ext, rdict_entry_t* entry, const void* key) {
    entry->key.ptr = (void*)key;
}

static void set_value_func_default(void* data_ext, rdict_entry_t* entry, const void* obj) {
    entry->value.ptr = (void*)obj;
}

rdict_t *rdict_create(rdict_size_t init_capacity, rdict_size_t bucket_capacity, void* data_ext,
    rdict_malloc_func_type malloc_func, rdict_calloc_func_type calloc_func, rdict_free_func_type free_func) {
    rdict_t* d = malloc_func == NULL ? rdata_new(rdict_t) : malloc_func(sizeof(rdict_t));

    d->entry = NULL;
    d->entry_null = NULL;

    d->size = 0;
    d->scale_factor = rdict_scale_factor_default;
    d->capacity = init_capacity > 0 ? init_capacity : rdict_init_capacity_default;
    d->buckets = 0;
    d->bucket_capacity = bucket_capacity <= 0 ? rdict_bucket_capacity_default : bucket_capacity;
    d->data_ext = data_ext;

    d->malloc_func = malloc_func == NULL ? NULL : malloc_func;
    d->calloc_func = calloc_func == NULL ? NULL : calloc_func;
    d->free_func = free_func == NULL ? NULL : free_func;
    d->hash_func = NULL;
    d->set_key_func = set_key_func_default;
    d->set_value_func = set_value_func_default;
    d->copy_key_func = NULL;
    d->copy_value_func = NULL;
    d->compare_key_func = NULL;
    d->free_key_func = NULL;
    d->free_value_func = NULL;
    d->expand_failed_func = expand_failed_func_default;

    _expand_buckets(d, d->capacity);

    return d;
}

static int _expand_buckets(rdict_t* d, rdict_size_t capacity) {
    if (d == NULL) {
        return rdict_code_error;
    }

    capacity = (rdict_size_t)(capacity / d->scale_factor);//按比率扩大

    if (capacity > rdict_size_max || capacity < rdict_size_min) {
		rerror("invalid size: "rdict_size_t_format, capacity);

        if (d->expand_failed_func) {
            d->expand_failed_func(d->data_ext);
        }
        return rdict_code_error;
    }

    //初始化所有entry对象空间
    rdict_size_t bucket_count = capacity % d->bucket_capacity == 0 ? capacity / d->bucket_capacity : capacity / d->bucket_capacity + 1;
    rdict_size_t dest_capacity = bucket_count * d->bucket_capacity;
    rdict_entry_t *new_entry = NULL;
    new_entry = d->calloc_func == NULL ? rdata_new_type_array(rdict_entry_t, dest_capacity) : d->calloc_func(dest_capacity, sizeof(rdict_entry_t));
    if (new_entry == NULL) {
		rerror("invalid malloc.");
        return rdict_code_error;
    }
    //for (size_t i = 0; i < dest_capacity; i++) {
    //    rdict_init_entry(&new_entry[i], 0, 0);
    //}

    rdict_entry_t *entry_old = rdict_get_entry(d, 0);
    if (entry_old == NULL) {
        d->capacity = dest_capacity;// *(d->entry));
        d->size = 0;
        d->buckets = bucket_count;

        rdict_set_buckets(d, new_entry);

        return rdict_code_ok;
    }//旧的空间必须到最后释放

	rdebug("expand map, size : capacity = %"rdict_size_t_format" : %"rdict_size_t_format" -> %"rdict_size_t_format,
        d->size, d->capacity, dest_capacity);
    if (d->size * 1.0 / d->capacity < rdict_used_factor_default) { //使用率太小，换hash算法
		rwarn("used ratio under the score of %1.3f", d->size * 1.0 / d->capacity);
    }

    rdict_entry_t* entry_next = NULL;
    rdict_entry_t* entry_start = entry_next = rdict_get_buckets(d);
    rdict_entry_t* new_bucket_cur = NULL;
    rdict_size_t inc = 0u;
    for (rdict_size_t i = 0; i < d->capacity; ) {
        entry_next = entry_start + i;

        if (entry_next->key.ptr == NULL) {
            i = (i / d->bucket_capacity + 1) * d->bucket_capacity;
            continue; //桶内元素保持连续
        }

        new_bucket_cur = _find_bucket_raw(d->hash_func, new_entry, entry_next->key.ptr, bucket_count, d->bucket_capacity);
        for (inc = 0u; new_bucket_cur && new_bucket_cur->key.ptr != NULL; ++new_bucket_cur, ++inc) {
            if (d->bucket_capacity > 0 && (inc >= d->bucket_capacity - 1)) {
                if likely(d->free_func == NULL) {
                    rayfree(new_entry);//释放当前申请的块
                }
                else {
                    d->free_func(new_entry);
                }
                new_entry = NULL;
                return rdict_expand(d, capacity * 2);//递归扩容
            }
        }
        memcpy(new_bucket_cur, entry_next, sizeof(rdict_entry_t));//浅拷贝

        i++;
    }

    //最后才能释放原始表
    if (d->entry) {
        if likely(d->free_func == NULL) {
            rayfree(d->entry);
        }
        else {
            d->free_func(d->entry);
        }
        d->entry = new_entry;
        d->capacity = dest_capacity;
        d->buckets = bucket_count;
    }

    return rdict_code_ok;
}

int rdict_expand(rdict_t* d, rdict_size_t capacity) {
    rdict_size_t real_capacity = capacity;
    if (real_capacity < rdict_hill_expand_capacity) {//小于峰值，按倍数扩容
        real_capacity *= rdict_expand_factor;
    }
    else {
        real_capacity += rdict_hill_add_capacity;
    }

    return _expand_buckets(d, real_capacity);
}

int rdict_add(rdict_t* d, void* key, void* val) {
    if (d == NULL) {
        return rdict_code_error;
    }
    if (!key) {
        if (!d->entry_null) {
            d->entry_null = d->malloc_func == NULL ? rdata_new(rdict_entry_t) : d->malloc_func(sizeof(rdict_entry_t));
            d->size += 1;
            rdict_set_key(d, d->entry_null, key);
        } else {
            rdict_free_value(d, d->entry_null);
        }
        rdict_set_value(d, d->entry_null, val);

        return rdict_code_ok;
    }

    rdict_entry_t *entry_cur = NULL;
    rdict_entry_t *entry_start = entry_cur  = _find_bucket(d, key, d->buckets, d->bucket_capacity);

    while (entry_cur->key.ptr && !rdict_is_key_equal(d, entry_cur->key.ptr, key)) { //桶内元素长度受限
        //rdebug("go next, entry_cur(%"PRId64") -> %"PRId64", key.ptr(%p) != key(%p)", entry_cur, entry_cur + 1, entry_cur->key.ptr, key);
        ++ entry_cur;
    }
    //rdebug("stop traval, entry_start(%p), entry_cur(%p) key.ptr(%p) != key(%p)", entry_start, entry_cur, entry_cur->key.ptr, key);

    if (entry_cur >= (entry_start + d->bucket_capacity - 1)) {//扩容
		rdebug("not enough space for adding, need expand.");
        int code_expand = rdict_expand(d, d->capacity);
        if (code_expand != rdict_code_ok) {
            return code_expand;
        }
        return rdict_add(d, key, val);
    }

    if (entry_cur->key.ptr == NULL) {//key不存在，则添加
        d->size += 1;
        rdict_set_key(d, entry_cur, key);
    }
    else {
        rdict_free_value(d, entry_cur);
    }

    rdict_set_value(d, entry_cur, val); //支持 NULL 元素
    //rdebug("add at, key(%p) -> entry_cur(%p)", key, entry_cur);

    //rdebug("add at: %"PRId64" -> %"PRId64" -> %"PRId64"", key, entry_start, entry_cur);

    return rdict_code_ok;
}

int rdict_remove(rdict_t* d, const void* key) {
    if (d == NULL) {
        return rdict_code_error;
    }
    if (!key) {
        if (d->entry_null != NULL) {
            rdict_free_key(d, d->entry_null);
            rdict_free_value(d, d->entry_null);

            if likely(d->free_func == NULL) {
                rdata_free(rdict_entry_t, d->entry_null);
            }
            else {
                d->free_func(d->entry_null);
            }

            d->entry_null = NULL;
            d->size -= 1;
            rassert(d->size >= 0, "size must > 0");
        }

        return rdict_code_ok;
    }

    rdict_entry_t* entry_cur = NULL;
    rdict_entry_t* entry_start = entry_cur = _find_bucket(d, key, d->buckets, d->bucket_capacity);
    rdict_entry_t* entry_end = entry_start + (d->bucket_capacity - 1);

    //if (key == 0x7f) {
    //    rdebug("rdict_remove key = %lld", key);
    //}

    while (entry_cur->key.ptr && !rdict_is_key_equal(d, entry_cur->key.ptr, key)) {
        if (entry_cur == entry_end) { //桶内元素长度受限
            return rdict_code_not_exist;
        }
        ++entry_cur;
    }
    //rdebug("remove at bucket: %"PRId64" -> %"PRId64", %"PRId64"", key, entry_start, entry_cur);

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
            memcpy(entry_cur, entry_start, sizeof(rdict_entry_t));
            memset(entry_start, 0, sizeof(rdict_entry_t));
            break;
        }
    }

    return rdict_code_ok;
}

void rdict_clear(rdict_t* d) {
    if (d == NULL) {
        return;
    }

    if (d->entry_null) {
        rdict_free_key(d, d->entry_null);
        rdict_free_value(d, d->entry_null);

        if likely(d->free_func == NULL) {
            rdata_free(rdict_entry_t, d->entry_null);
        }
        else {
            d->free_func(d->entry_null);
        }
        d->entry_null = NULL;
    }

    rdict_iterator_t it = rdict_it(d);
    for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        rdict_free_key(d, de);
        rdict_free_value(d, de);
    }

    d->size = 0;
    d->buckets = 0;
    if (d->entry) {
       memset(d->entry, 0, sizeof(rdict_entry_t) * d->capacity);
    }
}

void rdict_release(rdict_t* d) {
    rdict_clear(d);

    rdict_entry_t *entry = rdict_get_entry(d, 0);
    if likely(d->free_func == NULL) {
        if (entry) {
            rdata_free(rdict_entry_t, entry);
        }
        rdata_free(rdict_t, d);
    }
    else {
        if (entry) {
            d->free_func(entry);
        }
        d->free_func(d);
    }
}

rdict_entry_t * rdict_find(rdict_t* d, const void* key) {
    if (!key) {
        return d->entry_null;
    }

    rdict_entry_t* entry = _find_bucket(d, key, d->buckets, d->bucket_capacity);
    //rdebug("find bucket, entry(%p) key.ptr(%p) - key(%p)", entry, entry->key.ptr, key);
    while (entry->key.ptr != NULL) {
        if (rdict_is_key_equal(d, entry->key.ptr, key)) {
            //rdebug("find item, entry(%p) %p == %p", entry, entry->key.ptr, key);
            return entry;
        }
        ++entry;
    }

    return NULL;
}

rdict_iterator_t* rdict_it_heap(rdict_t* d) {
    rdict_iterator_t* it = d->malloc_func == NULL ? rdata_new(rdict_iterator_t) : d->malloc_func(sizeof(rdict_iterator_t));

    if (d == NULL || d->entry == NULL) {
        return it;
    }

    it->d = d;
    it->entry = it->next = d->entry;

    return it;
}

rdict_entry_t* rdict_next(rdict_iterator_t* it) {
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

static rdict_entry_t* _find_bucket(rdict_t* d, const void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity) {
    uint64_t hash = rdict_hash_key(d, key);
    uint64_t bucket_pos = (hash % bucket_count) * bucket_capacity;
    return rdict_get_entry(d, bucket_pos);
}

static rdict_entry_t* _find_bucket_raw(rdict_hash_func_type hash_func, rdict_entry_t* d_entry_start, const void* key, rdict_size_t bucket_count, rdict_size_t bucket_capacity) {
    uint64_t hash = hash_func(key);
    uint64_t bucket_pos = (hash % bucket_count) * bucket_capacity;
    return d_entry_start == NULL ? NULL : d_entry_start + bucket_pos;
}


rdict_block_define_type_key_func(rdata_type_int)
rdict_block_define_type_key_func(rdata_type_int64)
rdict_block_define_type_key_func(rdata_type_uint64)
rdict_block_define_type_key_func(rdata_type_string)
rdict_block_define_type_key_func(rdata_type_ptr)

rdict_block_define_type_value_func(rdata_type_int)
rdict_block_define_type_value_func(rdata_type_float)
rdict_block_define_type_value_func(rdata_type_double)
rdict_block_define_type_value_func(rdata_type_int64)
rdict_block_define_type_value_func(rdata_type_uint64)
rdict_block_define_type_value_func(rdata_type_string)
rdict_block_define_type_value_func(rdata_type_ptr)

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__