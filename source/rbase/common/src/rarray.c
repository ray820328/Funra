/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rfile.h"
#include "rlog.h"
#include "rarray.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

static int compare_func_default(const void* obj1, const void* obj2) {
    if (obj1 == obj2)
        return 1;
    return rcode_ok;
}

static void* copy_value_func_default(const void* obj) {
    return obj;
}

static void free_value_func_default(void* obj) {

}

static int _rarray_expand(rarray_t* d, rarray_size_t capacity) {
    d->items = rnew_data_array(d->type, capacity);
    if (d->items == NULL) {
        rerror("expand failed.\n");
        return rarray_code_error;
    }
    d->capacity = capacity;

    return rcode_ok;
}

rarray_t* rarray_create(rdata_plain_type_t type, rarray_size_t init_capacity) {
    rarray_t* d = rnew_data(rarray_t);

    d->items = NULL;

    d->type = type;
    d->size = 0;
    d->scale_factor = rarray_scale_factor;
    d->capacity = init_capacity > 0 ? init_capacity : rarray_init_capacity_default;
    d->keep_serial = true;

    d->copy_value_func = copy_value_func_default;
    d->compare_value_func = compare_func_default;
    d->free_value_func = free_value_func_default;

    rassert(_rarray_expand(d, d->capacity) == rcode_ok, "oom");

    return d;
}

int rarray_add(rarray_t* d, void* val) {
    if (d->size >= d->capacity) {
        rassert(_rarray_expand(d, d->capacity) == rcode_ok, "oom");
        return rarray_code_error;
    }

    int *temp = NULL;

    temp = (int*)d->items + d->size;
    *temp = copy_value_func_default(val);
    d->size ++;

    return rcode_ok;
}

int rarray_remove(rarray_t* d, const void* val) {

    return rcode_ok;
}

void rarray_clear(rarray_t* d) {

}

void rarray_release(rarray_t* d) {
    if (d == NULL) {
        return;
    }

    if (d->items != NULL) {
        rfree_data_array(d->items);
    }
    rfree_data(rarray_t, d);
}

bool rarray_exist(rarray_t* d, const void* data) {

    return true;
}

int rarray_at(rarray_t* d, rarray_size_t index, void** data) {
    if (!d || index >= d->capacity) {
        return rarray_code_index_overflow;
    }

    int *temp = NULL;
    temp = (int*)d->items + index;
    *data = *temp;

    return rcode_ok;
}

void* rarray_next(rarray_iterator_t* it) {

    return NULL;
}


//rarray_declare_alloc_array_func(int) {
//    return rnew_data_array(int, size);
//}
//
//rarray_declare_alloc_array_func(float) {
//    return rnew_data_array(float, size);
//}
//
//rarray_declare_alloc_array_func(double) {
//    return rnew_data_array(double, size);
//}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__