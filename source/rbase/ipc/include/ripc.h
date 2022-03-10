/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RIPC_H
#define RIPC_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct ripc_item
    {
        rdata_handler* handler;

        int (*init)(const void* cfg_data);
        int (*uninit)();
        int (*open)();
        int (*close)();
        int (*start)();
        int (*stop)();
        int (*send)(void* data);
        int (*check)(void* data);
        int (*receive)(void* data);
        int (*error)(void* data);
    } ripc_item;

    R_API int ripc_init(const void* cfg_data);
    R_API int ripc_uninit();

#ifdef __cplusplus
}
#endif

#endif
