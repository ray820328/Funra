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
#include "rsocket_s.h"

static uv_loop_t *loop;
static void on_new_connection(uv_stream_t *server, int status);
static void setup_workers();

struct child_worker {
    uv_process_t req;
    uv_process_options_t options;
    uv_pipe_t pipe;
} *workers;

int round_robin_counter;
int child_worker_count;
int backlog = 128;

uv_buf_t dummy_buf;
char worker_path[1024];

//typedef struct buf_s {
//    uv_buf_t uv_buf_t;
//    struct buf_s* next;
//} buf_t;
//static buf_t* buf_freelist = NULL;
//static void buf_alloc(uv_handle_t* tcp, size_t size, uv_buf_t* buf) {
//    buf_t* ab;
//
//    ab = buf_freelist;
//    if (ab != NULL)
//        buf_freelist = ab->next;
//    else {
//        ab = malloc(size + sizeof(*ab));
//        ab->uv_buf_t.len = size;
//        ab->uv_buf_t.base = (char*)(ab + 1);
//    }
//
//    *buf = ab->uv_buf_t;
//}
//static void buf_free(const uv_buf_t* buf) {
//    buf_t* ab = (buf_t*)buf->base - 1;
//    ab->next = buf_freelist;
//    buf_freelist = ab;
//}

static void close_process_handle(uv_process_t *req, int64_t exit_status, int term_signal) {
    rinfo("Process exited with status %" PRId64 ", signal %d\n", exit_status, term_signal);
    uv_close((uv_handle_t*)req, NULL);
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void on_new_connection_pipe(uv_stream_t *server, int status) {
    if (status == -1) {
        rerror("accept error, code = %d\n", status);
        return;
    }
    rinfo("server on accept success.\n");

    uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        uv_write_t *write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
        dummy_buf = uv_buf_init("a", 1);
        struct child_worker *worker = &workers[round_robin_counter];
        uv_write2(write_req, (uv_stream_t*)&worker->pipe, &dummy_buf, 1, (uv_stream_t*)client, NULL);
        round_robin_counter = (round_robin_counter + 1) % child_worker_count;
    }
    else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

//fork多个
static void setup_workers_pipe() {
    size_t path_size = 1024;
    uv_exepath(worker_path, &path_size);
    rstr_cat(worker_path, "workers", 0);
    rinfo("Worker path: %s\n", worker_path);

    char* args[2];
    args[0] = worker_path;
    args[1] = NULL;

    round_robin_counter = 0;

    // launch same number of workers as number of CPUs
    uv_cpu_info_t *info;
    int cpu_count;
    uv_cpu_info(&info, &cpu_count);
    uv_free_cpu_info(info, cpu_count);

    child_worker_count = cpu_count / 2 + 1;

    workers = calloc(cpu_count, sizeof(struct child_worker));
    while (cpu_count--) {
        struct child_worker *worker = &workers[cpu_count];
        uv_pipe_init(loop, &worker->pipe, 1);

        uv_stdio_container_t child_stdio[3];
        child_stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
        child_stdio[0].data.stream = (uv_stream_t*)&worker->pipe;
        child_stdio[1].flags = UV_IGNORE;
        child_stdio[2].flags = UV_INHERIT_FD;
        child_stdio[2].data.fd = 2;

        worker->options.stdio = child_stdio;
        worker->options.stdio_count = 3;

        worker->options.exit_cb = close_process_handle;
        worker->options.file = args[0];
        worker->options.args = args;

        uv_spawn(loop, &worker->req, &worker->options);
        rinfo("started worker %d\n", worker->req.pid);
    }
}


static int ripc_init(const void* cfg_data) {
    rinfo("socket server init.\n");

    loop = uv_default_loop();

    setup_workers();

    return rcode_ok;
}

static int ripc_uninit() {
    rinfo("socket server uninit.\n");

    return rcode_ok;
}

static int ripc_open() {
    rinfo("socket server open.\n");

    uv_tcp_t server;
    uv_tcp_init(loop, &server);
    uv_tcp_nodelay(&server, 1);
    //After delay has been reached, 10 successive probes, each spaced 1 second from the previous one, will still happen. 
    //If the connection is still lost at the end of this procedure, then the handle is destroyed 
    //with a UV_ETIMEDOUT error passed to the corresponding callback.
    uv_tcp_keepalive(&server, 1, 5);//delay is the initial delay in seconds
    //This setting is used to tune a TCP server for the desired performance.
    //Having simultaneous accepts can significantly improve the rate of accepting connections(which is why it is enabled by default) 
    //but may lead to uneven load distribution in multi - process setups.
    uv_tcp_simultaneous_accepts(&server, 0);//simultaneous asynchronous accept requests that are queued by the operating system when listening for new TCP connections.
    //uv_tcp_close_reset(&server, uv_close_cb close_cb);

    char* ip = "0.0.0.0";
    int port = 23000;
    struct sockaddr_in bind_addr;
    uv_ip4_addr(ip, port, &bind_addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);
    int r;
    if ((r = uv_listen((uv_stream_t*)&server, backlog, on_new_connection))) {
        rerror("Listen error %s\n", uv_err_name(r));
        return 2;
    }
    rinfo("Listen at %s:%d success.\n", ip, port);

    return uv_run(loop, UV_RUN_DEFAULT);
}

static int ripc_close() {
    rinfo("socket server close.\n");

    return rcode_ok;
}

static int ripc_start() {

    return rcode_ok;
}

static int ripc_stop() {

    return rcode_ok;
}

static ripc_item* get_ipc_item(const char* key) {

    return NULL;
}

const ripc_item rsocket_s = {
    NULL,// rdata_handler_t* handler;

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
