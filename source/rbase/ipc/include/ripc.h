﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RIPC_H
#define RIPC_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ripc_data_len_max 2048000

#define ripc_head_len_len 4
#define ripc_head_cmd_len 4
#define ripc_head_sid_len 8
#define ripc_head_crc_len 4
#define ripc_head_reserve0_len 8

typedef enum ripc_type_t {
    ripc_type_unknown = 0,           /** no type */
    ripc_type_tcp,
    ripc_type_udp,
    ripc_type_pipe,
    ripc_type_mix,
} ripc_type_t;

typedef int (*ripc_init_func)(void* ctx, const void* cfg_data);
typedef int (*ripc_uninit_func)(void* ctx);
typedef int (*ripc_open_func)(void* ctx);
typedef int (*ripc_close_func)(void* ctx);
typedef int (*ripc_start_func)(void* ctx);
typedef int (*ripc_stop_func)(void* ctx);
typedef int (*ripc_send_func)(void* ctx, void* data);
typedef int (*ripc_check_func)(void* ctx, void* data);
typedef int (*ripc_receive_func)(void* ctx, void* data);
typedef int (*ripc_error_func)(void* ctx, void* data);

typedef struct ripc_entry_s {
    ripc_init_func init;
    ripc_uninit_func uninit;
    ripc_open_func open;
    ripc_close_func close;
    ripc_start_func start;
    ripc_stop_func stop;
    ripc_send_func send;
    ripc_check_func check;
    ripc_receive_func receive;
    ripc_error_func error;
} ripc_entry_t;

typedef struct ripc_data_raw_s {
    uint32_t len;
    char* data;
} ripc_data_raw_t;


typedef struct ripc_data_s {
    uint32_t len;
    int32_t cmd;
    uint64_t sid;
    int32_t crc;
    uint64_t reserve0;
    char* data;
} ripc_data_t;

R_API int ripc_init(const void* cfg_data);
R_API int ripc_uninit();

#ifdef __cplusplus
}
#endif

#endif
