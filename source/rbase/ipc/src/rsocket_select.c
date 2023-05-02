/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS
# pragma warning(disable: 4244) /* int -> char */
# pragma warning(disable: 4127) /* const in if condition */
#endif

#include "rtime.h"
#include "rstring.h"
#include "rlog.h"
#include "rsocket_c.h"
#include "rsocket_s.h"
#include "rtools.h"

static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

static int ripc_close_c(void* ctx);

static int rsocket_setup(void) {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0);
    int ret_code = WSAStartup(wVersionRequested, &wsaData );
    if (ret_code != 0) return ret_code;
    if ((LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) &&
        (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)) {
        WSACleanup();
        return rcode_invalid;
    }
    return rcode_ok;
}

static int rsocket_cleanup(void) {
    WSACleanup();
    return rcode_ok;
}


static int ripc_init_c(void* ctx, const void* cfg_data) {
    rinfo("socket client init.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    int ret_code = 0;

    ret_code = rsocket_setup();
    if (ret_code != rcode_ok) {
        rerror("failed on open, code = %d", ret_code);
        return ret_code;
    }
    
    return rcode_ok;
}
static int ripc_uninit_c(void* ctx) {
    rinfo("socket client uninit.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = (ripc_data_source_t*)rsocket_ctx->ds;
    int ret_code = 0;

    if (ds_client->state == ripc_state_uninit) {
        return rcode_ok;
    }

    if (ds_client->state != ripc_state_closed) {
        ret_code = ripc_close_c(ctx);
        if (ret_code != rcode_ok) {
            rerror("failed on uninit, code = %d", ret_code);
            return ret_code;
        }
    }

    ret_code = rsocket_cleanup();
    if (ret_code != rcode_ok) {
        rerror("failed on close, code = %d", ret_code);
        return ret_code;
    }

    ds_client->state = ripc_state_uninit;

    return rcode_ok;
}
static int ripc_open_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    rsocket_cfg_t* cfg = rsocket_ctx->cfg;
	ripc_data_source_t* ds_client = rsocket_ctx->ds;
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
    rsock_item->fd = SOCKET_INVALID;
    int current_family = family;

    //指向用户设定的 struct addrinfo 结构体，只能设定 ai_family、ai_socktype、ai_protocol 和 ai_flags 四个域
    memset(&connect_hints, 0, sizeof(connect_hints));
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
        rerror("failed on getaddrinfo, code = %d, msg = %s", ret_code, rsocket_gaistrerror(ret_code));
        
        rgoto(1);
    }

    for (iterator = addrinfo_result; iterator; iterator = iterator->ai_next) {
        if (current_family != iterator->ai_family || rsock_item->fd == SOCKET_INVALID) {
            rsocket_close(rsock_item);

            ret_code = rsocket_create(rsock_item, family, socktype, protocol);
            if (ret_code != rcode_ok) {
                rinfo("failed on try create sock, code = %d", ret_code);
                continue;
            }

            if (family == AF_INET6) {
                setsockopt(rsock_item->fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&opt, sizeof(opt));
            }
            rsocket_setnonblocking(rsock_item);

            current_family = iterator->ai_family;
        }

        ret_code = rsocket_connect(rsock_item, (rsockaddr_t *)iterator->ai_addr, (socklen_t)iterator->ai_addrlen, &tm);
        /* success or timeout, break of loop */
        if (ret_code == rcode_ok) {
            family = current_family;
            rinfo("success on connect, family = %d", family);
            break;
        }
        if (rtimeout_done(&tm)) {
            ret_code = rcode_err_ipc_timeout;
        }
        if (ret_code != rcode_ok) {
            rinfo("failed on connect, code = %d, msg = %s", ret_code, rsocket_strerror(ret_code));
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
    ds_client->state = ripc_state_ready;

    return rcode_ok;

exit1:
    rsocket_close(rsock_item);
    rsocket_destroy(rsock_item);
    return ret_code;
}
static int ripc_close_c(void* ctx) {
	rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
	ripc_data_source_t* ds_client = rsocket_ctx->ds;

    rinfo("socket client close.");

    if (ds_client->state == ripc_state_closed || ds_client->stream == NULL) {
        return rcode_ok;
    }

    if (ds_client->state == ripc_state_ready || 
            ds_client->state == ripc_state_disconnect || 
            ds_client->state == ripc_state_start || 
            ds_client->state == ripc_state_stop) {
    	rbuffer_release(ds_client->read_cache);
    	rbuffer_release(ds_client->write_buff);

        rsocket_close((rsocket_t*)(ds_client->stream));
        rsocket_destroy((rsocket_t*)ds_client->stream);
        ds_client->stream = NULL;
    }

    ds_client->state = ripc_state_closed;

    return rcode_ok;
}
static int ripc_start_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->ds;

    rinfo("socket client start.");

    ds_client->state = ripc_state_start;

    return 0;
}
static int ripc_stop_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->ds;

    rinfo("socket client stop.");

    ds_client->state = ripc_state_stop;

    return rcode_ok;
}
static int ripc_send_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = rcode_io_done;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    if (ds_client->state != ripc_state_start) {
        rinfo("sock not ready, state: %d", ds_client->state);
        return rcode_err_ipc_disconnect;
    }

    if (rsocket_ctx->out_handler) {
        ret_code = rsocket_ctx->out_handler->process(rsocket_ctx->out_handler, ds_client, data);
        if (ret_code != rcode_err_ok) {
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
    rtimeout_init_millisec(&tm, 3, 3);
    rtimeout_start(&tm);

    while (total < count && ret_code == rcode_io_done) {
        ret_code = rsocket_send((rsocket_t*)(ds_client->stream), data_buff + total, count, &sent_len, &tm);

        if (ret_code != rcode_io_done) {
            rwarn("end client send_data, code: %d, sent_len: %d, total: %d", ret_code, sent_len, total);

            ripc_close_c(rsocket_ctx);//直接关闭
            if (ret_code == rcode_io_timeout) {
                return rcode_err_ipc_timeout;
            }
            else {
                return rcode_err_ipc_disconnect;//所有未知错误都断开
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

    if (ds_client->state != ripc_state_start) {
        rinfo("sock not ready, state: %d", ds_client->state);
        return rcode_err_ipc_disconnect;
    }
    
    char* data_buff = rbuffer_write_start_dest(ds_client->read_cache);
    int count = rbuffer_left(ds_client->read_cache);

    int received_len = 0;
    int total = 0;

    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 1, 1);
    rtimeout_start(&tm);

    ret_code = rcode_io_done;
    do {
        ret_code = rsocket_recv((rsocket_t*)ds_client->stream, data_buff + total, count, &received_len, &tm);
        if (received_len == 0) {
            break;
        }
        total += received_len;
    } while (ret_code == rcode_io_done);

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
        rwarn("end client recv_data, code: %d, received_len: %d, total: %d", ret_code, received_len, total);

        ripc_close_c(rsocket_ctx);//直接关闭
        if (ret_code == rcode_io_timeout) {
            return rcode_err_ipc_timeout;
        }
        else {
            return rcode_err_ipc_disconnect;//所有未知错误都断开
        }
    }

    if(total > 0 && rsocket_ctx->in_handler) {
        data_raw.len = total;

        ret_code = rsocket_ctx->in_handler->process(rsocket_ctx->in_handler, ds_client, &data_raw);
        if (ret_code != rcode_err_ok) {
            rerror("error on handler process, code: %d", ret_code);
            return rcode_err_ipc_decode;
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
const ripc_entry_t* rsocket_select_c = &impl_c;

const ripc_entry_t* rsocket_c = &impl_c;//windows默认client为select


//int inet_aton(const char *cp, struct in_addr *inp) {
//    unsigned int a = 0, b = 0, c = 0, d = 0;
//    int n = 0, r;
//    unsigned long int addr = 0;
//    r = sscanf(cp, "%u.%u.%u.%u%n", &a, &b, &c, &d, &n);
//    if (r == 0 || n == 0) return 0;
//    cp += n;
//    if (*cp) return 0;
//    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
//    if (inp) {
//        addr += a; addr <<= 8;
//        addr += b; addr <<= 8;
//        addr += c; addr <<= 8;
//        addr += d;
//        inp->s_addr = htonl(addr);
//    }
//    return 1;
//}
//
//int inet_pton(int af, const char *src, void *dst) {
//    struct addrinfo hints, *res;
//    int ret_code = 1;
//    memset(&hints, 0, sizeof(struct addrinfo));
//    hints.ai_family = af;
//    hints.ai_flags = AI_NUMERICHOST;
//    if (getaddrinfo(src, NULL, &hints, &res) != 0) return -1;
//    if (af == AF_INET) {
//        struct sockaddr_in *in = (struct sockaddr_in *) res->ai_addr;
//        memcpy(dst, &in->sin_addr, sizeof(in->sin_addr));
//    }
//    else if (af == AF_INET6) {
//        struct sockaddr_in6 *in = (struct sockaddr_in6 *) res->ai_addr;
//        memcpy(dst, &in->sin6_addr, sizeof(in->sin6_addr));
//    }
//    else {
//        ret_code = -1;
//    }
//    freeaddrinfo(res);
//    return ret_code;
//}