/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"
#include "rlog.h"
#include "rsocket_c.h"
#include "rtools.h"

typedef struct {
    ripc_data_source_t* ds;
    int write_size;
} local_write_req_t;


static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

static int ripc_close(void* ctx);

static void after_write(void *req, int status) {
    rinfo("end after_write state: %d", status);

    if (status != 0) {
        rerror("write error, code = %d, err = %s", status, uv_err_name(status));
    }

}
static void send_data(ripc_data_source_t* ds, void* data) {
    int ret_code = 0;
    local_write_req_t* wr;
    ripc_data_source_t* ds_client = ds;// ((uv_tcp_t*)(rsocket_ctx->peer))->data;
}

static void on_close(void* peer) {
    rinfo("on client close, id = %"PRIu64", ", 0);

}

static void after_read(void* handle, size_t nread, const void* buf) {
    int ret_code = 0;


    rinfo("after read: %d", nread);

}
/** 强制要求使用buffer/cache 编码解码 **/
static void read_alloc_static(void* handle, size_t suggested_size, void* buf) {
    /* up to 16 datagrams at once */
    //static char slab[16 * 1024];
    //todo Ray 多线程收发会有问题
}
static void connect_cb(void* req, int status) {
    if (req == NULL || status != 0) {
        rerror("client callback, req = %p, status: %d", req, status);
        rgoto(1);
    }

    rinfo("connect callback, status: %d", status);

exit1:
    if (req != NULL) {
        
    }
}

static void client_connect(rsocket_ctx_t* rsocket_ctx) {

    struct sockaddr_in addr;
    int ret_code;

    rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    char* ip = cfg->ip;
    int port = cfg->port;

    rinfo("client connecting to {%s:%d}..", ip, port);
    
exit1:
    if (ret_code != 0) {
        
    }
}


static int ripc_init(void* ctx, const void* cfg_data) {
    rinfo("socket client init.");

    int ret_code;
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    
    return rcode_ok;
}

static int ripc_uninit(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client uninit.");

    rsocket_ctx->stream_state = ripc_state_uninit;

    return rcode_ok;
}

static int ripc_open(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client open.");

    client_connect(rsocket_ctx);

    return rcode_ok;
}

static int ripc_close(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* datasource = NULL;// (ripc_data_source_t*)(rsocket_ctx->stream)->data;

    rinfo("socket client close.");

    if (rsocket_ctx->stream_state == ripc_state_closed) {
        return rcode_ok;
    }

    rsocket_ctx->stream_state = ripc_state_closed;

    //uv_stop(rsocket_ctx->loop);

    return rcode_ok;
}

static int ripc_start(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client start.");

    return 0;
}

static int ripc_stop(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client stop.");


    return rcode_ok;
}

static const ripc_entry_t impl = {
    (ripc_init_func)ripc_init,// ripc_init_func init;
    (ripc_uninit_func)ripc_uninit,// ripc_uninit_func uninit;
    (ripc_open_func)ripc_open,// ripc_open_func open;
    (ripc_close_func)ripc_close,// ripc_close_func close;
    (ripc_start_func)ripc_start,// ripc_start_func start;
    (ripc_stop_func)ripc_stop,// ripc_stop_func stop;
    (ripc_send_func)send_data,// ripc_send_func send;
    NULL,// ripc_check_func check;
    NULL,// ripc_receive_func receive;
    NULL// ripc_error_func error;
};
//const ripc_entry_t* rsocket_c = &impl;

#undef local_write_req_t
