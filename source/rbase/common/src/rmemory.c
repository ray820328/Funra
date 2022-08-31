/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "stdlib.h"

#include "rcommon.h"
#include "rstring.h"
#include "rfile.h"
#include "rmemory.h"
#include "rlog.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif //__GNUC__

typedef struct rmem_info_s {
    void* ptr;
    size_t elem_size;
    size_t count;
    long thread_id;
    char* filename;
    char* func;
    int line;
} rmem_info_t;

extern rdict_t* rmem_trace_map;
rdict_t* rmem_trace_map = NULL;

static void mem_dict_free_value_func(void* data_ext, rmem_info_t* obj) {
    free(obj); 
} 

int rmem_init() {
#ifdef rmemory_enable_tracer
    if (rmem_trace_map == NULL) {
        rdict_init_full(rmem_trace_map, rdata_type_uint64, rdata_type_ptr, rmemory_dict_size, 0, 
            malloc, calloc, free, NULL, NULL, NULL, NULL, NULL, mem_dict_free_value_func);
        rassert(rmem_trace_map != NULL, "");
        rmem_trace_map->free_value_func = (rdict_free_value_func_type)mem_dict_free_value_func;
    }
#endif // rmemory_enable_tracer

    return rcode_ok;
}

int rmem_uninit() {
#ifdef rmemory_enable_tracer
    if (rmem_trace_map == NULL) {
        return 1;
    }
#endif // rmemory_enable_tracer

    rmem_statistics(rmem_out_filepath_default);

    rdict_free(rmem_trace_map);
    rmem_trace_map = NULL;

    return rcode_ok;
}

void* rmem_malloc_trace(size_t elem_size, long thread_id, char* filename, char* func, int line) {
    void* ret = malloc(elem_size);
#ifdef rmemory_show_detail_realtime
    rdebug("malloc. %d-%p from %ld, %s:%s-%d"Li, elem_size, ret, thread_id, filename, func, line);
#endif // rmemory_show_detail_realtime

    rmem_info_t* info = malloc(sizeof(rmem_info_t));
    rassert(info, "");
    info->ptr = ret;
    info->elem_size = elem_size;
    info->count = 0;
    info->thread_id = thread_id;
    info->filename = filename;
    info->func = func;
    info->line = line;

    rdict_add(rmem_trace_map, (int64_t)ret, info);

    return ret;
}

void* rmem_cmalloc_trace(size_t elem_size, size_t count, long thread_id, char* filename, char* func, int line) {
    void* ret = calloc(count, elem_size);
#ifdef rmemory_show_detail_realtime
    rdebug("calloc. %d-%p from %ld, %s:%s-%d"Li, elem_size * count, ret, thread_id, filename, func, line);
#endif // rmemory_show_detail_realtime

    rmem_info_t* info = malloc(sizeof(rmem_info_t));
    rassert(info, "");
    info->ptr = ret;
    info->elem_size = elem_size;
    info->count = count;
    info->thread_id = thread_id;
    info->filename = filename;
    info->func = func;
    info->line = line;

    rdict_add(rmem_trace_map, (int64_t)ret, info);

    return ret;
}

int rmem_free(void* ptr, long thread_id, char* filename, char* func, int line) {
#ifdef rmemory_show_detail_realtime
    rdebug("free. %d-%p from %ld, %s:%s-%d"Li, 0, ptr, thread_id, filename, func, line);
#endif // rmemory_show_detail_realtime

    rdict_entry_t *de = rdict_find(rmem_trace_map, (int64_t)ptr);
    if (de == NULL || ((rmem_info_t*)(de->value.ptr))->ptr != ptr) {
        rerror("free error, not exists. %d-%p != (%p->%p) from %ld, %s:%s-%d"Li, 0, ptr, 
            de, de == NULL ? NULL : ((rmem_info_t*)(de->value.ptr))->ptr, thread_id, filename, func, line);
        return 1;
    }
    rdict_remove(rmem_trace_map, (int64_t)ptr);

    free(ptr);

    return rcode_ok;
}

int rmem_statistics(char* filepath) {
#ifdef rmemory_enable_tracer
    FILE* file_ptr = fopen(filepath, "w");
    if (file_ptr == NULL) {
        rinfo("error open file: %s"Li, filepath);
        return 1;
    }

    rinfo("rmemory leak count: %"rdict_size_t_format""Li, rdict_size(rmem_trace_map));
    if (rdict_size(rmem_trace_map) == 0) {
        fprintf(file_ptr, "nice, no memory leak.");
    }
    else {
        rmem_info_t* info = NULL;
        rdict_iterator_t it = rdict_it(rmem_trace_map);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            info = (rmem_info_t*)(de->value.ptr);
            fprintf(file_ptr, "(%d * %d)-%p from %ld, %s:%s-%d"Li, 
                info->count, info->elem_size, info->ptr, info->thread_id, info->filename, info->func, info->line);
        }
    }

    fflush(file_ptr);
    fclose(file_ptr);
#endif // rmemory_enable_tracer

    return rcode_ok;
}

#undef rmem_info_t

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__