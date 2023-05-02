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

#include <setjmp.h>

/* ------------------------------- Structs ------------------------------------*/

typedef enum {
    rcode_err_ok = 0,
    rcode_err_unknown = 1,

    //系统接口部分 10000 - 11999
    rcode_err_os_begin = 10000,
    rcode_err_os_unknown = 10001,
    rcode_err_os_end = 11999,

    //ipc部分 12000 - 12199
    rcode_err_ipc_begin = 12000,
    rcode_err_ipc_unknown = 12001,
    rcode_err_ipc_end = 12199,

    //rpc部分 12200 - 12399
    rcode_err_rpc_begin = 12200,
    rcode_err_rpc_unknown = 12201,
    rcode_err_rpc_end = 12399,

    //ecs部分 12400 - 12599
    rcode_err_ecs_begin = 12400,
    rcode_err_ecs_unknown = 12401,
    rcode_err_ecs_end = 12599,

    //script部分 12600 - 12799
    rcode_err_script_begin = 12600,
    rcode_err_script_unknown = 12601,
    rcode_err_script_end = 12799,

    //逻辑部分 20000 - 49999
    rcode_err_logic_begin = 20000,
    rcode_err_logic_unknown = 20001,
    rcode_err_logic_end = 49999,

} rcode_err_t;

struct rerror_jump_s;
typedef struct rerror_jump_s rerror_jump_t;

/* ------------------------------- Macros ------------------------------------*/

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

#define rstatement_empty

#define rerror_init(rerror_item, rerror_chain) \
    rerror_jump_t rerror_item; \
    if (rerror_chain != rnull) { \
        (rerror_item).previous = rerror_chain; \
    } else { \
        (rerror_item).previous = rnull; \
    } \
    (rerror_item).code = rcode_err_ok

#define rerror_uninit(rerror_item, rerror_chain) \
    if (rerror_chain != rnull) { \
        (rerror_item).previous = rerror_chain; \
    } else { \
        (rerror_item).previous = rnull; \
    } \
    (rerror_item).code = rcode_err_unknown

#if !defined(rerror_throw)

#if defined(__cplusplus) && defined(rbuild_as_cplusplus)

/* C++ implement */
#define rerror_throw(rerror_ptr)                    throw(rerror_ptr)
#define rerror_try(rerror_item, rstatements_try)     try { rstatements_try } 
#define rerror_catch(rerror_item, rstatements_catch) catch(...) { rstatements_catch }
#define rerror_jump_buffer_t     int  /* dummy variable */

#elif defined(ros_linux) || defined(ros_darwin)

/* in POSIX, try _longjmp/_setjmp (more efficient) */
#define rerror_throw(rerror_ptr)     _longjmp((rerror_ptr)->buffer, 1)
#define rerror_try(rerror_item, rstatements_try) \
    rerror_init(rerror_item, rnull); \
    if (_setjmp((rerror_item)->buffer) == 0) { rstatements_try }
#define rerror_catch(rerror_item, rstatements_catch) \
    if ((rerror_item).code != rcode_err_ok ) { rstatements_catch } \
    rerror_uninit(rerror_item, rnull)
#define rerror_jump_buffer_t     jmp_buf

#else /* windows */

/* ISO C handling with long jumps */
#define rerror_throw(rerror_ptr)     longjmp((rerror_ptr)->buffer, 1)
#define rerror_try(rerror_item, rstatements_try) \
    rerror_init(rerror_item, rnull); \
    if (setjmp((rerror_item).buffer) == 0) { rstatements_try }
#define rerror_catch(rerror_item, rstatements_catch) \
    if ((rerror_item).code != rcode_err_ok ) { rstatements_catch } \
    rerror_uninit(rerror_item, rnull)
#define rerror_jump_buffer_t     jmp_buf

#endif /* os */

#endif /* rerror_throw */


struct rerror_jump_s {
  volatile int code;  /* error code */
  rerror_jump_buffer_t buffer;
  rerror_jump_t* previous;
};

/* ------------------------------- 平台os相关信息描述 ------------------------------------*/


/* ------------------------------- socket相关信息描述 ------------------------------------*/

#define rtext_sock_host_not_found "host not found"
#define rtext_sock_address_in_use "address already in use"
#define rtext_sock_connected      "already connected"
#define rtext_sock_access_denied  "permission denied"
#define rtext_sock_conn_refused   "connection refused"
#define rtext_sock_conn_abort     "aborted"
#define rtext_sock_conn_reset     "reset"
#define rtext_sock_conn_timeout   "timeout"
#define rtext_sock_again          "temporary failure in name resolution"
#define rtext_sock_bad_flags      "invalid value for ai_flags"
#define rtext_sock_bad_hints      "invalid value for hints"
#define rtext_sock_recovery       "non-recoverable failure in name resolution"
#define rtext_sock_unknown_family "ai_family not supported"
#define rtext_sock_memory_failed  "memory allocation failure"
#define rtext_sock_no_name        "host or service not provided, or not known"
#define rtext_sock_buffer_overflow "argument buffer overflow"
#define rtext_sock_unknown_protocol "resolved protocol is unknown"
#define rtext_sock_service_unsupport "service not supported for socket type"
#define rtext_sock_type_unsupport "ai_socktype not supported"


#ifdef __cplusplus
}
#endif

#endif //RERROR_H
