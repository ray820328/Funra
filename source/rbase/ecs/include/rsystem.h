/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSYSTEM_H
#define RSYSTEM_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define recs_sys_type_t int

typedef int (*recs_system_init)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_system_uninit)(recs_context_t* ctx, const void* cfg_data);

typedef int (*recs_system_before_update)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_system_update)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_system_late_update)(recs_context_t* ctx, const void* cfg_data);

typedef int (*recs_system_update_fixed)(recs_context_t* ctx, const void* cfg_data);


/* ------------------------------- APIs ------------------------------------*/

R_API int recs_sys_new(recs_context_t* ctx, recs_sys_type_t sys_type, 
    recs_system_init init_sys, recs_system_uninit uninit_sys, 
    recs_system_before_update before_update_sys, recs_system_update update_sys, 
    recs_system_late_update late_update_sys, recs_system_update_fixed update_fixed_sys);

R_API int recs_sys_delete(recs_context_t* ctx, recs_sys_type_t sys_type, bool stop);

#ifdef __cplusplus
}
#endif

#endif //RSYSTEM_H
