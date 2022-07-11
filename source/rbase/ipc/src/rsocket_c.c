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


static uv_loop_t *loop;
static uv_pipe_t queue;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;


static void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*)req;
    free(wr->buf.base);
    free(wr);
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void on_write(uv_write_t *req, int status) {
    if (status) {
        rerror("write error, code = %d, err = %s\n", status, uv_err_name(status));
    }
    free_write_req(req);
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));
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

static void on_connected(uv_stream_t *q, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            rerror("read error %s\n", uv_err_name(nread));
        }
        uv_close((uv_handle_t*)q, NULL);
        return;
    }

    uv_pipe_t *pipe = (uv_pipe_t*)q;
    if (!uv_pipe_pending_count(pipe)) {
        rerror("No pending count\n");
        return;
    }

    uv_handle_type pending = uv_pipe_pending_type(pipe);
    assert(pending == UV_TCP);

    uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(q, (uv_stream_t*)client) == 0) {
        uv_os_fd_t fd;
        uv_fileno((const uv_handle_t*)client, &fd);
        rinfo("worker %d: accepted fd %d\n", getpid(), fd);
        uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);
    }
    else {
        uv_close((uv_handle_t*)client, NULL);
    }
}



static int ripc_init(const void* cfg_data) {
    loop = uv_default_loop();

    uv_pipe_init(loop, &queue, 1 /* ipc */);
    uv_pipe_open(&queue, 0);

    return rcode_ok;
}

static int ripc_uninit() {

    return rcode_ok;
}

static int ripc_open() {

    uv_read_start((uv_stream_t*)&queue, alloc_buffer, on_connected);

    return uv_run(loop, UV_RUN_DEFAULT);
}

static int ripc_close() {

    uv_stop(loop);

    return rcode_ok;
}

static int ripc_start() {

    return rcode_ok;
}

static int ripc_stop() {

    return rcode_ok;
}

const ripc_item rsocket_c = {
    NULL,// rdata_handler* handler;

    ripc_init,// ripc_init_func init;
    ripc_uninit,// ripc_uninit_func uninit;
    ripc_open,// ripc_open_func open;
    ripc_close,// ripc_close_func close;
    ripc_start,// ripc_start_func start;
    ripc_stop,// ripc_stop_func stop;
    NULL,// ripc_send_func send;
    NULL,// ripc_check_func check;
    NULL,// ripc_receive_func receive;
    NULL// ripc_error_func error;
};

