/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RCODEC_DEFAULT_H
#define RCODEC_DEFAULT_H

#include "rcommon.h"
#include "rinterface.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#endif //__GNUC__

#ifdef __cplusplus
extern "C" {
#endif

#define ripc_data_default_len_max 2048000

#define ripc_head_default_version_len 1
#define ripc_head_default_magic_len 3
#define ripc_head_default_len_len 4
#define ripc_head_default_cmd_len 4
#define ripc_head_default_sid_len 8
#define ripc_head_default_crc_len 4
#define ripc_head_default_reserve0_len 8

#define ripc_head_default_magic "Ray"

#pragma pack(1)
#pragma pack(show)

typedef struct ripc_data_default_s {
    int8_t version;
    char magic[ripc_head_default_magic_len];
    uint32_t len;
    int32_t cmd;
    uint64_t sid;
    int32_t crc;
    uint64_t reserve0;
    char* data;
    //char data[];
} ripc_data_default_t;

#pragma pack()
#pragma pack(show)

typedef struct rcodec_s {
    // rdata_handler_t* handler;

    int (*init)(void* ctx, const void* cfg_data);
    int (*uninit)(void* ctx);
    // int (*open)();
    // int (*close)();
    // int (*start)();
    // int (*stop)();
    // int (*send)(void* data);
    // int (*check)(void* data);
    // int (*receive)(void* data);
    // int (*error)(void* data);
} rcodec_t;

extern const rdata_handler_t rcodec_encode_default;
extern const rdata_handler_t rcodec_decode_default;

#ifdef __cplusplus
}
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__

#endif //RCODEC_DEFAULT_H
