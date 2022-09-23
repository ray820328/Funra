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
#include "rsocket_c.h"
#include "rsocket_s.h"
#include "rtools.h"

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
#include <sys/epoll.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
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

static int read_cache_size = 64 * 1024;
static int write_buff_size = 64 * 1024;

#define SOCKET_INVALID (-1)

#define WAITFD_R        POLLIN
#define WAITFD_W        POLLOUT
#define WAITFD_C        (POLLIN|POLLOUT)

enum {
    IO_DONE = 0,        /* operation completed successfully */
    IO_TIMEOUT = -1,    /* operation timed out */
    IO_CLOSED = -2,     /* the connection has been closed */
    IO_UNKNOWN = -3
};
static char *rio_strerror(int err) {
    switch (err) {
    case IO_DONE: return NULL;
    case IO_CLOSED: return "closed";
    case IO_TIMEOUT: return "timeout";
    default: return "unknown error";
    }
}

static void rsocket_setblocking(rsocket_t* sock_item) {
    int flags = fcntl(*sock_item, F_GETFL, 0);
    flags &= (~(O_NONBLOCK));
    fcntl(*sock_item, F_SETFL, flags);
}

static void rsocket_setnonblocking(rsocket_t* sock_item) {
    int flags = fcntl(*sock_item, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(*sock_item, F_SETFL, flags);
}

static int rsocket_gethostbyaddr(const char *addr, socklen_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) return IO_DONE;
    else if (h_errno) return h_errno;
    else if (errno) return errno;
    else return IO_UNKNOWN;
}

static int rsocket_gethostbyname(const char *addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) return IO_DONE;
    else if (h_errno) return h_errno;
    else if (errno) return errno;
    else return IO_UNKNOWN;
}

static char *rsocket_hoststrerror(int errno) {
    if (errno <= 0) return rio_strerror(errno);
    switch (errno) {
        case HOST_NOT_FOUND: return PIE_HOST_NOT_FOUND;
        default: return hstrerror(errno);
    }
}

static char *rsocket_strerror(int errno) {
    if (errno <= 0) return rio_strerror(errno);
    switch (errno) {
        case EADDRINUSE: return PIE_ADDRINUSE;
        case EISCONN: return PIE_ISCONN;
        case EACCES: return PIE_ACCESS;
        case ECONNREFUSED: return PIE_CONNREFUSED;
        case ECONNABORTED: return PIE_CONNABORTED;
        case ECONNRESET: return PIE_CONNRESET;
        case ETIMEDOUT: return PIE_TIMEDOUT;
        default: {
            return strerror(errno);
        }
    }
}

static char *rsocket_ioerror(rsocket_t* sock_item, int errno) {
    (void) sock_item;
    return rsocket_strerror(errno);
}

static char *rsocket_gaistrerror(int errno) {
    if (errno == 0) return NULL;
    switch (errno) {
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
        case EAI_SYSTEM: return strerror(errno);
        default: return gai_strerror(errno);
    }
}

#define RIO_POLLIN    0x001     //Can read without blocking
#define RIO_POLLPRI   0x002     //Priority data available
#define RIO_POLLOUT   0x004     //Can write without blocking
#define RIO_POLLERR   0x010     //Pending error
#define RIO_POLLHUP   0x020     //Hangup occurred
#define RIO_POLLNVAL  0x040     //非法fd
//EPOLLET //边缘触发(Edge Triggered)，这是相对于水平触发(Level Triggered)来说的
//EPOLLONESHOT //只监听一次事件，当监听完这次事件之后，如果还需要继续监听，需要再次把fd加入到EPOLL队列里

static void repoll_reset_oneshot(int epollfd, int fd) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

static int16_t get_epoll_event(int16_t event) {
    int16_t rv = 0;

    rv |= (event & RIO_POLLIN) ? EPOLLIN : 0;
    rv |= (event & RIO_POLLPRI) ? EPOLLPRI : 0;
    rv |= (event & RIO_POLLOUT) ? EPOLLOUT : 0;

    return rv;
}

