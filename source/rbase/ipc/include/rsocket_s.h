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

#include "uv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rsocket_ctx_uv_s {
    uint64_t id;
    uint64_t sid_min;
    uint64_t sid_max;

    rsocket_cfg_t* cfg;
    rdata_handler_t* handler;
    rdata_handler_t* sender;

    ripc_type_t server_type;
    uv_handle_t* server;
    uv_loop_t* loop;
    int server_state;//0 关闭
    // uv_tcp_t* server_tcp;
    // uv_udp_t* server_udp;
    // uv_pipe_t* server_pipe;
    uv_udp_send_t* udp_data_list_free;

    rdict_t* map_clients;
} rsocket_ctx_uv_t;


extern const ripc_entry_t rsocket_s;


#ifdef __cplusplus
}
#endif

#endif // RSOCKET_S_H
