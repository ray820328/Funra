/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rlog.h"

#include "recs.h"
#include "rcomponent.h"


R_API recs_cmp_t* recs_cmp_new(recs_context_t* ctx, recs_cmp_type_t data_type) {
    int ret_code = rcode_ok;
    recs_cmp_t* data = NULL;

    if (ctx == NULL) {
        rwarn("invalid params.");
        rgoto(1);
    }

    switch (data_type) {

    default:
        if (ctx->create_cmp != NULL) {
            data = ctx->create_cmp(ctx, data_type);
        }
        break;
    }

    if (data == NULL) {
        if (ret_code == rcode_ok) {
            ret_code = rcode_invalid;
        }
        rwarn("create item of (%d) failed, code = %d", data_type, ret_code);

        rgoto(1);
    }

    do {
        data->id = recs_get_next_id(ctx);

        if likely(!rdict_exists(ctx->map_components, data->id)) {
            break;
        }
        else {
            rwarn("id exists, value = %"PRIu64, data->id);
        }
    } while (true);

    rdict_add(ctx->map_components, data->id, data);//统一下层数据接口

    if (ctx->on_new_cmp != NULL) {
        ret_code = ctx->on_new_cmp(ctx, data);

        if (ret_code != rcode_ok) {
            rdict_remove(ctx->map_components, data->id);
            rwarn("create item of (%d) failed.", data_type);

            rgoto(1);
        }
    }

exit1:
    return ret_code;
}

R_API int recs_cmp_delete(recs_context_t* ctx, recs_cmp_t* data, bool destroy) {
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

    if (ctx->on_delete_cmp != NULL) {
        ret_code = ctx->on_delete_cmp(ctx, data);

        if (ret_code != rcode_ok) {
            rwarn("delete item of (%"PRIu64") failed.", data_id);

            rgoto(1);
        }
    }

    rdict_remove(ctx->map_components, data_id);

exit1:
    return ret_code;
}
