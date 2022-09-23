/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

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

static enum {
    IO_DONE = 0, //操作成功
    IO_TIMEOUT = -1,//操作超时
    IO_CLOSED = -2,//连接已关闭
    IO_UNKNOWN = -3
};

static char *io_strerror(int err) {
    switch (err) {
    case IO_DONE: return NULL;
    case IO_CLOSED: return "closed";
    case IO_TIMEOUT: return "timeout";
    default: return "unknown error";
    }
}

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

#define SOCKET_INVALID (INVALID_SOCKET)

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV (0)
#endif

#define WAITFD_R        1
#define WAITFD_W        2
#define WAITFD_E        4
#define WAITFD_C        (WAITFD_E|WAITFD_W)

static char *wstrerror(int ret_code);
static char *rsocket_strerror(int ret_code);
static char *rsocket_ioerror(rsocket_t* sock_item, int ret_code);
static char *rsocket_hoststrerror(int ret_code);
static char *rsocket_gaistrerror(int ret_code);

static int rsocket_setblocking(rsocket_t* sock_item) {
    u_long argp = 0;
    ioctlsocket(*sock_item, FIONBIO, &argp);
    return rcode_ok;
}

static int rsocket_setnonblocking(rsocket_t* sock_item) {
    u_long argp = 1;
    ioctlsocket(*sock_item, FIONBIO, &argp);
    return rcode_ok;
}

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

static int rsocket_waitfd(rsocket_t* sock_item, int sw, rtimeout_t* tm) {
    int ret_code = 0;
    fd_set rfds, wfds, efds, *rp = NULL, *wp = NULL, *ep = NULL;
    struct timeval tv, *tp = NULL;
    int64_t time_left = 0;

    if (rtimeout_done(tm)) {
        return IO_TIMEOUT;
    }

    //select使用bit，fd_set内核和用户态共同修改，要么保存状态，要么每次重新分别设置 read/write/error 掩码
    if (sw & WAITFD_R) {
        FD_ZERO(&rfds);
        FD_SET(*sock_item, &rfds);
        rp = &rfds;
    }
    if (sw & WAITFD_W) { 
        FD_ZERO(&wfds); 
        FD_SET(*sock_item, &wfds); 
        wp = &wfds; 
    }
    if (sw & WAITFD_C) { 
        FD_ZERO(&efds); 
        FD_SET(*sock_item, &efds); 
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
        return IO_TIMEOUT;
    }
    if (sw == WAITFD_C && FD_ISSET(*sock_item, &efds)) {
        return IO_CLOSED;
    }

    return IO_DONE;
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

static int rsocket_destroy(rsocket_t* sock_item) {
    if (*sock_item != SOCKET_INVALID) {
        rsocket_setblocking(sock_item); /* WIN32可能消耗时间很长 */
        closesocket(*sock_item);
        *sock_item = SOCKET_INVALID;
    }

    return rcode_ok;
}

static int rsocket_shutdown(rsocket_t* sock_item, int how) {
    rsocket_setblocking(sock_item);
    shutdown(*sock_item, how);
    rsocket_setnonblocking(sock_item);

    return rcode_ok;
}

static int rsocket_create(rsocket_t* sock_item, int domain, int type, int protocol) {
    *sock_item = socket(domain, type, protocol);
    if (*sock_item != SOCKET_INVALID) {
        return rcode_ok;
    }
    else {
        return WSAGetLastError();
    }
}

static int rsocket_connect(rsocket_t* sock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
	if (*sock_item == SOCKET_INVALID) {
		return IO_CLOSED;
	}

	if (connect(*sock_item, addr, len) == 0) {
		return IO_DONE;
	}
    
    ret_code = WSAGetLastError();
	if (ret_code != WSAEWOULDBLOCK && ret_code != WSAEINPROGRESS) {
		return ret_code;
	}
    
	if (rtimeout_done(tm)) {
		return IO_TIMEOUT;
	}

    /* wait until something happens */
    ret_code = rsocket_waitfd(sock_item, WAITFD_C, tm);

    if (ret_code == IO_CLOSED) {
        int elen = sizeof(ret_code);
        /* give windows time to set the error (yes, disgusting) */
        Sleep(10);

        getsockopt(*sock_item, SOL_SOCKET, SO_ERROR, (char *)&ret_code, &elen);
        
        return ret_code > 0 ? ret_code : IO_UNKNOWN;
	}
	else {
		return ret_code;
	}
}

static int rsocket_bind(rsocket_t* sock_item, rsockaddr_t *addr, rsocket_len_t len) {
    int ret_code = IO_DONE;

    rsocket_setblocking(sock_item);

    if (bind(*sock_item, addr, len) < 0) {
        ret_code = WSAGetLastError();
    }

    rsocket_setnonblocking(sock_item);

    return ret_code;
}

static int rsocket_listen(rsocket_t* sock_item, int backlog) {
    int ret_code = IO_DONE;

    rsocket_setblocking(sock_item);

    if (listen(*sock_item, backlog) < 0) {
        ret_code = WSAGetLastError();
    }

    rsocket_setnonblocking(sock_item);

    return ret_code;
}

static int rsocket_accept(rsocket_t* sock_item, rsocket_t* pa, rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    if (*sock_item == SOCKET_INVALID) {
        return IO_CLOSED;
    }

    for ( ;; ) {
        int ret_code = 0;
        /* try to get client socket */
        if ((*pa = accept(*sock_item, addr, len)) != SOCKET_INVALID) {
            return IO_DONE;
        }
        /* find out why we failed */
        ret_code = WSAGetLastError();
        /* if we failed because there was no connectoin, keep trying */
        if (ret_code != WSAEWOULDBLOCK && ret_code != WSAECONNABORTED) {
            return ret_code;
        }
        /* call select to avoid busy wait */
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != IO_DONE) {
            return ret_code;
        }
    }
}

