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
static void after_read(uv_stream_t*, ssize_t nread, const uv_buf_t* buf);
static void on_close(uv_handle_t* peer);
static void on_server_close(uv_handle_t* handle);
static void on_connection(uv_stream_t*, int status);

static void read_alloc_dynamic(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    rinfo("new buff, size: %d\n", suggested_size);
    buf->base = rstr_new(suggested_size);
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
//    rdebug("send_data: %d\n", data->len);
//
//    wr = rnew_data(local_write_req_t);
//    wr->buf = uv_buf_init(data->data, data->len);
//
//    ret_code = uv_write(&wr->req, handle, &wr->buf, 1, after_write);
//    if (ret_code != 0) {
//        rerror("uv_write failed. code: %d\n", ret_code);
//        rgoto(1);
//    }
//
//exit1:
//    if (wr != NULL) {
//        //rfree_data(local_write_req_t, wr);//释放在after_write里面
//    }
//}

static void on_connection(uv_stream_t* server, int status) {
    rinfo("on_connection accept, code: %d\n", status);

    ripc_data_source_t* datasource = (ripc_data_source_t*)(server->data);
    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)(datasource->ctx);
    ripc_data_source_t* ds_client = NULL;

    uv_stream_t* stream = NULL;
    int ret_code;

    if (status != 0) {
        rerror("accept error: %d, %s\n", status, uv_err_name(status));
        rgoto(1);
    }

    ds_client = rnew_data(ripc_data_source_t);

    switch (rsocket_ctx->share.stream_type) {
    case ripc_type_tcp:
        stream = rnew_data(uv_tcp_t);
        if (stream == NULL) {
            rerror("stream oom.");
            rgoto(1);
        }

        ret_code = uv_tcp_init(rsocket_ctx->share.loop, (uv_tcp_t*)stream);
        if (ret_code != 0) {
            rerror("streamuv_tcp_init error, %d.\n", ret_code);
            rgoto(1);
        }

        break;

    case ripc_type_pipe:
        stream = rnew_data(uv_pipe_t);
        ret_code = uv_pipe_init(rsocket_ctx->share.loop, (uv_pipe_t*)stream, 0);
        if (ret_code != 0) {
            rerror("error on uv_pipe_init: %d\n", ret_code);
            rgoto(1);
        }
        break;

    default:
        rerror("error of server_type: %d\n", rsocket_ctx->share.stream_type);
        rgoto(1);
    }

    ds_client->ds_type = ripc_data_source_type_client;
    ds_client->ds_id = ++ rsocket_ctx->sid_cur;
    ds_client->read_cache = NULL;
    rbuffer_init(ds_client->read_cache, read_cache_size);
    ds_client->write_buff = NULL;
    rbuffer_init(ds_client->write_buff, write_buff_size);
    ds_client->ctx = rsocket_ctx;
    ds_client->stream = stream;

    /* client关联到ds对象，ds->ctx = context*/
    stream->data = ds_client;

    rdict_add(rsocket_ctx->map_clients, ds_client->ds_id, ds_client);

    if (rsocket_ctx->share.stream_type == ripc_type_tcp) {
        ret_code = uv_accept(server, stream);
        if (ret_code != 0) {
            rerror("uv_accept error, %d.\n", ret_code);
            rgoto(1);
        }

        ret_code = uv_read_start(stream, read_alloc_static, after_read);
        if (ret_code != 0) {
            rerror("uv_read_start error, %d.\n", ret_code);
            rgoto(1);
        }

        rinfo("accept finished.\n");
    }

exit0:

exit1:
    if (ret_code != 0) {
        if (stream != NULL) {
            switch (rsocket_ctx->share.stream_type) {
            case ripc_type_tcp:
                rfree_data(uv_tcp_t, stream);
                break;
            case ripc_type_pipe:
                rfree_data(uv_pipe_t, stream);
                break;
            }
        }

        if (datasource != NULL) {
            rfree_data(ripc_data_source_t, stream);
        }
    }
}

static void after_shutdown_client(uv_shutdown_t* req, int status) {
    if (status == 0) {
        rinfo("after_shutdown_client success.\n");
    }
    else {
        rerror("error after_shutdown_client: %d\n", status);
    }

    uv_close((uv_handle_t*)req->handle, on_close);

    rfree_data(uv_shutdown_t, req);
}

