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

#include "recs_component.h"
#include "recs_entity.h"
#include "recs_system.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/

#define recs_cmp_get_all(ctx, entity) \
    riterator_new(rdata_type_rarray, &(rarray_it((entity)->components)))

/* ------------------------------- Structs ------------------------------------*/


/* ------------------------------- APIs ------------------------------------*/

R_API int recs_init(recs_context_t* ctx, const void* cfg_data);
R_API int recs_uninit(recs_context_t* ctx, const void* cfg_data);

R_API int recs_start(recs_context_t* ctx, const void* cfg_data);
R_API int recs_pause(recs_context_t* ctx, const void* cfg_data);
R_API int recs_resume(recs_context_t* ctx, const void* cfg_data);
R_API int recs_stop(recs_context_t* ctx, const void* cfg_data);

R_API int recs_run(recs_context_t* ctx, const void* cfg_data);

R_API uint64_t recs_get_next_id(recs_context_t* ctx);
R_API int recs_get_entity(recs_context_t* ctx, uint64_t entity_id, recs_entity_t** ret_entity);
R_API int recs_get_cmp(recs_context_t* ctx, uint64_t cmp_id, recs_cmp_t** ret_cmp);


R_API recs_cmp_t* recs_cmp_new(recs_context_t* ctx, recs_cmp_type_t data_type);
R_API int recs_cmp_delete(recs_context_t* ctx, recs_cmp_t* data, bool destroy);


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


R_API int recs_sys_add(recs_context_t* ctx, recs_system_t* sys_item);
R_API int recs_sys_remove(recs_context_t* ctx, recs_sys_type_t sys_type, bool stop);


#ifdef __cplusplus
}
#endif

#endif //RECS_H
