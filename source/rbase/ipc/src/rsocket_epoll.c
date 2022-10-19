﻿/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"
#include "rlog.h"
#include "rtime.h"
#include "rdict.h"
#include "rsocket_c.h"
#include "rsocket_s.h"
#include "rtools.h"

#if defined(__linux__)

#include "repoll.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

static int ripc_on_error_c(ripc_data_source_t* ds, void* data);
static int ripc_on_error_server(ripc_data_source_t* ds, void* data);
static int close_session(ripc_data_source_t* ds_client);

static int ripc_init_c(void* ctx, const void* cfg_data) {
    /* 忽略信号SIGPIPE，避免导致崩溃 */
    signal(SIGPIPE, SIG_IGN);//SIG_DFL

    rinfo("socket client init.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    int ret_code = rcode_ok;

    ds_client->stream = NULL;
    
    return ret_code;
}
static int ripc_uninit_c(void* ctx) {
    rinfo("socket client uninit.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    int ret_code = rcode_ok;

    rsocket_ctx->stream_state = ripc_state_uninit;

    return ret_code;
}
static int ripc_open_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    ripc_data_source_t* ds_client = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    int ret_code = 0;

    repoll_item_t* repoll_item = NULL;

    rtrace("socket client open.");

    char *nodename = cfg->ip;//主机名，域名或ipv4/6
    char *servname = NULL;//服务名可以是十进制的端口号("8000")字符串或NULL/ftp等/etc/services定义的服务
    rnum2str(servname, cfg->port, 0);
    struct addrinfo connect_hints;
    struct addrinfo* iterator = NULL;
    struct addrinfo* addrinfo_result;
    rtimeout_t tm;

    int family = AF_INET;// AF_INET | AF_INET6 | AF_UNSPEC
    int socktype = SOCK_STREAM; // udp = SOCK_DGRAM
    int protocol = 0;
    int opt = 1;
    rsocket_t rsock_item_obj;
    rsocket_t* rsock_item = &rsock_item_obj;
    *rsock_item = SOCKET_INVALID;
    int current_family = family;

    //指向用户设定的 struct addrinfo 结构体，只能设定 ai_family、ai_socktype、ai_protocol 和 ai_flags 四个域
    memset(&connect_hints, 0, sizeof(struct addrinfo));
    connect_hints.ai_socktype = socktype;//SOCK_STREAM、SOCK_DGRAM、SOCK_RAW, 设置为0表示所有类型都可以
    connect_hints.ai_family = family;
    connect_hints.ai_protocol = 0;//IPPROTO_TCP、IPPROTO_UDP 等，设置为0表示所有协议

    rtimeout_init_sec(&tm, 5, 5);
    rtimeout_start(&tm);

    ret_code = getaddrinfo(nodename, servname, &connect_hints, &addrinfo_result);
    if (ret_code != rcode_ok) {
        if (addrinfo_result) {
            freeaddrinfo(addrinfo_result);
        }
        rerror("failed on getaddrinfo, code = %d, msg = %s", ret_code, (char*)rsocket_gaistrerror(ret_code));

        rgoto(1);
    }

    for (iterator = addrinfo_result; iterator; iterator = iterator->ai_next) {
        if (current_family != iterator->ai_family || *rsock_item == SOCKET_INVALID) {
            rsocket_close(rsock_item);

            ret_code = rsocket_create(rsock_item, family, socktype, protocol);
            if (ret_code != rcode_ok) {
                rinfo("failed on try create sock, code = %d", ret_code);
                continue;
            }

            if (family == AF_INET6) {
                setsockopt(*rsock_item, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&opt, sizeof(opt));
            }
            rsocket_setnonblocking(rsock_item);

            current_family = iterator->ai_family;
        }

        ret_code = rsocket_connect(rsock_item, (rsockaddr_t *)iterator->ai_addr, (rsocket_len_t)iterator->ai_addrlen, &tm);
        /* success or timeout, break of loop */
        if (ret_code == rcode_io_done) {
            family = current_family;
            rinfo("success on connect, family = %d", family);
            break;
        }

        if (ret_code != rcode_ok) {
            rdebug("wait for next addr to connect, code = %d, msg = %s", ret_code, rsocket_strerror(ret_code));
        }

    }
    freeaddrinfo(addrinfo_result);

    if (ret_code != rcode_ok) {
        rgoto(1);
    }

    //添加到epoll对象
    repoll_item = rdata_new(repoll_item_t);
    repoll_item->fd = *rsock_item;
    repoll_item->ds = ds_client;
    repoll_item->event_val_req = 0;
    repoll_set_event_all(repoll_item->event_val_req);

    ret_code = repoll_add(container, repoll_item);
    if (ret_code != rcode_ok){
        //释放资源
        rerror("add to epoll failed. code = %d", ret_code);

        rgoto(1);
    }

    ds_client->stream = repoll_item;//stream间接指向sock, ds_client->stream->fd
    rsocket_ctx->stream_state = ripc_state_ready_pending;

    rtrace("socket client open success. fd = %d", repoll_item->fd);

    return rcode_ok;

exit1:
    rsocket_close(rsock_item);
    // rdata_free(rsocket_t, rsock_item);

    if(repoll_item != NULL) {
        rdata_free(repoll_item_t, repoll_item);
    }

    return ret_code;
}
static int ripc_close_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;

    rinfo("socket client close.");

    if (rsocket_ctx->stream_state == ripc_state_closed) {
        return rcode_ok;
    }

    rbuffer_release(ds_client->read_cache);
    rbuffer_release(ds_client->write_buff);

    //从epoll移除，销毁socket对象
    if (ds_client->stream != NULL) {
        repoll_remove(container, (repoll_item_t*)ds_client->stream);

        rsocket_close(&(((repoll_item_t*)ds_client->stream)->fd));
        // rdata_free(rsocket_t, ((repoll_item_t*)ds_client->stream)->fd);

        rdata_free(repoll_item_t, ds_client->stream);
    }

    rsocket_ctx->stream_state = ripc_state_closed;

    return rcode_ok;
}
static int ripc_start_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    int ret_code = 0;

    rtimeout_t tm;
    rtimeout_init_sec(&tm, 3, 3);
    rtimeout_start(&tm);

    do {
        ret_code = repoll_poll(container, 10);//不要用边缘触发模式，可能会调用多次，单线程不会惊群
        if (ret_code != rcode_ok){
            rerror("epoll_wait failed. code = %d", ret_code);
            return ret_code;
        }

        if (container->fd_dest_count > 0) {
            repoll_item_t* dest_item = NULL;

            for (int i = 0; i < container->fd_dest_count; i++) {
                dest_item = &(container->dest_items[i]);
                if (dest_item->fd != ((repoll_item_t*)ds->stream)->fd) {
                    continue;
                }

                if (repoll_check_event_err(dest_item->event_val_rsp)) {
                    rtrace("error of epoll socket client, fd = %d", dest_item->fd);
                    ripc_on_error_c(ds, NULL);
                    continue;
                }

                if (repoll_check_event_out(dest_item->event_val_rsp)) {
                    if (rsocket_ctx->stream_state == ripc_state_ready_pending) {
                        repoll_unset_event_out(dest_item->event_val_req);//重置掉

                        ret_code = repoll_modify(container, dest_item);
                        if (ret_code != rcode_ok){
                            rerror("modify (%d) to epoll failed. code = %d", dest_item->fd, ret_code);
                            return ret_code;
                        }

                        ds->read_cache = NULL;
                        rbuffer_init(ds->read_cache, read_cache_size);
                        ds->write_buff = NULL;
                        rbuffer_init(ds->write_buff, write_buff_size);

                        // rsocket_ctx->stream_state = ripc_state_ready;
                        rsocket_ctx->stream_state = ripc_state_start;
                        rinfo("epoll socket client start.");

                        return rcode_ok;
                    }
                }
            }
        }
    } while (!rtimeout_done(&tm));

    rtrace("epoll socket client connect timeout, fd = %d", ((repoll_item_t*)ds->stream)->fd);

    return rcode_io_timeout;
}
static int ripc_stop_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rsocket_ctx->stream_state = ripc_state_stop;
    rinfo("socket client stop.");


    return rcode_ok;
}
//写入自定义缓冲区
static int ripc_send_data_2buffer_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = rcode_io_done;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    repoll_item_t* repoll_item = NULL;

    if (rsocket_ctx->stream_state != ripc_state_start) {
        rinfo("sock not ready, state: %d", rsocket_ctx->stream_state);
        return rcode_err_sock_disconnect;
    }

    if (rsocket_ctx->out_handler) {
        ret_code = rsocket_ctx->out_handler->process(rsocket_ctx->out_handler, ds_client, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return ret_code;
        }
        ret_code = rcode_io_done;
    }

    if (rbuffer_size(ds_client->write_buff) > 0) {
        repoll_item = (repoll_item_t*)ds_client->stream;
        repoll_set_event_out(repoll_item->event_val_req);

        ret_code = repoll_modify(container, repoll_item);
        if (ret_code != rcode_ok){
            rerror("modify to epoll failed. code = %d", ret_code);
            return ret_code;
        }
    }

    rdebug("end send_data_2buffer, code: %d", ret_code);

    return rcode_ok;
}
static int ripc_send_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = rcode_io_done;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    repoll_item_t* repoll_item = NULL;

    const char* data_buff = rbuffer_read_start_dest(ds_client->write_buff);
    int count = rbuffer_size(ds_client->write_buff);
    int sent_len = 0;//立即处理
    
    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 3, 3);
    rtimeout_start(&tm);

    ret_code = rsocket_send((rsocket_t*)(&((repoll_item_t*)ds_client->stream)->fd), 
        data_buff, (size_t)count, (size_t*)&sent_len, &tm);
    if (ret_code != rcode_io_done) {
        rwarn("end send_data, code: %d, sent_len: %d, buff_size: %d", ret_code, sent_len, count);

        ripc_on_error_c(ds_client, NULL);

        return rcode_io_closed;//所有未知错误都断开
    }
    rdebug("end send_data, code: %d, sent_len: %d", ret_code, sent_len);

    rbuffer_skip(ds_client->write_buff, sent_len);
    if (rbuffer_size(ds_client->write_buff) == 0) {
        repoll_item = (repoll_item_t*)ds_client->stream;
        repoll_unset_event_out(repoll_item->event_val_req);

        ret_code = repoll_modify(container, repoll_item);
        if (ret_code != rcode_ok){
            rerror("modify to epoll failed. code = %d", ret_code);
            return ret_code;
        }
    }

    return rcode_ok;
}