static void after_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    int ret_code = 0;

    ripc_data_source_t* datasource = (ripc_data_source_t*)(handle->data);
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)(datasource->ctx);
    ripc_data_raw_t data_raw;//直接在栈上

    local_write_req_t *wr;
    uv_shutdown_t* sreq;
    int shutdown = 0;

    rinfo("after read: %d\n", nread);

    if (nread < 0) {
        //rstr_free(((uv_buf_t*)buf)->base);

        /* Error or EOF */
        if (nread != UV_EOF) {//异常，未主动关闭等
            rerror("error on read: %d\n", nread);
            uv_close(handle, on_close);
            return;
        }

        if (uv_is_writable(handle)) {
            sreq = rnew_data(uv_shutdown_t);
            ret_code = uv_shutdown(sreq, handle, after_shutdown_client);
            if (ret_code != 0) {
                rerror("error on shutdown, code: %d\n", ret_code);
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
            rerror("error on handler process, code: %d\n", ret_code);
            return;
        }

    }

    ///*
    // * Scan for the letter c which signals that we should quit the server.
    // * If we get cs it means close the stream.
    // * If we get css it means shutdown the stream.
    // * If we get cso it means disable linger before close the socket.
    // */
    //for (int i = 0; i < nread; i++) {
    //    if (buf->base[i] == 'c') {
    //        if (i + 1 < nread && buf->base[i + 1] == 's') {
    //            int reset = 0;
    //            if (i + 2 < nread && buf->base[i + 2] == 's')
    //                shutdown = 1;
    //            if (i + 2 < nread && buf->base[i + 2] == 'o')
    //                reset = 1;
    //            if (reset && handle->type == UV_TCP) {
    //                ret_code = uv_tcp_close_reset((uv_tcp_t*)handle, on_close);
    //                if (ret_code != 0) {
    //                    rerror("uv_tcp_close_reset failed.\n");
    //                    return;
    //                }
    //            }
    //            else if (shutdown) {
    //                break;
    //            }
    //            else {
    //                uv_close((uv_handle_t*)handle, on_close);
    //            }
    //            free(buf->base);
    //            return;
    //        }
    //    }
    //}
    //wr = (local_write_req_t*)malloc(sizeof *wr);
    //wr->buf = uv_buf_init(buf->base, nread);
    //ret_code = uv_write(&wr->req, handle, &wr->buf, 1, after_write);
    //if (ret_code != 0) {
    //    rerror("uv_write failed.\n");
    //    return;
    //}
    //if (shutdown) {
    //    ret_code = uv_shutdown(malloc(sizeof* sreq), handle, on_shutdown);
    //    if (ret_code != 0) {
    //        rerror("uv_write failed, code: %d\n", ret_code);
    //        return;
    //    }
    //}
}

static void after_write(uv_write_t *req, int status) {
    rinfo("end after_write state: %d\n", status);

    if (status != 0) {
        rerror("write error, code = %d, err = %s\n", status, uv_err_name(status));
    }

    local_write_req_t* wr = (local_write_req_t*)req->data;
    ripc_data_source_t* datasource = wr->ds;
    rbuffer_skip(datasource->write_buff, wr->write_size);
    rfree_data(local_write_req_t, wr);
    rfree_data(uv_write_t, req);
    //free_write_req(req);

    rdebug("writing buff left bytes: %d - %d\n", rbuffer_write_start_pos(datasource->write_buff), rbuffer_size(datasource->write_buff));
}
static void send_data(ripc_data_source_t* ds, void* data) {
    int ret_code = 0;
    local_write_req_t* wr;
    ripc_data_source_t* datasource = (ripc_data_source_t*)ds;
    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ds->ctx;
    uv_write_t* req = NULL;

    if (rsocket_ctx->out_handler) {
        ret_code = rsocket_ctx->out_handler->process(rsocket_ctx->out_handler, datasource, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d\n", ret_code);
            return;
        }
    }

    uv_buf_t buf;
    //buf = uv_buf_init(data->data, data->len);//结构体内容复制
    buf.base = rbuffer_read_start_dest(datasource->write_buff);
    buf.len = rbuffer_size(datasource->write_buff);

    wr = rnew_data(local_write_req_t);
    wr->ds = datasource;
    wr->write_size = buf.len;

    req = rnew_data(uv_write_t);
    req->data = wr;

    //unix间接调用uv_write2 malloc了buf放到req里再cb，但是win里tcp实现是直接WSASend！操蛋
    ret_code = uv_write(req, (uv_stream_t*)(datasource->stream), &buf, 1, after_write);
    rdebug("send_data, req: %p, buf: %p\n", req, &buf);
    //rdebug("send_data, len: %d, dest_len: %p, data_buf: %p\n", data->len, data->data, buf.base);

    if (ret_code != 0) {
        rerror("uv_write failed. code: %d\n", ret_code);
        return;
    }
}

