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
#include "rdict.h"
#include "rsocket_s.h"
#include "rcodec_default.h"

typedef struct {
    ripc_data_source_t* ds;
    int write_size;
} local_write_req_t;


static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

static void send_data(ripc_data_source_t* ds, void* data);
static void after_read(void*, size_t nread, const void* buf);
static void on_session_close(void* peer);
static void on_server_close(void* handle);
static void on_connection(void*, int status);

/** 强制要求使用buffer/cache 编码解码 **/
static void read_alloc_static(void* handle, size_t suggested_size, void* buf) {
    //todo Ray 多线程收发会有问题
    //ripc_data_source_t* datasource = (ripc_data_source_t*)(handle->data);
    //buf->base = rbuffer_write_start_dest(datasource->read_cache);
    //buf->len = rbuffer_left(datasource->read_cache);
}

static void on_connection(void* server, int status) {
    rinfo("on_connection accept, code: %d", status);

}

static void after_shutdown_client(void* req, int status) {
    
}

static void after_read(void* handle, size_t nread, const void* buf) {
    int ret_code = 0;
    
    rdebug("after read: %d", nread);

}

static void after_write(void *req, int status) {
    rinfo("end after_write state: %d", status);

}
static void send_data(ripc_data_source_t* ds, void* data) {
    int ret_code = 0;
    local_write_req_t* wr;
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ds->ctx;
}

static void on_session_close(void* peer) {
    //todo Ray 暂时仅tcp
    //ripc_data_source_t* datasource = (ripc_data_source_t*)(peer->data);
    //rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)(datasource->ctx);
    //rinfo("on session close, id = %"PRIu64", peer = %p", datasource->ds_id, peer);

    //if (datasource->ds_type == ripc_data_source_type_session && rdict_exists(rsocket_ctx->map_clients, (const void*)datasource->ds_id)) {
    //    rdict_remove(rsocket_ctx->map_clients, (const void*)datasource->ds_id);

    //    rbuffer_release(datasource->read_cache);
    //    rbuffer_release(datasource->write_buff);
    //    rdata_free(uv_tcp_t, datasource->stream);
    //    rdata_free(ripc_data_source_t, datasource);
    //}
    //else if (datasource->ds_type == ripc_data_source_type_server) {
    //    rerror("on_session_close error, cant release server.");
    //}
}

static void on_server_close(void* server) {//不能再close callback里调用uv方法
    rinfo("server close success. server = %p", server);
}

static int start_server_tcp4(rsocket_ctx_t* rsocket_ctx) {
    int ret_code;
    rinfo("socket server open.");

    rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    char* ip = cfg->ip;
    int port = cfg->port;
    struct sockaddr_in bind_addr;


    rinfo("%p listen at %s:%d success.", rsocket_ctx->cfg, ip, port);

    return rcode_ok;
}


//static int start_server_tcp6(int port) {
//    return 0;
//}

static int ripc_init(void* ctx, const void* cfg_data) {
    rinfo("socket server init.");

    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ctx;

    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, 2000, 0);
    rassert(dict_ins != NULL, "");
	rsocket_ctx->map_clients = dict_ins;

    return rcode_ok;
}

static int ripc_uninit(void* ctx) {
    rinfo("socket server uninit.");

    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ctx;

    if (rsocket_ctx->map_clients != NULL) {
        rdict_free(rsocket_ctx->map_clients);
    }

    return rcode_ok;
}

static int ripc_open(void* ctx) {
    rinfo("socket server open.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    int ret_code = start_server_tcp4(rsocket_ctx);
    if (ret_code != rcode_ok) {
        rerror("error on start server, code: %d", ret_code);
        return ret_code;
    }
    rsocket_ctx->stream_state = ripc_state_ready;

    return ret_code;
}

static int ripc_close(void* ctx) {
    rinfo("socket server close.");

    int ret_code = 0;
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket server close success.");

    return rcode_ok;
}

static int ripc_start(void* ctx) {
    rinfo("socket server start.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("end, socket server start.");

    return rcode_ok;
}

static int ripc_stop(void* ctx) {
	rinfo("socket server stop.");

    //直接调用只能在uv线程内，跨线程用asnc接口

    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ctx;
    rsocket_ctx->stream_state = ripc_state_closed;

    if (rsocket_ctx->map_clients && rdict_size(rsocket_ctx->map_clients) > 0) {
        ripc_data_source_t* ds_client = NULL;
        rdict_iterator_t it = rdict_it(rsocket_ctx->map_clients);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            ds_client = (ripc_data_source_t*)(de->value.ptr);
            //uv_close((uv_stream_t*)ds_client->stream, on_session_close);
        }
        rdict_clear(rsocket_ctx->map_clients);
    }

    //uv_close(rsocket_ctx->stream, on_server_close);

    return rcode_ok;
}

const ripc_entry_t rsocket_s = {
    ripc_init,// ripc_init_func init;
    ripc_uninit,// ripc_uninit_func uninit;
    ripc_open,// ripc_open_func open;
    ripc_close,// ripc_close_func close;
    ripc_start,// ripc_start_func start;
    ripc_stop,// ripc_stop_func stop;
    send_data,// ripc_send_func send;
    NULL,// ripc_check_func check;
    NULL,// ripc_receive_func receive;
    NULL// ripc_error_func error;
};

#undef local_write_req_t