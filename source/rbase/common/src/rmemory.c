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

int rmem_init() {
#ifdef rmemory_enable_tracer
    if (rmem_trace_map == NULL) {
        rdict_init(rmem_trace_map, rdata_type_uint64, rdata_type_ptr, rmemory_dict_size, 0);
        rassert(rmem_trace_map != NULL, "");
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
    printf("malloc. %d-%p from %ld, %s:%s-%d"Li, elem_size, ret, thread_id, filename, func, line);
#endif // rmemory_show_detail_realtime



    return ret;
}

void* rmem_cmalloc_trace(size_t elem_size, size_t count, long thread_id, char* filename, char* func, int line) {
    void* ret = calloc(count, elem_size);
#ifdef rmemory_show_detail_realtime
    printf("calloc. %d-%p from %ld, %s:%s-%d"Li, elem_size * count, ret, thread_id, filename, func, line);
#endif // rmemory_show_detail_realtime

    return ret;
}

int rmem_free(void* ptr, long thread_id, char* filename, char* func, int line) {
#ifdef rmemory_show_detail_realtime
    printf("free. %d-%p from %ld, %s:%s-%d"Li, 0, ptr, thread_id, filename, func, line);
#endif // rmemory_show_detail_realtime
    free(ptr);

    return rcode_ok;
}

int rmem_statistics(char* filepath) {
#ifdef rmemory_enable_tracer
    FILE* file_ptr = fopen(filepath, "w");
    if (file_ptr == NULL) {
        return 1;
    }

    fprintf(file_ptr, "good, no memory leak.");
    fflush(file_ptr);
    fclose(file_ptr);
#endif // rmemory_enable_tracer

    return rcode_ok;
}

#undef rmem_info_t

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__