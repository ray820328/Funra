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
#include "rsocket_uv_c.h"
#include "rtools.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

typedef struct {
    ripc_data_source_t* ds;
    int write_size;
} local_write_req_t;


static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

static int ripc_close(void* ctx);

static void set_nonblocking(uv_os_sock_t sock) {
    int r;
#ifdef _WIN32
    unsigned long on = 1;
    r = ioctlsocket(sock, FIONBIO, &on);
    rassert(r == 0, "");
#else
    int flags = fcntl(sock, F_GETFL, 0);
    rassert(flags >= 0, "");
    r = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    rassert(r >= 0, "");
#endif
}

static void after_write(uv_write_t *req, int status) {
    rinfo("end after_write state: %d", status);

    if (status != 0) {
        rerror("write error, code = %d, err = %s", status, uv_err_name(status));
    }

    local_write_req_t* wr = (local_write_req_t*)req->data;
    ripc_data_source_t* datasource = wr->ds;
    rbuffer_skip(datasource->write_buff, wr->write_size);
    rdata_free(local_write_req_t, wr);
    rdata_free(uv_write_t, req);

    rdebug("writing buff left bytes: %d - %d", rbuffer_write_start_pos(datasource->write_buff), rbuffer_size(datasource->write_buff));
}
static int send_data(ripc_data_source_t* ds, void* data) {
    int ret_code = rcode_ok;
    local_write_req_t* wr;
    ripc_data_source_t* ds_client = ds;// ((uv_tcp_t*)(rsocket_ctx->peer))->data;
    rsocket_ctx_uv_t* ctx = (rsocket_ctx_uv_t*)ds->ctx;
    uv_write_t* req = NULL;

    if (ctx->out_handler) {
        ret_code = ctx->out_handler->process(ctx->out_handler, ds_client, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return ret_code;
        }
    }

    uv_buf_t buf;
    //buf = uv_buf_init(data->data, data->len);//结构体内容复制
    buf.base = rbuffer_read_start_dest(ds_client->write_buff);
    buf.len = rbuffer_size(ds_client->write_buff);

    wr = rdata_new(local_write_req_t);
    wr->ds = ds_client;
    wr->write_size = buf.len;

    req = rdata_new(uv_write_t);
    req->data = wr;

    //unix间接调用uv_write2 malloc了buf放到req里再cb，但是win里tcp实现是直接WSASend！操蛋
    ret_code = uv_write(req, (uv_stream_t*)(ctx->stream), &buf, 1, after_write);
    rdebug("end client send_data, req: %p, buf: %p", req, &buf);
    //rdebug("send_data, len: %d, dest_len: %p, data_buf: %p", data->len, data->data, buf.base);

    if (ret_code != rcode_ok) {
        rerror("uv_write failed. code: %d", ret_code);
        return ret_code;
    }

    return rcode_ok;
}

static void on_close(uv_handle_t* peer) {
    //todo Ray 暂时仅tcp
    ripc_data_source_t* ds_client = (ripc_data_source_t*)(peer->data);
    //rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)(datasource->ctx);
    rinfo("on client close, id = %"PRIu64", ", ds_client->ds_id);

    rbuffer_release(ds_client->read_cache);
    rbuffer_release(ds_client->write_buff);
    //rdata_free(ripc_data_source_t, ds_client);//外面释放
    //rdata_free(uv_tcp_t, peer);
}

