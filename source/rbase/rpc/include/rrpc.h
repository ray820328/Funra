/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RRPC_H
#define RRPC_H

#include "rcommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    rrpc_type_unknown = 0,
} rrpc_type_t;

typedef int (*rrpc_init_func)(void* ctx, const void* cfg_data);

typedef struct rrpc_entry_s {
    rrpc_init_func init;
} rrpc_entry_t;

#ifdef __cplusplus
}
#endif

#endif //RRPC_H
