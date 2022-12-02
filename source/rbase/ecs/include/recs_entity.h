/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RECS_ENTITY_H
#define RECS_ENTITY_H

#include "rcommon.h"
#include "rarray.h"

#include "recs_component.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/

#define recs_entity_type_t int

/* ------------------------------- Structs ------------------------------------*/

typedef enum {
    recs_etype_unknown = 0,
    recs_etype_shared = 1, //system共享数据集合

    recs_etype_base_end = 10,
} recs_etype_base_t;

typedef struct recs_entity_s {//ECS模块entity定义, 宗旨为仅含有component
    uint64_t id;
    recs_entity_type_t type_id;
    char* name;
    rarray_t* components;

    rdata_userdata_t* userdata;
} recs_entity_t;

/* ------------------------------- APIs ------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif //RECS_ENTITY_H
