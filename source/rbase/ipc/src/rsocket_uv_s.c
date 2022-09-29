/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "3rd/libuv/1.42.0/src/uv-common.h"

#include "rstring.h"
#include "rlog.h"
#include "rdict.h"
#include "rsocket_uv_s.h"
#include "rcodec_default.h"

typedef struct {
    ripc_data_source_t* ds;
    int write_size;
} local_write_req_t;


static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

static void send_data(ripc_data_source_t* ds, void* data);
static void after_read(uv_stream_t*, ssize_t nread, const uv_buf_t* buf);
static void on_session_close(uv_handle_t* peer);
static void on_server_close(uv_handle_t* handle);
static void on_connection(uv_stream_t*, int status);

static void read_alloc_dynamic(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    rinfo("new buff, size: %d", suggested_size);
    buf->base = rdata_new_size(suggested_size);
    buf->len = suggested_size;
}
/** 强制要求使用buffer/cache 编码解码 **/
static void read_alloc_static(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    /* up to 16 datagrams at once */
    //static char slab[16 * 1024];
    //todo Ray 多线程收发会有问题
    ripc_data_source_t* datasource = (ripc_data_source_t*)(handle->data);
    buf->base = rbuffer_write_start_dest(datasource->read_cache);
    buf->len = rbuffer_left(datasource->read_cache);
}

//static void send_data(uv_stream_t* handle, ripc_data_default_t* data) {
//    int ret_code = 0;
//    local_write_req_t *wr;
//
//    rdebug("send_data: %d", data->len);
//
//    wr = rdata_new(local_write_req_t);
//    wr->buf = uv_buf_init(data->data, data->len);
//
//    ret_code = uv_write(&wr->req, handle, &wr->buf, 1, after_write);
//    if (ret_code != 0) {
//        rerror("uv_write failed. code: %d", ret_code);
//        rgoto(1);
//    }
//
//exit1:
//    if (wr != NULL) {
//        //rdata_free(local_write_req_t, wr);//释放在after_write里面
//    }
//}

static void on_connection(uv_stream_t* server, int status) {
    rinfo("on_connection accept, code: %d", status);

    ripc_data_source_t* datasource = (ripc_data_source_t*)(server->data);
    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)(datasource->ctx);
    ripc_data_source_t* ds_client = NULL;

    uv_stream_t* stream = NULL;
    int ret_code;

    if (status != 0) {
        rerror("accept error: %d, %s", status, uv_err_name(status));
        rgoto(1);
    }

    ds_client = rdata_new(ripc_data_source_t);

    switch (rsocket_ctx->stream_type) {
    case ripc_type_tcp:
        stream = rdata_new(uv_tcp_t);
        if (stream == NULL) {
            rerror("stream oom.");
            rgoto(1);
        }

        ret_code = uv_tcp_init(rsocket_ctx->loop, (uv_tcp_t*)stream);
        if (ret_code != 0) {
            rerror("streamuv_tcp_init error, %d.", ret_code);
            rgoto(1);
        }

        break;

    case ripc_type_pipe:
        stream = rdata_new(uv_pipe_t);
        ret_code = uv_pipe_init(rsocket_ctx->loop, (uv_pipe_t*)stream, 0);
        if (ret_code != 0) {
            rerror("error on uv_pipe_init: %d", ret_code);
            rgoto(1);
        }
        break;

    default:
        rerror("error of server_type: %d", rsocket_ctx->stream_type);
        rgoto(1);
    }

    ds_client->ds_type = ripc_data_source_type_session;
    ds_client->ds_id = ++ rsocket_ctx->sid_cur;
    ds_client->read_cache = NULL;
    rbuffer_init(ds_client->read_cache, read_cache_size);
    ds_client->write_buff = NULL;
    rbuffer_init(ds_client->write_buff, write_buff_size);
    ds_client->ctx = rsocket_ctx;
    ds_client->stream = stream;

    /* client关联到ds对象，ds->ctx = context*/
    stream->data = ds_client;

    rdict_add(rsocket_ctx->map_clients, (void*)ds_client->ds_id, ds_client);

    if (rsocket_ctx->stream_type == ripc_type_tcp) {
        ret_code = uv_accept(server, stream);
        if (ret_code != 0) {
            rerror("uv_accept error, %d.", ret_code);
            rgoto(1);
        }

        ret_code = uv_read_start(stream, read_alloc_static, after_read);
        if (ret_code != 0) {
            rerror("uv_read_start error, %d.", ret_code);
            rgoto(1);
        }

        rinfo("accept finished.");
    }

    rinfo("on_connection accept success, peer = %p", ds_client->stream);
exit0:

