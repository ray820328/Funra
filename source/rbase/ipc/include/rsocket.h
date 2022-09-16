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
#include "ripc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _rsocket_session_fields \
    ripc_type_t type; \
    uint64_t id; \
    void* userdata; \
    void* context; \
    int state; \
    bool is_encrypt

#define rsocket_ctx_fields \
    uint64_t id; \
    ripc_type_t stream_type; \
    rsocket_cfg_t* cfg; \
    ripc_entry_t* ipc_entry; \
    rdata_handler_t* in_handler; \
    rdata_handler_t* out_handler; \
    ripc_state_t stream_state

typedef struct rsocket_cfg_s {
    uint64_t id;
    uint64_t sid_min;
    uint64_t sid_max;

    char ip[20];
    int port;
    int backlog;
    uint32_t sock_flag;
    bool encrypt_msg;
} rsocket_cfg_t;

typedef struct rsocket_ctx_s {
    rsocket_ctx_fields;

    ripc_data_source_stream_t* stream;
} rsocket_ctx_t;

#ifdef __cplusplus
}
#endif

#endif
