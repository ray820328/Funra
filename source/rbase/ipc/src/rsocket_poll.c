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
#include "rsocket_c.h"
#include "rsocket_s.h"
#include "rtools.h"

#include <sys/poll.h>


static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

#define WAITFD_R        POLLIN
#define WAITFD_W        POLLOUT
#define WAITFD_C        (POLLIN|POLLOUT)


static int rsocket_waitfd(rsocket_t* sock_item, int sw, rtimeout_t* tm) {
    int ret_code = 0;

    struct pollfd pfd;
    pfd.fd = *sock_item;
    pfd.events = sw;//poll用户态修改的是events
    pfd.revents = 0;//poll内核态修改的是revents

    if (rtimeout_done(tm)) {
        return rcode_io_timeout;
    }

    do {
        int t = (int)(rtimeout_get_total(tm) / 1000);//毫秒
        ret_code = poll(&pfd, 1, t >= 0 ? t : -1); //timeout为时间差值，精度是毫秒，-1时无限等待
    } while (ret_code == -1 && rerror_get_osnet_err() == EINTR);

    if (ret_code == -1) {
        return rerror_get_osnet_err();
    }
    if (ret_code == 0) {
        return rcode_io_timeout;
    }
    if (sw == WAITFD_C && (pfd.revents & (POLLIN | POLLERR))) {
        return rcode_io_closed;
    }

    return rcode_io_done;
}

static int rsocket_open() {
    /* 避免sigpipe导致崩溃 */
    signal(SIGPIPE, SIG_IGN);
    return rcode_ok;
}

static int rsocket_select(rsocket_t rsock, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm) {
    int ret_code = 0;
    do {
        struct timeval tv;
        int64_t time_left = rtimeout_get_block(tm);
        rtimeout_2timeval(tm, &tv, time_left);
        /* timeout = 0 means no wait */
        ret_code = select(rsock, rfds, wfds, efds, time_left >= 0 ? &tv: NULL);
    } while (ret_code < 0 && rerror_get_osnet_err() == EINTR);

    return ret_code;
}

static int rsocket_connect(rsocket_t* sock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
    /* avoid calling on closed sockets */
    if (*sock_item == SOCKET_INVALID) return rcode_io_closed;
    /* call connect until done or failed without being interrupted */
    do if (connect(*sock_item, addr, len) == 0) return rcode_io_done;
    while ((ret_code = rerror_get_osnet_err()) == EINTR);
    /* if connection failed immediately, return error code */
    if (ret_code != EINPROGRESS && ret_code != EAGAIN) return ret_code;
    /* zero timeout case optimization */
    if (rtimeout_done(tm)) return rcode_io_timeout;
    /* wait until we have the result of the connection attempt or timeout */
    ret_code = rsocket_waitfd(sock_item, WAITFD_C, tm);
    if (ret_code == rcode_io_closed) {
        if (recv(*sock_item, (char *) &ret_code, 0, 0) == 0) return rcode_io_done;
        else return rerror_get_osnet_err();
    } else return ret_code;
}

static int rsocket_send(rsocket_t* sock_item, const char *data, size_t count, size_t *sent, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    
    if (*sock_item == SOCKET_INVALID) return rcode_io_closed;
    
    for ( ;; ) {
        long put = (long) send(*sock_item, data, count, 0);
        /* if we sent anything, we are done */
        if (put >= 0) {
            *sent = put;
            return rcode_io_done;
        }
        ret_code = rerror_get_osnet_err();
        /* EPIPE means the connection was closed */
        if (ret_code == EPIPE) return rcode_io_closed;
        /* EPROTOTYPE means the connection is being closed (on Yosemite!)*/
        if (ret_code == EPROTOTYPE) continue;
        /* we call was interrupted, just try again */
        if (ret_code == EINTR) continue;
        /* if failed fatal reason, report error */
        if (ret_code != EAGAIN) return ret_code;
        /* wait until we can send something or we timeout */
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != rcode_io_done) return ret_code;
    }
    
    return rcode_io_unknown;
}

static int rsocket_sendto(rsocket_t* sock_item, const char *data, size_t count, size_t *sent,
        rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    if (*sock_item == SOCKET_INVALID) return rcode_io_closed;
    for ( ;; ) {
        long put = (long) sendto(*sock_item, data, count, 0, addr, len); 
        if (put >= 0) {
            *sent = put;
            return rcode_io_done;
        }
        ret_code = rerror_get_osnet_err();
        if (ret_code == EPIPE) return rcode_io_closed;
        if (ret_code == EPROTOTYPE) continue;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != rcode_io_done) return ret_code;
    }
    return rcode_io_unknown;
}