static void after_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    int ret_code = 0;

    ripc_data_source_t* ds_client = (ripc_data_source_t*)handle->data;
    rsocket_ctx_uv_t* ctx = (rsocket_ctx_uv_t*)(ds_client->ctx);
    ripc_data_raw_t data_raw;//直接在栈上

    local_write_req_t *wr;
    int shutdown = 0;

    rinfo("after read: %d", nread);

    if (nread < 0) {
        //rstr_free(((uv_buf_t*)buf)->base);

        /* Error or EOF */
        if (nread != UV_EOF) {//异常，未主动关闭等
            rerror("error on read: %d", nread);
            ret_code = ripc_close(ctx);
            if (ret_code != 0) {
                rerror("error on ripc_close eof, code: %d", ret_code);
            }
            return;
        }

        if (uv_is_writable(handle)) {
            ret_code = ripc_close(ctx);
            if (ret_code != 0) {
                rerror("error on ripc_close, code: %d", ret_code);
                return;
            }
        }
        return;
    }

    if (nread == 0) {
        /* Everything OK, but nothing read. */
        //rstr_free(((uv_buf_t*)buf)->base);//buffer
        return;
    }

    if (ctx->in_handler) {
        //if (datasource->read_type == append_new) { //每一次都是new一个空间去读
            //data_raw.len = nread;
            //data_raw.data = buf->base;
        //} else if (append_cache) {//

        data_raw.len = nread;
        //data_raw.data = datasource->read_cache;
        //}

        ret_code = ctx->in_handler->process(ctx->in_handler, ds_client, &data_raw);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return;
        }
    }

}
/** 强制要求使用buffer/cache 编码解码 **/
static void read_alloc_static(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    /* up to 16 datagrams at once */
    //static char slab[16 * 1024];
    //todo Ray 多线程收发会有问题
    ripc_data_source_t* ds_client = (ripc_data_source_t*)(handle->data);
    buf->base = rbuffer_write_start_dest(ds_client->read_cache);
    buf->len = rbuffer_left(ds_client->read_cache);
}
static void connect_cb(uv_connect_t* req, int status) {
    if (req == NULL || status != 0) {
        rerror("client callback, req = %p, status: %d", req, status);
        rgoto(1);
    }

    int ret_code = 0;
    rsocket_ctx_uv_t* ctx = (rsocket_ctx_uv_t*)(req->data);
    ripc_data_source_t* ds_client = (ripc_data_source_t*)ctx->stream->data;
    ds_client->stream = req->handle;
    ds_client->read_cache = NULL;
    rbuffer_init(ds_client->read_cache, read_cache_size);
    ds_client->write_buff = NULL;
    rbuffer_init(ds_client->write_buff, write_buff_size);

	ctx->stream_state = ripc_state_ready;
    if (ctx->stream_type == ripc_type_tcp) {
        ret_code = uv_read_start((uv_stream_t*)(ctx->stream), read_alloc_static, after_read);
        if (ret_code != 0) {
            rerror("uv_read_start error, %d.", ret_code);
            rgoto(1);
        }
    }

    rinfo("connect callback, status: %d", status);

exit1:
    if (req != NULL) {
        rdata_free(uv_connect_t, req);
    }
}

static void client_connect(rsocket_ctx_uv_t* rsocket_ctx) {
    uv_connect_t* connect_req = rdata_new(uv_connect_t);

    struct sockaddr_in addr;
    int ret_code;

    rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    char* ip = cfg->ip;
    int port = cfg->port;
    ret_code = uv_ip4_addr(ip, port, &addr);
    if (ret_code != 0) {
        rerror("uv_ip4_addr error, code: %d", ret_code);
        rgoto(1);
    }

    connect_req->data = rsocket_ctx;
    ret_code = uv_tcp_connect(connect_req, (uv_tcp_t*)rsocket_ctx->stream, (const struct sockaddr*) &addr, connect_cb);
    if (ret_code != 0) {
        rerror("error on connecting, code: %d", ret_code);
        rgoto(1);
    }

    rinfo("client connecting to {%s:%d}..", ip, port);
    
exit1:
    if (ret_code != 0) {
        rdata_free(uv_connect_t, connect_req);
    }
}


static int ripc_init(void* ctx, const void* cfg_data) {
    rinfo("socket client init.");

    int ret_code;
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;

    ret_code = uv_tcp_init(rsocket_ctx->loop, (uv_tcp_t*)rsocket_ctx->stream);
    if (ret_code != 0) {
        rerror("error on init loop.");
        return ret_code;
    }

    //uv_pipe_init(loop, &queue, 1 /* ipc */);
    //uv_pipe_open(&queue, 0);

    return rcode_ok;
}

static int ripc_uninit(void* ctx) {
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;

    rinfo("socket client uninit.");

    rsocket_ctx->stream_state = ripc_state_uninit;

    return rcode_ok;
}

static int ripc_open(void* ctx) {
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;

    rinfo("socket client open.");

    client_connect(rsocket_ctx);

    return rcode_ok;
}

static int ripc_close(void* ctx) {
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;
    ripc_data_source_t* datasource = (ripc_data_source_t*)((uv_tcp_t*)(rsocket_ctx->stream))->data;

    rinfo("socket client close.");

    if (rsocket_ctx->stream_state == ripc_state_closed) {
        return rcode_ok;
    }

    rsocket_ctx->stream_state = ripc_state_closed;

    uv_stop(rsocket_ctx->loop);

    return rcode_ok;
}

static int ripc_start(void* ctx) {
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;

    rinfo("socket client start.");

    return uv_run(rsocket_ctx->loop, UV_RUN_DEFAULT);
}

static int ripc_stop(void* ctx) {
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;

    rinfo("socket client stop.");
    
    uv_close((uv_handle_t*)rsocket_ctx->stream, on_close);

    return rcode_ok;
}

const ripc_entry_t rsocket_uv_c = {
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

#undef local_write_req_t

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif