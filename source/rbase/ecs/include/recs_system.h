/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RECS_SYSTEM_H
#define RECS_SYSTEM_H

#include "rcommon.h"

#include "recs_context.h"
#include "recs_entity.h"
#include "recs_component.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/

#define recs_sys_type_t int

/* ------------------------------- Structs ------------------------------------*/

typedef enum {
    recs_sype_unknown = 0,
    recs_stype_initialize = 1,
    recs_stype_execute = 2,
    recs_stype_execute_base_start = 20001,
    recs_stype_execute_base_end = 20100,//框架系统
    recs_stype_reactive = 3,
    recs_stype_teardown = 4,
    recs_stype_cleanup = 5,
} recs_stype_base_t;

typedef int (*recs_sys_init)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_sys_uninit)(recs_context_t* ctx, const void* cfg_data);

typedef int (*recs_sys_before_update)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_sys_update)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_sys_late_update)(recs_context_t* ctx, const void* cfg_data);

typedef int (*recs_sys_update_fixed)(recs_context_t* ctx, const void* cfg_data);


typedef struct recs_system_s {//ECS模块system定义, 宗旨为组件仅含有逻辑函数没有数据
    recs_sys_type_t type;

    recs_sys_init init;
    recs_sys_uninit uninit; 
    recs_sys_before_update before_update;
    recs_sys_update update;
    recs_sys_late_update late_update;
    recs_sys_update_fixed update_fixed;

    char name[0];
} recs_system_t;

/* ------------------------------- APIs ------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif //RECS_SYSTEM_H