static int rsocket_recv(rsocket_t* sock_item, char *data, size_t count, size_t *got, rtimeout_t* tm) {
    int ret_code;
    *got = 0;
    if (*sock_item == SOCKET_INVALID) return rcode_io_closed;
    for ( ;; ) {
        long taken = (long) recv(*sock_item, data, count, 0);
        if (taken > 0) {
            *got = taken;
            return rcode_io_done;
        }
        ret_code = rerror_get_osnet_err();
        if (taken == 0) return rcode_io_closed;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != rcode_io_done) return ret_code;
    }
    return rcode_io_unknown;
}

static int rsocket_recvfrom(rsocket_t* sock_item, char *data, int count, int *got, rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    int ret_code;
    *got = 0;
    if (*sock_item == SOCKET_INVALID) return rcode_io_closed;
    for ( ;; ) {
        long taken = (long) recvfrom(*sock_item, data, count, 0, addr, len);
        if (taken > 0) {
            *got = taken;
            return rcode_io_done;
        }
        ret_code = rerror_get_osnet_err();
        if (taken == 0) return rcode_io_closed;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != rcode_io_done) return ret_code;
    }
    return rcode_io_unknown;
}


static int ripc_init_c(void* ctx, const void* cfg_data) {
    rinfo("socket client init.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    int ret_code = 0;

    ret_code = rsocket_open();
    if (ret_code != rcode_ok) {
        rerror("failed on open, code = %d", ret_code);
        return ret_code;
    }
    
    return rcode_ok;
}
static int ripc_uninit_c(void* ctx) {
    rinfo("socket client uninit.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->stream;
    int ret_code = 0;

    ret_code = rsocket_close(ds_client->stream);
    if (ret_code != rcode_ok) {
        rerror("failed on close, code = %d", ret_code);
        return ret_code;
    }

    rsocket_ctx->stream_state = ripc_state_uninit;

    return rcode_ok;
}
static int ripc_open_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    rsocket_cfg_t* cfg = rsocket_ctx->cfg;
	ripc_data_source_t* ds_client = rsocket_ctx->stream;
    int ret_code = 0;

    rinfo("socket client open.");

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
	rsocket_t* rsock_item = rdata_new(rsocket_t);
	*rsock_item = SOCKET_INVALID;
    int current_family = family;

    //指向用户设定的 struct addrinfo 结构体，只能设定 ai_family、ai_socktype、ai_protocol 和 ai_flags 四个域
    memset(&connect_hints, 0, sizeof(struct addrinfo));
    connect_hints.ai_socktype = socktype;//SOCK_STREAM、SOCK_DGRAM、SOCK_RAW, 设置为0表示所有类型都可以
    connect_hints.ai_family = family;
    connect_hints.ai_protocol = 0;//IPPROTO_TCP、IPPROTO_UDP 等，设置为0表示所有协议

    rtimeout_init_sec(&tm, 5, -1);
    rtimeout_start(&tm);

    ret_code = getaddrinfo(nodename, servname, &connect_hints, &addrinfo_result);
    if (ret_code != rcode_ok) {
        if (addrinfo_result) {
            freeaddrinfo(addrinfo_result);
        }
        rerror("failed on getaddrinfo, code = %d, msg = %s", ret_code, rsocket_gaistrerror(ret_code));
        
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
        if (ret_code == rcode_ok) {
            family = current_family;
            rinfo("success on connect, family = %d", family);
            break;
        }
        if (rtimeout_done(&tm)) {
            ret_code = rcode_err_sock_timeout;
        }
        if (ret_code != rcode_ok) {
            rinfo("failed on connect, code = %d, msg = %s", ret_code, (char*)rsocket_strerror(ret_code));
        }

    }
    freeaddrinfo(addrinfo_result);

	if (ret_code != rcode_ok) {
	   rgoto(1);
    }

    ds_client->read_cache = NULL;
    rbuffer_init(ds_client->read_cache, read_cache_size);
    ds_client->write_buff = NULL;
    rbuffer_init(ds_client->write_buff, write_buff_size);

    ds_client->stream = rsock_item;
    rsocket_ctx->stream_state = ripc_state_ready;

    return rcode_ok;

exit1:
    
    rsocket_close(rsock_item);
    rdata_free(rsocket_t, rsock_item);
    return ret_code;
}
static int ripc_close_c(void* ctx) {
	rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
	ripc_data_source_t* ds_client = rsocket_ctx->stream;

    rinfo("socket client close.");

    if (rsocket_ctx->stream_state == ripc_state_closed) {
        return rcode_ok;
    }


    if (rsocket_ctx->stream_state == ripc_state_ready || 
            rsocket_ctx->stream_state == ripc_state_disconnect || 
            rsocket_ctx->stream_state == ripc_state_start || 
            rsocket_ctx->stream_state == ripc_state_stop || 
            rsocket_ctx->stream_state == ripc_state_disconnect) {
    	rbuffer_release(ds_client->read_cache);
    	rbuffer_release(ds_client->write_buff);

        rsocket_close(ds_client->stream);
        rdata_free(rsocket_t, ds_client->stream);
    }

    rsocket_ctx->stream_state = ripc_state_closed;

    return rcode_ok;
}
static int ripc_start_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client start.");

    rsocket_ctx->stream_state = ripc_state_start;

    return 0;
}
static int ripc_stop_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client stop.");

    rsocket_ctx->stream_state = ripc_state_stop;

    return rcode_ok;
}
static int ripc_send_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = rcode_io_done;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;

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

    const char* data_buff = rbuffer_read_start_dest(ds_client->write_buff);
    int count = rbuffer_size(ds_client->write_buff);
    int sent_len = 0;//立即处理
    int total = 0;
    
    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 100, -1);
    rtimeout_start(&tm);

    while (total < count && ret_code == rcode_io_done) {
        ret_code = rsocket_send((rsocket_t*)(ds_client->stream), data_buff + total, count, &sent_len, &tm);

        if (ret_code != rcode_io_done) {
            rwarn("end client send_data, code: %d, sent_len: %d, total: %d", ret_code, sent_len, total);

            ripc_close_c(rsocket_ctx);//直接关闭
            if (ret_code == rcode_io_timeout) {
                return rcode_err_sock_timeout;
            }
            else {
                return rcode_err_sock_disconnect;//所有未知错误都断开
            }
        }
        total += sent_len;
    }
	rdebug("end client send_data, code: %d, sent_len: %d", ret_code, sent_len);

	rbuffer_skip(ds_client->write_buff, total);
    return rcode_ok;
}

