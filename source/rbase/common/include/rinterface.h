﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RINTERFACE_H
#define RINTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*rdata_handler_on_before_func)(void* data);
typedef int (*rdata_handler_process_func)(void* data);
typedef int (*rdata_handler_on_after_func)(void* data);
typedef void (*rdata_handler_on_next_func)(void* data);

typedef void (*rdata_handler_on_notify)(void* data);
typedef void (*rdata_handler_notify)(void* data);

typedef struct rdata_handler {
    struct rdata_handler* prev;
    struct rdata_handler* next;

    rdata_handler_on_before_func on_before;
    rdata_handler_process_func process;
    rdata_handler_on_after_func on_after;
    rdata_handler_on_next_func on_next;

    rdata_handler_on_notify on_notify;// 被通知
    rdata_handler_notify notify; // 通知其他
} rdata_handler;

#ifdef __cplusplus
}
#endif

#endif //RBASE_H
