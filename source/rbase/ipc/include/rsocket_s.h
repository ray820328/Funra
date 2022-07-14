/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSOCKET_S_H
#define RSOCKET_S_H

#include "rcommon.h"
#include "rdict.h"
#include "ripc.h"
#include "rsocket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rsocket_context_t {
    uint64_t id;

    rsocket_cfg_t* cfg;
} rsocket_context_t;

extern const ripc_item rsocket_s;


#ifdef __cplusplus
}
#endif

#endif // RSOCKET_S_H
