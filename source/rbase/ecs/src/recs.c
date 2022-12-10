/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rcommon.h"
#include "rlog.h"

#include "recs.h"

#ifndef recs_entity_map_init_size
#define recs_entity_map_init_size 2000
#endif
#ifndef recs_cmp_map_init_size
#define recs_cmp_map_init_size 2000
#endif
#ifndef recs_sys_init_size
#define recs_sys_init_size 30
#endif

//在remove和clear的时候都会执行
static void recs_entity_dict_free_value(void* data_ext, recs_entity_t* entity) {
    rinfo("free entity, id = %"PRIu64, entity->id);
}

static void recs_cmp_dict_free_value(void* data_ext, recs_cmp_t* cmp) {
    rinfo("free cmp, id = %"PRIu64, cmp->id);
}

static recs_system_t* rsystem_copy_value_func(const recs_system_t* obj) {
    //recs_system_t* dest = rdata_new_size(sizeof(*obj)); //todo Ray 柔性数组申请内存，内存池不好管理！

    //dest->sys_type = obj->sys_type;
    //dest->init_sys = obj->init_sys;
    //dest->uninit_sys = obj->uninit_sys;
    //dest->before_update_sys = obj->before_update_sys;
    //dest->update_sys = obj->update_sys;
    //dest->late_update_sys = obj->late_update_sys;
    //dest->update_fixed_sys = obj->update_fixed_sys;

    //dest->sys_name = rstr_cpy(obj->sys_name, 0);
    return (recs_system_t*)obj;
}
static void rsystem_free_value_func(recs_system_t* obj) {
    //if (obj == NULL) {
    //    return;
    //}
    //rdata_free(recs_system_t, obj);
}

int recs_init(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    if (ctx == NULL) {
        ret_code = rcode_invalid;

        rassert_goto(false, "invalid ctx.", 1);
    }

    ctx->exec_state = recs_execute_state_uninit;

    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, recs_entity_map_init_size, 0);
    rassert_goto(dict_ins != NULL, "", 1);
    (dict_ins)->free_value_func = (rdict_free_value_func_type)recs_entity_dict_free_value;
    ctx->map_entities = dict_ins;

    dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, recs_cmp_map_init_size, 0);
    rassert_goto(dict_ins != NULL, "", 1);
    (dict_ins)->free_value_func = (rdict_free_value_func_type)recs_cmp_dict_free_value;
    ctx->map_components = dict_ins;

    rarray_t* array_ins = NULL;
    rarray_init(array_ins, rdata_type_ptr, recs_sys_init_size);
    array_ins->copy_value_func = (rarray_type_copy_value_func)rsystem_copy_value_func;
    array_ins->free_value_func = (rarray_type_free_value_func)rsystem_free_value_func;
    rassert_goto(array_ins != NULL, "", 1);
    ctx->systems = array_ins;

    if (ctx->on_init != NULL) {
        ret_code = ctx->on_init(ctx, cfg_data);

        rassert_goto(ret_code == rcode_ok, "invalid code of logic, code = %d", 1, ret_code);
    }

    ctx->admin_entity = recs_entity_new(ctx, recs_etype_shared);
    
    ctx->exec_state = recs_execute_state_init;

    return rcode_ok;

exit1:
    recs_uninit(ctx, cfg_data);

    return ret_code;
}

int recs_uninit(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    if (ctx == NULL) {
        ret_code = rcode_invalid;

        rassert_goto(false, "invalid ctx.", 1);
    }

    if (ctx->admin_entity != NULL) {
        recs_entity_delete(ctx, ctx->admin_entity, true);
    }

    if (ctx->on_init != NULL) {
        ret_code = ctx->on_uninit(ctx, cfg_data);        
    }

    rarray_release(ctx->systems);

    if (ctx->map_entities && rdict_size(ctx->map_entities) > 0) {
        recs_entity_t* entity = NULL;
        rdict_iterator_t it = rdict_it(ctx->map_entities);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            entity = (recs_entity_t*)(de->value.ptr);
            if (entity != NULL && ctx->on_delete_entity != NULL) {
                ctx->on_delete_entity(ctx, entity);
            }
        }
        rdict_clear(ctx->map_entities);
        ctx->map_entities = NULL;
    }

    //最后释放，避免sys，entity引用
    if (ctx->map_components && rdict_size(ctx->map_components) > 0) {
        recs_cmp_t* cmp = NULL;
        rdict_iterator_t it = rdict_it(ctx->map_components);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            cmp = (recs_cmp_t*)(de->value.ptr);
            if (cmp != NULL && ctx->on_delete_cmp != NULL) {
                ctx->on_delete_cmp(ctx, cmp);
            }
        }
        rdict_clear(ctx->map_components);
        ctx->map_components = NULL;
    }

    rassert_goto(ret_code == rcode_ok, "invalid code of logic, code = %d", 1, ret_code);

    ctx->exec_state = recs_execute_state_uninit;

exit1:
    return ret_code;
}

R_API int recs_start(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    ctx->exec_state = recs_execute_state_running;

    return ret_code;
}

R_API int recs_pause(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    ctx->exec_state = recs_execute_state_paused;

    return ret_code;
}

R_API int recs_resume(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    ctx->exec_state = recs_execute_state_running;

    return ret_code;
}

R_API int recs_stop(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    ctx->exec_state = recs_execute_state_stopped;

    return ret_code;
}

R_API int recs_run(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    if (ctx->exec_state != recs_execute_state_running) {
        return ret_code;
    }

    recs_system_t* cur_system = NULL;

    rarray_iterator_t it = rarray_it(ctx->systems);
    for (; cur_system = rarray_next(&it), rarray_has_next(&it); ) {
        cur_system->before_update(ctx, cfg_data);
    }

    rarray_it_first(&it);
    for (; cur_system = rarray_next(&it), rarray_has_next(&it); ) {
        cur_system->update(ctx, cfg_data);
    }
    
    rarray_it_first(&it);
    for (; cur_system = rarray_next(&it), rarray_has_next(&it); ) {
        cur_system->late_update(ctx, cfg_data);
    }

    return ret_code;
}

R_API uint64_t recs_get_next_id(recs_context_t* ctx) {
    if (ctx->sid_cur > ctx->sid_max) {
        ctx->sid_cur = ctx->sid_min;
        rwarn("wrap id of ecs finished.");
    }

    return ++ctx->sid_cur;
}

R_API int recs_get_entity(recs_context_t* ctx, uint64_t entity_id, recs_entity_t** ret_entity) {
    rdict_entry_t* item = rdict_find(ctx->map_entities, (const void*)entity_id);
    if (item != NULL) {
        *ret_entity = rdict_get_value(item);
    }

    return rcode_ok;
}

R_API int recs_get_cmp(recs_context_t* ctx, uint64_t cmp_id, recs_cmp_t** ret_cmp) {
    rdict_entry_t* item = rdict_find(ctx->map_components, (const void*)cmp_id);
    if (item != NULL) {
        *ret_cmp = rdict_get_value(item);
    }

    return rcode_ok;
}