/** windows单次尽量不要发送1m以上的数据，卡死 **/
static int rsocket_send(rsocket_t* sock_item, const char *data, int count, int *sent, rtimeout_t* tm) {
    int ret_code = 0;

    *sent = 0;
	if (*sock_item == SOCKET_INVALID) {
		return IO_CLOSED;
	}

    int sent_once = 0;
    for ( ;; ) {
        sent_once = send(*sock_item, data, (int) count, 0);
        /* if we sent something, we are done */
        if (sent_once > 0) {
            *sent = sent_once;
            return IO_DONE;
        }

        ret_code = WSAGetLastError();
        /* we can only proceed if there was no serious error */
		if (ret_code != WSAEWOULDBLOCK) {
			return ret_code;
		}
        /* avoid busy wait */
		if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != IO_DONE) {
			return ret_code;
		}
    }
}

static int rsocket_send2_addr(rsocket_t* sock_item, const char *data, size_t count, size_t *sent,
        rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code = 0;

    *sent = 0;
    if (*sock_item == SOCKET_INVALID) {
        return IO_CLOSED;
    }

    int sent_once = 0;
    for ( ;; ) {
        sent_once = sendto(*sock_item, data, (int) count, 0, addr, len);
        if (sent_once > 0) {
            *sent = sent_once;
            return IO_DONE;
        }

        ret_code = WSAGetLastError();
        if (ret_code != WSAEWOULDBLOCK) {
            return ret_code;
        }
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != IO_DONE) {
            return ret_code;
        }
    }
}

static int rsocket_recv(rsocket_t* sock_item, char *data, int count, int *got, rtimeout_t* tm) {
    int ret_code = 0, prev = IO_DONE;

    *got = 0;
    if (*sock_item == SOCKET_INVALID) {
        return IO_CLOSED;
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
        recv_once = recv(*sock_item, data, (int)count, 0);

        if (recv_once > 0) {
            *got = recv_once;
            return IO_DONE;
        }
        if (recv_once == 0) {
            return IO_CLOSED;
        }

        ret_code = WSAGetLastError();
        if (ret_code == WSAEWOULDBLOCK) {
            return IO_DONE;
        } else {
            /* UDP, conn reset simply means the previous send failed. try again.
             * TCP, it means our socket is now useless, so the error passes.
             * (We will loop again, exiting because the same error will happen) */
            if (ret_code != WSAECONNRESET || prev == WSAECONNRESET) {
                return ret_code;
            }
            prev = ret_code;

            //select阻塞直到read 或者 timeout
            ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm);
            if (ret_code != IO_DONE) {
                return ret_code;
            }
        }
    }
}