static int ripc_receive_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = 0;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    ripc_data_raw_t data_raw;//直接在栈上

    if (rsocket_ctx->stream_state != ripc_state_start) {
        rinfo("sock not ready, state: %d", rsocket_ctx->stream_state);
        return rcode_err_sock_disconnect;
    }
    
    char* data_buff = rbuffer_write_start_dest(ds_client->read_cache);
    int count = rbuffer_left(ds_client->read_cache);

    int received_len = 0;

    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 1, 1);
    rtimeout_start(&tm);

    ret_code = rsocket_recv((rsocket_t*)(&((repoll_item_t*)ds_client->stream)->fd), 
        data_buff, (size_t)count, (size_t*)&received_len, &tm);

    if (ret_code == rcode_io_timeout) {
        ret_code = rcode_io_done;
    } 

    if (ret_code != rcode_io_done) {
        rwarn("end client receive_data, code: %d, received_len: %d, buff_size: %d", ret_code, received_len, count);

        ripc_on_error_c(ds_client, NULL);

        return rcode_io_closed;//所有未知错误都断开
    }

    if(received_len > 0 && rsocket_ctx->in_handler) {
        data_raw.len = received_len;

        ret_code = rsocket_ctx->in_handler->process(rsocket_ctx->in_handler, ds_client, &data_raw);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return rcode_err_logic_decode;
        }
        ret_code = rcode_io_done;
    }

    return rcode_ok;
}

