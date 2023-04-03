/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RECS_CONTEXT_H
#define RECS_CONTEXT_H

#include "rcommon.h"
#include "rarray.h"
#include "rdict.h"
#include "rstring.h"

#include "recs_component.h"
#include "recs_entity.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/


/* ------------------------------- Structs ------------------------------------*/

struct recs_context_s;

typedef enum {
    recs_execute_state_unknown = 0,
    recs_execute_state_init = 1,
    recs_execute_state_running = 2,
    recs_execute_state_paused = 3,
    recs_execute_state_stopped = 4,
    recs_execute_state_uninit = 5,
} recs_execute_state_t;

typedef struct recs_context_s recs_context_t;

typedef int (*recs_on_init_func)(recs_context_t* ctx, const void* cfg_data);
typedef int (*recs_on_uninit_func)(recs_context_t* ctx, const void* cfg_data);

typedef int (*recs_on_new_entity_func)(recs_context_t* ctx, recs_entity_t* item);
typedef int (*recs_on_delete_entity_func)(recs_context_t* ctx, recs_entity_t* item);
typedef int (*recs_on_new_cmp_func)(recs_context_t* ctx, recs_cmp_t* item);
typedef int (*recs_on_delete_cmp_func)(recs_context_t* ctx, recs_cmp_t* item);

typedef recs_cmp_t* (*recs_create_cmp_func)(recs_context_t* ctx, recs_cmp_type_t data_type);

struct recs_context_s {
    uint64_t sid_min;
    uint64_t sid_max;
    uint64_t sid_cur;

    recs_entity_t* admin_entity;
    recs_execute_state_t exec_state;
    
    rdict_t* map_entities;
    rdict_t* map_components;
    rarray_t* systems;

    recs_on_init_func on_init;
    recs_on_uninit_func on_uninit;

    recs_on_new_entity_func on_new_entity;
    recs_on_delete_entity_func on_delete_entity;
    recs_on_new_cmp_func on_new_cmp;
    recs_on_delete_cmp_func on_delete_cmp;

    recs_create_cmp_func create_cmp;
};

/* ------------------------------- APIs ------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif //RECS_CONTEXT_H