static void on_close(uv_handle_t* peer) {
    //todo Ray 暂时仅tcp
    ripc_data_source_t* datasource = (ripc_data_source_t*)(peer->data);
    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)(datasource->ctx);
    rinfo("on close, id = %"PRIu64", \n", datasource->ds_id);

    if (datasource->ds_type == ripc_data_source_type_client) {
        rdict_remove(rsocket_ctx->map_clients, datasource->ds_id);

        rbuffer_release(datasource->read_cache);
        rbuffer_release(datasource->write_buff);
        rfree_data(ripc_data_source_t, datasource);
        rfree_data(uv_tcp_t, peer);
    }
    else if (datasource->ds_type == ripc_data_source_type_server) {
        rerror("on_close error, cant release server.\n");
    }
}

static void on_server_close(uv_handle_t* handle) {

    rinfo("on_server_close success.\n");
}

//static void on_shutdown(uv_shutdown_t* req, int status) {
//    if (status == 0) {
//        rinfo("shutdown success.\n");
//    }
//    else {
//        rerror("error on shutdown: %d\n", status);
//    }
//
//    rfree_data(uv_shutdown_t, req);
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
//        rerror("on_send failed. req = %p, status = %d\n", req, status);
//        return;
//    }
//    rinfo("on send: %d\n", status);
//
//    req->data = udp_data_list_free;
//    udp_data_list_free = req;
//}
//static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf, const struct sockaddr* addr, unsigned flags) {
//    rinfo("on_recv: %d\n", nread);
//    uv_buf_t sndbuf;
//    uv_udp_send_t* req;
//
//    if (nread == 0) {
//        /* Everything OK, but nothing read. */
//        return;
//    }
//
//    if (nread <= 0 || addr->sa_family != AF_INET) {
//        rerror("on_recv error. nread = %d, addr->sa_family = %u\n", nread, addr->sa_family);
//        return;
//    }
//
//    req = send_alloc();
//    sndbuf = uv_buf_init(rcvbuf->base, nread);
//    int send_len = uv_udp_send(req, handle, &sndbuf, 1, addr, on_send);
//    if (send_len < 0) {
//        rerror("on_recv error. nread = %d, send_len = %d\n", nread, send_len);
//        return;
//    }
//}

static int start_server_tcp4(rsocket_ctx_uv_t* rsocket_ctx) {
    int ret_code;
    rinfo("socket server open.\n");

    rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    char* ip = cfg->ip;
    int port = cfg->port;
    struct sockaddr_in bind_addr;
    ret_code = uv_ip4_addr(ip, port, &bind_addr);
    if (ret_code != 0) {
        rerror("uv_ip4_addr error, code: %d\n", ret_code);
        return 1;
    }
    
    ret_code = uv_tcp_init(rsocket_ctx->loop, (uv_handle_t*)rsocket_ctx->stream);
    if (ret_code != 0) {
        rerror("socket creation error, code: %d\n", ret_code);
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
        rerror("bind error at %s:%d\n", ip, port);
        return 1;
    }

    ret_code = uv_listen((uv_stream_t*)rsocket_ctx->stream, 128, on_connection);
    if (ret_code) {
        rerror("listen error %s:%d, %s\n", ip, port, uv_err_name(ret_code));
        return 1;
    }
    rinfo("listen at %s:%d success.\n", ip, port);

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
    rinfo("socket server init.\n");

    rsocket_server_ctx_uv_t* rsocket_ctx = (rsocket_server_ctx_uv_t*)ctx;

    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, 2000, 0);
    rassert(dict_ins != NULL, "");
	rsocket_ctx->map_clients = dict_ins;

    return rcode_ok;
}

static int ripc_uninit(void* ctx) {
    rinfo("socket server uninit.\n");

    return rcode_ok;
}

static int ripc_open(void* ctx) {
    rinfo("socket server open.\n");

    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;
    int ret_code = start_server_tcp4(rsocket_ctx);
    if (ret_code != rcode_ok) {
        rerror("error on start server, code: %d\n", ret_code);
        return ret_code;
    }

    ret_code = uv_run(rsocket_ctx->loop, UV_RUN_DEFAULT);
    if (ret_code != rcode_ok) {
        rerror("error on run loop, code: %d\n", ret_code);
        return ret_code;
    }
    rsocket_ctx->stream_state = 1;

    return ret_code;
}

static int ripc_close(void* ctx) {
    rinfo("socket server close.\n");

    rsocket_ctx_uv_t* rsocket_ctx = (rsocket_ctx_uv_t*)ctx;
    uv_close(rsocket_ctx->stream, on_server_close);
    rsocket_ctx->stream_state = 0;

    return rcode_ok;
}

static int ripc_start(void* ctx) {
    
    return rcode_ok;
}

static int ripc_stop(void* ctx) {

    return rcode_ok;
}

static ripc_entry_t* get_ipc_entry(const char* key) {

    return NULL;
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