static int ripc_check_data_c(ripc_data_source_t* ds, void* data) {
    rsocket_ctx_t* rsocket_ctx = ds->ctx;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    int ret_code = 0;

    ret_code = repoll_poll(container, 1);//不要用边缘触发模式，可能会调用多次，单线程不会惊群
    if (ret_code != rcode_ok){
        rerror("epoll_wait failed. code = %d", ret_code);
        return ret_code;
    }

    if (container->fd_dest_count > 0) {
        repoll_item_t* dest_item = NULL;

        for (int i = 0; i < container->fd_dest_count; i++) {
            dest_item = &(container->dest_items[i]);
            if (dest_item->fd == 0) {//取回后调用了del
                continue;
            }

            if (repoll_check_event_err(dest_item->event_val_rsp)) {
                rtrace("error of socket, fd = ", dest_item->fd);

                ret_code = ripc_on_error_c(ds, NULL);
                if (ret_code != rcode_ok){
                    rerror("process event of error failed. code = %d", ret_code);
                    return ret_code;
                }

                continue;
            }

            if (repoll_check_event_out(dest_item->event_val_rsp)) {
                if likely(rsocket_ctx->stream_state == ripc_state_start) {
                    ripc_send_data_c(ds, data);
                }
            }

            if (repoll_check_event_in(dest_item->event_val_rsp)) {
                if likely(rsocket_ctx->stream_state == ripc_state_start) {
                    ripc_receive_data_c(ds, data);
                }
            }

        }
    }

    return rcode_ok;
}

