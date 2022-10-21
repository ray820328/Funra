/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSOCKET_UV_H
#define RSOCKET_UV_H

#include "rsocket.h"

#include "uv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rsocket_uv_s {
    uv_handle_t* stream;
} rsocket_uv_t;

#define rsocket_ctx_uv_fields \
    uv_loop_t* loop

typedef struct rsocket_ctx_uv_s {
    rsocket_ctx_fields;

    rsocket_ctx_uv_fields;
} rsocket_ctx_uv_t;

#ifdef __cplusplus
}
#endif

#endif //RSOCKET_UV_H