static int rsocket_recvfrom(rsocket_t* sock_item, char *data, int count, int *got, 
        rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    int ret_code, prev = IO_DONE;

    *got = 0;
    if (*sock_item == SOCKET_INVALID) {
        return IO_CLOSED;
    }

    int recv_once = 0;
    for ( ;; ) {
        recv_once = recvfrom(*sock_item, data, (int) count, 0, addr, len);

        if (recv_once > 0) {
            *got = recv_once;
            return IO_DONE;
        }
        if (recv_once == 0) {
            return IO_CLOSED;
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
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != IO_DONE) {
            return ret_code;
        }
    }
}

static int rsocket_gethostbyaddr(const char *addr, rsocket_len_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) {
        return IO_DONE;
    }
    else {
        return WSAGetLastError();
    }
}

static int rsocket_gethostbyname(const char *addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) {
        return IO_DONE;
    }
    else {
        return  WSAGetLastError();
    }
}


static char *rsocket_hoststrerror(int ret_code) {
    if (ret_code <= 0) {
        return io_strerror(ret_code);
    }

    switch (ret_code) {
        case WSAHOST_NOT_FOUND: 
            return PIE_HOST_NOT_FOUND;
        default: 
            return wstrerror(ret_code);
    }
}

static char *rsocket_strerror(int ret_code) {
    if (ret_code <= 0) return io_strerror(ret_code);
    switch (ret_code) {
        case WSAEADDRINUSE: return PIE_ADDRINUSE;
        case WSAECONNREFUSED : return PIE_CONNREFUSED;
        case WSAEISCONN: return PIE_ISCONN;
        case WSAEACCES: return PIE_ACCESS;
        case WSAECONNABORTED: return PIE_CONNABORTED;
        case WSAECONNRESET: return PIE_CONNRESET;
        case WSAETIMEDOUT: return PIE_TIMEDOUT;
        default: return wstrerror(ret_code);
    }
}

static char *rsocket_ioerror(rsocket_t* sock_item, int ret_code) {
    (void) sock_item;
    return rsocket_strerror(ret_code);
}

static char *wstrerror(int ret_code) {
    switch (ret_code) {
        case WSAEINTR: return "Interrupted function call";
        case WSAEACCES: return PIE_ACCESS; // "Permission denied";
        case WSAEFAULT: return "Bad address";
        case WSAEINVAL: return "Invalid argument";
        case WSAEMFILE: return "Too many open files";
        case WSAEWOULDBLOCK: return "Resource temporarily unavailable";
        case WSAEINPROGRESS: return "Operation now in progress";
        case WSAEALREADY: return "Operation already in progress";
        case WSAENOTSOCK: return "Socket operation on non socket";
        case WSAEDESTADDRREQ: return "Destination address required";
        case WSAEMSGSIZE: return "Message too long";
        case WSAEPROTOTYPE: return "Protocol wrong type for socket";
        case WSAENOPROTOOPT: return "Bad protocol option";
        case WSAEPROTONOSUPPORT: return "Protocol not supported";
        case WSAESOCKTNOSUPPORT: return PIE_SOCKTYPE; // "Socket type not supported";
        case WSAEOPNOTSUPP: return "Operation not supported";
        case WSAEPFNOSUPPORT: return "Protocol family not supported";
        case WSAEAFNOSUPPORT: return PIE_FAMILY; // "Address family not supported by protocol family";
        case WSAEADDRINUSE: return PIE_ADDRINUSE; // "Address already in use";
        case WSAEADDRNOTAVAIL: return "Cannot assign requested address";
        case WSAENETDOWN: return "Network is down";
        case WSAENETUNREACH: return "Network is unreachable";
        case WSAENETRESET: return "Network dropped connection on reset";
        case WSAECONNABORTED: return "Software caused connection abort";
        case WSAECONNRESET: return PIE_CONNRESET; // "Connection reset by peer";
        case WSAENOBUFS: return "No buffer space available";
        case WSAEISCONN: return PIE_ISCONN; // "Socket is already connected";
        case WSAENOTCONN: return "Socket is not connected";
        case WSAESHUTDOWN: return "Cannot send after socket shutdown";
        case WSAETIMEDOUT: return PIE_TIMEDOUT; // "Connection timed out";
        case WSAECONNREFUSED: return PIE_CONNREFUSED; // "Connection refused";
        case WSAEHOSTDOWN: return "Host is down";
        case WSAEHOSTUNREACH: return "No route to host";
        case WSAEPROCLIM: return "Too many processes";
        case WSASYSNOTREADY: return "Network subsystem is unavailable";
        case WSAVERNOTSUPPORTED: return "Winsock.dll version out of range";
        case WSANOTINITIALISED:
            return "Successful WSAStartup not yet performed";
        case WSAEDISCON: return "Graceful shutdown in progress";
        case WSAHOST_NOT_FOUND: return PIE_HOST_NOT_FOUND; // "Host not found";
        case WSATRY_AGAIN: return "Non authoritative host not found";
        case WSANO_RECOVERY: return PIE_FAIL; // "Nonrecoverable name lookup error";
        case WSANO_DATA: return "Valid name, no data record of requested type";
        default: return "Unknown error";
    }
}