static int ripc_on_error_c(ripc_data_source_t* ds, void* data) {
    rtrace("socket error.");

    int ret_code = rcode_ok;
    rsocket_ctx_t* rsocket_ctx = ds->ctx;

    if (rsocket_ctx->in_handler) {
        ret_code = rsocket_ctx->in_handler->on_error(rsocket_ctx->in_handler, ds, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handle error, code: %d", ret_code);
            rgoto(0);
        }
    }

exit0:
    ripc_close_c(rsocket_ctx);

    return ret_code;
}

static const ripc_entry_t impl_c = {
    (ripc_init_func)ripc_init_c,// ripc_init_func init;
    (ripc_uninit_func)ripc_uninit_c,// ripc_uninit_func uninit;
    (ripc_open_func)ripc_open_c,// ripc_open_func open;
    (ripc_close_func)ripc_close_c,// ripc_close_func close;
    (ripc_start_func)ripc_start_c,// ripc_start_func start;
    (ripc_stop_func)ripc_stop_c,// ripc_stop_func stop;
    (ripc_send_func)ripc_send_data_2buffer_c,// ripc_send_func send;
    (ripc_check_func)ripc_check_data_c,// ripc_check_func check;
    (ripc_receive_func)ripc_receive_data_c,// ripc_receive_func receive;
    (ripc_error_func)ripc_on_error_c// ripc_error_func error;
};
const ripc_entry_t* rsocket_epoll_c = &impl_c;






static int ripc_init_server(void* ctx, const void* cfg_data) {
    /* 忽略信号SIGPIPE，避免导致崩溃 */
    signal(SIGPIPE, SIG_IGN);//SIG_DFL

    rinfo("socket server init.");

    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ctx;
    ripc_data_source_t* ds_server = (ripc_data_source_t*)rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;

    int ret_code = rcode_ok;

    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_uint64, rdata_type_ptr, 2000, 0);
    rassert(dict_ins != NULL, "");
    rsocket_ctx->map_clients = dict_ins;

    ds_server->stream = NULL;

    return ret_code;
}

static int ripc_uninit_server(void* ctx) {
    rinfo("socket server uninit.");

    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ctx;
    int ret_code = rcode_ok;

    if (rsocket_ctx->map_clients != NULL) {
        rdict_free(rsocket_ctx->map_clients);
    }

    rsocket_ctx->stream_state = ripc_state_uninit;

    return ret_code;
}

