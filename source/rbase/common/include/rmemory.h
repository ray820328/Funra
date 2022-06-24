/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RMEMORY_H
#define RMEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

// #define rmemory_enable_tracer 1

#ifndef rmemory_enable_tracer

#define raymalloc(x) malloc((x))
#define raycmalloc(x, elem_type) (elem_type*) calloc((x), sizeof(elem_type))
#define rayfree(x) \
    do { \
        free((x)); \
        (x) = NULL; \
    } while (0)

#else //rmemory_enable_tracer

#include "rdict.h"

extern rdict_t* mem_trace_map;

#define raymalloc(x) malloc((x))
#define raycmalloc(x, elem_type) (elem_type*) calloc((x), sizeof(elem_type))
#define rayfree(x) \
    do { \
        free((x)); \
        (x) = NULL; \
    } while (0)

#define rmem_trace_define_global() \
    rdict_t* dict_ins = rdict_create(count, 0, NULL); \
    assert_true(dict_ins); \
    dict_ins->hash_func = rhash_func_int;


#endif //rmemory_enable_tracer

#ifdef __cplusplus
}
#endif

#endif //RMEMORY_H
