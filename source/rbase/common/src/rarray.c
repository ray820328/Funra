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

static int set_value_func_default(rarray_t* ar, const rarray_size_t offset, const void* obj) {
    void** dest_ptr = (void**)(ar)->items + offset;
    if (ar->copy_value_func) {
        *dest_ptr = ar->copy_value_func(obj);
    }
    else {
        *dest_ptr = obj;
    }
    return rcode_ok;
}

static void* get_value_func_default(rarray_t* ar, const rarray_size_t offset) {
    void** dest_ptr = (void**)(ar)->items + offset;
    return *dest_ptr;
}

static void free_value_func_default(void* obj) {

}

static int _rarray_expand(rarray_t* d, rarray_size_t capacity) {
    d->items = rnew_data_array(d->value_size, capacity);
    if (d->items == NULL) {
        rerror("expand failed.\n");
        return rarray_code_error;
    }
    d->capacity = capacity;

    return rcode_ok;
}

rarray_t* rarray_create(rarray_size_t value_size, rarray_size_t init_capacity) {
    rarray_t* d = rnew_data(rarray_t);

    d->items = NULL;

    d->value_size = value_size;
    d->size = 0;
    d->scale_factor = rarray_scale_factor;
    d->capacity = init_capacity > 0 ? init_capacity : rarray_init_capacity_default;
    d->keep_serial = true;

    d->set_value_func = set_value_func_default;
    d->get_value_func = get_value_func_default;
    d->copy_value_func = NULL;
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

    int code = d->set_value_func(d, d->size, val);
    d->size ++;

    return code;
}

int rarray_remove(rarray_t* d, const void* val) {

    return rcode_ok;
}

void rarray_clear(rarray_t* d) {
    rclear_data_array(d->items, d->value_size * d->capacity);
    d->size = 0;
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

void* rarray_at(rarray_t* d, rarray_size_t index) {
    //void* temp = NULL;
    if (!d || index >= d->capacity) {
        return NULL;
    }
    //temp = d->get_value_func(d->items, index);

    return d->get_value_func(d, index);
}

void* rarray_next(rarray_iterator_t* it) {

    return rarray_at(it->d, it->index ++);
}


rarray_define_set_value_func(int);

rarray_define_get_value_func(int);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__