static int ripc_open_server(void* ctx) {
    rinfo("socket server open.");

    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ctx;
    ripc_data_source_t* ds_server = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    int ret_code = 0;

    char *nodename = rstr_eq(cfg->ip, "0.0.0.0") ? NULL : cfg->ip;//主机名，域名或ipv4/6
    char *servname = NULL;//服务名可以是十进制的端口号("8000")字符串或NULL/ftp等/etc/services定义的服务
    rnum2str(servname, cfg->port, 0);
    // struct addrinfo connect_hints;
    struct addrinfo* iterator = NULL;
    struct addrinfo* addrinfo_result;
    // rtimeout_t tm;

    int family = AF_INET;// AF_INET | AF_INET6 | AF_UNSPEC
    int socktype = SOCK_STREAM; // udp = SOCK_DGRAM
    int protocol = 0;
    int opt = 1;
    // rsocket_t* rsock_item = rdata_new(rsocket_t);
    rsocket_t rsock_item_obj;
    rsocket_t* rsock_item = &rsock_item_obj;
    *rsock_item = SOCKET_INVALID;

    repoll_item_t* repoll_item = NULL;
    int current_family = family;

    struct addrinfo bind_hints;
    //指向用户设定的 struct addrinfo 结构体，只能设定 ai_family、ai_socktype、ai_protocol 和 ai_flags 四个域
    memset(&bind_hints, 0, sizeof(bind_hints));
    bind_hints.ai_socktype = socktype;//SOCK_STREAM、SOCK_DGRAM、SOCK_RAW, 设置为0表示所有类型都可以
    bind_hints.ai_family = family;
    bind_hints.ai_flags = AI_PASSIVE;//nodename=NULL, 返回socket地址可用于bind()函数（*.*.*.* Any），AI_NUMERICHOST则不能是域名
    //nodename 不是NULL，那么 AI_PASSIVE 标志被忽略；
    //未设置AI_PASSIVE标志, 返回的socket地址可以用于connect(), sendto(), 或者 sendmsg()函数。
    //nodename 是NULL，网络地址会被设置为lookback接口地址，这种情况下，应用是想与运行在同一个主机上另一个应用通信
    bind_hints.ai_protocol = 0;//IPPROTO_TCP、IPPROTO_UDP 等，设置为0表示所有协议

    ret_code = getaddrinfo(nodename, servname, &bind_hints, &addrinfo_result);
    if (ret_code != rcode_ok) {
        if (addrinfo_result) {
            freeaddrinfo(addrinfo_result);
        }
        rerror("failed on getaddrinfo, code = %d, msg = %s", ret_code, (char*)rsocket_gaistrerror(ret_code));

        rgoto(1);
    }

    for (iterator = addrinfo_result; iterator; iterator = iterator->ai_next) {
        if (current_family != iterator->ai_family || *rsock_item == SOCKET_INVALID) {
            rsocket_close(rsock_item);

            ret_code = rsocket_create(rsock_item, family, socktype, protocol);
            if (ret_code != rcode_ok) {
                rinfo("failed on try create sock, code = %d", ret_code);
                continue;
            }

            if (family == AF_INET6) {
                setsockopt(*rsock_item, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&opt, sizeof(opt));
            }

            current_family = iterator->ai_family;
        }
       
        rsocket_setblocking(rsock_item);
        ret_code = rsocket_bind(rsock_item, (rsockaddr_t*)iterator->ai_addr, (rsocket_len_t)iterator->ai_addrlen);

        /* keep trying unless bind succeeded */
        if (ret_code == rcode_ok) {
           family = current_family;
           break;
        }
    }
    freeaddrinfo(addrinfo_result);

    if (ret_code != rcode_ok) {
        rerror("error on start server, error: %s", rsocket_strerror(ret_code));

        rgoto(1);
    }

    rsocket_setblocking(rsock_item);
    ret_code = rsocket_listen(rsock_item, 128);

    if (ret_code != rcode_ok) {
        rerror("error on start server, listen failed, code: %d", ret_code);

        rgoto(1);
    }

    rsocket_setnonblocking(rsock_item);

    //添加到epoll对象
    repoll_item = rdata_new(repoll_item_t);
    repoll_item->fd = *rsock_item;
    repoll_item->ds = ds_server;
    repoll_item->event_val_req = 0;
    repoll_set_event_all(repoll_item->event_val_req);

    ret_code = repoll_add(container, repoll_item);
    if (ret_code != rcode_ok){
        //释放资源
        rerror("add to epoll failed. code = %d", ret_code);

        rgoto(1);
    }

    ds_server->stream = repoll_item;//stream间接指向sock, ds->stream->fd
    // rsocket_ctx->stream_state = ripc_state_ready_pending;

    rsocket_ctx->stream_state = ripc_state_ready;

    // rdata_free(rsocket_t, rsock_item);
    return rcode_ok;

exit1:
    rsocket_close(rsock_item);
    // rdata_free(rsocket_t, rsock_item);

    if(repoll_item != NULL) {
        rdata_free(repoll_item_t, repoll_item);
    }

    return ret_code;
}

static int ripc_close_server(void* ctx) {
    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ctx;
    ripc_data_source_t* ds_server = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;

    rinfo("socket server close. state = %d", rsocket_ctx->stream_state);

    if (rsocket_ctx->stream_state == ripc_state_closed) {
        return rcode_ok;
    }

    //从epoll移除，销毁socket对象
    if (ds_server->stream != NULL) {
        repoll_remove(container, (repoll_item_t*)ds_server->stream);

        rsocket_close(&(((repoll_item_t*)ds_server->stream)->fd));
        // rdata_free(rsocket_t, ((repoll_item_t*)ds_server->stream)->fd);

        rdata_free(repoll_item_t, ds_server->stream);
    }

    if (rsocket_ctx->map_clients && rdict_size(rsocket_ctx->map_clients) > 0) {
        ripc_data_source_t* ds_client = NULL;
        rdict_iterator_t it = rdict_it(rsocket_ctx->map_clients);
        for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
            ds_client = (ripc_data_source_t*)(de->value.ptr);
            close_session(ds_client);
        }
        rdict_clear(rsocket_ctx->map_clients);
    }

    rsocket_ctx->stream_state = ripc_state_closed;

    return rcode_ok;
}
static int ripc_start_server(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds = rsocket_ctx->stream;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    int ret_code = 0;

    rsocket_ctx->stream_state = ripc_state_start;
    rinfo("socket server start.");

    return ret_code;
}
static int ripc_stop_server(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rsocket_ctx->stream_state = ripc_state_stop;
    rinfo("socket server stop.");


    return rcode_ok;
}

