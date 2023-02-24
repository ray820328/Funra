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

typedef enum {
    rscript_type_unknown = 0,
    rscript_type_lua = 10,

    rscript_type_ext_start = 100, //自定义扩展
} rscript_type_base_t;


/* ------------------------------- APIs ------------------------------------*/

typedef int (*rscript_init_func)(rscript_context_t* ctx, const void* cfg_data);
typedef int (*rscript_uninit_func)(rscript_context_t* ctx, const void* cfg_data);
typedef int (*rscript_call_func)(rscript_context_t* ctx, char* func_name, int params_amount, int ret_amouont);
typedef rstr_t* (*rscript_dump_stack_func)(rscript_context_t* ctx);

typedef struct rscript_s {
    rscript_type_t type;

    rscript_init_func init;
    rscript_uninit_func uninit;
    rscript_call_func call_script;
    rscript_dump_stack_func dump;

    char* name;
} rscript_t;

#ifdef __cplusplus
}
#endif

#endif //RSCRIPT_H
