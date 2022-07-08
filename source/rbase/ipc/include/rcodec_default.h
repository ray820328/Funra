﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RCODEC_DEFAULT_H
#define RCODEC_DEFAULT_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct rcodec
    {
        // rdata_handler* handler;

        int (*init)(const void* cfg_data);
        int (*uninit)();
        // int (*open)();
        // int (*close)();
        // int (*start)();
        // int (*stop)();
        // int (*send)(void* data);
        // int (*check)(void* data);
        // int (*receive)(void* data);
        // int (*error)(void* data);
    } rcodec;

    extern const rcodec rcodec_default;

#ifdef __cplusplus
}
#endif

#endif //RCODEC_DEFAULT_H