static int close_session(ripc_data_source_t* ds_client) {
    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ds_client->ctx;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;

    rinfo("socket session close.");

    // if (rsocket_ctx->stream_state == ripc_state_closed) {
    //     return rcode_ok;
    // }

    rdict_remove(rsocket_ctx->map_clients, (const void*)ds_client->ds_id);

    rbuffer_release(ds_client->read_cache);
    rbuffer_release(ds_client->write_buff);

    //从epoll移除，销毁socket对象
    if (ds_client->stream != NULL) {
        repoll_remove(container, (repoll_item_t*)ds_client->stream);

        rsocket_close(&(((repoll_item_t*)ds_client->stream)->fd));

        rdata_free(repoll_item_t, ds_client->stream);
    }

    rdata_free(ripc_data_source_t, ds_client);

    // rsocket_ctx->stream_state = ripc_state_closed;

    return rcode_ok;
}

//写入自定义缓冲区
static int ripc_send_data_2buffer_server(ripc_data_source_t* ds_client, void* data) {
    int ret_code = rcode_io_done;
    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ds_client->ctx;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    repoll_item_t* repoll_item = NULL;

    if (rsocket_ctx->stream_state != ripc_state_start) {
        rinfo("sock not ready, state: %d", rsocket_ctx->stream_state);
        return rcode_err_sock_disconnect;
    }

    if (rsocket_ctx->out_handler) {
        ret_code = rsocket_ctx->out_handler->process(rsocket_ctx->out_handler, ds_client, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return ret_code;
        }
        ret_code = rcode_io_done;
    }

    if (rbuffer_size(ds_client->write_buff) > 0) {
        repoll_item = (repoll_item_t*)ds_client->stream;
        repoll_set_event_out(repoll_item->event_val_req);

        ret_code = repoll_modify(container, repoll_item);
        if (ret_code != rcode_ok){
            rerror("modify to epoll failed. code = %d", ret_code);
            return ret_code;
        }
    }

    rtrace("end send_data_2buffer, code: %d", ret_code);

    return rcode_ok;
}
static int ripc_send_data_server(ripc_data_source_t* ds_client, void* data) {
    int ret_code = rcode_io_done;
    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ds_client->ctx;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    repoll_item_t* repoll_item = NULL;

    const char* data_buff = rbuffer_read_start_dest(ds_client->write_buff);
    int count = rbuffer_size(ds_client->write_buff);
    int sent_len = 0;//立即处理
    
    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 3, 3);
    rtimeout_start(&tm);

    ret_code = rsocket_send((rsocket_t*)(&((repoll_item_t*)ds_client->stream)->fd), 
        data_buff, (size_t)count, (size_t*)&sent_len, &tm);
    if (ret_code != rcode_io_done) {
        rwarn("end send_data, code: %d, sent_len: %d, buff_size: %d", ret_code, sent_len, count);

        if (ret_code == rcode_io_timeout) {
            return rcode_err_sock_timeout;
        }
        else {
            return rcode_err_sock_disconnect;//所有未知错误都断开
        }
    }

    rbuffer_skip(ds_client->write_buff, sent_len);
    if (rbuffer_size(ds_client->write_buff) == 0) {
        repoll_item = (repoll_item_t*)ds_client->stream;
        repoll_unset_event_out(repoll_item->event_val_req);

        ret_code = repoll_modify(container, repoll_item);
        if (ret_code != rcode_ok){
            rerror("modify to epoll failed. code = %d", ret_code);
            return ret_code;
        }
    }

    rtrace("end send_data, code: %d, sent_len: %d", ret_code, sent_len);

    return rcode_ok;
}

static int ripc_receive_data_server(ripc_data_source_t* ds_client, void* data) {
    int ret_code = 0;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    ripc_data_raw_t data_raw;//直接在栈上

    if (rsocket_ctx->stream_state != ripc_state_start) {
        rinfo("sock not ready, state: %d", rsocket_ctx->stream_state);
        return rcode_err_sock_disconnect;
    }
    
    char* data_buff = rbuffer_write_start_dest(ds_client->read_cache);
    int count = rbuffer_left(ds_client->read_cache);

    int received_len = 0;

    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 1, 1);
    rtimeout_start(&tm);

    ret_code = rsocket_recv((rsocket_t*)(&((repoll_item_t*)ds_client->stream)->fd), 
        data_buff, (size_t)count, (size_t*)&received_len, &tm);

    if (ret_code == rcode_io_timeout) {
        ret_code = rcode_io_done;
    }

    if (ret_code != rcode_io_done) {
        rtrace("end client recv_data, code: %d, received_len: %d, buff_size: %d", ret_code, received_len, count);

        ripc_on_error_server(ds_client, NULL);

        return rcode_io_closed;//所有未知错误都断开
    }

    if(received_len > 0 && rsocket_ctx->in_handler) {
        data_raw.len = received_len;

        ret_code = rsocket_ctx->in_handler->process(rsocket_ctx->in_handler, ds_client, &data_raw);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return rcode_err_logic_decode;
        }
        ret_code = rcode_io_done;
    }

    return rcode_ok;
}