exit1:
    if (ret_code != 0) {
        if (stream != NULL) {
            switch (rsocket_ctx->stream_type) {
            case ripc_type_tcp:
                rdata_free(uv_tcp_t, stream);
                break;
            case ripc_type_pipe:
                rdata_free(uv_pipe_t, stream);
                break;
            }
        }

        if (datasource != NULL) {
            rdata_free(ripc_data_source_t, stream);
        }
    }
}

static void after_shutdown_client(uv_shutdown_t* req, int status) {
    if (status == 0) {
        rinfo("after_shutdown_client success.");
    }
    else {
        rerror("error after_shutdown_client: %d", status);
    }

    uv_close((uv_handle_t*)req->handle, on_session_close);

    rdata_free(uv_shutdown_t, req);
}

static void after_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    int ret_code = 0;

    ripc_data_source_t* datasource = (ripc_data_source_t*)(handle->data);
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)(datasource->ctx);
    ripc_data_raw_t data_raw;//直接在栈上

    local_write_req_t *wr;
    uv_shutdown_t* sreq;
    int shutdown = 0;

    rdebug("after read: %d", nread);

    if (nread < 0) {
        //rstr_free(((uv_buf_t*)buf)->base);

        /* Error or EOF */
        if (nread != UV_EOF) {//异常，未主动关闭等
            rerror("error on read: %d", nread);
            uv_close(handle, on_session_close);
            return;
        }

        if (uv_is_writable(handle)) {
            sreq = rdata_new(uv_shutdown_t);
            ret_code = uv_shutdown(sreq, handle, after_shutdown_client);
            if (ret_code != 0) {
                rerror("error on shutdown, code: %d", ret_code);
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

    if (rsocket_ctx->in_handler) {
        //if (datasource->read_type == append_new) { //每一次都是new一个空间去读
            //data_raw.len = nread;
            //data_raw.data = buf->base;
        //} else if (append_cache) {//

        data_raw.len = nread;
        //data_raw.data = datasource->read_cache;
        //}

        ret_code = rsocket_ctx->in_handler->process(rsocket_ctx->in_handler, datasource, &data_raw);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return;
        }

    }
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
    //free_write_req(req);

    rdebug("writing buff left bytes: %d - %d", rbuffer_write_start_pos(datasource->write_buff), rbuffer_size(datasource->write_buff));
}
static void send_data(ripc_data_source_t* ds, void* data) {
    int ret_code = 0;
    local_write_req_t* wr;
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ds->ctx;
    uv_write_t* req = NULL;

    //if (datasource == NULL || datasource->stream)

    if (rsocket_ctx->out_handler) {
        ret_code = rsocket_ctx->out_handler->process(rsocket_ctx->out_handler, ds, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return;
        }
    }

    uv_buf_t buf;
    //buf = uv_buf_init(data->data, data->len);//结构体内容复制
    buf.base = rbuffer_read_start_dest(ds->write_buff);
    buf.len = rbuffer_size(ds->write_buff);

    wr = rdata_new(local_write_req_t);
    wr->ds = ds;
    wr->write_size = buf.len;

    req = rdata_new(uv_write_t);
    req->data = wr;

    //unix间接调用uv_write2 malloc了buf放到req里再cb，但是win里tcp实现是直接WSASend！操蛋
    ret_code = uv_write(req, (uv_stream_t*)(ds->stream), &buf, 1, after_write);
    rdebug("end server send_data, req: %p, buf: %p", req, &buf);
    //rdebug("send_data, len: %d, dest_len: %p, data_buf: %p", data->len, data->data, buf.base);

    if (ret_code != 0) {
        rerror("uv_write failed. code: %d", ret_code);
        return;
    }
}

static void on_session_close(uv_handle_t* peer) {
    //todo Ray 暂时仅tcp
    ripc_data_source_t* datasource = (ripc_data_source_t*)(peer->data);
    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)(datasource->ctx);
    rinfo("on session close, id = %"PRIu64", peer = %p", datasource->ds_id, peer);

    if (datasource->ds_type == ripc_data_source_type_session && rdict_exists(rsocket_ctx->map_clients, (const void*)datasource->ds_id)) {
        rdict_remove(rsocket_ctx->map_clients, (const void*)datasource->ds_id);

        rbuffer_release(datasource->read_cache);
        rbuffer_release(datasource->write_buff);
        rdata_free(uv_tcp_t, datasource->stream);
        rdata_free(ripc_data_source_t, datasource);
    }
    else if (datasource->ds_type == ripc_data_source_type_server) {
        rerror("on_session_close error, cant release server.");
    }
}

static void on_server_close(uv_handle_t* server) {//不能再close callback里调用uv方法
    rinfo("server close success. server = %p", server);
}

