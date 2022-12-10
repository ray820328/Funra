/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rlog.h"
#include "rdict.h"

#include "recs.h"

R_API recs_entity_t* recs_entity_new(recs_context_t* ctx, recs_entity_type_t data_type) {
    int ret_code = rcode_ok;
    recs_entity_t* data = rdata_new(recs_entity_t);
    rdata_init(data, sizeof(recs_entity_t));

    if (data == NULL) {
        rwarn("create item of (%d) failed.", data_type);

        rgoto(1);
    }

    data->type_id = data_type;

    switch (data_type) {
    case recs_etype_shared:
        data->name = "Shared";
        break;
    default:
        break;
    }

    do {
        data->id = recs_get_next_id(ctx);

        if likely(!rdict_exists(ctx->map_entities, (const void*)data->id)) {
            break;
        } else {
            rwarn("id exists, value = %"PRIu64, data->id);
        }
    } while (true);

    rdict_add(ctx->map_entities, (void*)data->id, data);//统一下层数据接口

    if (ctx->on_new_entity != NULL) {
        ret_code = ctx->on_new_entity(ctx, data);

        if (ret_code != rcode_ok) {
            rdict_remove(ctx->map_entities, (const void*)data->id);
            rwarn("create item of (%d) failed.", data_type);

			recs_entity_delete(ctx, data, true);
            data = NULL;

            rgoto(1);
        }
    }

exit1:
    return data;
}

R_API int recs_entity_delete(recs_context_t* ctx, recs_entity_t* data, bool destroy) {
    int ret_code = rcode_ok;
    uint64_t data_id = 0;

    if (data == NULL) {
        rwarn("invalid params.");

        rgoto(1);
    }

    data_id = data->id;

    switch (data->type_id) {

    default:
        break;
    }

    recs_cmp_remove_all(ctx, data);

    rdict_remove(ctx->map_entities, (const void*)data_id);

    if (ctx->on_delete_entity != NULL) {
        ret_code = ctx->on_delete_entity(ctx, data);

        if (ret_code != rcode_ok) {
            rwarn("delete item of (%"PRIu64") failed.", data_id);

            rgoto(1);
        }
    }

    if (destroy) {
    	//todo Ray components 销毁
    	
    	rdata_free(recs_entity_t, data);
    }
    
exit1:
    return ret_code;
}

R_API int recs_cmp_add(recs_context_t* ctx, recs_entity_t* entity, recs_cmp_t* cmp) {
    int ret_code = rcode_ok;
    ret_code = rarray_add(entity->components, cmp);
    if (ret_code != rcode_ok) {
        rerror("add component [%d, %"PRIu64"] to [%s, %"PRIu64"] failed", cmp->type_id, cmp->id, entity->name, entity->id);
    }
    return ret_code;
}

R_API int recs_cmp_get(recs_context_t* ctx, recs_entity_t* entity, uint64_t cmp_id, recs_cmp_t** ret_cmp) {
    int ret_code = rcode_ok;
    recs_cmp_t* cmp = NULL;

    recs_get_cmp(ctx, cmp_id, &cmp);

    if likely(cmp != NULL) {
        if likely(rarray_exist(entity->components, (const void*)cmp)) {
            *ret_cmp = cmp;
        } else {
            rerror("get component [%d, %"PRIu64"] from [%s, %"PRIu64"] failed, not attached", cmp->type_id, cmp->id, entity->name, entity->id);
        }
    } else {
        ret_code = rcode_invalid;

        rwarn("get component [%"PRIu64"] from [%s, %"PRIu64"] failed, not exist", cmp_id, entity->name, entity->id);
    }

    return ret_code;
}

R_API int recs_cmp_remove(recs_context_t* ctx, recs_entity_t* entity, uint64_t cmp_id, recs_cmp_t** ret_cmp) {
    int ret_code = rcode_ok;
    recs_cmp_t* cmp = NULL;

    recs_get_cmp(ctx, cmp_id, &cmp);

    if likely(cmp != NULL) {
        ret_code = rarray_remove(entity->components, (const void*)cmp);

        if likely(ret_code != rcode_ok) {
            *ret_cmp = cmp;
        } else {
            rerror("remove component [%d, %"PRIu64"] from [%s, %"PRIu64"] failed", cmp->type_id, cmp->id, entity->name, entity->id);
        }
    } else {
        ret_code = rcode_invalid;

        rwarn("remove component [%"PRIu64"] from [%s, %"PRIu64"] failed, not exist", cmp_id, entity->name, entity->id);
    }

    return ret_code;
}

R_API int recs_cmp_remove_all(recs_context_t* ctx, recs_entity_t* entity) {
    if (entity == NULL) {
        return rcode_ok;
    }

    if (entity->components != NULL) {
        rarray_clear(entity->components);
    }

    return rcode_ok;
}

R_API int recs_cmp_active(recs_context_t* ctx, recs_entity_t* entity, uint64_t cmp_id, bool active) {

    return rcode_ok;
}

R_API int recs_cmp_active_all(recs_context_t* ctx, recs_entity_t* entity, bool active) {

    return rcode_ok;
}

