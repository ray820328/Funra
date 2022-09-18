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
#include "rsocket_c.h"
#include "rsocket_s.h"
#include "rtools.h"

#include <winsock2.h>
#include <ws2tcpip.h>

static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

/* timeout control structure */
typedef struct rtimeout_s {
    double block;          /* maximum time for blocking calls */
    double total;          /* total number of miliseconds for operation */
    double start;          /* time of start of operation */
} rtimeout_t;

typedef struct sockaddr SA;

typedef int rsock_len_t;
typedef SOCKADDR_STORAGE t_sockaddr_storage;
typedef SOCKET rsocket_t;

#define rtimeout_is_zero(tm)   ((tm)->block == 0.0)

#ifdef _WIN32
static double rtimeout_gettime(void) {
    FILETIME ft;
    double t;
    GetSystemTimeAsFileTime(&ft);
    /* Windows file time (time since January 1, 1601 (UTC)) */
    t  = ft.dwLowDateTime/1.0e7 + ft.dwHighDateTime*(4294967296.0/1.0e7);
    /* convert to Unix Epoch time (time since January 1, 1970 (UTC)) */
    return (t - 11644473600.0);
}
#else
double rtimeout_gettime(void) {
    struct timeval v;
    gettimeofday(&v, (struct timezone *) NULL);
    /* Unix Epoch time (time since January 1, 1970 (UTC)) */
    return v.tv_sec + v.tv_usec/1.0e6;
}
#endif

static void rtimeout_init(rtimeout_t* tm, double block, double total) {
    tm->block = block;
    tm->total = total;
}

static double rtimeout_get(rtimeout_t* tm) {
    if (tm->block < 0.0 && tm->total < 0.0) {
        return -1;
    } else if (tm->block < 0.0) {
        double t = tm->total - rtimeout_gettime() + tm->start;
        return rmax(t, 0.0);
    } else if (tm->total < 0.0) {
        return tm->block;
    } else {
        double t = tm->total - rtimeout_gettime() + tm->start;
        return rmin(tm->block, rmax(t, 0.0));
    }
}

static double rtimeout_getstart(rtimeout_t* tm) {
    return tm->start;
}

static double rtimeout_getretry(rtimeout_t* tm) {
    if (tm->block < 0.0 && tm->total < 0.0) {
        return -1;
    } else if (tm->block < 0.0) {
        double t = tm->total - rtimeout_gettime() + tm->start;
        return rmax(t, 0.0);
    } else if (tm->total < 0.0) {
        double t = tm->block - rtimeout_gettime() + tm->start;
        return rmax(t, 0.0);
    } else {
        double t = tm->total - rtimeout_gettime() + tm->start;
        return rmin(tm->block, rmax(t, 0.0));
    }
}

static rtimeout_t* rtimeout_markstart(rtimeout_t* tm) {
    tm->start = rtimeout_gettime();
    return tm;
}

