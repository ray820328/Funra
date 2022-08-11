/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "uv.h"

#include "rstring.h"
#include "rlog.h"
#include "rsocket_c.h"
#include "rtools.h"


//static uv_pipe_t queue;

//typedef struct rsocket_session_uv_s {
//    rsocket_session_fields;
//
//    uv_tcp_t tcp;
//    uv_connect_t connect_req;
//    uv_shutdown_t shutdown_req;
//} rsocket_session_uv_t;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} local_write_req_t;

static void set_nonblocking(uv_os_sock_t sock) {
    int r;
#ifdef _WIN32
    unsigned long on = 1;
    r = ioctlsocket(sock, FIONBIO, &on);
    ASSERT(r == 0);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    ASSERT(flags >= 0);
    r = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    ASSERT(r >= 0);
#endif
}

static void free_write_req(uv_write_t *req) {
    local_write_req_t *wr = (local_write_req_t*)req;
    free(wr->buf.base);
    free(wr);
}

static void after_write(local_write_req_t* req, int status) {
    rinfo("write finished: %d\n", status);
    local_write_req_t* wr;

    /* Free the read/write buffer and the request */
    wr = (local_write_req_t*)req;

    rdebug("after_write, wr: %p, req: %p, buf: %p\n", wr, &wr->req, &wr->buf);
    /* rstr_free(wr->buf.base); */
    rfree_data(local_write_req_t, wr);

    if (status == 0) {
        return;
    }

    rerror("uv_write error: %s - %s\n", uv_err_name(status), uv_strerror(status));
}

static void send_data(void* ctx, ripc_data_default_t* data) {
    int ret_code = 0;
    local_write_req_t *wr;
    rsocket_session_uv_t* rsocket_ctx = (rsocket_session_uv_t*)ctx;
    ripc_data_source_t* datasource = ((uv_tcp_t*)(rsocket_ctx->peer))->data;

    if (rsocket_ctx->out_handler) {
        ret_code = rsocket_ctx->in_handler->process(rsocket_ctx->in_handler, datasource, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d\n", ret_code);
            return;
        }
    }

    wr = (local_write_req_t*)rnew_data(local_write_req_t);
    //wr->buf = uv_buf_init(data->data, data->len);//结构体内容复制

    wr->buf.base = rbuffer_read_start_dest(datasource->write_buff);
    wr->buf.len = rbuffer_size(datasource->write_buff);

    ret_code = uv_write(&wr->req, (uv_stream_t*)rsocket_ctx->peer, &wr->buf, 1, after_write);
    rdebug("send_data, wr: %p, req: %p, buf: %p\n", wr, &wr->req, &wr->buf);
    rdebug("send_data, len: %d, data_org: %p, data_buf: %p\n", data->len, data->data, wr->buf.base);

    if (ret_code != 0) {
        rerror("uv_write failed. code: %d\n", ret_code);
        return;
    }
}

static void on_write(uv_write_t *req, int status) {
    rinfo("on_write: %d\n", status);

    if (status) {
        rerror("write error, code = %d, err = %s\n", status, uv_err_name(status));
    }
    free_write_req(req);
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    rinfo("on_read: %d\n", nread);

    if (nread > 0) {
        rdebug("on_read, content: %s\n", buf->base);
        rtools_wait_mills(5000);

        local_write_req_t *req = (local_write_req_t*)malloc(sizeof(local_write_req_t));
        req->buf = uv_buf_init(buf->base, nread);
        uv_write((uv_write_t*)req, client, &req->buf, 1, on_write);
        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF) {
            rerror("read error %s\n", uv_err_name(nread));
        }
        uv_close((uv_handle_t*)client, NULL);
    }

    free(buf->base);
}

static void close_cb(uv_handle_t* handle) {
    if (handle == NULL) {
        rerror("client close error.\n");
        return;
    }

    rinfo("client close callback success.\n");
}

static void connect_cb(uv_connect_t* req, int status) {
    if (req == NULL || status != 0) {
        rerror("client callback, req = %p, status: %d\n", req, status);
        rgoto(1);
    }

    rinfo("client callback, status: %d\n", status);

exit1:
    if (req != NULL) {
        rfree_data(uv_connect_t, req);
    }
}

static void client_connect(rsocket_session_uv_t* rsocket_ctx) {
    uv_connect_t* connect_req = rnew_data(uv_connect_t);

    struct sockaddr_in addr;
    int ret_code;

    rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    char* ip = cfg->ip;
    int port = cfg->port;
    ret_code = uv_ip4_addr(ip, port, &addr);
    if (ret_code != 0) {
        rerror("uv_ip4_addr error, code: %d\n", ret_code);
        rgoto(1);
    }

    ret_code = uv_tcp_connect(connect_req, rsocket_ctx->peer, (const struct sockaddr*) &addr, connect_cb);
    if (ret_code != 0) {
        rerror("error on connecting, code: %d\n", ret_code);
        rgoto(1);
    }

    rinfo("client connect to {%s:%d} success.\n", ip, port);
    
exit1:
    if (ret_code != 0) {
        rfree_data(uv_connect_t, connect_req);
    }
}


static int ripc_init(void* ctx, const void* cfg_data) {
    rinfo("socket client init.\n");

    int ret_code;
    rsocket_session_uv_t* rsocket_ctx = (rsocket_session_uv_t*)ctx;

    ret_code = uv_tcp_init(rsocket_ctx->loop, rsocket_ctx->peer);
    if (ret_code != 0) {
        rerror("error on init loop.\n");
        return;
    }

    //uv_pipe_init(loop, &queue, 1 /* ipc */);
    //uv_pipe_open(&queue, 0);

    return rcode_ok;
}

static int ripc_uninit(void* ctx) {
    rsocket_session_uv_t* rsocket_ctx = (rsocket_session_uv_t*)ctx;

    rinfo("socket client uninit.\n");

    uv_stop(rsocket_ctx->loop);

    rsocket_ctx->peer_state = 0;

    return rcode_ok;
}

static int ripc_open(void* ctx) {
    rsocket_session_uv_t* rsocket_ctx = (rsocket_session_uv_t*)ctx;

    rinfo("socket client open.\n");

    client_connect(rsocket_ctx);

    rsocket_ctx->peer_state = 1;

    return uv_run(rsocket_ctx->loop, UV_RUN_DEFAULT);
}

static int ripc_close(void* ctx) {
    rsocket_session_uv_t* rsocket_ctx = (rsocket_session_uv_t*)ctx;

    rinfo("socket client close.\n");

    uv_close((uv_handle_t*)rsocket_ctx->peer, close_cb);

    return rcode_ok;
}

static int ripc_start(void* ctx) {

    return rcode_ok;
}

static int ripc_stop(void* ctx) {

    return rcode_ok;
}

const ripc_entry_t rsocket_c = {
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
