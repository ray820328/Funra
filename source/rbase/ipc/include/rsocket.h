/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RSOCKET_H
#define RSOCKET_H

#include "rcommon.h"
#include "rinterface.h"
#include "ripc.h"
#include "rtime.h"

#if defined(__linux__)
#define ntohll(val) be64toh(val)
#define htonll(val) htobe64(val)
#endif //__linux__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sockaddr rsockaddr_t;

#if defined(__linux__)

#include <errno.h>
/* close function */
#include <unistd.h>
/* fnctnl function and associated constants */
#include <fcntl.h>
/* struct sockaddr */
#include <sys/types.h>
/* socket function */
#include <sys/socket.h>
/* struct timeval */
#include <sys/time.h>
/* gethostbyname and gethostbyaddr functions */
#include <netdb.h>
/* sigpipe handling */
#include <signal.h>
/* IP stuff*/
#include <netinet/in.h>
#include <arpa/inet.h>
/* TCP options (nagle algorithm disable) */
#include <netinet/tcp.h>
#include <net/if.h>

typedef socklen_t rsocket_len_t;
typedef int rsocket_fd_t;
typedef struct sockaddr_storage rsockaddr_storage_t;

#define SOCKET_INVALID (-1)

#endif //__linux__


#if defined(_WIN32) || defined(_WIN64)

 /* POSIX defines 1024 for the FD_SETSIZE，必须在winsock2.h之前定义，默认64 */
#define FD_SETSIZE 1024

#include <winsock2.h>
#include <ws2tcpip.h>


typedef int rsocket_len_t;
typedef SOCKET rsocket_fd_t;
typedef SOCKADDR_STORAGE rsockaddr_storage_t;

#define SOCKET_INVALID (INVALID_SOCKET)

#endif //_WIN64


/* ------------------------------- Macros ------------------------------------*/

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV (0)
#endif

#ifndef IPV6_ADD_MEMBERSHIP
#ifdef IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif //IPV6_JOIN_GROUP
#endif //!IPV6_ADD_MEMBERSHIP

#ifndef IPV6_DROP_MEMBERSHIP
#ifdef IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif //IPV6_LEAVE_GROUP
#endif //!IPV6_DROP_MEMBERSHIP

#define rsocket_check_option(rsock_item, option)  \
    (((rsock_item)->options & (option)) == (option))

#define rsocket_set_option(rsock_item, option, on) \
    do { \
        if (on) { \
            (rsock_item)->options |= (option); \
        } else { \
            (rsock_item)->options &= ~(option); \
        } \
    } while (0)


/* ------------------------------- Structs ------------------------------------*/

#define rsocket_ctx_fields \
    uint64_t id; \
    ripc_type_t stream_type; \
    rsocket_cfg_t* cfg; \
    ripc_entry_t* ipc_entry; \
    rdata_handler_t* in_handler; \
    rdata_handler_t* out_handler; \
    ripc_data_source_t* ds; \
    void* user_data

typedef struct rsocket_cfg_s {
    uint64_t id;
    uint64_t sid_min;
    uint64_t sid_max;

    char ip[32];
    int port;
    int backlog;
    uint32_t sock_flag;
    bool encrypt_msg;
} rsocket_cfg_t;

typedef struct rsocket_ctx_s {
    rsocket_ctx_fields;

} rsocket_ctx_t;

typedef struct rsocket_s {
    rsocket_fd_t fd;
    int type;
    int protocol;
    rsockaddr_t* local_addr;
    rsockaddr_t* remote_addr;
    int state;
    int local_port_unknown;
    int local_interface_unknown;
    int remote_addr_unknown;
    int32_t options;
    int32_t inherit;
    rdata_userdata_t userdata;
} rsocket_t;

typedef enum {
    rcode_io_unknown = -3,
    rcode_io_closed = -2,//连接已关闭
    rcode_io_timeout = -1,//操作超时
    rcode_io_done = 0, //操作成功
    rcode_io_nothing = 1, //操作成功但是无数据
} rcode_io_state;


/* ------------------------------- APIs ------------------------------------*/

int rsocket_create(rsocket_t* rsock_item, int domain, int type, int protocol);
int rsocket_close(rsocket_t* rsock_item);
int rsocket_shutdown(rsocket_t* rsock_item, int how);
int rsocket_destroy(rsocket_t* rsock_item);

int rsocket_setblocking(rsocket_t* rsock_item);
int rsocket_setnonblocking(rsocket_t* rsock_item);

int rsocket_bind(rsocket_t* rsock_item, rsockaddr_t* addr, rsocket_len_t len);
int rsocket_listen(rsocket_t* rsock_item, int backlog);
int rsocket_connect(rsocket_t* rsock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm);
int rsocket_accept(rsocket_t* sock_listen, rsocket_t* rsock_item, 
        rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm);
int rsocket_select(rsocket_t* rsock_item, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm);
int rsocket_send(rsocket_t* rsock_item, const char *data, size_t count, size_t *sent, rtimeout_t* tm);
int rsocket_sendto(rsocket_t* rsock_item, const char *data, size_t count, size_t *sent,
        rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm);
int rsocket_recv(rsocket_t* rsock_item, char *data, size_t count, size_t *got, rtimeout_t* tm);
int rsocket_recvfrom(rsocket_t* rsock_item, char *data, int count, int *got, 
        rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm);

int rsocket_gethostbyaddr(const char *addr, socklen_t len, struct hostent **hp);
int rsocket_gethostbyname(const char *addr, struct hostent **hp);
char* rio_strerror(int err);
char* rsocket_hoststrerror(int err);
char* rsocket_strerror(int err);
char* rsocket_ioerror(rsocket_t* rsock_item, int err);
char* rsocket_gaistrerror(int err);

#ifdef __cplusplus
}
#endif

#endif