static int16_t get_epoll_revent(int16_t event) {
    int16_t rv = 0;

    rv |= (event & EPOLLIN) ? RIO_POLLIN : 0;
    rv |= (event & EPOLLPRI) ? RIO_POLLPRI : 0;
    rv |= (event & EPOLLOUT) ? RIO_POLLOUT : 0;
    rv |= (event & EPOLLERR) ? RIO_POLLERR : 0;
    rv |= (event & EPOLLHUP) ? RIO_POLLHUP : 0;

    return rv;
}

typedef struct repoll_item_s {
    int type;
    int fd;
    int16_t event_val_req;//请求监听的events
    int16_t event_val_rsp;//内核返回的events
    void *client_data;//自定义数据
} repoll_item_t;
typedef struct repoll_container_s {
    int epoll_fd;
    int fd_amount;//event_list字段对应的最大item个数
    int fd_dest_count;//当前dest_items字段待处理的fd个数
    struct epoll_event* event_list;//fd当前状态列表
    repoll_item_t* dest_items;//poll结果列表
} repoll_container_t;

static int repoll_create(repoll_container_t* container, uint32_t size) {
    int ret_code = rcode_ok;

    int fd = 0;
    fd = epoll_create(size);
    if (fd < 0) {
        return rerror_get_osnet_err();
    }

    int fd_flags = 0;
    if ((fd_flags = fcntl(fd, F_GETFD)) == -1) {
        ret_code = rerror_get_osnet_err();
        close(fd);
        return ret_code;
    }

    fd_flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, fd_flags) == -1) {
        ret_code = rerror_get_osnet_err();
        close(fd);
        return ret_code;
    }

    container->epoll_fd = fd;
    container->fd_amount = size;
    container->event_list = rdata_new_array(sizeof(struct epoll_event), size);
    container->dest_items = rdata_new_array(sizeof(repoll_item_t), size);

    return rcode_ok;
}
static int repoll_destroy(repoll_container_t* container) {
    rdata_free(container->event_list);
    rdata_free(container->dest_items);
}

static int repoll_add(repoll_container_t *container, const repoll_item_t *repoll_item) {
    int ret_code = rcode_ok;

    struct epoll_event ev = {0}; //linux内核版本小于 2.6.9 必须初始化
    pfd_elem_t *elem = NULL;

    ev.events = get_epoll_event(repoll_item->event_val_req);

    ev.data.ptr = (void *)repoll_item;

    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_ADD, repoll_item->fd, &ev);

    if (ret_code != 0) {
        ret_code = rerror_get_osnet_err();
    }

    return ret_code;
}

static int repoll_remove(repoll_container_t *container, const repoll_item_t *repoll_item) {
    int ret_code = rcode_ok;

    struct epoll_event ev = {0}; //linux内核版本小于 2.6.9 必须初始化
    int ret_code = rcode_ok;

    //read & write
    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_DEL, repoll_item->fd, &ev);

    if (ret_code < 0) {
        return ret_code; //not found
    }

    return rcode_ok;
}

static int repoll_poll(repoll_container_t *container, int timeout) {
    int ret_code = rcode_ok;

    int poll_amount = epoll_wait(container->epoll_fd, container->event_list, container->fd_amount, timeout);//毫秒

    if (ret_code < 0) {
        ret_code = rerror_get_osnet_err();
    }
    else if (ret_code == 0) {
        ret_code = IO_TIMEOUT;
    }
    else {
        const repoll_item_t *fdptr;

        for (int i = 0; i < poll_amount; i++) {
            fdptr = (repoll_item_t*)(container->event_list[i].data.ptr);
            //fdptr = &(((pfd_elem_t *) (container->event_list[i].data.ptr))->pfd);

            container->dest_items[i] = *fdptr;
            container->dest_items[i].event_val_rsp = get_epoll_revent(container->event_list[i].events);
        }

        container->fd_dest_count = poll_amount;

        ret_code = rcode_ok;
    }

    return ret_code;
}


static repoll_container_t test_container_obj;
static repoll_container_t* test_container;

