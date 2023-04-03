/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSCRIPT_LUA_H
#define RSCRIPT_LUA_H

#include "lua.h"

#include "rcommon.h"
#include "rstring.h"
#include "rarray.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/


/* ------------------------------- Structs ------------------------------------*/

typedef struct rscript_context_lua_s {
    lua_State* L;

    rarray_t* all_states;

} rscript_context_lua_t;

typedef struct rscript_lua_cfg_s {
    char* entry;
    char* name;
} rscript_lua_cfg_t;


/* ------------------------------- APIs ------------------------------------*/


extern const rscript_t* rscript_lua;

#ifdef __cplusplus
}
#endif

#endif //RSCRIPT_LUA_H
