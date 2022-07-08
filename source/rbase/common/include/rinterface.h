/**
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

typedef int (*rdata_on_before_func)(void* data);
typedef int (*rdata_on_process_func)(void* data);
typedef int (*rdata_on_after_func)(void* data);
typedef void (*rdata_on_next_func)(void* data);

typedef struct rdata_handler {
    struct rdata_handler* prev;
    struct rdata_handler* next;

    int (*on_before)(void* data);
    int (*on_process)(void* data);
    int (*on_after)(void* data);
    void (*on_next)(void* data);

    void (*on_notify)(void* data);
    void (*notify)(void* data);
} rdata_handler;

#ifdef __cplusplus
}
#endif

#endif //RBASE_H
