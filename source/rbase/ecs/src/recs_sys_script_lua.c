/** 
 * Copyright (c) 2016
 *
 * This library is free software { you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "lua.h"

#include "rlog.h"

#include "recs.h"

static int init_lua(recs_context_t* ctx, const void* cfg_data) {

    //recs_entity_t* admin_entity = NULL;
    //
    //lua_State* LS = luaL_newstate();

    //rassert(LS != NULL, "new lua state failed.");

    //luaL_openlibs(LS);

    return rcode_ok;
}

static int uninit_lua(recs_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static int before_update_lua(recs_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static int update_lua(recs_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static int late_update_lua(recs_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static int update_fixed_lua(recs_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static recs_system_t recs_sys_script_lua_obj = {
    .type = recs_stype_execute_script_lua,// recs_sys_type_t
    .init = init_lua,// recs_sys_init
    .uninit = uninit_lua,// recs_sys_uninit
    .before_update = before_update_lua,// recs_sys_before_update
    .update = update_lua,// recs_sys_update
    .late_update = late_update_lua,// late_update
    .update_fixed = update_fixed_lua,// update_fixed
    .name = "lua_script_system"// char name[0]
};

const recs_system_t* recs_sys_script_lua = &recs_sys_script_lua_obj;