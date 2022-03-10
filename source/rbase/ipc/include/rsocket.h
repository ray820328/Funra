/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSOCKET_H
#define RSOCKET_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    R_API int rsocket_init(const void* cfg_data);
    R_API int rsocket_uninit();

    R_API ripc_item* get_ipc_item(const char* key);

#ifdef __cplusplus
}
#endif

#endif
