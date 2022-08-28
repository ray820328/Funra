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

#include "rbase.h"
#include "rdict.h"

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

#define raymalloc(x) rmem_malloc_trace((x), get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__)
#define raycmalloc(x, elem_type) (elem_type*) calloc((x), sizeof(elem_type))
#define rayfree(x) \
    do { \
        free((x)); \
        (x) = NULL; \
    } while (0)


#endif //rmemory_enable_tracer

// extern const 
extern rdict_t* rmem_trace_map;

#define rmemory_define_trace_map() \
extern rdict_t* rmem_trace_map;
rdict_t* rmem_trace_map = NULL \

#define rmemory_init_trace_map() \
if (rmem_trace_map == NULL) { \
    rdict_init(rmem_trace_map, rdata_type_uint64, rdata_type_string, 200000, 0); \
    rassert(rmem_trace_map != NULL, ""); \
}

int rmem_init();
int rmem_uninit();
int rmem_statistics(char* filepath);

void* rmem_malloc_trace(size_t size, long thread_id, char* filename, char* func, int line);

#ifdef __cplusplus
}
#endif

#endif //RMEMORY_H
