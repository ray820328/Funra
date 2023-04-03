/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RERROR_H
#define RERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)

#define rerror_get_os_err() GetLastError()
#define rerror_set_os_err(errno) SetLastError(errno)
#define rerror_get_osnet_err() WSAGetLastError()
#define rerror_set_osnet_err(errno) WSASetLastError(errno)

#else //_WIN64
//linux
#include <errno.h>

#define rerror_get_os_err() errno
#define rerror_set_os_err(eno) errno = eno
#define rerror_get_osnet_err() errno
#define rerror_set_osnet_err(eno) errno = eno

#endif //_WIN64

typedef enum {
    //系统接口部分 10000 - 19999
    rcode_err_os_unknown = 10000,

    //套接字部分 10500 - 10600
    rcode_err_sock_unknown = 10500,
    rcode_err_sock_timeout,
    rcode_err_sock_disconnect,

    //逻辑部分 20000 - 50000
    rcode_err_logic_unknown = 20000,
    rcode_err_logic_encode,
    rcode_err_logic_decode,

} rcode_err;

/* ------------------------------- 平台os相关 ------------------------------------*/

#define PIE_HOST_NOT_FOUND "host not found"
#define PIE_ADDRINUSE      "address already in use"
#define PIE_ISCONN         "already connected"
#define PIE_ACCESS         "permission denied"
#define PIE_CONNREFUSED    "connection refused"
#define PIE_CONNABORTED    "closed"
#define PIE_CONNRESET      "closed"
#define PIE_TIMEDOUT       "timeout"
#define PIE_AGAIN          "temporary failure in name resolution"
#define PIE_BADFLAGS       "invalid value for ai_flags"
#define PIE_BADHINTS       "invalid value for hints"
#define PIE_FAIL           "non-recoverable failure in name resolution"
#define PIE_FAMILY         "ai_family not supported"
#define PIE_MEMORY         "memory allocation failure"
#define PIE_NONAME         "host or service not provided, or not known"
#define PIE_OVERFLOW       "argument buffer overflow"
#define PIE_PROTOCOL       "resolved protocol is unknown"
#define PIE_SERVICE        "service not supported for socket type"
#define PIE_SOCKTYPE       "ai_socktype not supported"


#ifdef __cplusplus
}
#endif

#endif //RERROR_H