static int ripc_accept_server(ripc_data_source_t* ds_server, void* data) {
    int ret_code = rcode_ok;
    rsocket_server_ctx_t* rsocket_ctx = (rsocket_server_ctx_t*)ds_server->ctx;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;

    ripc_data_source_t* ds_client = NULL;

    uint16_t port = 0;
    char* ip = NULL;
    char ip_buffer[64];

    int family = AF_INET;
    rsockaddr_storage_t addr;
    rsocket_len_t addr_len;

    rsocket_t rsock_item_obj;
    rsocket_t* rsock_item = &rsock_item_obj;
    *rsock_item = SOCKET_INVALID;

    repoll_item_t* repoll_item = NULL;

    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 3, 3);
    rtimeout_start(&tm);

    switch (family) {
        case AF_INET6: 
            addr_len = sizeof(struct sockaddr_in6); 
            break;
        case AF_INET: 
            addr_len = sizeof(struct sockaddr_in); 
            break;
        default: 
            addr_len = sizeof(addr); 
            break;
    }

    ret_code = rsocket_accept(&(((repoll_item_t*)ds_server->stream)->fd), rsock_item, (rsockaddr_t*)&addr, &addr_len, &tm);

    if (ret_code != rcode_io_done) {
        rerror("accept error. code = %d", ret_code);

        rgoto(1);
    }

    if (*rsock_item == SOCKET_INVALID) {
        rgoto(0);
    }

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in* addr_v4 = (struct sockaddr_in*)&addr;
        ip = inet_ntoa(addr_v4->sin_addr);
        port = ntohs(addr_v4->sin_port);
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6* addr_v6 = (struct sockaddr_in6*)&addr;
        inet_ntop(AF_INET6, &addr_v6->sin6_addr, ip_buffer, 64);
        ip = ip_buffer;
        port = ntohs(addr_v6->sin6_port);
    } else {
        rerror("accept client, unknown address family. family = %d, AF_INET = %d, family = %d", 
            addr.ss_family, AF_INET, AF_INET6);

        rgoto(1);
    }

    ds_client = rdata_new(ripc_data_source_t);
    ds_client->ds_type = ripc_data_source_type_session;
    ds_client->ds_id = ++ rsocket_ctx->sid_cur;
    ds_client->read_cache = NULL;
    rbuffer_init(ds_client->read_cache, read_cache_size);
    ds_client->write_buff = NULL;
    rbuffer_init(ds_client->write_buff, write_buff_size);
    ds_client->ctx = rsocket_ctx;

    rdict_add(rsocket_ctx->map_clients, (void*)ds_client->ds_id, ds_client);

    rsocket_setnonblocking(rsock_item);
    //添加到epoll对象
    repoll_item = rdata_new(repoll_item_t);
    repoll_item->fd = *rsock_item;
    repoll_item->ds = ds_client;
    repoll_item->event_val_req = 0;
    repoll_set_event_in(repoll_item->event_val_req);

    ret_code = repoll_add(container, (const repoll_item_t*)repoll_item);
    if (ret_code != rcode_ok){
        //释放资源
        rerror("add to epoll failed. code = %d", ret_code);

        rgoto(1);
    }

    ds_client->stream = repoll_item;//stream间接指向sock, ds->stream->fd

    // rsocket_ctx->stream_state = ripc_state_ready_pending;

    rinfo("accept client %d (%s:%d)", addr.ss_family, ip, port);

exit0:
    return rcode_ok;

exit1:
    if (ds_client != NULL) {
        rdict_remove(rsocket_ctx->map_clients, (const void*)ds_client->ds_id);

        rbuffer_release(ds_client->read_cache);
        rbuffer_release(ds_client->write_buff);

        rdata_free(ripc_data_source_t, ds_client);
    }

    if (repoll_item != NULL) {
        if (repoll_check(container, (const repoll_item_t*)repoll_item) == rcode_ok) {
            repoll_remove(container, (const repoll_item_t*)repoll_item);
        }
        rdata_free(repoll_item_t, repoll_item);
    }

    rsocket_close(rsock_item);

    return ret_code;
}

