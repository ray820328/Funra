/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RARRAY_H
#define RARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"

/* ------------------------------- Macros ------------------------------------*/
#define rarray_size_t unsigned long
#define rarray_size_max 0xFFFFFFFFUL
#define rarray_size_min 0L
#define rarray_size_t_format "lu"
#define rarray_init_capacity_default 16
#define rarray_scale_factor 2

#define rarray_size(ar) ((ar)->size)
//#define rarray_init_normal(ar, T, size) \
//    do { \
//        rassert((ar) == NULL, ""); \
//        (ar) = rarray_create((size)); \
//        (ar)->alloc_array_func = rarray_alloc_array_##T; \
//    } while(0)


#define rarray_declare_alloc_array_func(T) T* rarray_alloc_array_##T(size)

    //rarray_iterator* rarray_it(rarray* d);
#define rarray_it(ar) \
    { \
        (ar)->items, (ar)->items \
    }
#define rarray_it_first(it) \
    do { \
        (it)->entry = (it)->d->entry; \
        (it)->next = (it)->d->entry_null; \
    } while(0)

#define rarray_free(d) \
    if (d) { \
        rarray_release(d); \
        d = NULL; \
    }

/* ------------------------------- Structs ------------------------------------*/

    typedef enum rarray_code_t {
        rarray_code_error = 1,
        rarray_code_not_exist = 2,
        rarray_code_index_overflow = 3,
    } rarray_code_t;

    /** 0 - 不相等; !0 - 相等 **/
    typedef int(*rarray_compare_func_type)(const void* obj1, const void* obj2);

    typedef struct rarray_t {
        rdata_plain_type_t type;
        rarray_size_t size;
        rarray_size_t capacity;
        float scale_factor;
        bool keep_serial;

        void* (*copy_value_func)(const void* obj);
        rarray_compare_func_type compare_value_func;
        void(*free_value_func)(void* obj);

        void** items;
    } rarray_t;

    typedef struct rarray_iterator_t {
        void *items, *next;
    } rarray_iterator_t;

/* ------------------------------- APIs ------------------------------------*/

    //rarray_declare_alloc_array_func(int);
    //rarray_declare_alloc_array_func(float);
    //rarray_declare_alloc_array_func(double);

    rarray_t* rarray_create(rdata_plain_type_t type, rarray_size_t init_capacity);
    int rarray_add(rarray_t* d, void* val);
    int rarray_remove(rarray_t* d, const void* val);
    void rarray_clear(rarray_t* d);
    void rarray_release(rarray_t* d);
    bool rarray_exist(rarray_t* d, const void* data);
    int rarray_at(rarray_t* d, rarray_size_t index, void** data);

    void* rarray_next(rarray_iterator_t* it);

#ifdef __cplusplus
}
#endif

#endif //RTHREAD_H
