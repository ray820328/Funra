/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "uv.h"

#include "rinterface.h"
#include "rstring.h"

#include "ripc.h"
#include "rsocket.h"
#include "rlog.h"

static int init_tcp_server();
static void on_new_conn(uv_stream_t *server, int status);
static void on_file_open(uv_fs_t *req);
static void on_file_read(uv_fs_t *req);

int rsocket_init(const void* cfg_data) {

    init_tcp_server();

    return rcode_ok;
}

int rsocket_uninit() {

    return rcode_ok;
}

ripc_item* get_ipc_item(const char* key) {

    return NULL;
}


const static int backlog = 128;
const static int buffer_size = 1024;
static uv_fs_t open_req;
static uv_fs_t read_req;
static uv_tcp_t *client;

static int init_tcp_server() {
    uv_loop_t* loop = uv_default_loop();
    uv_tcp_t server;

    uv_tcp_init(loop, &server);

    struct sockaddr_in addr_ip4; //struct sockaddr_in6
    uv_ip4_addr("0.0.0.0", 7000, &addr_ip4);
    //if (uv_ip6_addr("::", 0, &uv_addr_ip6_any_)) {
    //    abort();
    //}
    uv_tcp_bind(&server, (const struct sockaddr*)&addr_ip4, 0);

    // listen
    int r = uv_listen((uv_stream_t*)&server, backlog, on_new_conn);
    if (r) {
        rerror("error init_tcp_server\n");
        return 1;
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}

static uv_buf_t alloc_buffer(uv_handle_t *handle, size_t suggested_size) {
    return uv_buf_init((char*)malloc(suggested_size), suggested_size);
}

static void on_file_open(uv_fs_t *req) {
    if (req->result == -1) {
        rerror("error on_file_read");
        uv_close((uv_handle_t*)client, NULL);
        return;
    }

    char *buffer = (char *)malloc(sizeof(char) * buffer_size);//todo Ray 没有释放

    int offset = -1;
    read_req.data = (void*)buffer;
    uv_fs_read(uv_default_loop(), &read_req, req->result, buffer, sizeof(char) * buffer_size, offset, on_file_read);
    uv_fs_req_cleanup(req);
}

static void on_client_write(uv_write_t *req, int status) {
    if (status == -1) {
        rerror("error on_client_write");
        uv_close((uv_handle_t*)client, NULL);
        return;
    }

    free(req);
    char *buffer = (char*)req->data;
    free(buffer);

    uv_close((uv_handle_t*)client, NULL);
}

static void on_file_read(uv_fs_t *req) {
    if (req->result < 0) {
        rerror("error on_file_read");
        uv_close((uv_handle_t*)client, NULL);
    }
    else if (req->result == 0) { //
        uv_fs_t close_req;
        uv_fs_close(uv_default_loop(), &close_req, open_req.result, NULL);
        uv_close((uv_handle_t*)client, NULL);
    }
    else { // 
        uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));

        char *message = (char*)req->data;

        uv_buf_t buf = uv_buf_init(message, sizeof(message));
        buf.len = req->result;
        buf.base = message;
        int buf_count = 1;

        write_req->data = (void*)message;

        uv_write(write_req, (uv_stream_t*)client, &buf, buf_count, on_client_write);
    }

    uv_fs_req_cleanup(req);
}

static void on_client_read(uv_stream_t *_client, ssize_t nread, uv_buf_t buf) {
    if (nread == -1) {
        rerror("error on_client_read");
        uv_close((uv_handle_t*)client, NULL);
        return;
    }

    char *filename = buf.base;
    int mode = 0;

    uv_fs_open(uv_default_loop(), &open_req, filename, O_RDONLY, mode, on_file_open);
}

static void on_new_conn(uv_stream_t *server, int status) {
    if (status == -1) {
        rerror("error on_new_connection");
        uv_close((uv_handle_t*)client, NULL);
        return;
    }

    client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), client);

    // accept
    int result = uv_accept(server, (uv_stream_t*)client);

    if (result == 0) { // success
        uv_read_start((uv_stream_t*)client, alloc_buffer, on_client_read);
    }
    else { // error
        uv_close((uv_handle_t*)client, NULL);
    }
}

