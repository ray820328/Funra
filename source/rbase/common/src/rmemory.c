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

int rmem_init() {

    return 1;
}

int rmem_uninit() {

    return 1;
}

void* rmem_malloc_trace(size_t size, long thread_id, char* filename, char* func, int line) {
    void* ret = malloc(size);

    return ret;
}

int rmem_statistics(char* filepath) {
    return 1;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__