static int ripc_append_server(ripc_data_source_t* ds_server, void* data) {
    int ret_code = rcode_ok;
    rsocket_ctx_t* rsocket_ctx = ds_server->ctx;

    rdebug("client on service.");

    return ret_code;
}

static int ripc_check_data_server(ripc_data_source_t* ds, void* data) {
    rsocket_ctx_t* rsocket_ctx = ds->ctx;
    repoll_container_t* container = (repoll_container_t*)rsocket_ctx->user_data;
    int ret_code = 0;

    ret_code = repoll_poll(container, 0);//不要用边缘触发模式，可能会调用多次，单进程单线程不会惊群，不等待
    if (ret_code != rcode_ok){
        rerror("epoll_wait failed. code = %d", ret_code);
        return ret_code;
    }
    
    if (container->fd_dest_count > 0) {
        rtrace("fd count = %d", container->fd_dest_count);
        repoll_item_t* dest_item = NULL;

        for (int i = 0; i < container->fd_dest_count; i++) {
            dest_item = &(container->dest_items[i]);
            if (dest_item->fd == 0) {//取回后调用了del
                continue;
            }

            //监听端口事件处理
            if (dest_item->ds == ds){
                if (repoll_check_event_err(dest_item->event_val_rsp)) {
                    rerror("error of socket, fd = ", dest_item->fd);
                    ripc_close_server(rsocket_ctx);//直接关闭
                    continue;
                }

                if (repoll_check_event_out(dest_item->event_val_rsp)) {//add success
                    if likely(rsocket_ctx->stream_state == ripc_state_start) {
                        ripc_append_server(ds, data);
                    } else {
                        rwarn("server not on service.");
                    }
                }

                if (repoll_check_event_in(dest_item->event_val_rsp)) {//accept
                    if likely(rsocket_ctx->stream_state == ripc_state_start) {
                        int accept_count = 10;
                        while (ripc_accept_server(ds, NULL) == rcode_ok && --accept_count > 0) {
                            
                        }
                    } else {
                        rwarn("server not on service.");
                    }
                }

                continue;
            }

            //已连接端口事件处理
            if (repoll_check_event_err(dest_item->event_val_rsp)) {
                rtrace("error of socket, fd = %d", dest_item->fd);
                ripc_on_error_server(dest_item->ds, NULL);
                continue;
            }

            if (repoll_check_event_out(dest_item->event_val_rsp)) {
                if likely(rsocket_ctx->stream_state == ripc_state_start) {
                    ripc_send_data_server(dest_item->ds, data);
                }
            }

            if (repoll_check_event_in(dest_item->event_val_rsp)) {
                if likely(rsocket_ctx->stream_state == ripc_state_start) {
                    ripc_receive_data_server(dest_item->ds, data);
                }
            }
        }
    }

    if (rsocket_ctx->stream_state != ripc_state_start) {
        rwarn("server not on service.");
        return rcode_invalid;
    }

    return rcode_ok;
}

static int ripc_on_error_server(ripc_data_source_t* ds, void* data) {
    rtrace("socket error server.");

    rsocket_ctx_t* rsocket_ctx = ds->ctx;

    // if (ds->stream_type == ) {
        close_session(ds);//直接关闭
    // }
    
    if (rsocket_ctx->in_handler) {
        // ret_code = rsocket_ctx->in_handler->on_error(rsocket_ctx->out_handler, ds_client, data);
        // if (ret_code != ripc_code_success) {
        //     rerror("error on handler error, code: %d", ret_code);
        //     return ret_code;
        // }
    }

    return rcode_ok;
}

static const ripc_entry_t impl_s = {
    (ripc_init_func)ripc_init_server,// ripc_init_func init;
    (ripc_uninit_func)ripc_uninit_server,// ripc_uninit_func uninit;
    (ripc_open_func)ripc_open_server,// ripc_open_func open;
    (ripc_close_func)ripc_close_server,// ripc_close_func close;
    (ripc_start_func)ripc_start_server,// ripc_start_func start;
    (ripc_stop_func)ripc_stop_server,// ripc_stop_func stop;
    (ripc_send_func)ripc_send_data_2buffer_server,// ripc_send_func send;
    (ripc_check_func)ripc_check_data_server,// ripc_check_func check;
    (ripc_receive_func)ripc_receive_data_server,// ripc_receive_func receive;
    (ripc_error_func)ripc_on_error_server// ripc_error_func error;
};
const ripc_entry_t* rsocket_s = &impl_s;


#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#else

#endif //__linux__
