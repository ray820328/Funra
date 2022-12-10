/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSCRIPT_H
#define RSCRIPT_H

#include "rcommon.h"
#include "rarray.h"
#include "rdict.h"
#include "rstring.h"

#include "rscript_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/


/* ------------------------------- Structs ------------------------------------*/


/* ------------------------------- APIs ------------------------------------*/

typedef int (*rscript_init)(rscript_context_t* ctx, const void* cfg_data);
typedef int (*rscript_uninit)(rscript_context_t* ctx, const void* cfg_data);

typedef struct rscript_s {
    rscript_type_t type;

    rscript_init init;
    rscript_uninit uninit;

    char name[0];
} rscript_t;

extern const rscript_t* rscript_lua;

#ifdef __cplusplus
}
#endif

#endif //RSCRIPT_H
