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

#define rmemory_enable_tracer 1

#ifndef rmemory_enable_tracer

#define raymalloc(x) malloc((x))
#define raycmalloc(x, elem_type) (elem_type*) calloc((x), sizeof(elem_type))
#define rayfree(x) \
    do { \
        free((x)); \
        (x) = NULL; \
    } while (0)

#else //rmemory_enable_tracer

#define raymalloc(x) malloc((x))
#define raycmalloc(x, elem_type) (elem_type*) calloc((x), sizeof(elem_type))
#define rayfree(x) \
    do { \
        free((x)); \
        (x) = NULL; \
    } while (0)


#endif //rmemory_enable_tracer


#include "rbase.h"
#include "rdict.h"

// extern const 
extern rdict_t* mem_trace_map;

#define rmemory_define_trace_map() \
extern rdict_t* mem_trace_map; \
rdict_t* mem_trace_map = NULL \

#define rmemory_init_trace_map() \
if (mem_trace_map == NULL) { \
    rdict_init(mem_trace_map, rdata_type_uint64, rdata_type_string, 200000, 0); \
    rassert(mem_trace_map != NULL, ""); \
}

#define rmemory_uninit_trace_map() \

#ifdef __cplusplus
}
#endif

#endif //RMEMORY_H
