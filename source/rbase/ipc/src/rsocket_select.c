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

#ifdef WIN32
 /* POSIX defines 1024 for the FD_SETSIZE，必须在winsock2.h之前定义，默认64 */
#define FD_SETSIZE 1024
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

#define WAITFD_R        1
#define WAITFD_W        2
#define WAITFD_E        4
#define WAITFD_C        (WAITFD_E|WAITFD_W)

static int rsocket_open(void) {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0);
    int ret_code = WSAStartup(wVersionRequested, &wsaData );
    if (ret_code != 0) return ret_code;
    if ((LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) &&
        (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)) {
        WSACleanup();
        return 1;
    }
    return 0;
}

static int rsocket_close(void) {
    WSACleanup();
    return 0;
}

static int rsocket_waitfd(rsocket_t* rsock_item, int sw, rtimeout_t* tm) {
    int ret_code = 0;
    fd_set rfds, wfds, efds, *rp = NULL, *wp = NULL, *ep = NULL;
    struct timeval tv, *tp = NULL;
    int64_t time_left = 0;

    if (rtimeout_done(tm)) {
        return rcode_io_timeout;
    }

    //select使用bit，fd_set内核和用户态共同修改，要么保存状态，要么每次重新分别设置 read/write/error 掩码
    if (sw & WAITFD_R) {
        FD_ZERO(&rfds);
        FD_SET(*rsock_item, &rfds);
        rp = &rfds;
    }
    if (sw & WAITFD_W) { 
        FD_ZERO(&wfds); 
        FD_SET(*rsock_item, &wfds); 
        wp = &wfds; 
    }
    if (sw & WAITFD_C) { 
        FD_ZERO(&efds); 
        FD_SET(*rsock_item, &efds); 
        ep = &efds; 
    }

    if ((time_left = rtimeout_get_block(tm)) >= 0) {
        rtimeout_2timeval(tm, &tv, time_left);
        tp = &tv;
    }

    ret_code = select(0, rp, wp, ep, tp); //timeout为timeval表示的时间戳微秒，NULL时无限等待

    if (ret_code == -1) {
        return WSAGetLastError();
    }
    if (ret_code == 0) {
        return rcode_io_timeout;
    }
    if (sw == WAITFD_C && FD_ISSET(*rsock_item, &efds)) {
        return rcode_io_closed;
    }

    return rcode_io_done;
}

static int rsocket_select(rsocket_t rsock, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm) {
    struct timeval tv;
    int64_t time_left = rtimeout_get_block(tm);
    rtimeout_2timeval(tm, &tv, time_left);
    if (rsock <= 0) {
        Sleep((DWORD) (time_left / 1000));
        return 0;
    } else {
        return select(0, rfds, wfds, efds, time_left >= 0 ? &tv: NULL);
    }
}

static int rsocket_connect(rsocket_t* rsock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
	if (*rsock_item == SOCKET_INVALID) {
		return rcode_io_closed;
	}

	if (connect(*rsock_item, addr, len) == 0) {
		return rcode_io_done;
	}
    
    ret_code = WSAGetLastError();
	if (ret_code != WSAEWOULDBLOCK && ret_code != WSAEINPROGRESS) {
		return ret_code;
	}
    
	if (rtimeout_done(tm)) {
		return rcode_io_timeout;
	}

    /* wait until something happens */
    ret_code = rsocket_waitfd(rsock_item, WAITFD_C, tm);

    if (ret_code == rcode_io_closed) {
        int elen = sizeof(ret_code);
        /* give windows time to set the error (yes, disgusting) */
        Sleep(10);

        getsockopt(*rsock_item, SOL_SOCKET, SO_ERROR, (char *)&ret_code, &elen);
        
        return ret_code > 0 ? ret_code : rcode_io_unknown;
	}
	else {
		return ret_code;
	}
}

static int rsocket_bind(rsocket_t* rsock_item, rsockaddr_t *addr, rsocket_len_t len) {
    int ret_code = rcode_io_done;

    rsocket_setblocking(rsock_item);

    if (bind(*rsock_item, addr, len) < 0) {
        ret_code = WSAGetLastError();
    }

    rsocket_setnonblocking(rsock_item);

    return ret_code;
}

static int rsocket_listen(rsocket_t* rsock_item, int backlog) {
    int ret_code = rcode_io_done;

    rsocket_setblocking(rsock_item);

    if (listen(*rsock_item, backlog) < 0) {
        ret_code = WSAGetLastError();
    }

    rsocket_setnonblocking(rsock_item);

    return ret_code;
}

