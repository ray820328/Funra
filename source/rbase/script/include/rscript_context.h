﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RECS_CONTEXT_H
#define RECS_CONTEXT_H

#include "rcommon.h"
#include "rarray.h"
#include "rdict.h"
#include "rstring.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/


/* ------------------------------- Structs ------------------------------------*/

struct recs_context_s;

typedef enum {
    rscript_type_unknown = 0,
    rscript_type_lua = 1,
} rscript_type_t;

typedef struct rscript_context_s rscript_context_t;

struct rscript_context_s {
    rscript_type_t type;


    char name[0];
};

/* ------------------------------- APIs ------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif //RECS_CONTEXT_H
