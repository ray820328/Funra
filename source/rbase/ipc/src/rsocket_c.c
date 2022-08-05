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


static uv_loop_t *loop;
//static uv_pipe_t queue;
static uv_tcp_t client;

typedef struct rsocket_session_uv_s {
    rsocket_session_fields;

    uv_tcp_t tcp;
    uv_connect_t connect_req;
    uv_shutdown_t shutdown_req;
} rsocket_session_uv_t;

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

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void after_write(local_write_req_t* req, int status) {
    rinfo("write finished: %d\n", status);
    local_write_req_t* wr;

    /* Free the read/write buffer and the request */
    wr = (local_write_req_t*)req;
    rstr_free(wr->buf.base);
    rfree_data(local_write_req_t, wr);

    if (status == 0) {
        return;
    }

    rerror("uv_write error: %s - %s\n", uv_err_name(status), uv_strerror(status));
}

static void send_data(void* ctx, ripc_data_default_t* data) {
    int ret_code = 0;
    local_write_req_t *wr;

    wr = (local_write_req_t*)rnew_data(local_write_req_t);
    wr->buf = uv_buf_init(data->data, data->len);

    ret_code = uv_write(&wr->req, (uv_stream_t*)&client, &wr->buf, 1, after_write);
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
        return;
    }

    rinfo("client callback, status: %d\n", status);

    free(req);
}

static void client_connect() {
    uv_connect_t* connect_req = malloc(sizeof *connect_req);

    struct sockaddr_in addr;
    int ret_code;

    char* ip = "127.0.0.1";
    int port = 23000;
    if (uv_ip4_addr(ip, port, &addr) != 0) {
        rerror("error on init ip-addr.\n");
        return;
    }

    ret_code = uv_tcp_connect(connect_req, &client, (const struct sockaddr*) &addr, connect_cb);
    if (ret_code != 0) {
        rerror("error on connecting.\n");
        return;
    }

    rinfo("client connect to {%s:%d} success.\n", ip, port);
}


static int ripc_init(void* ctx, const void* cfg_data) {
    rinfo("socket client init.\n");

    int ret_code;

    loop = uv_default_loop();

    ret_code = uv_tcp_init(loop, &client);
    if (ret_code != 0) {
        rerror("error on init loop.\n");
        return;
    }

    //uv_pipe_init(loop, &queue, 1 /* ipc */);
    //uv_pipe_open(&queue, 0);

    return rcode_ok;
}

static int ripc_uninit(void* ctx) {
    rinfo("socket client uninit.\n");

    uv_stop(loop);
    return rcode_ok;
}

static int ripc_open(void* ctx) {
    rinfo("socket client open.\n");

    client_connect();

    return uv_run(loop, UV_RUN_DEFAULT);
}

static int ripc_close(void* ctx) {
    rinfo("socket client close.\n");

    uv_close((uv_handle_t*)&client, close_cb);

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
