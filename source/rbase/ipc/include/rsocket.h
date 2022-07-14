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

#define rsocket_session_fields \
    ripc_type_t type; \
    uint64_t id; \
    void* userdata; \
    void* context; \
    int state; \
    bool is_encrypt

typedef struct rsocket_cfg_t {
    uint64_t id;

    char ip[20];
    int port;
    int backlog;
    uint32_t sock_flag;
    bool encrypt_msg;
} rsocket_cfg_t;

R_API int rsocket_init(const void* cfg_data);
R_API int rsocket_uninit();

R_API ripc_item* get_ipc_item(const char* key);

#ifdef __cplusplus
}
#endif

#endif
