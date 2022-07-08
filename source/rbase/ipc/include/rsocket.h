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

typedef enum ripc_session_type {
    ripc_session_type_unknown = 0,           /** no type */
    ripc_session_type_TCP,
    ripc_session_type_UDP,
    ripc_session_type_MIX,
} ripc_session_type;

typedef struct ripc_session {
    ripc_session_type type;
    uint64_t id;

    void* userdata;
    void* context;
    bool is_working;
    bool is_encrypt;
} ripc_session;

R_API int rsocket_init(const void* cfg_data);
R_API int rsocket_uninit();

R_API ripc_item* get_ipc_item(const char* key);

#ifdef __cplusplus
}
#endif

#endif
