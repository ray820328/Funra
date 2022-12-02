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
    
    rarray_add(ctx->systems, sys_item);

	return rcode_ok;
}

R_API int recs_sys_remove(recs_context_t* ctx, recs_sys_type_t sys_type, bool stop) {

	return rcode_ok;
}
