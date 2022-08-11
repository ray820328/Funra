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

#define rsocket_session_fields \
    ripc_type_t type; \
    uint64_t id; \
    void* userdata; \
    void* context; \
    int state; \
    bool is_encrypt

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

typedef struct rsocket_session_uv_s {
    uint64_t id;

    rsocket_cfg_t* cfg;
    rdata_handler_t* in_handler;
    rdata_handler_t* out_handler;

    ripc_type_t peer_type;
    uv_handle_t* peer;
    //uv_connect_t* connect_req;
    uv_loop_t* loop;
    int peer_state;//0 关闭

} rsocket_session_uv_t;


R_API int rsocket_init(const void* cfg_data);
R_API int rsocket_uninit();

R_API ripc_entry_t* get_ipc_entry(const char* key);

#ifdef __cplusplus
}
#endif

#endif