//static void on_shutdown(uv_shutdown_t* req, int status) {
//    if (status == 0) {
//        rinfo("shutdown success.");
//    }
//    else {
//        rerror("error on shutdown: %d", status);
//    }
//
//    rdata_free(uv_shutdown_t, req);
//}
//static uv_udp_send_t* send_alloc(void) {
//    uv_udp_send_t* req = udp_data_list_free;
//    if (req != NULL) {
//        udp_data_list_free = req->data;
//    }
//    else {
//        req = malloc(sizeof(*req));
//    }
//
//    return req;
//}
//static void on_send(uv_udp_send_t* req, int status) {
//    if (req == NULL || status != 0) {
//        rerror("on_send failed. req = %p, status = %d", req, status);
//        return;
//    }
//    rinfo("on send: %d", status);
//
//    req->data = udp_data_list_free;
//    udp_data_list_free = req;
//}
//static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf, const struct sockaddr* addr, unsigned flags) {
//    rinfo("on_recv: %d", nread);
//    uv_buf_t sndbuf;
//    uv_udp_send_t* req;
//
//    if (nread == 0) {
//        /* Everything OK, but nothing read. */
//        return;
//    }
//
//    if (nread <= 0 || addr->sa_family != AF_INET) {
//        rerror("on_recv error. nread = %d, addr->sa_family = %u", nread, addr->sa_family);
//        return;
//    }
//
//    req = send_alloc();
//    sndbuf = uv_buf_init(rcvbuf->base, nread);
//    int send_len = uv_udp_send(req, handle, &sndbuf, 1, addr, on_send);
//    if (send_len < 0) {
//        rerror("on_recv error. nread = %d, send_len = %d", nread, send_len);
//        return;
//    }
//}

static int start_server_tcp4(rsocket_ctx_uv_t* rsocket_ctx) {
    int ret_code;
    rinfo("socket server open.");

    rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    char* ip = cfg->ip;
    int port = cfg->port;
    struct sockaddr_in bind_addr;
    ret_code = uv_ip4_addr(ip, port, &bind_addr);
    if (ret_code != 0) {
        rerror("uv_ip4_addr error, code: %d", ret_code);
        return 1;
    }
    
    uv_stream_set_blocking((uv_handle_t*)rsocket_ctx->stream, 0);//Windows for uv_pipe_t handles. On UNIX platforms, all uv_stream_t handles
    ret_code = uv_tcp_init(rsocket_ctx->loop, (uv_handle_t*)rsocket_ctx->stream);
    if (ret_code != 0) {
        rerror("socket creation error, code: %d", ret_code);
        return 1;
    }
    //uv_tcp_nodelay(&server_tcp, 1);
    //After delay has been reached, 10 successive probes, each spaced 1 second from the previous one, will still happen. 
    //If the connection is still lost at the end of this procedure, then the handle is destroyed 
    //with a UV_ETIMEDOUT error passed to the corresponding callback.
    //uv_tcp_keepalive(&server_tcp, 1, 5);//delay is the initial delay in seconds
    //This setting is used to tune a TCP server for the desired performance.
    //Having simultaneous accepts can significantly improve the rate of accepting connections(which is why it is enabled by default) 
    //but may lead to uneven load distribution in multi - process setups.
    //uv_tcp_simultaneous_accepts(&server_tcp, 0);//simultaneous asynchronous accept requests that are queued by the operating system when listening for new TCP connections.
    //uv_tcp_close_reset(&server, uv_close_cb close_cb);


    ret_code = uv_tcp_bind((uv_tcp_t*)rsocket_ctx->stream, (const struct sockaddr*) &bind_addr, 0);
    if (ret_code) {
        rerror("bind error at %s:%d", ip, port);
        return 1;
    }

    ret_code = uv_listen((uv_stream_t*)rsocket_ctx->stream, 128, on_connection);
    if (ret_code) {
        rerror("listen error %s:%d, %s", ip, port, uv_err_name(ret_code));
        return 1;
    }
    rinfo("%p listen at %s:%d success.", rsocket_ctx->stream, ip, port);

    return rcode_ok;
}