static int rsocket_waitfd(rsocket_t* sock_item, int sw, rtimeout_t* tm) {
    int ret_code = 0;

    struct pollfd pfd;
    pfd.fd = *sock_item;
    pfd.events = sw;//poll用户态修改的是events
    pfd.revents = 0;//poll内核态修改的是revents

    if (rtimeout_done(tm)) {
        return IO_TIMEOUT;
    }

    do {
        int t = (int)(rtimeout_get_total(tm) / 1000);//毫秒
        ret_code = poll(&pfd, 1, t >= 0 ? t : -1); //timeout为时间差值，精度是毫秒，-1时无限等待
    } while (ret_code == -1 && rerror_get_osnet_err() == EINTR);

    if (ret_code == -1) {
        return rerror_get_osnet_err();
    }
    if (ret_code == 0) {
        return IO_TIMEOUT;
    }
    if (sw == WAITFD_C && (pfd.revents & (POLLIN | POLLERR))) {
        return IO_CLOSED;
    }

    return IO_DONE;
}

static void rsocket_destroy(rsocket_t* sock_item) {
    if (*sock_item != SOCKET_INVALID) {
        close(*sock_item);
        *sock_item = SOCKET_INVALID;
    }
}

static int rsocket_select(rsocket_t rsock, fd_set *rfds, fd_set *wfds, fd_set *efds, rtimeout_t* tm) {
    int ret_code = 0;
    do {
        struct timeval tv;
        int64_t time_left = rtimeout_get_block(tm);
        rtimeout_2timeval(tm, &tv, time_left);
        /* timeout = 0 means no wait */
        ret_code = select(rsock, rfds, wfds, efds, time_left >= 0 ? &tv: NULL);
    } while (ret_code < 0 && errno == EINTR);

    return ret_code;
}

static int rsocket_create(rsocket_t* sock_item, int domain, int type, int protocol) {
    *sock_item = socket(domain, type, protocol);
    if (*sock_item != SOCKET_INVALID) return IO_DONE;
    else return errno;
}

static int rsocket_bind(rsocket_t* sock_item, rsockaddr_t *addr, rsocket_len_t len) {
    int ret_code = IO_DONE;
    rsocket_setblocking(sock_item);
    if (bind(*sock_item, addr, len) < 0) ret_code = errno;
    rsocket_setnonblocking(sock_item);
    return ret_code;
}

static int rsocket_listen(rsocket_t* sock_item, int backlog) {
    int ret_code = IO_DONE;
    if (listen(*sock_item, backlog)) ret_code = errno;
    return ret_code;
}

static void socket_shutdown(rsocket_t* sock_item, int how) {
    shutdown(*sock_item, how);
}

static int rsocket_connect(rsocket_t* sock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
    /* avoid calling on closed sockets */
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    /* call connect until done or failed without being interrupted */
    do if (connect(*sock_item, addr, len) == 0) return IO_DONE;
    while ((ret_code = errno) == EINTR);
    /* if connection failed immediately, return error code */
    if (ret_code != EINPROGRESS && ret_code != EAGAIN) return ret_code;
    /* zero timeout case optimization */
    if (rtimeout_done(tm)) return IO_TIMEOUT;
    
    repoll_poll(repoll_container_t *container, int timeout)
}

static int rsocket_accept(rsocket_t* sock_item, rsocket_t* pa, rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        int ret_code;
        if ((*pa = accept(*sock_item, addr, len)) != SOCKET_INVALID) return IO_DONE;
        ret_code = errno;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN && ret_code != ECONNABORTED) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != IO_DONE) return ret_code;
    }
    
    return IO_UNKNOWN;
}

static int rsocket_send(rsocket_t* sock_item, const char *data, size_t count, size_t *sent, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    
    for ( ;; ) {
        long put = (long) send(*sock_item, data, count, 0);
        /* if we sent anything, we are done */
        if (put >= 0) {
            *sent = put;
            return IO_DONE;
        }
        ret_code = errno;
        /* EPIPE means the connection was closed */
        if (ret_code == EPIPE) return IO_CLOSED;
        /* EPROTOTYPE means the connection is being closed (on Yosemite!)*/
        if (ret_code == EPROTOTYPE) continue;
        /* we call was interrupted, just try again */
        if (ret_code == EINTR) continue;
        /* if failed fatal reason, report error */
        if (ret_code != EAGAIN) return ret_code;
        /* wait until we can send something or we timeout */
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != IO_DONE) return ret_code;
    }
    
    return IO_UNKNOWN;
}