enum {
    IO_DONE = 0,        /* operation completed successfully */
    IO_TIMEOUT = -1,    /* operation timed out */
    IO_CLOSED = -2,     /* the connection has been closed */
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

// typedef struct {
//     ripc_data_source_t* ds;
//     int write_size;
// } local_write_req_t;

#ifdef WIN32
/* POSIX defines 1024 for the FD_SETSIZE */
#define FD_SETSIZE 1024
#endif

static char *wstrerror(int err);
static char *socket_strerror(int err);
static char *socket_ioerror(rsocket_t* ps, int err);
static char *socket_hoststrerror(int err);
static char *rsocket_gaistrerror(int err);

static int rsocket_setblocking(rsocket_t* ps) {
    u_long argp = 0;
    ioctlsocket(*ps, FIONBIO, &argp);
    return rcode_ok;
}

static int rsocket_setnonblocking(rsocket_t* ps) {
    u_long argp = 1;
    ioctlsocket(*ps, FIONBIO, &argp);
    return rcode_ok;
}

static int rsocket_open(void) {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0);
    int err = WSAStartup(wVersionRequested, &wsaData );
    if (err != 0) return err;
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

#define WAITFD_R        1
#define WAITFD_W        2
#define WAITFD_E        4
#define WAITFD_C        (WAITFD_E|WAITFD_W)

static int rsocket_waitfd(rsocket_t* ps, int sw, rtimeout_t* tm) {
    int ret;
    fd_set rfds, wfds, efds, *rp = NULL, *wp = NULL, *ep = NULL;
    struct timeval tv, *tp = NULL;
    double t;
    if (rtimeout_is_zero(tm)) return IO_TIMEOUT;  /* optimize timeout == 0 case */
    if (sw & WAITFD_R) {
        FD_ZERO(&rfds);
        FD_SET(*ps, &rfds);
        rp = &rfds;
    }
    if (sw & WAITFD_W) { FD_ZERO(&wfds); FD_SET(*ps, &wfds); wp = &wfds; }
    if (sw & WAITFD_C) { FD_ZERO(&efds); FD_SET(*ps, &efds); ep = &efds; }
    if ((t = rtimeout_get(tm)) >= 0.0) {
        tv.tv_sec = (int) t;
        tv.tv_usec = (int) ((t-tv.tv_sec)*1.0e6);
        tp = &tv;
    }
    ret = select(0, rp, wp, ep, tp);
    if (ret == -1) return WSAGetLastError();
    if (ret == 0) return IO_TIMEOUT;
    if (sw == WAITFD_C && FD_ISSET(*ps, &efds)) return IO_CLOSED;
    return IO_DONE;
}

static int rsocket_select(rsocket_t rsock, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm) {
    struct timeval tv;
    double t = rtimeout_get(tm);
    tv.tv_sec = (int) t;
    tv.tv_usec = (int) ((t - tv.tv_sec) * 1.0e6);
    if (rsock <= 0) {
        Sleep((DWORD) (1000 * t));
        return 0;
    } else {
        return select(0, rfds, wfds, efds, t >= 0.0 ? &tv: NULL);
    }
}

static int rsocket_destroy(rsocket_t* ps) {
    if (*ps != SOCKET_INVALID) {
        rsocket_setblocking(ps); /* WIN32可能消耗时间很长 */
        closesocket(*ps);
        *ps = SOCKET_INVALID;
    }

    return rcode_ok;
}

static int rsocket_shutdown(rsocket_t* ps, int how) {
    rsocket_setblocking(ps);
    shutdown(*ps, how);
    rsocket_setnonblocking(ps);

    return rcode_ok;
}

static int rsocket_create(rsocket_t* ps, int domain, int type, int protocol) {
    *ps = socket(domain, type, protocol);
    if (*ps != SOCKET_INVALID) {
        return rcode_ok;
    }
    else {
        return WSAGetLastError();
    }
}

static int rsocket_connect(rsocket_t* ps, SA *addr, rsock_len_t len, rtimeout_t* tm) {
    int err;
	if (*ps == SOCKET_INVALID) {
		return IO_CLOSED;
	}

	if (connect(*ps, addr, len) == 0) {
		return IO_DONE;
	}
    
    err = WSAGetLastError();
	if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
		return err;
	}
    
	if (rtimeout_is_zero(tm)) {
		return IO_TIMEOUT;
	}
    /* wait until something happens */
    err = rsocket_waitfd(ps, WAITFD_C, tm);
    if (err == IO_CLOSED) {
        int elen = sizeof(err);
        /* give windows time to set the error (yes, disgusting) */
        Sleep(10);

        getsockopt(*ps, SOL_SOCKET, SO_ERROR, (char *)&err, &elen);
        
        return err > 0 ? err : IO_UNKNOWN;
	}
	else {
		return err;
	}
}

static int rsocket_bind(rsocket_t* ps, SA *addr, rsock_len_t len) {
    int err = IO_DONE;
    rsocket_setblocking(ps);
    if (bind(*ps, addr, len) < 0) err = WSAGetLastError();
    rsocket_setnonblocking(ps);
    return err;
}

static int rsocket_listen(rsocket_t* ps, int backlog) {
    int err = IO_DONE;
    rsocket_setblocking(ps);
    if (listen(*ps, backlog) < 0) err = WSAGetLastError();
    rsocket_setnonblocking(ps);
    return err;
}

static int rsocket_accept(rsocket_t* ps, rsocket_t* pa, SA *addr, rsock_len_t *len, rtimeout_t* tm) {
    if (*ps == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        int err;
        /* try to get client socket */
        if ((*pa = accept(*ps, addr, len)) != SOCKET_INVALID) return IO_DONE;
        /* find out why we failed */
        err = WSAGetLastError();
        /* if we failed because there was no connectoin, keep trying */
        if (err != WSAEWOULDBLOCK && err != WSAECONNABORTED) return err;
        /* call select to avoid busy wait */
        if ((err = rsocket_waitfd(ps, WAITFD_R, tm)) != IO_DONE) return err;
    }
}

