/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RRPC_H
#define RRPC_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct rrpc_i
    {
        rdata_handler_t* handler;

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
    } rrpc_i;

    R_API int rrpc_init(const void* cfg_data);
    R_API int rrpc_uninit();

#ifdef __cplusplus
}
#endif

#endif //RRPC_H