static char *rsocket_gaistrerror(int ret_code) {
    if (ret_code == 0) return NULL;
    switch (ret_code) {
        case EAI_AGAIN: return PIE_AGAIN;
        case EAI_BADFLAGS: return PIE_BADFLAGS;
#ifdef EAI_BADHINTS
        case EAI_BADHINTS: return PIE_BADHINTS;
#endif
        case EAI_FAIL: return PIE_FAIL;
        case EAI_FAMILY: return PIE_FAMILY;
        case EAI_MEMORY: return PIE_MEMORY;
        case EAI_NONAME: return PIE_NONAME;
#ifdef EAI_OVERFLOW
        case EAI_OVERFLOW: return PIE_OVERFLOW;
#endif
#ifdef EAI_PROTOCOL
        case EAI_PROTOCOL: return PIE_PROTOCOL;
#endif
        case EAI_SERVICE: return PIE_SERVICE;
        case EAI_SOCKTYPE: return PIE_SOCKTYPE;
#ifdef EAI_SYSTEM
        case EAI_SYSTEM: return strerror(errno);
#endif
        default: return gai_strerror(ret_code);
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

    //指向用户设定的 struct addrinfo 结构体，只能设定 ai_family、ai_socktype、ai_protocol 和 ai_flags 四个域
    memset(&connect_hints, 0, sizeof(connect_hints));
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
        return ret_code;
    }

    int current_family = family;
    for (iterator = addrinfo_result; iterator; iterator = iterator->ai_next) {
        if (current_family != iterator->ai_family || *rsock_item == SOCKET_INVALID) {
            rsocket_destroy(rsock_item);

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

	if (ret_code == rcode_ok) {
		ds_client->read_cache = NULL;
		rbuffer_init(ds_client->read_cache, read_cache_size);
		ds_client->write_buff = NULL;
		rbuffer_init(ds_client->write_buff, write_buff_size);

        ds_client->stream = rsock_item;
		rsocket_ctx->stream_state = ripc_state_ready;
	}

    //struct addrinfo bind_hints;
    ////指向用户设定的 struct addrinfo 结构体，只能设定 ai_family、ai_socktype、ai_protocol 和 ai_flags 四个域
    //memset(&bind_hints, 0, sizeof(bind_hints));
    //bind_hints.ai_socktype = socktype;//SOCK_STREAM、SOCK_DGRAM、SOCK_RAW, 设置为0表示所有类型都可以
    //bind_hints.ai_family = family;
    //bind_hints.ai_flags = AI_PASSIVE;//nodename=NULL, 返回socket地址可用于bind()函数（*.*.*.* Any），AI_NUMERICHOST则不能是域名
    ////nodename 不是NULL，那么 AI_PASSIVE 标志被忽略；
    ////未设置AI_PASSIVE标志, 返回的socket地址可以用于connect(), sendto(), 或者 sendmsg()函数。
    ////nodename 是NULL，网络地址会被设置为lookback接口地址，这种情况下，应用是想与运行在同一个主机上另一个应用通信
    //bind_hints.ai_protocol = 0;//IPPROTO_TCP、IPPROTO_UDP 等，设置为0表示所有协议

    //for (iterator = addrinfo_result; iterator; iterator = iterator->ai_next) {
    //    if (current_family != iterator->ai_family || &rsock_item == SOCKET_INVALID) {
    //        rsocket_destroy(&rsock_item);
    //        ret_code = inet_trycreate(&rsock_item, iterator->ai_family,
    //            iterator->ai_socktype, iterator->ai_protocol);
    //        if (ret_code) continue;
    //        current_family = iterator->ai_family;
    //    }
    //    /* try binding to local address */
    //    ret_code = rsocket_strerror(rsocket_bind(&rsock_item, (rsockaddr_t *)iterator->ai_addr, (socklen_t)iterator->ai_addrlen));
    //    /* keep trying unless bind succeeded */
    //    if (ret_code == NULL) {
    //        *family = current_family;
    //        /* set to non-blocking after bind */
    //        rsocket_setnonblocking(&rsock_item);
    //        break;
    //    }
    //}
    //freeaddrinfo(addrinfo_result);


    return rcode_ok;
}
static int ripc_close_c(void* ctx) {
	rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
	ripc_data_source_t* ds_client = rsocket_ctx->stream;

    rinfo("socket client close.");

    if (rsocket_ctx->stream_state == ripc_state_closed) {
        return rcode_ok;
    }

	rbuffer_release(ds_client->read_cache);
	rbuffer_release(ds_client->write_buff);

    rdata_free(rsocket_t, ds_client->stream);
    rsocket_ctx->stream_state = ripc_state_closed;

    return rcode_ok;
}
static int ripc_start_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client start.");

    return 0;
}
static int ripc_stop_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rinfo("socket client stop.");


    return rcode_ok;
}
static int ripc_send_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = IO_DONE;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    if (rsocket_ctx->stream_state != ripc_state_ready) {
        rinfo("sock not ready, state: %d", rsocket_ctx->stream_state);
        return rcode_err_sock_disconnect;
    }

    if (rsocket_ctx->out_handler) {
        ret_code = rsocket_ctx->out_handler->process(rsocket_ctx->out_handler, ds_client, data);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return ret_code;
        }
        ret_code = IO_DONE;
    }

    const char* data_buff = rbuffer_read_start_dest(ds_client->write_buff);
    int count = rbuffer_size(ds_client->write_buff);
    int sent_len = 0;//立即处理
    int total = 0;
    
    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 3, -1);
    rtimeout_start(&tm);

    while (total < count && ret_code == IO_DONE) {
        ret_code = rsocket_send((rsocket_t*)(ds_client->stream), data_buff, count, &sent_len, &tm);

        if (ret_code != IO_DONE) {
            rwarn("end client send_data, code: %d, sent_len: %d, total: %d", ret_code, sent_len, total);

            ripc_close_c(rsocket_ctx);//直接关闭
            if (ret_code == IO_TIMEOUT) {
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
    int ret_code = IO_DONE;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;
    ripc_data_raw_t data_raw;//直接在栈上

    if (rsocket_ctx->stream_state != ripc_state_ready) {
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

    ret_code = IO_DONE;
    while (ret_code == IO_DONE) {
        ret_code = rsocket_recv(ds_client->stream, data_buff, count, &received_len, &tm);
        if (received_len == 0) {
            break;
        }
        total += received_len;
    }

    if (ret_code == IO_TIMEOUT) {
        ret_code = IO_DONE;
    } else if (ret_code == IO_CLOSED) {//udp & tcp
        if (total > 0) {//有可能已经关闭了，下一次再触发会是0
            ret_code = IO_DONE;
        }
        else {
            ret_code = IO_CLOSED;
        }
    }

    if (ret_code != IO_DONE) {
        rwarn("end client send_data, code: %d, received_len: %d, total: %d", ret_code, received_len, total);

        ripc_close_c(rsocket_ctx);//直接关闭
        if (ret_code == IO_TIMEOUT) {
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
        ret_code = IO_DONE;
    }

    return rcode_ok;
}

static const ripc_entry_t impl_c = {
    (ripc_init_func)ripc_init_c,// ripc_init_func init;
    (ripc_uninit_func)ripc_uninit_c,// ripc_uninit_func uninit;
    (ripc_open_func)ripc_open_c,// ripc_open_func open;
    (ripc_close_func)ripc_close_c,// ripc_close_func close;
    (ripc_start_func)ripc_start_c,// ripc_start_func start;
    (ripc_stop_func)ripc_stop_c,// ripc_stop_func stop;
    (ripc_send_func)ripc_send_data_c,// ripc_send_func send;
    NULL,// ripc_check_func check;
    (ripc_receive_func)ripc_receive_data_c,// ripc_receive_func receive;
    NULL// ripc_error_func error;
};
const ripc_entry_t* rsocket_select_c = &impl_c;


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