/** windows单次尽量不要发送1m以上的数据，卡死 **/
static int rsocket_send(rsocket_t* ps, const char *data, int count, int *sent, rtimeout_t* tm) {
    int err;
    *sent = 0;
    /* avoid making system calls on closed sockets */
	if (*ps == SOCKET_INVALID) {
		rinfo("990");
		return IO_CLOSED;
	}
    /* loop until we send something or we give up on error */
    for ( ;; ) {
        /* try to send something */
        int put = send(*ps, data, (int) count, 0);
		rinfo("999");
        /* if we sent something, we are done */
        if (put > 0) {
            *sent = put;
            return IO_DONE;
        }
        /* deal with failure */
        err = WSAGetLastError();
        /* we can only proceed if there was no serious error */
		if (err != WSAEWOULDBLOCK) {
			return err;
		}
        /* avoid busy wait */
		if ((err = rsocket_waitfd(ps, WAITFD_W, tm)) != IO_DONE) {
			return err;
		}
    }
}

static int rsocket_sendto(rsocket_t* ps, const char *data, size_t count, size_t *sent,
        SA *addr, rsock_len_t len, rtimeout_t* tm) {
    int err;
    *sent = 0;
    if (*ps == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        int put = sendto(*ps, data, (int) count, 0, addr, len);
        if (put > 0) {
            *sent = put;
            return IO_DONE;
        }
        err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) return err;
        if ((err = rsocket_waitfd(ps, WAITFD_W, tm)) != IO_DONE) return err;
    }
}

static int rsocket_recv(rsocket_t* ps, char *data, size_t count, size_t *got, rtimeout_t* tm) {
    int err, prev = IO_DONE;
    *got = 0;
    if (*ps == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        int taken = recv(*ps, data, (int) count, 0);
        if (taken > 0) {
            *got = taken;
            return IO_DONE;
        }
        if (taken == 0) return IO_CLOSED;
        err = WSAGetLastError();
        /* On UDP, a connreset simply means the previous send failed.
         * So we try again.
         * On TCP, it means our socket is now useless, so the error passes.
         * (We will loop again, exiting because the same error will happen) */
        if (err != WSAEWOULDBLOCK) {
            if (err != WSAECONNRESET || prev == WSAECONNRESET) return err;
            prev = err;
        }
        if ((err = rsocket_waitfd(ps, WAITFD_R, tm)) != IO_DONE) return err;
    }
}

static int rsocket_recvfrom(rsocket_t* ps, char *data, size_t count, size_t *got, SA *addr, rsock_len_t *len, rtimeout_t* tm) {
    int err, prev = IO_DONE;
    *got = 0;
    if (*ps == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        int taken = recvfrom(*ps, data, (int) count, 0, addr, len);
        if (taken > 0) {
            *got = taken;
            return IO_DONE;
        }
        if (taken == 0) return IO_CLOSED;
        err = WSAGetLastError();
        /* On UDP, a connreset simply means the previous send failed.
         * So we try again.
         * On TCP, it means our socket is now useless, so the error passes.
         * (We will loop again, exiting because the same error will happen) */
        if (err != WSAEWOULDBLOCK) {
            if (err != WSAECONNRESET || prev == WSAECONNRESET) return err;
            prev = err;
        }
        if ((err = rsocket_waitfd(ps, WAITFD_R, tm)) != IO_DONE) return err;
    }
}

static int rsocket_gethostbyaddr(const char *addr, rsock_len_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) return IO_DONE;
    else return WSAGetLastError();
}

static int rsocket_gethostbyname(const char *addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) return IO_DONE;
    else return  WSAGetLastError();
}


static char *socket_hoststrerror(int err) {
    if (err <= 0) return io_strerror(err);
    switch (err) {
        case WSAHOST_NOT_FOUND: return PIE_HOST_NOT_FOUND;
        default: return wstrerror(err);
    }
}

static char *socket_strerror(int err) {
    if (err <= 0) return io_strerror(err);
    switch (err) {
        case WSAEADDRINUSE: return PIE_ADDRINUSE;
        case WSAECONNREFUSED : return PIE_CONNREFUSED;
        case WSAEISCONN: return PIE_ISCONN;
        case WSAEACCES: return PIE_ACCESS;
        case WSAECONNABORTED: return PIE_CONNABORTED;
        case WSAECONNRESET: return PIE_CONNRESET;
        case WSAETIMEDOUT: return PIE_TIMEDOUT;
        default: return wstrerror(err);
    }
}