static int ripc_receive_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = rcode_io_done;
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
    int total = 0;

    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 1, -1);
    rtimeout_start(&tm);

    ret_code = rcode_io_done;
    while (ret_code == rcode_io_done) {
        ret_code = rsocket_recv(ds_client->stream, data_buff + total, count, &received_len, &tm);
        if (received_len == 0) {
            break;
        }
        total += received_len;
    }

    if (ret_code == rcode_io_timeout) {
        ret_code = rcode_io_done;
    } else if (ret_code == rcode_io_closed) {//udp & tcp
        if (total > 0) {//有可能已经关闭了，下一次再触发会是0
            ret_code = rcode_io_done;
        }
        else {
            ret_code = rcode_io_closed;
        }
    }

    if (ret_code != rcode_io_done) {
        rwarn("end client send_data, code: %d, received_len: %d, total: %d", ret_code, received_len, total);

        ripc_close_c(rsocket_ctx);//直接关闭
        if (ret_code == rcode_io_timeout) {
            return rcode_err_sock_timeout;
        }
        else {
            return rcode_err_sock_disconnect;//所有未知错误都断开
        }
    }

    if(total > 0 && rsocket_ctx->in_handler) {
        data_raw.len = total;

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

    return ripc_receive_data_c(ds, data);
}

static const ripc_entry_t impl_c = {
    (ripc_init_func)ripc_init_c,// ripc_init_func init;
    (ripc_uninit_func)ripc_uninit_c,// ripc_uninit_func uninit;
    (ripc_open_func)ripc_open_c,// ripc_open_func open;
    (ripc_close_func)ripc_close_c,// ripc_close_func close;
    (ripc_start_func)ripc_start_c,// ripc_start_func start;
    (ripc_stop_func)ripc_stop_c,// ripc_stop_func stop;
    (ripc_send_func)ripc_send_data_c,// ripc_send_func send;
    (ripc_check_func)ripc_check_data_c,// ripc_check_func check;
    (ripc_receive_func)ripc_receive_data_c,// ripc_receive_func receive;
    NULL// ripc_error_func error;
};
const ripc_entry_t* rsocket_select_c = &impl_c;//linux默认poll代替select

const ripc_entry_t* rsocket_c = &impl_c;//linux默认client为poll

#undef SOCKET_INVALID
#undef WAITFD_R
#undef WAITFD_W
#undef WAITFD_C
