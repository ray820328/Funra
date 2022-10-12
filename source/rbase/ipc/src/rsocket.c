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

int rsocket_create(rsocket_t* sock_item, int domain, int type, int protocol) {
    *sock_item = socket(domain, type, protocol);
    if (*sock_item != SOCKET_INVALID) {
        return rcode_io_done;
    }
    else {
        return rerror_get_osnet_err();
    }
}

int rsocket_close(rsocket_t* sock_item) {
    if (sock_item && *sock_item != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)
        rsocket_setblocking(sock_item); /* WIN32可能消耗时间很长，SO_LINGER */
        closesocket(*sock_item);
#else
        close(*sock_item);
#endif
        rdebug("close socket, %p", sock_item);
        *sock_item = SOCKET_INVALID;
    }
    return rcode_ok;
}

int rsocket_shutdown(rsocket_t* sock_item, int how) {
    int ret_code = 0;
    if (sock_item && *sock_item != SOCKET_INVALID) {
#if defined(_WIN32) || defined(_WIN64)
        ret_code = shutdown(*sock_item, how);//how: SD_RECEIVE，SD_SEND，SD_BOTH
#else
        ret_code = shutdown(*sock_item, how);
#endif
        if (ret_code < 0) {
            rerror("error on shutdown %p, %d - %d", sock_item, how, ret_code);
        }
        rdebug("shutdown socket, %p", sock_item);
        *sock_item = SOCKET_INVALID;
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

// int rsocket_setoptions(rsocket_t* sock_item, int option, bool on, int flag) {
//     if (on) {
//         sock_item->options |= (option);
//     } else {
//         sock_item->options &= ~(option);
//     }
//     setsockopt(*sock_item, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&flag, sizeof(flag));
//     int ret = setsockopt(*sock_item, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    // if (ret < 0) {
    //     return rcode_invalid;
    // }
//     return rcode_ok;    
// }

int rsocket_setblocking(rsocket_t* sock_item) {
    int flags = fcntl(*sock_item, F_GETFL, 0);
    flags &= (~(O_NONBLOCK));
    fcntl(*sock_item, F_SETFL, flags);
    return rcode_ok;
}

int rsocket_setnonblocking(rsocket_t* sock_item) {
    int flags = fcntl(*sock_item, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(*sock_item, F_SETFL, flags);
    return rcode_ok;
}

int rsocket_gethostbyaddr(const char* addr, socklen_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) return rcode_io_done;
    else if (h_errno) return h_errno;
    else if (errno) return errno;
    else return rcode_io_unknown;
}

int rsocket_gethostbyname(const char* addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) return rcode_io_done;
    else if (h_errno) return h_errno;
    else if (errno) return errno;
    else return rcode_io_unknown;

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
        default: return hstrerror(err);
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
            return strerror(err);
        }
    }
}

char* rsocket_ioerror(rsocket_t* sock_item, int err) {
    (void) sock_item;
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
        case EAI_SYSTEM: return strerror(err);
        default: return gai_strerror(err);
    }
}

#endif //__linux__




#if defined(_WIN32) || defined(_WIN64)

int rsocket_setblocking(rsocket_t* sock_item) {
    u_long argp = 0;
    ioctlsocket(*sock_item, FIONBIO, &argp);
    return rcode_ok;
}

int rsocket_setnonblocking(rsocket_t* sock_item) {
    u_long argp = 1;
    ioctlsocket(*sock_item, FIONBIO, &argp);
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

char* rsocket_ioerror(rsocket_t* sock_item, int ret_code) {
    (void) sock_item;
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
        case EAI_SYSTEM: return strerror(errno);
#endif
        default: return gai_strerror(ret_code);
    }
}

#endif //_WIN64
