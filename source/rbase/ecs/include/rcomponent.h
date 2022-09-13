/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RCOMPONENT_H
#define RCOMPONENT_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct rcomponent
    {
        recs_cmp_type_t type;
        char* type_name;

    } rrpc_i;

#ifdef __cplusplus
}
#endif

#endif //RCOMPONENT_H
