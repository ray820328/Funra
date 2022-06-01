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

#define rarray_declare_alloc_array_func(T) T* rarray_alloc_array_##T(size)

#define rarray_declare_set_get_value_func(T) \
    int rarray_set_value_func_##T(rarray_t* ar, const rarray_size_t offset, T##_inner_type obj); \
    T##_inner_type rarray_get_value_func_##T(rarray_t* ar, const rarray_size_t offset); \
    int rarray_remove_value_func_##T(rarray_t* ar, const rarray_size_t index); \
    void rarray_free_value_func_##T(void* obj)

#define rarray_define_set_get_value_func(T) \
    int rarray_set_value_func_##T(rarray_t* ar, const rarray_size_t offset, T##_inner_type obj) { \
        T##_inner_type* dest_ptr = (T##_inner_type*)((ar)->items) + (offset); \
        *dest_ptr = (obj); \
        return rcode_ok; \
    } \
    T##_inner_type rarray_get_value_func_##T(rarray_t* ar, const rarray_size_t offset) { \
        T##_inner_type* dest_ptr = (T##_inner_type*)((ar)->items) + (offset); \
        return *dest_ptr; \
    } \
    int rarray_remove_value_func_##T(rarray_t* ar, const rarray_size_t index) { \
        T##_inner_type* dest_ptr = (T##_inner_type*)((ar)->items) + (index); \
        size_t copy_len = (ar)->size - (index) - 1; \
        if (copy_len > 0) { \
            memcpy(dest_ptr, dest_ptr + 1, copy_len * (ar)->value_size); \
        } \
        dest_ptr = (T##_inner_type*)((ar)->items) + (ar)->size - 1; \
        *dest_ptr = 0; \
        (ar)->size --; \
        return rcode_ok; \
    } \
    void rarray_free_value_func_##T(void* obj) {}

#define rarray_init(ar, T, size) \
    do { \
        rassert((ar) == NULL, ""); \
        (ar) = rarray_create(T##_size, (size)); \
        rassert((ar) != NULL, ""); \
        (ar)->set_value_func = rarray_set_value_func_##T; \
        (ar)->get_value_func = rarray_get_value_func_##T; \
        (ar)->remove_value_func = rarray_remove_value_func_##T; \
        (ar)->free_value_func = rarray_free_value_func_##T; \
    } while(0)

    //rarray_iterator* rarray_it(rarray* d);
#define rarray_it(ar) \
    { \
        (ar), 0 \
    }
#define rarray_it_first(it) (it)->index = 0
#define rarray_has_next(it) ((it)->index <= (it)->d->size)

#define rarray_free(d) \
    if (d) { \
        rarray_release(d); \
        d = NULL; \
    }

/* ------------------------------- Structs ------------------------------------*/

    typedef enum rarray_code_t {
        rarray_code_error = 1,
        rarray_code_equal,
        rarray_code_not_equal,
        rarray_code_not_exist,
        rarray_code_index_out4_size,
        rarray_code_index_out4_capacity,
    } rarray_code_t;

    /** 0 - 不相等; !0 - 相等 **/
    typedef int(*rarray_compare_func_type)(const void* obj1, const void* obj2);

    typedef struct rarray_t {
        rarray_size_t value_size;
        rarray_size_t size;
        rarray_size_t capacity;
        float scale_factor;
        bool keep_serial;

        int (*set_value_func)(struct rarray_t* ar, const rarray_size_t offset, const void* obj);
        void* (*get_value_func)(struct rarray_t* ar, const rarray_size_t offset);
        void* (*copy_value_func)(const void* obj);
        int (*remove_value_func)(struct rarray_t* ar, const rarray_size_t index);
        rarray_compare_func_type compare_value_func;
        void(*free_value_func)(void* obj);

        void** items;
    } rarray_t;

    typedef struct rarray_iterator_t {
        rarray_t* d;
        rarray_size_t index;
    } rarray_iterator_t;

/* ------------------------------- APIs ------------------------------------*/

    rarray_declare_set_get_value_func(rdata_type_int);
    rarray_declare_set_get_value_func(rdata_type_float);
    rarray_declare_set_get_value_func(rdata_type_double);
    rarray_declare_set_get_value_func(rdata_type_string);
    rarray_declare_set_get_value_func(rdata_type_ptr);

    rarray_t* rarray_create(rarray_size_t value_size, rarray_size_t init_capacity);
    int rarray_add(rarray_t* d, void* val);
    int rarray_remove(rarray_t* d, const void* val);
    int rarray_remove_at(rarray_t* d, rarray_size_t index);
    void rarray_clear(rarray_t* d);
    void rarray_release(rarray_t* d);
    bool rarray_exist(rarray_t* d, const void* data);
    void* rarray_at(rarray_t* d, rarray_size_t index);
    void** rarray_get_all(rarray_t* d);

    void* rarray_next(rarray_iterator_t* it);

#ifdef __cplusplus
}
#endif

#endif //RARRAY_H
