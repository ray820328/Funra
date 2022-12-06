/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rlog.h"
#include "rarray.h"

#include "recs.h"

/**按添加自然顺序**/
R_API int recs_sys_add(recs_context_t* ctx, recs_system_t* sys_item) {
    if (sys_item == NULL) {
        rerror("invalid system");
        return rcode_invalid;
    }

    if (rarray_exist(ctx->systems, sys_item)) {
        rerror("system exists, type = %d, name = %s", sys_item->type, sys_item->name);
        return rcode_invalid;
    }
    
    sys_item->init(ctx, NULL);

    rarray_add(ctx->systems, sys_item);

	return rcode_ok;
}

R_API int recs_sys_remove(recs_context_t* ctx, recs_system_t* sys_item) {
    if (sys_item == NULL) {
        rerror("invalid system");
        return rcode_ok;
    }

    if (!rarray_exist(ctx->systems, sys_item)) {
        rerror("system not exists, type = %d, name = %s", sys_item->type, sys_item->name);
        return rcode_invalid;
    }

    rinfo("remove system, type = %d", sys_item->type);

    rarray_remove(ctx->systems, sys_item);

    sys_item->uninit(ctx, NULL);

	return rcode_ok;
}

//system单实例，同一个type仅有一个
R_API recs_system_t* recs_sys_find(recs_context_t* ctx, recs_sys_type_t sys_type) {
    recs_system_t* sys_item = NULL;

    rarray_iterator_t it = rarray_it(ctx->systems);
    for (; sys_item = rarray_next(&it), rarray_has_next(&it); ) {
        if (sys_item->type == sys_type) {
            break;
        }
        sys_item = NULL;
    }

    return sys_item;
}