//static int start_server_tcp6(int port) {
//    struct sockaddr_in6 addr6;
//    int r;
//
//    rassert(0 == uv_ip6_addr("::1", port, &addr6));
//
//    server = (uv_handle_t*)&tcpServer;
//    server_type = ripc_type_tcp;
//
//    r = uv_tcp_init(loop, &tcpServer);
//    if (r) {
//        fprintf(stderr, "Socket creation error\n");
//        return 1;
//    }
//
//    /* IPv6 is optional as not all platforms support it */
//    r = uv_tcp_bind(&tcpServer, (const struct sockaddr*) &addr6, 0);
//    if (r) {
//        /* show message but return OK */
//        fprintf(stderr, "IPv6 not supported\n");
//        return 0;
//    }
//
//    r = uv_listen((uv_stream_t*)&tcpServer, SOMAXCONN, on_connection);
//    if (r) {
//        fprintf(stderr, "Listen error\n");
//        return 1;
//    }
//
//    return 0;
//}
//
//static int start_server_udp6(int port) {
//    struct sockaddr_in addr;
//    int r;
//
//    rassert(0 == uv_ip4_addr("127.0.0.1", port, &addr));
//    server = (uv_handle_t*)&server_udp;
//    server_type = ripc_type_udp;
//
//    r = uv_udp_init(loop, &server_udp);
//    if (r) {
//        fprintf(stderr, "uv_udp_init: %s\n", uv_strerror(r));
//        return 1;
//    }
//
//    r = uv_udp_bind(&server_udp, (const struct sockaddr*) &addr, 0);
//    if (r) {
//        fprintf(stderr, "uv_udp_bind: %s\n", uv_strerror(r));
//        return 1;
//    }
//
//    r = uv_udp_recv_start(&server_udp, slab_alloc, on_recv);
//    if (r) {
//        fprintf(stderr, "uv_udp_recv_start: %s\n", uv_strerror(r));
//        return 1;
//    }
//
//    return 0;
//}
//static int start_server_pipe(char* pipeName) {
//    int r;
//
//#ifndef _WIN32
//    {
//        uv_fs_t req;
//        uv_fs_unlink(NULL, &req, pipeName, NULL);
//        uv_fs_req_cleanup(&req);
//    }
//#endif
//
//    server = (uv_handle_t*)&server_pipe;
//    server_type = ripc_type_pipe;
//
//    r = uv_pipe_init(loop, &server_pipe, 0);
//    if (r) {
//        fprintf(stderr, "uv_pipe_init: %s\n", uv_strerror(r));
//        return 1;
//    }
//
//    r = uv_pipe_bind(&server_pipe, pipeName);
//    if (r) {
//        fprintf(stderr, "uv_pipe_bind: %s\n", uv_strerror(r));
//        return 1;
//    }
//
//    r = uv_listen((uv_stream_t*)&server_pipe, SOMAXCONN, on_connection);
//    if (r) {
//        fprintf(stderr, "uv_pipe_listen: %s\n", uv_strerror(r));
//        return 1;
//    }
//
//    return 0;
//}


static int ripc_init(void* ctx, const void* cfg_data) {
    rinfo("socket server init.");

    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)ctx;

    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, 2000, 0);
    rassert(dict_ins != NULL, "");
	rsocket_ctx->map_clients = dict_ins;

    return rcode_ok;
}

static int ripc_uninit(void* ctx) {
    rinfo("socket server uninit.");

    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)ctx;

    if (rsocket_ctx->map_clients != NULL) {
        rdict_free(rsocket_ctx->map_clients);
    }

    return rcode_ok;
}

static int ripc_open(void* ctx) {
    rinfo("socket server open.");

    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;
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
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;

    QUEUE* q;
    uv_handle_t* h;
    if (uv__has_active_reqs(rsocket_ctx->loop)) {
        rerror("has reqs");
        return UV_EBUSY;
    }

    QUEUE_FOREACH(q, &(rsocket_ctx->loop->handle_queue)) {
        h = QUEUE_DATA(q, uv_handle_t, handle_queue);
        if (!(h->flags & UV_HANDLE_INTERNAL)) {
            rerror("has handler, %p", h);
            return UV_EBUSY;
        }
    }

    ret_code = uv_loop_close(rsocket_ctx->loop);
    if (ret_code != rcode_ok) {
        rerror("error on close loop, code: %d", ret_code);
        return ret_code;
    }

    uv_stop(rsocket_ctx->loop);

    rinfo("socket server close success.");

    return rcode_ok;
}

static int ripc_start(void* ctx) {
    rinfo("socket server start.");

    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;
    int ret_code = uv_run(rsocket_ctx->loop, UV_RUN_DEFAULT);
    if (ret_code != rcode_ok) {
        rerror("error on run loop, code: %d", ret_code);
        return ret_code;
    }
    rinfo("end, socket server start.");

    return rcode_ok;
}

static int ripc_stop(void* ctx) {
	rinfo("socket server stop.");

    //直接调用只能在uv线程内，跨线程用asnc接口

    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)ctx;
    rsocket_ctx->stream_state = ripc_state_closed;

    if (rsocket_ctx->map_clients && rdict_size(rsocket_ctx->map_clients) > 0) {
        ripc_data_source_t* ds_client = NULL;
        rdict_iterator_t it = rdict_it(rsocket_ctx->map_clients);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            ds_client = (ripc_data_source_t*)(de->value.ptr);
            uv_close((uv_stream_t*)ds_client->stream, on_session_close);
        }
        rdict_clear(rsocket_ctx->map_clients);
    }

    //uv_shutdown(sreq, rsocket_ctx->stream, after_shutdown_server);
    uv_close(rsocket_ctx->stream, on_server_close);

    return rcode_ok;
}

const ripc_entry_t rsocket_uv_s = {
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