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

int rsocket_create(rsocket_t* rsock_item, int domain, int type, int protocol) {
    *rsock_item = socket(domain, type, protocol);
    if (*rsock_item != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)


#else   
        int flag = 0;
        if (setsockopt(*rsock_item, SOL_SOCKET, SO_REUSEADDR, (void *)&flag, sizeof(flag)) == -1) {
            return rerror_get_osnet_err();
        }
#endif
        return rcode_io_done;
    } else {
        return rerror_get_osnet_err();
    }
}

int rsocket_close(rsocket_t* rsock_item) {
    if (rsock_item && *rsock_item != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)
        rsocket_setblocking(rsock_item); /* WIN32可能消耗时间很长，SO_LINGER */
        closesocket(*rsock_item);
#else
        close(*rsock_item);
#endif
        rtrace("close socket, %p", rsock_item);
        *rsock_item = SOCKET_INVALID;
    }
    return rcode_ok;
}

int rsocket_shutdown(rsocket_t* rsock_item, int how) {
    int ret_code = 0;
    if (rsock_item && *rsock_item != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)
        rsocket_setblocking(rsock_item);
        ret_code = shutdown(*rsock_item, how);//how: SD_RECEIVE，SD_SEND，SD_BOTH
        rsocket_setnonblocking(rsock_item);
#else
        ret_code = shutdown(*rsock_item, how);
#endif
        if (ret_code < 0) {
            rerror("error on shutdown %p, %d - %d", rsock_item, how, ret_code);
        }
        rtrace("shutdown socket, %p", rsock_item);
        *rsock_item = SOCKET_INVALID;
    }
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

// int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
// int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
// int rsocket_setopt(rsocket_t* rsock_item, int option, bool on, int flag) {
//     // if (on) {
//     //     rsock_item->options |= (option);
//     // } else {
//     //     rsock_item->options &= ~(option);
//     // }
//     setsockopt(*rsock_item, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&flag, sizeof(flag));
//     int ret = setsockopt(*rsock_item, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
//     if (ret < 0) {
//         return rcode_invalid;
//     }
//     return rcode_ok;    
// }

int rsocket_setblocking(rsocket_t* rsock_item) {
    int flags = fcntl(*rsock_item, F_GETFL, 0);
    flags &= (~(O_NONBLOCK));
    fcntl(*rsock_item, F_SETFL, flags);
    return rcode_ok;
}

int rsocket_setnonblocking(rsocket_t* rsock_item) {
    int flags = fcntl(*rsock_item, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(*rsock_item, F_SETFL, flags);
    return rcode_ok;
}


int rsocket_bind(rsocket_t* rsock_item, rsockaddr_t* addr, rsocket_len_t len) {
    int ret_code = rcode_ok;
    if (bind(*rsock_item, addr, len) < 0) {
        ret_code = rerror_get_osnet_err();
        rinfo("bind failed, code = %d", ret_code);
    }
    return ret_code;
}

int rsocket_listen(rsocket_t* rsock_item, int backlog) {
    int ret_code = rcode_ok;
    if (listen(*rsock_item, backlog)) {
        ret_code = rerror_get_osnet_err();
        rinfo("listen failed, code = %d", ret_code);
    }
    return ret_code;
}

int rsocket_connect(rsocket_t* rsock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code = rcode_io_done;

    /* avoid calling on closed sockets */
    if (*rsock_item == SOCKET_INVALID) {
        rerror("invalid socket. code = %d", ret_code);
        return rcode_io_closed;
    }
    /* call connect until done or failed without being interrupted */
    do {
        if (connect(*rsock_item, addr, len) == 0) {
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
        rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    int ret_code = 0;
    if (*sock_listen == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    for ( ;; ) {
// #if defined(SOCK_NONBLOCK_NOT_INHERITED)
//         //With FreeBSD accept4() (avail in 10+), O_NONBLOCK is not inherited
//         flags |= SOCK_NONBLOCK;//从外面传
//         *rsock_item = accept4(*sock_listen, addr, len, flags);
// #else
        *rsock_item = accept(*sock_listen, addr, len);
// #endif
        if (*rsock_item != SOCKET_INVALID) {
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

int rsocket_select(rsocket_t rsock, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm) {
    int ret_code = 0;
    do {
        struct timeval tv;
        int64_t time_left = rtimeout_get_block(tm);
        rtimeout_2timeval(tm, &tv, time_left);
        /* timeout = 0 means no wait */
        ret_code = select(rsock, rfds, wfds, efds, time_left >= 0 ? &tv : NULL);

    } while (ret_code < 0 && rerror_get_osnet_err() == EINTR);

    return ret_code;
}

int rsocket_send(rsocket_t* rsock_item, const char *data, size_t count, size_t *sent, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    long sent_len = 0;
    
    if (*rsock_item == SOCKET_INVALID) {
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
        // 3)当操作发生错误时,返回-1,且设置错误为相应的错误号(errno)
        // MSG_NOSIGNAL: 使用这个标志。不让其发送SIG_PIPE信号，导致程序退出。
        sent_len = (long) send(*rsock_item, data, count, 0);

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

    if (*rsock_item == SOCKET_INVALID) return rcode_io_closed;
    for ( ;; ) {
        sent_len = (long) sendto(*rsock_item, data, count, 0, addr, len); 

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

    if (*rsock_item == SOCKET_INVALID) {
        return rcode_io_closed;
    }

    for ( ;; ) {
        read_len = (long) recv(*rsock_item, data, count, 0);//读不到数据返回-1，errorno为EAGAIN或者EINPROGRESS

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

    if (*rsock_item == SOCKET_INVALID) return rcode_io_closed;
    for ( ;; ) {
        read_len = (long) recvfrom(*rsock_item, data, count, 0, addr, len);

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
    //       rinfo("accept %s:%d socket(%d)", ip, port, *sock);
    //   } else if (addr.ss_family == AF_INET) {
    //       struct sockaddr_in* addrV4 = (struct sockaddr_in*)&addr;
    //       ip = inet_ntoa(addrV4->sin_addr);
    //       port = ntohs(addrV4->sin_port);
    //       rinfo("accept %s:%d socket(%d)", ip, port, *sock);
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

// Linux（errno table) 懒得man
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

int rsocket_setblocking(rsocket_t* rsock_item) {
    u_long argp = 0;
    ioctlsocket(*rsock_item, FIONBIO, &argp);
    return rcode_ok;
}

int rsocket_setnonblocking(rsocket_t* rsock_item) {
    u_long argp = 1;
    ioctlsocket(*rsock_item, FIONBIO, &argp);
    return rcode_ok;
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
    }
    else {
        return WSAGetLastError();
    }
}

int rsocket_gethostbyname(const char* addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) {
        return rcode_io_done;
    }
    else {
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
        case EAI_SYSTEM: return (char*)strerror(errno);
#endif
        default: return (char*)gai_strerror(ret_code);
    }
}

#endif //_WIN64