static int rsocket_accept(rsocket_t* rsock_item, rsocket_t* pa, rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    if (*rsock_item == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    for ( ;; ) {
        int ret_code = 0;
        /* try to get client socket */
        if ((*pa = accept(*rsock_item, addr, len)) != SOCKET_INVALID) {
            return rcode_io_done;
        }
        /* find out why we failed */
        ret_code = WSAGetLastError();
        /* if we failed because there was no connectoin, keep trying */
        if (ret_code != WSAEWOULDBLOCK && ret_code != WSAECONNABORTED) {
            return ret_code;
        }
        /* call select to avoid busy wait */
        if ((ret_code = rsocket_waitfd(rsock_item, WAITFD_R, tm)) != rcode_io_done) {
            return ret_code;
        }
    }
}

/** windows单次尽量不要发送1m以上的数据，卡死 **/
static int rsocket_send(rsocket_t* rsock_item, const char *data, int count, int *sent, rtimeout_t* tm) {
    int ret_code = 0;

    *sent = 0;
	if (*rsock_item == SOCKET_INVALID) {
		return rcode_io_closed;
	}

    int sent_once = 0;
    for ( ;; ) {
        sent_once = send(*rsock_item, data, (int) count, 0);
        /* if we sent something, we are done */
        if (sent_once > 0) {
            *sent = sent_once;
            return rcode_io_done;
        }

        ret_code = WSAGetLastError();
        /* we can only proceed if there was no serious error */
		if (ret_code != WSAEWOULDBLOCK) {
			return ret_code;
		}
        /* avoid busy wait */
		if ((ret_code = rsocket_waitfd(rsock_item, WAITFD_W, tm)) != rcode_io_done) {
			return ret_code;
		}
    }
}

static int rsocket_send2_addr(rsocket_t* rsock_item, const char *data, size_t count, size_t *sent,
        rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code = 0;

    *sent = 0;
    if (*rsock_item == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    int sent_once = 0;
    for ( ;; ) {
        sent_once = sendto(*rsock_item, data, (int) count, 0, addr, len);
        if (sent_once > 0) {
            *sent = sent_once;
            return rcode_io_done;
        }

        ret_code = WSAGetLastError();
        if (ret_code != WSAEWOULDBLOCK) {
            return ret_code;
        }
        if ((ret_code = rsocket_waitfd(rsock_item, WAITFD_W, tm)) != rcode_io_done) {
            return ret_code;
        }
    }
}

static int rsocket_recv(rsocket_t* rsock_item, char *data, int count, int *got, rtimeout_t* tm) {
    int ret_code = 0, prev = rcode_io_done;

    *got = 0;
    if (*rsock_item == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    int recv_once = 0;
    for (;; ) {
        //EAGAIN：套接字已标记为非阻塞，而接收操作被阻塞或者接收超时
        //EBADF：sock不是有效的描述词
        //ECONNREFUSE：远程主机阻绝网络连接
        //EFAULT：内存空间访问出错
        //EINTR：操作被信号中断
        //EINVAL：参数无效
        //ENOMEM：内存不足
        //ENOTCONN：与面向连接关联的套接字尚未被连接上
        //ENOTSOCK：sock索引的不是套接字 当返回值是0时，为正常关闭连接；
        //注意：返回值 < 0 时并且(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)的情况下认为连接是正常的，继续接收
        //对非阻塞socket而言，EAGAIN不是一种错误。在VxWorks和Windows上，EAGAIN的名字叫做EWOULDBLOCK
        recv_once = recv(*rsock_item, data, (int)count, 0);

        if (recv_once > 0) {
            *got = recv_once;
            return rcode_io_done;
        }
        if (recv_once == 0) {
            return rcode_io_closed;
        }

        ret_code = WSAGetLastError();
        if (ret_code == WSAEWOULDBLOCK) {
            return rcode_io_done;
        } else {
            /* UDP, conn reset simply means the previous send failed. try again.
             * TCP, it means our socket is now useless, so the error passes.
             * (We will loop again, exiting because the same error will happen) */
            if (ret_code != WSAECONNRESET || prev == WSAECONNRESET) {
                return ret_code;
            }
            prev = ret_code;

            //select阻塞直到read 或者 timeout
            ret_code = rsocket_waitfd(rsock_item, WAITFD_R, tm);
            if (ret_code != rcode_io_done) {
                return ret_code;
            }
        }
    }
}

static int rsocket_recvfrom(rsocket_t* rsock_item, char *data, int count, int *got, 
        rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    int ret_code, prev = rcode_io_done;

    *got = 0;
    if (*rsock_item == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    int recv_once = 0;
    for ( ;; ) {
        recv_once = recvfrom(*rsock_item, data, (int) count, 0, addr, len);

        if (recv_once > 0) {
            *got = recv_once;
            return rcode_io_done;
        }
        if (recv_once == 0) {
            return rcode_io_closed;
        }

        ret_code = WSAGetLastError();
        /* UDP, conn reset simply means the previous send failed. try again.
         * TCP, it means our socket is now useless, so the error passes.
         * (We will loop again, exiting because the same error will happen) */
        if (ret_code != WSAEWOULDBLOCK) {
            if (ret_code != WSAECONNRESET || prev == WSAECONNRESET) {
                return ret_code;
            }
            prev = ret_code;
        }
        if ((ret_code = rsocket_waitfd(rsock_item, WAITFD_R, tm)) != rcode_io_done) {
            return ret_code;
        }
    }
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
    int ret_code = 0;

    ret_code = rsocket_close();
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

        ret_code = rsocket_connect(rsock_item, (rsockaddr_t *)iterator->ai_addr, (socklen_t)iterator->ai_addrlen, &tm);
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
    rtimeout_init_millisec(&tm, 3, 3);
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
    rtimeout_init_millisec(&tm, 1, 1);
    rtimeout_start(&tm);

    ret_code = rcode_io_done;
    do {
        ret_code = rsocket_recv(ds_client->stream, data_buff + total, count, &received_len, &tm);
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
const ripc_entry_t* rsocket_select_c = &impl_c;

const ripc_entry_t* rsocket_c = &impl_c;//windows默认client为select

#undef SOCKET_INVALID
#undef WAITFD_R
#undef WAITFD_W
#undef WAITFD_E
#undef WAITFD_C

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