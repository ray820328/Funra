/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

/**
 * recs开发宗旨：
 * componet只有数据
 * system不包含任何数据
 * 共享entity而不共享componet
 * 理论上在某个时间点，仅复制componet和entity，即可实现服务镜像
 */

#ifndef RECS_H
#define RECS_H

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

#define recs_cmp_fields \
    uint64_t id; \
    recs_cmp_type_t type_id

#define recs_sys_fields \
    uint64_t id; \
    recs_sys_type_t type_id

typedef enum {
    recs_etype_unknown = 0,
    recs_etype_shared = 1, //system共享数据集合

} recs_entity_type_t;

typedef enum {
    recs_ctype_unknown = 0,

} recs_cmp_type_t;

typedef enum {
    recs_sype_unknown = 0,
    recs_stype_initialize = 1,
    recs_stype_execute = 2,
    recs_stype_reactive = 3,
    recs_stype_teardown = 4,
    recs_stype_cleanup = 5,
} recs_sys_type_t;

typedef struct recs_entity_s {//ECS模块entity定义, 宗旨为仅含有component
    uint64_t id;
    recs_entity_type_t type_id;
    rstr_t* name;
    rarray_t* components;

    rdata_userdata_t* userdata;
} recs_entity_t;

typedef struct recs_component_s {//ECS模块组件定义, 宗旨为组件仅含有数据没有逻辑函数
    recs_cmp_fields;

} recs_cmp_t;

typedef struct recs_system_s {//ECS模块sys定义, 宗旨为仅不含有数据仅逻辑函数
    recs_sys_fields;

} recs_sys_t;

typedef struct recs_context_s recs_context_t;

typedef int (*recs_on_init_func)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_on_uninit_func)(recs_context_t* ctx, const void* cfg_data);

typedef int (*recs_on_new_entity_func)(recs_context_t* ctx, recs_entity_t* item);
typedef int (*recs_on_delete_entity_func)(recs_context_t* ctx, recs_entity_t* item);
typedef int (*recs_on_new_cmp_func)(recs_context_t* ctx, recs_cmp_t* item);
typedef int (*recs_on_delete_cmp_func)(recs_context_t* ctx, recs_cmp_t* item);

typedef recs_cmp_t* (*recs_create_cmp_func)(recs_context_t* ctx, recs_cmp_type_t data_type);

struct recs_context_s {
    uint64_t sid_min;
    uint64_t sid_max;
    uint64_t sid_cur;
    
    rdict_t* map_entities;
    rdict_t* map_components;
    rdict_t* map_systems;

    recs_on_init_func on_init;
    recs_on_uninit_func on_uninit;

    recs_on_new_entity_func on_new_entity;
    recs_on_delete_entity_func on_delete_entity;
    recs_on_new_cmp_func on_new_cmp;
    recs_on_delete_cmp_func on_delete_cmp;

    recs_create_cmp_func create_cmp;
};

#define recs_cmp_get_all(ctx, entity) \
    riterator_new(rdata_type_rarray, &(rarray_it((entity)->components)))

/* ------------------------------- APIs ------------------------------------*/

R_API int recs_init(recs_context_t* ctx, const void* cfg_data);
R_API int recs_uninit(recs_context_t* ctx, const void* cfg_data);

R_API uint64_t recs_get_next_id(recs_context_t* ctx);
R_API int recs_get_entity(recs_context_t* ctx, uint64_t entity_id, recs_cmp_t** ret_entity);
R_API int recs_get_cmp(recs_context_t* ctx, uint64_t cmp_id, recs_entity_t** ret_cmp);

R_API recs_entity_t* recs_entity_new(recs_context_t* ctx, recs_entity_type_t data_type);
R_API int recs_entity_delete(recs_context_t* ctx, recs_entity_t* data, bool destroy);

R_API int recs_cmp_add(recs_context_t* ctx, recs_entity_t* entity, recs_cmp_t* cmp);
R_API int recs_cmp_get(recs_context_t* ctx, recs_entity_t* entity, uint64_t cmp_id, recs_cmp_t** ret_entity);
// R_API int recs_cmp_get_all(recs_context_t* ctx, recs_entity_t* entity, riterator_t* cmp_id);
//很少会删
R_API int recs_cmp_remove(recs_context_t* ctx, recs_entity_t* entity, uint64_t cmp_id, recs_cmp_t** ret_cmp);
R_API int recs_cmp_remove_all(recs_context_t* ctx, recs_entity_t* entity);
R_API int recs_cmp_active(recs_context_t* ctx, recs_entity_t* entity, uint64_t cmp_id, bool active);
R_API int recs_cmp_active_all(recs_context_t* ctx, recs_entity_t* entity, bool active);

R_API recs_cmp_t* recs_cmp_new(recs_context_t* ctx, recs_cmp_type_t data_type);
R_API int recs_cmp_delete(recs_context_t* ctx, recs_cmp_t* data, bool destroy);

#ifdef __cplusplus
}
#endif

#endif //RECS_H
