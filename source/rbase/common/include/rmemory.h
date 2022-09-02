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

#define raymalloc(elem_size) malloc((elem_size))
#define raymalloc_type(elem_type) malloc(sizeof(elem_type))
#define raycmalloc(count, elem_size) calloc((count), (elem_size))
#define raycmalloc_type(count, elem_type) calloc((count), sizeof(elem_type))
#define rayfree(ptr) \
do { \
    free((ptr)); \
    (ptr) = NULL; \
} while (0)

#else //rmemory_enable_tracer

#define raymalloc(elem_size) rmem_malloc_trace((elem_size), get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__)
#define raymalloc_type(elem_type) rmem_malloc_trace(sizeof(elem_type), get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__)
#define raycmalloc(count, elem_size) rmem_cmalloc_trace((elem_size), (count), get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__)
#define raycmalloc_type(count, elem_type) (elem_type*)rmem_cmalloc_trace(sizeof(elem_type), (count), get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__)
#define rayfree(ptr) \
do { \
    rmem_free((ptr), get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__); \
    (ptr) = NULL; \
} while (0)

#endif //rmemory_enable_tracer

//#define rmemory_show_detail_realtime
#define rmemory_dict_size 200000
#define rmem_out_filepath_default "./rmem_leak.out"

// extern const 
extern rdict_t* rmem_trace_map;

int rmem_init();
int rmem_uninit();
int rmem_statistics(char* filepath);

void* rmem_malloc_trace(size_t size, long thread_id, char* filename, char* func, int line);
void* rmem_cmalloc_trace(size_t elem_size, size_t count, long thread_id, char* filename, char* func, int line);
int rmem_free(void* ptr, long thread_id, char* filename, char* func, int line);

#ifdef __cplusplus
}
#endif

#endif //RMEMORY_H
