/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSCRIPT_CONTEXT_H
#define RSCRIPT_CONTEXT_H

#include "rcommon.h"
#include "rarray.h"
#include "rdict.h"
#include "rstring.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/

#define rscript_type_t int
    

/* ------------------------------- Structs ------------------------------------*/

struct recs_context_s;

typedef struct rscript_context_s rscript_context_t;

struct rscript_context_s {
    rscript_type_t type;

    void* ctx_script;

    char name[0];
};

/* ------------------------------- APIs ------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif //RSCRIPT_CONTEXT_H
