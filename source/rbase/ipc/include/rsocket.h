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

#include "uv.h"

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
    rsocket_cfg_t* cfg; \
    ripc_entry_t* ipc_entry; \
    rdata_handler_t* in_handler; \
    rdata_handler_t* out_handler; \
    ripc_state_t stream_state

#define rsocket_ctx_uv_fields \
    uv_loop_t* loop; \
    ripc_type_t stream_type; \
    uv_handle_t* stream

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


typedef struct rsocket_ctx_uv_s {
    rsocket_ctx_fields;

    rsocket_ctx_uv_fields;
} rsocket_ctx_uv_t;


R_API int rsocket_init(const void* cfg_data);
R_API int rsocket_uninit();

R_API ripc_entry_t* get_ipc_entry(const char* key);

#ifdef __cplusplus
}
#endif

#endif
