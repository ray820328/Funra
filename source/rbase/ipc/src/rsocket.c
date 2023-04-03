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
#include "rtime.h"
#include "rsocket.h"

#define rsocket_linger_max_secs 30

int rsocket_create(rsocket_t* rsock_item, int domain, int type, int protocol) {
    rsock_item->fd = socket(domain, type, protocol);
    if (rsock_item->fd != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)


#else   
        int rv = 0;
        if ((rv = rsocket_setopt(rsock_item, RSO_REUSEADDR, true)) != rcode_ok) {
            return rv;
        }
#endif
        return rcode_io_done;
    } else {
        return rerror_get_osnet_err();
    }
}

int rsocket_close(rsocket_t* rsock_item) {
    if (rsock_item && rsock_item->fd != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)
        rsocket_setblocking(rsock_item); /* WIN32可能消耗时间很长，SO_LINGER */
        closesocket(rsock_item->fd);
#else
        close(rsock_item->fd);
#endif
        rtrace("close socket, %p", rsock_item);
        rsock_item->fd = SOCKET_INVALID;
    }
    return rcode_ok;
}

int rsocket_shutdown(rsocket_t* rsock_item, int how) {
    int ret_code = 0;
    if (rsock_item && rsock_item->fd != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)
        rsocket_setblocking(rsock_item);
        ret_code = shutdown(rsock_item->fd, how);//how: SD_RECEIVE，SD_SEND，SD_BOTH
        rsocket_setnonblocking(rsock_item);
#else
        ret_code = shutdown(rsock_item->fd, how);
#endif
        if (ret_code < 0) {
            rerror("error on shutdown %p, %d - %d", rsock_item, how, ret_code);
        }
        rtrace("shutdown socket, %p", rsock_item);
        rsock_item->fd = SOCKET_INVALID;
    }
    return ret_code;
}

int rsocket_destroy(rsocket_t* rsock_item) {
    int ret_code = rcode_ok;

    rdata_free(rsocket_t, rsock_item);
    return ret_code;
}

char* rio_strerror(int err) {
    switch (err) {
    case rcode_io_done: return rstr_null;
    case rcode_io_closed: return "closed";
    case rcode_io_timeout: return "timeout";
    default: return "unknown error";
    }
}


#if defined(__linux__)

#include <sys/poll.h>
#include <sys/epoll.h>

