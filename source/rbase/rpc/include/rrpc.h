/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 *
 * Node提供服务，Proxy模块提供代理转发/Node负载数据同步（如果有多个Proxy连接）
 */

#ifndef RRPC_H
#define RRPC_H

#include "rcommon.h"

#include "ripc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/

#define rrpc_service_type_default \
    rrpc_service_type_unknown = 0, \
    rrpc_service_type_config_center = 1, \
    rrpc_service_type_registry_center = 2, \
    rrpc_service_type_retain = 10

#define rrpc_service_type_t int


/* ------------------------------- Structs ------------------------------------*/

typedef struct rrpc_node_s {
    uint32_t id;
    rrpc_service_type_t service_type;
    ripc_data_source_t* ds;

    void* cfg;
} rrpc_node_t;

typedef struct rrpc_stream_s {
    uint32_t seq_id;//约定service提供者为偶数，请求者为奇数
    uint32_t len;
    char* data;
} rrpc_stream_t;

typedef int (*rrpc_init_func)(void* ctx, const void* cfg_data);

typedef struct rrpc_entry_s {
    rrpc_init_func init;
} rrpc_entry_t;


/* ------------------------------- APIs ------------------------------------*/

typedef int (*rrpc_register_node_func)(rrpc_service_type_t service_type, ripc_data_source_t* ds);
typedef int (*rrpc_on_node_register_func)(rrpc_service_type_t service_type, ripc_data_source_t* ds);
typedef int (*rrpc_on_node_unregister_func)(rrpc_service_type_t service_type, ripc_data_source_t* ds);
typedef int (*rrpc_call_service_func)(rrpc_service_type_t service_type, void* params);


#ifdef __cplusplus
}
#endif

#endif //RRPC_H
