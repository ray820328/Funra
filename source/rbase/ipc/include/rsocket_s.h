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

typedef struct rsocket_server_ctx_s {
    rsocket_ctx_fields;

    //rsocket_ctx_uv_fields;

    uint64_t sid_cur;

    rdict_t* map_clients;
} rsocket_server_ctx_t;


extern const ripc_entry_t* rsocket_s;


#ifdef __cplusplus
}
#endif

#endif // RSOCKET_S_H
