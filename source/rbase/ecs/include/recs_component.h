/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RECS_COMPONENT_H
#define RECS_COMPONENT_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/

#define recs_cmp_type_t int

#define recs_cmp_fields \
    uint64_t id; \
    recs_cmp_type_t type_id

/* ------------------------------- Structs ------------------------------------*/

typedef enum {
    recs_ctype_unknown = 0,

    recs_ctype_base_end = 10,
} recs_ctype_base_t;

typedef struct recs_component_s {//ECS模块组件定义, 宗旨为组件仅含有数据没有逻辑函数
    recs_cmp_fields;

} recs_cmp_t;

/* ------------------------------- APIs ------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif //RECS_COMPONENT_H
