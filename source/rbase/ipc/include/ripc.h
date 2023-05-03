/**
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
#include "rbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Macros ------------------------------------*/


/* ------------------------------- Structs ------------------------------------*/

typedef enum {
    ripc_data_source_type_unknown = 0,
    ripc_data_source_type_server = 1,
    ripc_data_source_type_client = 2,
    ripc_data_source_type_peer = 3,
    ripc_data_source_type_session = 4,
    ripc_data_source_type_mix = 5,
} ripc_data_source_type_t;

typedef enum {
    ripc_type_unknown = 0,           /** no type */
    ripc_type_tcp,
    ripc_type_udp,
    ripc_type_pipe,
    ripc_type_mix,
} ripc_type_t;

typedef enum {
    ripc_state_unknown = 0,           /** invalid */
    ripc_state_init = 1,
    ripc_state_ready_pending = 2,
    ripc_state_ready = 3,
    ripc_state_start,
    ripc_state_stop,
    ripc_state_disconnect,
    ripc_state_closed,
    ripc_state_error,
    ripc_state_full,
    ripc_state_timeout,
    ripc_state_uninit,
} ripc_state_t;

typedef enum {

    //ipc部分 12000 - 12199
    // rcode_err_ipc_begin = 12000,
    // rcode_err_ipc_unknown = 12001,
    rcode_err_ipc_connect = 12002,
    rcode_err_ipc_timeout,
    rcode_err_ipc_disconnect,
    rcode_err_ipc_broken_pipe_in,
    rcode_err_ipc_broken_pipe_out,
    rcode_err_ipc_encode,
    rcode_err_ipc_decode,
    rcode_err_ipc_version,
    rcode_err_ipc_magic,
    rcode_err_ipc_cache_full,
    rcode_err_ipc_cache_null,
    // rcode_err_ipc_end = 12199,

} rcode_err_t;

typedef struct ripc_data_raw_s {
    uint32_t len;
    char* data;
} ripc_data_raw_t;

typedef struct ripc_data_source_s {
    uint64_t ds_id;
    ripc_data_source_type_t ds_type;
    ripc_state_t state;
    rbuffer_t* read_cache;
    rbuffer_t* write_buff;
    void* ctx;
    void* stream;
} ripc_data_source_t;

typedef int (*ripc_init_func)(void* ctx, const void* cfg_data);
typedef int (*ripc_uninit_func)(void* ctx);
typedef int (*ripc_open_func)(void* ctx);
typedef int (*ripc_close_func)(void* ctx);
typedef int (*ripc_start_func)(void* ctx);
typedef int (*ripc_stop_func)(void* ctx);
typedef int (*ripc_send_func)(ripc_data_source_t* ds, void* data);
typedef int (*ripc_check_func)(ripc_data_source_t* ds, void* data);
typedef int (*ripc_receive_func)(ripc_data_source_t* ds, void* data);
typedef int (*ripc_error_func)(ripc_data_source_t* ds, void* data);

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


/* ------------------------------- APIs ------------------------------------*/

int ripc_on_code(rdata_handler_t* handler, void* ds, void* data, int code);

#ifdef __cplusplus
}
#endif

#endif //RIPC_H
