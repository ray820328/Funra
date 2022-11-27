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
#include "rsystem.h"

R_API int recs_sys_new(recs_context_t* ctx, recs_sys_type_t sys_type, 
    recs_system_init init_sys, recs_system_uninit uninit_sys, 
    recs_system_before_update before_update_sys, recs_system_update update_sys, 
    recs_system_late_update late_update_sys, recs_system_update_fixed update_fixed_sys) {

	return rcode_ok;
}

R_API int recs_sys_delete(recs_context_t* ctx, recs_sys_type_t sys_type, bool stop) {

	return rcode_ok;
}
