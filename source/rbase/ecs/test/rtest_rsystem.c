/** 
 * Copyright (c) 2016
 *
 * This library is free software { you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "rlog.h"

#include "recs_context.h"
#include "recs.h"

#include "rtest.h"

static int init(recs_context_t* ctx, const void* cfg_data) {
    int status = 0;
    // recs_entity_t* admin_entity = NULL;
    
    rinfo("system init finished.");

    return rcode_ok;
}

static int uninit(recs_context_t* ctx, const void* cfg_data) {

    rinfo("system uninit finished.");

    return rcode_ok;
}

static int before_update(recs_context_t* ctx, const void* cfg_data) {

    rinfo("test before update");

    return rcode_ok;
}

static int update(recs_context_t* ctx, const void* cfg_data) {
    int ret_code = 0;

    rinfo("test update");

    return ret_code;
}

static int late_update(recs_context_t* ctx, const void* cfg_data) {

    rinfo("test late update");

    return rcode_ok;
}

static int update_fixed(recs_context_t* ctx, const void* cfg_data) {

    rinfo("test fix update");

    return rcode_ok;
}

static recs_system_t rtest_recs_test_system_obj = {
    .type = recs_stype_execute_test,// recs_sys_type_t
    .init = init,// recs_sys_init
    .uninit = uninit,// recs_sys_uninit
    .before_update = before_update,// recs_sys_before_update
    .update = update,// recs_sys_update
    .late_update = late_update,// late_update
    .update_fixed = update_fixed,// update_fixed
    .name = "rtest_system"// char name[0]
};

const recs_system_t* rtest_recs_test_system = &rtest_recs_test_system_obj;


#ifdef __cplusplus
}
#endif