static char *socket_ioerror(rsocket_t* ps, int err) {
    (void) ps;
    return socket_strerror(err);
}

static char *wstrerror(int err) {
    switch (err) {
        case WSAEINTR: return "Interrupted function call";
        case WSAEACCES: return PIE_ACCESS; // "Permission denied";
        case WSAEFAULT: return "Bad address";
        case WSAEINVAL: return "Invalid argument";
        case WSAEMFILE: return "Too many open files";
        case WSAEWOULDBLOCK: return "Resource temporarily unavailable";
        case WSAEINPROGRESS: return "Operation now in progress";
        case WSAEALREADY: return "Operation already in progress";
        case WSAENOTSOCK: return "Socket operation on nonsocket";
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
        case WSATRY_AGAIN: return "Nonauthoritative host not found";
        case WSANO_RECOVERY: return PIE_FAIL; // "Nonrecoverable name lookup error";
        case WSANO_DATA: return "Valid name, no data record of requested type";
        default: return "Unknown error";
    }
}

static char *rsocket_gaistrerror(int err) {
    if (err == 0) return NULL;
    switch (err) {
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
        default: return gai_strerror(err);
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

    rtimeout_init(&tm, 5, -1);
    rtimeout_markstart(&tm);

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

        ret_code = rsocket_connect(rsock_item, (SA *)iterator->ai_addr, (socklen_t)iterator->ai_addrlen, &tm);
        /* success or timeout, break of loop */
        if (ret_code == rcode_ok) {
            family = current_family;
            rinfo("success on connect, family = %d", family);
            break;
        }
        if (rtimeout_is_zero(&tm)) {
            ret_code = WSAETIMEDOUT;
        }
        if (ret_code != rcode_ok) {
            rinfo("failed on connect, code = %d, msg = %s", ret_code, socket_strerror(ret_code));
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
    //        err = inet_trycreate(&rsock_item, iterator->ai_family,
    //            iterator->ai_socktype, iterator->ai_protocol);
    //        if (err) continue;
    //        current_family = iterator->ai_family;
    //    }
    //    /* try binding to local address */
    //    err = socket_strerror(rsocket_bind(&rsock_item, (SA *)iterator->ai_addr, (socklen_t)iterator->ai_addrlen));
    //    /* keep trying unless bind succeeded */
    //    if (err == NULL) {
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
static void send_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = 0;
	rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
	//rsocket_cfg_t* cfg = rsocket_ctx->cfg;

	if (rsocket_ctx->out_handler) {
		ret_code = rsocket_ctx->out_handler->process(rsocket_ctx->out_handler, ds_client, data);
		if (ret_code != ripc_code_success) {
			rerror("error on handler process, code: %d", ret_code);
			return;
		}
	}

	const char* data_buff = rbuffer_read_start_dest(ds_client->write_buff);
	int count = rbuffer_size(ds_client->write_buff);
	int* sent_len = 0;

	rtimeout_t tm;
	rtimeout_init(&tm, 2, -1);
	rtimeout_markstart(&tm);
	rinfo("11 = %p", ds_client->stream);
	ret_code = rsocket_send(ds_client->stream, data_buff, count, sent_len, &tm);
	rinfo("22 = %p", ds_client->stream);
	
	if (ret_code != IO_DONE) {
		rerror("end client send_data, code: %d, sent_len: %d", ret_code, sent_len);
		return;
	}
	rdebug("end client send_data, code: %d, sent_len: %d", ret_code, sent_len);

	rbuffer_skip(ds_client->write_buff, sent_len);
}

static const ripc_entry_t impl_c = {
    (ripc_init_func)ripc_init_c,// ripc_init_func init;
    (ripc_uninit_func)ripc_uninit_c,// ripc_uninit_func uninit;
    (ripc_open_func)ripc_open_c,// ripc_open_func open;
    (ripc_close_func)ripc_close_c,// ripc_close_func close;
    (ripc_start_func)ripc_start_c,// ripc_start_func start;
    (ripc_stop_func)ripc_stop_c,// ripc_stop_func stop;
    (ripc_send_func)send_data_c,// ripc_send_func send;
    NULL,// ripc_check_func check;
    NULL,// ripc_receive_func receive;
    NULL// ripc_error_func error;
};
const ripc_entry_t* rsocket_select_c = &impl_c;


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
//    int ret = 1;
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
//        ret = -1;
//    }
//    freeaddrinfo(res);
//    return ret;
//}