static int rsocket_sendto(rsocket_t* sock_item, const char *data, size_t count, size_t *sent,
        rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        long put = (long) sendto(*sock_item, data, count, 0, addr, len); 
        if (put >= 0) {
            *sent = put;
            return IO_DONE;
        }
        ret_code = errno;
        if (ret_code == EPIPE) return IO_CLOSED;
        if (ret_code == EPROTOTYPE) continue;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != IO_DONE) return ret_code;
    }
    return IO_UNKNOWN;
}

static int rsocket_recv(rsocket_t* sock_item, char *data, size_t count, size_t *got, rtimeout_t* tm) {
    int ret_code;
    *got = 0;
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        long taken = (long) recv(*sock_item, data, count, 0);
        if (taken > 0) {
            *got = taken;
            return IO_DONE;
        }
        ret_code = errno;
        if (taken == 0) return IO_CLOSED;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != IO_DONE) return ret_code;
    }
    return IO_UNKNOWN;
}

static int rsocket_recvfrom(rsocket_t* sock_item, char *data, int count, int *got, rsockaddr_t *addr, rsocket_len_t *len, rtimeout_t* tm) {
    int ret_code;
    *got = 0;
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        long taken = (long) recvfrom(*sock_item, data, count, 0, addr, len);
        if (taken > 0) {
            *got = taken;
            return IO_DONE;
        }
        ret_code = errno;
        if (taken == 0) return IO_CLOSED;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != IO_DONE) return ret_code;
    }
    return IO_UNKNOWN;
}

static int rsocket_write(rsocket_t* sock_item, const char *data, int count, int *sent, rtimeout_t* tm) {
    int ret_code;
    *sent = 0;
    /* avoid making system calls on closed sockets */
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    /* loop until we send something or we give up on error */
    for ( ;; ) {
        long put = (long) write(*sock_item, data, count);
        /* if we sent anything, we are done */
        if (put >= 0) {
            *sent = put;
            return IO_DONE;
        }
        ret_code = errno;
        /* EPIPE means the connection was closed */
        if (ret_code == EPIPE) return IO_CLOSED;
        /* EPROTOTYPE means the connection is being closed (on Yosemite!)*/
        if (ret_code == EPROTOTYPE) continue;
        /* we call was interrupted, just try again */
        if (ret_code == EINTR) continue;
        /* if failed fatal reason, report error */
        if (ret_code != EAGAIN) return ret_code;
        /* wait until we can send something or we timeout */
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != IO_DONE) return ret_code;
    }
    /* can't reach here */
    return IO_UNKNOWN;
}

static int rsocket_read(rsocket_t* sock_item, char *data, size_t count, size_t *got, rtimeout_t* tm) {
    int ret_code;
    *got = 0;
    if (*sock_item == SOCKET_INVALID) return IO_CLOSED;
    for ( ;; ) {
        long taken = (long) read(*sock_item, data, count);
        if (taken > 0) {
            *got = taken;
            return IO_DONE;
        }
        ret_code = errno;
        if (taken == 0) return IO_CLOSED;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_R, tm)) != IO_DONE) return ret_code;
    }
    return IO_UNKNOWN;
}


static int ripc_init_c(void* ctx, const void* cfg_data) {
    /* 避免sigpipe导致崩溃 */
    signal(SIGPIPE, SIG_IGN);

    rinfo("socket client init.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    int ret_code = 0;

    ret_code = repoll_create(test_container, 10);
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

    ret_code = repoll_destroy(test_container);
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

        ret_code = rsocket_connect(rsock_item, (rsockaddr_t *)iterator->ai_addr, (rsocket_len_t)iterator->ai_addrlen, &tm);
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
    //    ret_code = rsocket_strerror(rsocket_bind(&rsock_item, (rsockaddr_t *)iterator->ai_addr, (rsocket_len_t)iterator->ai_addrlen));
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
    rtimeout_init_millisec(&tm, 100, -1);
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
const ripc_entry_t* rsocket_s = &impl_c;

#else

#endif //__linux__

#undef SOCKET_INVALID
#undef WAITFD_R
#undef WAITFD_W
#undef WAITFD_C
