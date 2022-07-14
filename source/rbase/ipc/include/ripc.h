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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ripc_type_t {
    ripc_type_unknown = 0,           /** no type */
    ripc_type_tcp,
    ripc_type_udp,
    ripc_type_pipe,
    ripc_type_mix,
} ripc_type_t;

typedef int (*ripc_init_func)(const void* cfg_data);
typedef int (*ripc_uninit_func)();
typedef int (*ripc_open_func)();
typedef int (*ripc_close_func)();
typedef int (*ripc_start_func)();
typedef int (*ripc_stop_func)();
typedef int (*ripc_send_func)(void* data);
typedef int (*ripc_check_func)(void* data);
typedef int (*ripc_receive_func)(void* data);
typedef int (*ripc_error_func)(void* data);

typedef struct ripc_item {
    rdata_handler* handler;

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
} ripc_item;

R_API int ripc_init(const void* cfg_data);
R_API int ripc_uninit();

#ifdef __cplusplus
}
#endif

#endif
