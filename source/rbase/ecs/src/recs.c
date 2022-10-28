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
#include "rentity.h"

#ifndef recs_entity_map_init_size
#define recs_entity_map_init_size 2000
#endif
#ifndef recs_cmp_map_init_size
#define recs_cmp_map_init_size 2000
#endif
#ifndef recs_sys_map_init_size
#define recs_sys_map_init_size 200
#endif

int recs_init(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = rcode_ok;

    if (ctx == NULL) {
        ret_code = rcode_invalid;

        rassert_goto(false, "invalid ctx.", 1);
    }

    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, recs_entity_map_init_size, 0);
    rassert_goto(dict_ins != NULL, "", 1);
    ctx->map_entities = dict_ins;

    dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, recs_cmp_map_init_size, 0);
    rassert_goto(dict_ins != NULL, "", 1);
    ctx->map_components = dict_ins;

    dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, recs_sys_map_init_size, 0);
    rassert_goto(dict_ins != NULL, "", 1);
    ctx->map_systems = dict_ins;

    if (ctx->on_init != NULL) {
        ret_code = ctx->on_init(ctx, cfg_data);

        rassert_goto(ret_code == rcode_ok, "invalid code of logic, code = %d", 1, ret_code);
    }

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

    if (ctx->on_init != NULL) {
        ret_code = ctx->on_uninit(ctx, cfg_data);        
    }

    if (ctx->map_systems && rdict_size(ctx->map_systems) > 0) {
        recs_sys_t* sys = NULL;
        rdict_iterator_t it = rdict_it(ctx->map_systems);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            sys = (recs_sys_t*)(de->value.ptr);
        }
        rdict_clear(ctx->map_systems);
        ctx->map_systems = NULL;
    }

    if (ctx->map_entities && rdict_size(ctx->map_entities) > 0) {
        recs_entity_t* entity = NULL;
        rdict_iterator_t it = rdict_it(ctx->map_entities);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            entity = (recs_entity_t*)(de->value.ptr);
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
        }
        rdict_clear(ctx->map_components);
        ctx->map_components = NULL;
    }

    rassert_goto(ret_code == rcode_ok, "invalid code of logic, code = %d", 1, ret_code);

exit1:
    return ret_code;
}

R_API uint64_t recs_get_next_id(recs_context_t* ctx) {
    if (ctx->sid_cur > ctx->sid_max) {
        ctx->sid_cur = ctx->sid_min;
        rwarn("wrap id of ecs finished.");
    }

    return ++ctx->sid_cur;
}

R_API int recs_get_entity(recs_context_t* ctx, uint64_t entity_id, recs_cmp_t** ret_entity) {
    rdict_entry_t* item = rdict_find(ctx->map_entities, entity_id);
    if (item != NULL) {
        *ret_entity = rdict_get_value(item);
    }

    return rcode_ok;
}

R_API int recs_get_cmp(recs_context_t* ctx, uint64_t cmp_id, recs_entity_t** ret_cmp) {
    rdict_entry_t* item = rdict_find(ctx->map_components, cmp_id);
    if (item != NULL) {
        *ret_cmp = rdict_get_value(item);
    }

    return rcode_ok;
}
