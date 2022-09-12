/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RECS_H
#define RECS_H

#include "rcommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    recs_cmp_type_unknown = 0,
} recs_cmp_type_t;

typedef int (*recs_init_func)(void* ctx, const void* cfg_data);

typedef struct recs_system_s {
    recs_init_func init;
} recs_system_t;

#ifdef __cplusplus
}
#endif

#endif //RECS_H