int rsocket_setopt(rsocket_t* rsock_item, uint32_t option, bool on) {
    int flag = 0;
    int ret_code = rcode_io_done;

    flag = on ? 1 : 0;

    switch(option) {
    case RSO_KEEPALIVE:
#ifdef SO_KEEPALIVE
        if (on != rsocket_check_option(rsock_item, RSO_KEEPALIVE)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    case RSO_DEBUG:
        if (on != rsocket_check_option(rsock_item, RSO_DEBUG)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_DEBUG, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    case RSO_BROADCAST:
#ifdef SO_BROADCAST
        if (on != rsocket_check_option(rsock_item, RSO_BROADCAST)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_BROADCAST, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    case RSO_REUSEADDR:
        if (on != rsocket_check_option(rsock_item, RSO_REUSEADDR)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    case RSO_SNDBUF:
#ifdef SO_SNDBUF
        if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_SNDBUF, (void *)&on, sizeof(int)) == -1) {
            ret_code = rerror_get_osnet_err();
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    case RSO_RCVBUF:
#ifdef SO_RCVBUF
        if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_RCVBUF, (void *)&on, sizeof(int)) == -1) {
            ret_code = rerror_get_osnet_err();
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    case RSO_NONBLOCK:
        if (on != rsocket_check_option(rsock_item, RSO_NONBLOCK)) {
            if (on) {
                if ((ret_code = rsocket_setnonblocking(rsock_item)) != rcode_ok) {
                    ret_code = ret_code;
                }
            } else {
                if ((ret_code = rsocket_setblocking(rsock_item)) != rcode_ok) {
                    ret_code = ret_code;
                }
            }
        }
        break;
    case RSO_LINGER:
#ifdef SO_LINGER
        if (on != rsocket_check_option(rsock_item, RSO_LINGER)) {
            struct linger li;
            li.l_onoff = on;
            li.l_linger = rsocket_linger_max_secs;
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    case RTCP_DEFER_ACCEPT:
#if defined(TCP_DEFER_ACCEPT)
        if (on != rsocket_check_option(rsock_item, RTCP_DEFER_ACCEPT)) {
            int optlevel = IPPROTO_TCP;
            int optname = TCP_DEFER_ACCEPT;

            if (setsockopt(rsock_item->fd, optlevel, optname,  (void *)&on, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    case RTCP_NODELAY:
#if defined(TCP_NODELAY)
        if (on != rsocket_check_option(rsock_item, RTCP_NODELAY)) {
            int optlevel = IPPROTO_TCP;
            int optname = TCP_NODELAY;
            // if (rsock_item->protocol == IPPROTO_SCTP) {
            //     optlevel = IPPROTO_SCTP;
            //     optname = SCTP_NODELAY;
            // }
            if (setsockopt(rsock_item->fd, optlevel, optname, (void *)&on, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
#else
        /* TCP_NODELAY set by default, and can't be turned off*/
        ret_code = rcode_invalid;
#endif
        break;
    case RSO_FREEBIND:
#if defined(IP_FREEBIND)
        if (setsockopt(rsock_item->fd, SOL_IP, IP_FREEBIND, (void *)&flag, sizeof(int)) == -1) {
            ret_code = rerror_get_osnet_err();
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    default:
        rwarn("not supported of option = %u", option);
        ret_code = rcode_invalid;
    }

    if (ret_code != rcode_ok) {
        rwarn("set option (%u) with (%d) failed, value = %o", option, on, rsock_item->options);
        return ret_code;
    }

    //设置缓存变量
    rsocket_set_option(rsock_item, option, on);

    rtrace("set option (%u) with (%d) success, value = %o", option, on, rsock_item->options);

    return rcode_io_done;
}

int rsocket_setblocking(rsocket_t* rsock_item) {
    int flags = fcntl(rsock_item->fd, F_GETFL, 0);
    flags &= (~(O_NONBLOCK));
    fcntl(rsock_item->fd, F_SETFL, flags);
    return rcode_ok;
}

int rsocket_setnonblocking(rsocket_t* rsock_item) {
    int flags = fcntl(rsock_item->fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(rsock_item->fd, F_SETFL, flags);
    return rcode_ok;
}


int rsocket_bind(rsocket_t* rsock_item, rsockaddr_t* addr, rsocket_len_t len) {
    int ret_code = rcode_ok;
    if (bind(rsock_item->fd, addr, len) < 0) {
        ret_code = rerror_get_osnet_err();
        rinfo("bind failed, code = %d", ret_code);
    }
    return ret_code;
}

int rsocket_listen(rsocket_t* rsock_item, int backlog) {
    int ret_code = rcode_ok;
    if (listen(rsock_item->fd, backlog)) {
        ret_code = rerror_get_osnet_err();
        rinfo("listen failed, code = %d", ret_code);
    }
    return ret_code;
}

int rsocket_connect(rsocket_t* rsock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code = rcode_io_done;

    /* avoid calling on closed sockets */
    if (rsock_item->fd == SOCKET_INVALID) {
        rerror("invalid socket. code = %d", ret_code);
        return rcode_io_closed;
    }
    /* call connect until done or failed without being interrupted */
    do {
        if (connect(rsock_item->fd, addr, len) == 0) {
            return rcode_io_done;
        }
    } while ((ret_code = rerror_get_osnet_err()) == EINTR);

    /* timeout */
    if (rtimeout_done(tm)) {
        rerror("connect timeout. code = %d", ret_code);
        return rcode_io_timeout;
    }

    /* if connection failed immediately, return error code */
    if (ret_code != EINPROGRESS && ret_code != EAGAIN) {
        rerror("connect failed. code = %d", ret_code);
        return ret_code;
    }

    return rcode_io_done;//epoll connect 返回 EINPROGRESS 也视为成功，监听write事件处理
}

int rsocket_accept(rsocket_t* sock_listen, rsocket_t* rsock_item, 
        rsockaddr_t* addr, rsocket_len_t* len, rtimeout_t* tm) {
    int ret_code = 0;
    if (sock_listen->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    for ( ;; ) {
// #if defined(SOCK_NONBLOCK_NOT_INHERITED)
//         //With FreeBSD accept4() (avail in 10+), O_NONBLOCK is not inherited
//         flags |= SOCK_NONBLOCK;//从外面传
//         rsock_item->fd = accept4(*(sock_listen->fd), addr, len, flags);
// #else
        rsock_item->fd = accept(sock_listen->fd, addr, len);
// #endif
        if (rsock_item->fd != SOCKET_INVALID) {
            return rcode_io_done;
        }

        ret_code = rerror_get_osnet_err();
        
        if (rtimeout_done(tm)) {
            rwarn("accept timeout. code = %d", ret_code);
            return rcode_io_timeout;
        }
        if (ret_code == EINTR) {
            continue;
        }
        if (ret_code == EAGAIN || ret_code == EWOULDBLOCK) {
            return rcode_io_done;
        }

        // ECONNABORTED： software caused connection abort, 三次握手后，客户 TCP 发送了一个 RST
        // ECONNRESET： connection reset by peer，对方复位连接
        // ETIMEDOUT： connect time out
        // EPIPE： broken pipe
        rtrace("accept with code = %d, (%d, %d, %d, %d)", ret_code, ECONNABORTED, ECONNRESET, ETIMEDOUT, EPIPE);
        return ret_code;
    }
    
    return rcode_io_unknown;
}

int rsocket_select(rsocket_t* rsock_item, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm) {
    int ret_code = 0;
    do {
        struct timeval tv;
        int64_t time_left = rtimeout_get_block(tm);
        rtimeout_2timeval(tm, &tv, time_left);
        /* timeout = 0 means no wait */
        ret_code = select(rsock_item->fd, rfds, wfds, efds, time_left >= 0 ? &tv : NULL);

    } while (ret_code < 0 && rerror_get_osnet_err() == EINTR);

    return ret_code;
}

int rsocket_send(rsocket_t* rsock_item, const char *data, size_t count, size_t *sent, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    long sent_len = 0;
    
    if (rsock_item->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }
    
    for ( ;; ) {
        // flags （同recv）
        // flags = 0,则recv/send和read/write一样的操作
        // MSG_DONTROUTE: send函数使用这个标志告诉目的主机在本地网络上面,没有必要查找表.一般用网络诊断和路由程序里面.
        // MSG_OOB: 表示可以接收和发送带外的数据.
        // MSG_PEEK: recv函数的使用标志, 只从系统缓冲区中读取内容,而不清除系统缓冲区的内容.下次读的时候,仍然是一样的内容.比如多个进程读写数据时
        // MSG_WAITALL: recv函数的使用标志,等到所有的信息到达时才返回.recv一直阻塞,直到指定的条件满足,或者是发生了错误. 
        // 1)当读到了指定的字节时,函数正常返回.返回值等于len 
        // 2)当读到了文件的结尾时,函数正常返回.返回值小于len 
        // 3)当操作发生错误时,返回-1,且设置错误为相应的错误号(rerror_get_os_err())
        // MSG_NOSIGNAL: 使用这个标志。不让其发送SIG_PIPE信号，导致程序退出。
        sent_len = (long) send(rsock_item->fd, data, count, 0);

        /* if we sent anything, we are done */
        if (sent_len >= 0) {
            *sent = sent_len;
            return rcode_io_done;
        }
        ret_code = rerror_get_osnet_err();

        if (rtimeout_done(tm)) {
            rerror("send timeout. code = %d", ret_code);
            return rcode_io_timeout;
        }

        /* EPIPE means the connection was closed */
        if (ret_code == EPIPE) {
            return rcode_io_closed;
        }
        /* EPROTOTYPE means the connection is being closed */
        if (ret_code == EPROTOTYPE || ret_code == EINTR) {
            continue;
        }
        /* if failed fatal reason, report error */
        if (ret_code != EAGAIN && ret_code != EINPROGRESS) {//INPROGRESS
            return ret_code;
        }
        return rcode_io_done;//下次外层触发

    }
    
    return rcode_io_unknown;
}

int rsocket_sendto(rsocket_t* rsock_item, const char *data, size_t count, size_t *sent,
        rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    long sent_len = 0;

    if (rsock_item->fd == SOCKET_INVALID) return rcode_io_closed;
    for ( ;; ) {
        sent_len = (long) sendto(rsock_item->fd, data, count, 0, addr, len); 

        if (sent_len >= 0) {
            *sent = sent_len;
            return rcode_io_done;
        }

        ret_code = rerror_get_osnet_err();
        if (ret_code == EPIPE) {
            return rcode_io_closed;
        }
        if (ret_code == EPROTOTYPE || ret_code == EINTR) {
            continue;
        }
        if (ret_code != EAGAIN && ret_code != EINPROGRESS) {//INPROGRESS
            return ret_code;
        }
        return rcode_io_done;//下次外层触发
    }
    return rcode_io_unknown;
}

int rsocket_recv(rsocket_t* rsock_item, char *data, size_t count, size_t *got, rtimeout_t* tm) {
    int ret_code = 0;
    *got = 0;
    long read_len = 0;

    if (rsock_item->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    for ( ;; ) {
        read_len = (long) recv(rsock_item->fd, data, count, 0);//读不到数据返回-1，errorno为EAGAIN或者EINPROGRESS

        if (read_len > 0) {
            *got = read_len;
            return rcode_io_done;
        }
        if (read_len == 0) {
            rtrace("closed by peer.");
            return rcode_io_closed;
        }

        ret_code = rerror_get_osnet_err();

        if (rtimeout_done(tm)) {
            rtrace("recv timeout. code = %d", ret_code);
            return rcode_io_timeout;
        }
        // if (read_len == 0) return rcode_io_closed;
        if (ret_code == EINTR) {
            continue;
        }
        if (ret_code != EAGAIN && ret_code != EINPROGRESS) {//INPROGRESS
            return ret_code;
        }
        return rcode_io_done;//下次外层触发
    }
    return rcode_io_unknown;
}

int rsocket_recvfrom(rsocket_t* rsock_item, char *data, int count, int *got, rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    int ret_code;
    *got = 0;
    long read_len = 0;

    if (rsock_item->fd == SOCKET_INVALID) return rcode_io_closed;
    for ( ;; ) {
        read_len = (long) recvfrom(rsock_item->fd, data, count, 0, addr, len);

        if (read_len > 0) {
            *got = read_len;
            return rcode_io_done;
        }

        ret_code = rerror_get_osnet_err();
        if (read_len == 0) {
            return rcode_io_closed;
        }

        if (ret_code == EINTR) {
            continue;
        }
        if (ret_code != EAGAIN && ret_code != EINPROGRESS) {
            return ret_code;
        }

        return rcode_io_done;//下次外层触发
    }
    return rcode_io_unknown;
}


int rsocket_gethostbyaddr(const char* addr, socklen_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) {
        return rcode_io_done;
    } else if (h_errno) {
        return h_errno;
    } else if (rerror_get_os_err()) {
        return rerror_get_os_err();
    } else {
        return rcode_io_unknown;
    }
}

int rsocket_gethostbyname(const char* addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) {
        return rcode_io_done;
    } else if (h_errno) {
        return h_errno;
    } else if (rerror_get_os_err()) {
        return rerror_get_os_err();
    } else {
        return rcode_io_unknown;
    }

    // if (addr.ss_family == AF_INET6) {
    //       ipSize = sizeof(ip);
    //       struct sockaddr_in6* addrV6 = (struct sockaddr_in6*)&addr;
    //       inet_ntop(AF_INET6, &addrV6->sin6_addr, ip, ipSize);
    //       port = ntohs(addrV6->sin6_port);
    //       rinfo("accept %s:%d socket(%d)", ip, port, *rsock_item);
    //   } else if (addr.ss_family == AF_INET) {
    //       struct sockaddr_in* addrV4 = (struct sockaddr_in*)&addr;
    //       ip = inet_ntoa(addrV4->sin_addr);
    //       port = ntohs(addrV4->sin_port);
    //       rinfo("accept %s:%d socket(%d)", ip, port, *rsock_item);
    //   } else {
    //       rassert(false, "");
    //       rinfo("accept error net family (%d)", (int)addr.ss_family);
    //   }
}

char* rsocket_hoststrerror(int err) {
    if (err <= 0) return rio_strerror(err);
    switch (err) {
        case HOST_NOT_FOUND: return PIE_HOST_NOT_FOUND;
        default: return (char*)hstrerror(err);
    }
}

char* rsocket_strerror(int err) {
    if (err <= 0) {
        return rio_strerror(err);
    }
    switch (err) {
        case EADDRINUSE: return PIE_ADDRINUSE;
        case EISCONN: return PIE_ISCONN;
        case EACCES: return PIE_ACCESS;
        case ECONNREFUSED: return PIE_CONNREFUSED;
        case ECONNABORTED: return PIE_CONNABORTED;
        case ECONNRESET: return PIE_CONNRESET;
        case ETIMEDOUT: return PIE_TIMEDOUT;
        default: {
            return (char*)strerror(err);
        }
    }
}

char* rsocket_ioerror(rsocket_t* rsock_item, int err) {
    (void) rsock_item;
    return rsocket_strerror(err);
}

char* rsocket_gaistrerror(int err) {
    if (err == 0) return rstr_null;
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
        case EAI_SYSTEM: return (char*)strerror(err);
        default: return (char*)gai_strerror(err);
    }
}

// Linux（rerror_get_os_err() table) 懒得man
// 124 EMEDIUMTYPEWrong medium type
// 123 ENOMEDIUMNo medium found
// 122 EDQUOTDisk quota exceeded
// 121 EREMOTEIORemote I/O error
// 120 EISNAMIs a named type file
// 119 ENAVAIL_No XENIX semaphores available
// 118 ENOTNAM_Not a XENIX named type file
// 117 EUCLEAN_Structure needs cleaning
// 116 ESTALEStale NFS file handle
// 115 EINPROGRESS  +Operation now in progress
// 114 EALREADY Operation already in progress
// 113 EHOSTUNREACH  No route to host
// 112 EHOSTDOWNHost is down
// 111 ECONNREFUSED  Connection refused
// 110 ETIMEDOUT+Connection timed out
// 109 ETOOMANYREFS  Too many references: cannot splice
// 108 ESHUTDOWNCannot send after transport endpoint shutdown
// 107 ENOTCONN Transport endpoint is not connected
// 106 EISCONN_Transport endpoint is already connected
// 105 ENOBUFS_No buffer space available
// 104 ECONNRESETConnection reset by peer
// 103 ECONNABORTED  Software caused connection abort
// 102 ENETRESETNetwork dropped connection on reset
// 101 ENETUNREACHNetwork is unreachable
// 100 ENETDOWN Network is down
// 99 EADDRNOTAVAIL Cannot assign requested address
// 98 EADDRINUSEAddress already in use
// 97 EAFNOSUPPORT  Address family not supported by protocol
// 96 EPFNOSUPPORT  Protocol family not supported
// 95 EOPNOTSUPPOperation not supported
// 94 ESOCKTNOSUPPORT Socket type not supported
// 93 EPROTONOSUPPORT Protocol not supported
// 92 ENOPROTOOPTProtocol not available
// 91 EPROTOTYPEProtocol wrong type for socket
// 90 EMSGSIZE+Message too long
// 89 EDESTADDRREQ  Destination address required
// 88 ENOTSOCK Socket operation on non-socket
// 87 EUSERSToo many users
// 86 ESTRPIPE Streams pipe error
// 85 ERESTART Interrupted system call should be restarted
// 84 EILSEQInvalid or incomplete multibyte or wide character
// 83 ELIBEXEC Cannot exec a shared library directly
// 82 ELIBMAX_Attempting to link in too many shared libraries
// 81 ELIBSCN_.lib section in a.out corrupted
// 80 ELIBBAD_Accessing a corrupted shared library
// 79 ELIBACC_Can not access a needed shared library
// 78 EREMCHG_Remote address changed
// 77 EBADFDFile descriptor in bad state
// 76 ENOTUNIQ Name not unique on network
// 75 EOVERFLOWValue too large for defined data type
// 74 EBADMSG +Bad message
// 73 EDOTDOT_RFS specific error
// 72 EMULTIHOPMultihop attempted
// 71 EPROTOProtocol error
// 70 ECOMM__Communication error on send
// 69 ESRMNTSrmount error
// 68 EADV_Advertise error
// 67 ENOLINK_Link has been severed
// 66 EREMOTE_Object is remote
// 65 ENOPKGPackage not installed
// 64 ENONETMachine is not on the network
// 63 ENOSR__Out of streams resources
// 62 ETIME__Timer expired
// 61 ENODATA_No data available
// 60 ENOSTRDevice not a stream
// 59 EBFONTBad font file format
// 57 EBADSLT_Invalid slot
// 56 EBADRQC_Invalid request code
// 55 ENOANONo anode
// 54 EXFULLExchange full
// 53 EBADR__Invalid request descriptor
// 52 EBADE__Invalid exchange
// 51 EL2HLTLevel 2 halted
// 50 ENOCSINo CSI structure available
// 49 EUNATCH_Protocol driver not attached
// 48 ELNRNGLink number out of range
// 47 EL3RSTLevel 3 reset
// 46 EL3HLTLevel 3 halted
// 45 EL2NSYNC Level 2 not synchronized
// 44 ECHRNGChannel number out of range
// 43 EIDRM__Identifier removed
// 42 ENOMSGNo message of desired type
// 40 ELOOP__Too many levels of symbolic links
// 39 ENOTEMPTY+Directory not empty
// 38 ENOSYS_+Function not implemented
// 37 ENOLCK_+No locks available
// 36 ENAMETOOLONG +File name too long
// 35 EDEADLK +Resource deadlock avoided
// 34 ERANGE_+Numerical result out of range
// 33 EDOM__+Numerical argument out of domain
// 32 EPIPE+Broken pipe
// 31 EMLINK_+Too many links
// 30 EROFS+Read-only file system
// 29 ESPIPE_+Illegal seek
// 28 ENOSPC_+No space left on device
// 27 EFBIG+File too large
// 26 ETXTBSY_Text file busy
// 25 ENOTTY_+Inappropriate ioctl for device
// 24 EMFILE_+Too many open files
// 23 ENFILE_+Too many open files in system
// 22 EINVAL_+Invalid argument
// 21 EISDIR_+Is a directory
// 20 ENOTDIR +Not a directory
// 19 ENODEV_+No such device
// 18 EXDEV+Invalid cross-device link
// 17 EEXIST_+File exists
// 16 EBUSY+Device or resource busy
// 15 ENOTBLK_Block device required
// 14 EFAULT_+Bad address
// 13 EACCES_+Permission denied
// 12 ENOMEM_+Cannot allocate memory
// 11 EAGAIN_+Resource temporarily unavailable
// 10 ECHILD_+No child processes
// 9 EBADF+Bad file descriptor
// 8 ENOEXEC +Exec format error
// 7 E2BIG+Argument list too long
// 6 ENXIO+No such device or address
// 5 EIO_+Input/output error
// 4 EINTR+Interrupted system call
// 3 ESRCH+No such process
// 2 ENOENT_+No such file or directory
// 1 EPERM+Operation not permitted
// 0 --__Success

#endif //__linux__




#if defined(_WIN32) || defined(_WIN64)

#define WAITFD_R        1
#define WAITFD_W        2
#define WAITFD_E        4
#define WAITFD_C        (WAITFD_E|WAITFD_W)

//todo Ray 懒得写了，win为非部署环境，默认用libuv实现
int rsocket_setopt(rsocket_t* rsock_item, uint32_t option, bool on) {
    int flag = 0;
    int ret_code = rcode_io_done;

    flag = on ? 1 : 0;

    switch(option) {
    case RSO_KEEPALIVE:
        if (on != rsocket_check_option(rsock_item, RSO_KEEPALIVE)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    case RSO_DEBUG:
        if (on != rsocket_check_option(rsock_item, RSO_DEBUG)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_DEBUG, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    case RSO_BROADCAST:
        if (on != rsocket_check_option(rsock_item, RSO_BROADCAST)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_BROADCAST, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    case RSO_REUSEADDR:
        if (on != rsocket_check_option(rsock_item, RSO_REUSEADDR)) {
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flag, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    case RSO_SNDBUF:
        if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_SNDBUF, (void *)&on, sizeof(int)) == -1) {
            ret_code = rerror_get_osnet_err();
        }
        break;
    case RSO_RCVBUF:
        if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_RCVBUF, (void *)&on, sizeof(int)) == -1) {
            ret_code = rerror_get_osnet_err();
        }
        break;
    case RSO_NONBLOCK:
        if (on != rsocket_check_option(rsock_item, RSO_NONBLOCK)) {
            if (on) {
                if ((ret_code = rsocket_setnonblocking(rsock_item)) != rcode_ok) {
                    
                }
            } else {
                if ((ret_code = rsocket_setblocking(rsock_item)) != rcode_ok) {
                    
                }
            }
        }
        break;
    case RSO_LINGER:
        if (on != rsocket_check_option(rsock_item, RSO_LINGER)) {
            struct linger li;
            li.l_onoff = on;
            li.l_linger = rsocket_linger_max_secs;
            if (setsockopt(rsock_item->fd, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    case RTCP_DEFER_ACCEPT:
#if defined(TCP_DEFER_ACCEPT)
        if (on != rsocket_check_option(rsock_item, RTCP_DEFER_ACCEPT)) {
            int optlevel = IPPROTO_TCP;
            int optname = TCP_DEFER_ACCEPT;

            if (setsockopt(rsock_item->fd, optlevel, optname, (void *)&on, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
#else
        ret_code = rcode_invalid;
#endif
        break;
    case RTCP_NODELAY:
        if (on != rsocket_check_option(rsock_item, RTCP_NODELAY)) {
            int optlevel = IPPROTO_TCP;
            int optname = TCP_NODELAY;
            // if (rsock_item->protocol == IPPROTO_SCTP) {
            //     optlevel = IPPROTO_SCTP;
            //     optname = SCTP_NODELAY;
            // }
            if (setsockopt(rsock_item->fd, optlevel, optname, (void *)&on, sizeof(int)) == -1) {
                ret_code = rerror_get_osnet_err();
            }
        }
        break;
    default:
        rwarn("not supported of option = %u", option);
        ret_code = rcode_invalid;
    }

    if (ret_code != rcode_ok) {
        rwarn("set option (%u) with (%d) failed, value = %o", option, on, rsock_item->options);
        return ret_code;
    }

    //设置缓存变量
    rsocket_set_option(rsock_item, option, on);

    rtrace("set option (%u) with (%d) success, value = %o", option, on, rsock_item->options);

    return rcode_io_done;
}

int rsocket_setblocking(rsocket_t* rsock_item) {
    u_long argp = 0;
    ioctlsocket(rsock_item->fd, FIONBIO, &argp);
    return rcode_ok;
}

int rsocket_setnonblocking(rsocket_t* rsock_item) {
    u_long argp = 1;
    ioctlsocket(rsock_item->fd, FIONBIO, &argp);
    return rcode_ok;
}

int rsocket_waitfd(rsocket_t* rsock_item, int sw, rtimeout_t* tm) {
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
        FD_SET(rsock_item->fd, &rfds);
        rp = &rfds;
    }
    if (sw & WAITFD_W) {
        FD_ZERO(&wfds);
        FD_SET(rsock_item->fd, &wfds);
        wp = &wfds;
    }
    if (sw & WAITFD_C) {
        FD_ZERO(&efds);
        FD_SET(rsock_item->fd, &efds);
        ep = &efds;
    }

    if ((time_left = rtimeout_get_block(tm)) >= 0) {
        rtimeout_2timeval(tm, &tv, time_left);
        tp = &tv;
    }

    ret_code = select(0, rp, wp, ep, tp); //timeout为timeval表示的时间戳微秒，NULL时无限等待

    if (ret_code == -1) {
        return rerror_get_osnet_err();
    }
    if (ret_code == 0) {
        return rcode_io_timeout;
    }
    if (sw == WAITFD_C && FD_ISSET(rsock_item->fd, &efds)) {
        return rcode_io_closed;
    }

    return rcode_io_done;
}

int rsocket_select(rsocket_t* rsock, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm) {
    struct timeval tv;
    int64_t time_left = rtimeout_get_block(tm);
    rtimeout_2timeval(tm, &tv, time_left);
    if (rsock->fd <= 0) {
        Sleep((DWORD)(time_left / 1000));
        return 0;
    } else {
        return select(0, rfds, wfds, efds, time_left >= 0 ? &tv : NULL);
    }
}

int rsocket_connect(rsocket_t* rsock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
    if (rsock_item->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    if (connect(rsock_item->fd, addr, len) == 0) {
        return rcode_io_done;
    }

    ret_code = rerror_get_osnet_err();
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

        getsockopt(rsock_item->fd, SOL_SOCKET, SO_ERROR, (char *)&ret_code, &elen);

        return ret_code > 0 ? ret_code : rcode_io_unknown;
    } else {
        return ret_code;
    }
}

int rsocket_bind(rsocket_t* rsock_item, rsockaddr_t *addr, rsocket_len_t len) {
    int ret_code = rcode_io_done;

    if ((ret_code = rsocket_setopt(rsock_item, RSO_NONBLOCK, false)) != rcode_ok) {
        rerror("set block error, code = %d", ret_code);
        rgoto(1);
    }

    if (bind(rsock_item->fd, addr, len) < 0) {
        ret_code = rerror_get_osnet_err();
    }

    if ((ret_code = rsocket_setopt(rsock_item, RSO_NONBLOCK, true)) != rcode_ok) {
        rerror("set non block error, code = %d", ret_code);
        rgoto(1);
    }

    return rcode_io_done;

exit1:

    return ret_code;
}

int rsocket_listen(rsocket_t* rsock_item, int backlog) {
    int ret_code = rcode_io_done;

    if ((ret_code = rsocket_setopt(rsock_item, RSO_NONBLOCK, false)) != rcode_ok) {
        rerror("set block error, code = %d", ret_code);
        rgoto(1);
    }

    if (listen(rsock_item->fd, backlog) < 0) {
        ret_code = rerror_get_osnet_err();
    }

    if ((ret_code = rsocket_setopt(rsock_item, RSO_NONBLOCK, true)) != rcode_ok) {
        rerror("set non block error, code = %d", ret_code);
        rgoto(1);
    }

    return rcode_io_done;

exit1:

    return ret_code;
}

int rsocket_accept(rsocket_t* rsock_item, rsocket_t* pa, rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    if (rsock_item->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    for (;; ) {
        int ret_code = 0;
        /* try to get client socket */
        if ((pa->fd = accept(rsock_item->fd, addr, len)) != SOCKET_INVALID) {
            return rcode_io_done;
        }
        /* find out why we failed */
        ret_code = rerror_get_osnet_err();
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
int rsocket_send(rsocket_t* rsock_item, const char *data, int count, int *sent, rtimeout_t* tm) {
    int ret_code = 0;

    *sent = 0;
    if (rsock_item->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    int sent_once = 0;
    for (;; ) {
        sent_once = send(rsock_item->fd, data, (int)count, 0);
        /* if we sent something, we are done */
        if (sent_once > 0) {
            *sent = sent_once;
            return rcode_io_done;
        }

        ret_code = rerror_get_osnet_err();
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

int rsocket_send2_addr(rsocket_t* rsock_item, const char *data, size_t count, size_t *sent,
    rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code = 0;

    *sent = 0;
    if (rsock_item->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    int sent_once = 0;
    for (;; ) {
        sent_once = sendto(rsock_item->fd, data, (int)count, 0, addr, len);
        if (sent_once > 0) {
            *sent = sent_once;
            return rcode_io_done;
        }

        ret_code = rerror_get_osnet_err();
        if (ret_code != WSAEWOULDBLOCK) {
            return ret_code;
        }
        if ((ret_code = rsocket_waitfd(rsock_item, WAITFD_W, tm)) != rcode_io_done) {
            return ret_code;
        }
    }
}

int rsocket_recv(rsocket_t* rsock_item, char *data, int count, int *got, rtimeout_t* tm) {
    int ret_code = 0, prev = rcode_io_done;

    *got = 0;
    if (rsock_item->fd == SOCKET_INVALID) {
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
        //注意：返回值 < 0 时并且(rerror_get_os_err() == EINTR || rerror_get_os_err() == EWOULDBLOCK || rerror_get_os_err() == EAGAIN)的情况下认为连接是正常的，继续接收
        //对非阻塞socket而言，EAGAIN不是一种错误。在VxWorks和Windows上，EAGAIN的名字叫做EWOULDBLOCK
        recv_once = recv(rsock_item->fd, data, (int)count, 0);

        if (recv_once > 0) {
            *got = recv_once;
            return rcode_io_done;
        }
        if (recv_once == 0) {
            return rcode_io_closed;
        }

        ret_code = rerror_get_osnet_err();
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

int rsocket_recvfrom(rsocket_t* rsock_item, char *data, int count, int *got,
    rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    int ret_code, prev = rcode_io_done;

    *got = 0;
    if (rsock_item->fd == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    int recv_once = 0;
    for (;; ) {
        recv_once = recvfrom(rsock_item->fd, data, (int)count, 0, addr, len);

        if (recv_once > 0) {
            *got = recv_once;
            return rcode_io_done;
        }
        if (recv_once == 0) {
            return rcode_io_closed;
        }

        ret_code = rerror_get_osnet_err();
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


static char* wstrerror(int ret_code) {
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

int rsocket_gethostbyaddr(const char* addr, rsocket_len_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) {
        return rcode_io_done;
    } else {
        return WSAGetLastError();
    }
}

int rsocket_gethostbyname(const char* addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) {
        return rcode_io_done;
    } else {
        return WSAGetLastError();
    }
}

char* rsocket_hoststrerror(int ret_code) {
    if (ret_code <= 0) {
        return rio_strerror(ret_code);
    }

    switch (ret_code) {
        case WSAHOST_NOT_FOUND: 
            return PIE_HOST_NOT_FOUND;
        default: 
            return wstrerror(ret_code);
    }
}

char* rsocket_strerror(int ret_code) {
    if (ret_code <= 0) return rio_strerror(ret_code);
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

char* rsocket_ioerror(rsocket_t* rsock_item, int ret_code) {
    (void) rsock_item;
    return rsocket_strerror(ret_code);
}

char* rsocket_gaistrerror(int ret_code) {
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
        case EAI_SYSTEM: return (char*)strerror(rerror_get_os_err());
#endif
        default: return (char*)gai_strerror(ret_code);
    }
}


#undef WAITFD_R
#undef WAITFD_W
#undef WAITFD_E
#undef WAITFD_C

#endif //_WIN64


#undef rsocket_linger_max_secs