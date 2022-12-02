/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RRPC_H
#define RRPC_H

#include "rcommon.h"

#include "ripc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define rrpc_service_type_default \
    rrpc_service_type_unknown = 0, \
    rrpc_service_type_config_center = 1, \
    rrpc_service_type_registry_center = 2, \
    rrpc_service_type_retain = 10

#ifndef rrpc_service_type_ext
#define rrpc_service_type_ext
#endif //rrpc_service_type_ext 给外部定义逻辑服务类型

typedef enum {
    rrpc_service_type_default,

    rrpc_service_type_ext
} rrpc_service_type_t;

// typedef struct {
//    uint32_t method_index;
//    uint32_t request_id;
//    ProtobufCMessage *message;
// } ProtobufC_RPC_Payload;

typedef int (*rrpc_init_func)(void* ctx, const void* cfg_data);

typedef struct rrpc_entry_s {
    rrpc_init_func init;
} rrpc_entry_t;


int rrpc_register_node(rrpc_service_type_t server_type, ripc_data_source_t* ds);
int rrpc_on_node_register(rrpc_service_type_t server_type, ripc_data_source_t* ds);
int rrpc_on_node_unregister(rrpc_service_type_t server_type, ripc_data_source_t* ds);
int rrpc_call_service(rrpc_service_type_t server_type, void* params);


#ifdef __cplusplus
}
#endif

#endif //RRPC_H
