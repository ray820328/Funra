/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"
#include "rlog.h"
#include "rarray.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

static int compare_func_default(const void* obj1, const void* obj2) {
    if (obj1 == obj2) {
        return rcode_eq;
    }
    return rcode_neq;
}

static int _rarray_alloc(rarray_t* ar, rarray_size_t capacity) {
    void** dest_ptr = rnew_data_array(ar->value_size, capacity);
    if (dest_ptr == NULL) {
        rerror("expand failed."Li);
        return rarray_code_error;
    }

    if (ar->items != NULL && ar->items != dest_ptr) {//数据移动
        rdebug("rarray realloc."Li);

        int copy_len = (ar)->size;
        if (copy_len > 0) {
            memcpy(dest_ptr, ar->items, copy_len * (ar)->value_size);
        }

        rfree_data_array(ar->items);
    }

    ar->items = dest_ptr;
    ar->capacity = capacity;

    return rcode_ok;
}

rarray_t* rarray_create(rarray_size_t value_size, rarray_size_t init_capacity) {
    rarray_t* ar = rnew_data(rarray_t);

    ar->items = NULL;

    ar->value_size = value_size;
    ar->size = 0;
    ar->scale_factor = rarray_scale_factor;
    ar->capacity = init_capacity > 0 ? init_capacity : rarray_init_capacity_default;
    ar->keep_serial = true;

    ar->set_value_func = NULL;
    ar->get_value_func = NULL;
    ar->copy_value_func = NULL;
    ar->remove_value_func = NULL;
    ar->compare_value_func = (rcom_compare_func_type)compare_func_default;
    ar->free_value_func = NULL;

    rassert(_rarray_alloc(ar, ar->capacity) == rcode_ok, "oom");

    return ar;
}

int rarray_add(rarray_t* ar, void* val) {
    if (ar->size >= ar->capacity) {
        rassert(_rarray_alloc(ar, (rarray_size_t)(ar->capacity * ar->scale_factor)) == rcode_ok, "oom");
    }

    int code = ar->set_value_func(ar, ar->size, val);
    ar->size ++;

    return code;
}

int rarray_remove(rarray_t* ar, const void* val) {
    rarray_iterator_t it = rarray_it(ar);
    void* temp = NULL;
    for (rarray_size_t index = 0; temp = rarray_next(&it), rarray_has_next(&it); index ++) {
        if (ar->compare_value_func(temp, val) == rcode_eq) {
            ar->remove_value_func(ar, index);
            return rcode_ok;
        }
    }
    return rarray_code_not_exist;
}

int rarray_remove_at(rarray_t* ar, rarray_size_t index) {
    if ((index) < 0 || (index) >= (ar)->size) {
        return rarray_code_index_out4_size;
    }

    return ar->remove_value_func(ar, index);
}

void** rarray_get_all(rarray_t* ar) {
    if (ar == NULL || ar->items == NULL) {
        return NULL;
    }

    void** dest_ptr = rnew_data_array(ar->value_size, rarray_size(ar));
    if (dest_ptr == NULL) {
        rerror("rarray_get_all failed."Li);
        return NULL;
    }

    int copy_len = rarray_size(ar);
    if (copy_len > 0) {
        memcpy(dest_ptr, ar->items, copy_len * (ar)->value_size);
    }
    return dest_ptr;
}

void rarray_clear(rarray_t* ar) {
    if (ar->free_value_func) {
        rarray_iterator_t it = rarray_it(ar);
        void* temp = NULL;
        for (; temp = rarray_next(&it), rarray_has_next(&it); ) {
            ar->free_value_func(temp);
        }
    }
    rclear_data_array(ar->items, ar->value_size * ar->capacity);
    ar->size = 0;
}

void rarray_release(rarray_t* ar) {
    if (ar == NULL) {
        return;
    }

    rarray_clear(ar);

    if (ar->items != NULL) {
        rfree_data_array(ar->items);
    }
    rfree_data(rarray_t, ar);
}

bool rarray_exist(rarray_t* ar, const void* data) {
    rarray_iterator_t it = rarray_it(ar);
    void* temp = NULL;
    for (; temp = rarray_next(&it), rarray_has_next(&it);) {
        if (ar->compare_value_func(temp, data) == rcode_eq) {
            return true;
        }
    }
    return false;
}

void* rarray_at(rarray_t* ar, rarray_size_t index) {
    //void* temp = NULL;
    if (!ar || index >= ar->capacity) {
        return NULL;
    }
    //temp = ar->get_value_func(ar->items, index);

    return ar->get_value_func(ar, index);
}

void* rarray_next(rarray_iterator_t* it) {

    return rarray_at(it->d, it->index ++);
}



int rarray_set_value_func_rdata_type_ptr(rarray_t* ar, const rarray_size_t offset, rdata_type_ptr_inner_type obj) {
    void** dest_ptr = (void**)(ar)->items + offset;
    if (*dest_ptr) {//位置已经存在对象，直接释放
        ar->free_value_func(*dest_ptr);
    }

    if (ar->copy_value_func) {
        *dest_ptr = ar->copy_value_func(obj);
    }
    else {
        *dest_ptr = obj;
    }
    return rcode_ok;
}
void** rarray_get_value_func_rdata_type_ptr(rarray_t* ar, const rarray_size_t offset) {
    void** dest_ptr = (void**)(ar)->items + offset;
    return *dest_ptr;
}
int rarray_remove_value_func_rdata_type_ptr(rarray_t* ar, const rarray_size_t index) {
    void** dest_ptr = (void**)(ar)->items + (index);
    if (*dest_ptr) {//位置已经存在对象，直接释放
        (ar)->free_value_func(*dest_ptr);
    }

    int copy_len = (ar)->size - (index) - 1;
    if (copy_len > 0) {
        memcpy(dest_ptr, dest_ptr + 1, copy_len * (ar)->value_size);
    }

    dest_ptr = (void**)(ar)->items + (ar)->size - 1;
    *dest_ptr = NULL;

    (ar)->size--;

    return rcode_ok;
}
void rarray_free_value_func_rdata_type_ptr(void* obj) {

}

int rarray_set_value_func_rdata_type_string(rarray_t* ar, const rarray_size_t offset, char* obj) {
    char** dest_ptr = (char**)(ar)->items + offset;
    *dest_ptr = rstr_cpy(obj, 0);
    return rcode_ok;
}
char* rarray_get_value_func_rdata_type_string(rarray_t* ar, const rarray_size_t offset) {
    char** dest_ptr = (char**)(ar)->items + offset;
    return *dest_ptr;
}
int rarray_remove_value_func_rdata_type_string(rarray_t* ar, const rarray_size_t index) {
    char** dest_ptr = (char**)(ar)->items + (index);
    if (*dest_ptr) {//位置已经存在对象，直接释放
        (ar)->free_value_func(*dest_ptr);
    }

    int copy_len = (ar)->size - (index) - 1;
    if (copy_len > 0) {
        memcpy(dest_ptr, dest_ptr + 1, copy_len * (ar)->value_size);
    }

    dest_ptr = (char**)(ar)->items + (ar)->size - 1;
    *dest_ptr = NULL;

    (ar)->size--;

    return rcode_ok;
}
void rarray_free_value_func_rdata_type_string(void* obj) {
    rstr_free(obj);
}

rarray_define_set_get_value_func(rdata_type_int);
rarray_define_set_get_value_func(rdata_type_float);
rarray_define_set_get_value_func(rdata_type_double);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__