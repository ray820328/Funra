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
#include <sys/poll.h>
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

static char* rio_strerror(int err) {
    switch (err) {
    case IO_DONE: return rstr_null;
    case IO_CLOSED: return "closed";
    case IO_TIMEOUT: return "timeout";
    default: return "unknown error";
    }
}
static char* rsocket_hoststrerror(int err) {
    if (err <= 0) return rio_strerror(err);
    switch (err) {
        case HOST_NOT_FOUND: return PIE_HOST_NOT_FOUND;
        default: return hstrerror(err);
    }
}
static char* rsocket_strerror(int err) {
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

static char* rsocket_ioerror(rsocket_t* sock_item, int err) {
    (void) sock_item;
    return rsocket_strerror(err);
}

static char* rsocket_gaistrerror(int err) {
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

#define RIO_POLLIN    0x001     //Can read without blocking
#define RIO_POLLPRI   0x002     //Priority data available
#define RIO_POLLOUT   0x004     //Can write without blocking
#define RIO_POLLERR   0x010     //Pending error
#define RIO_POLLHUP   0x020     //Hangup occurred POLLHUP永远不会被发送到一个普通的文件
#define RIO_POLLNVAL  0x040     //非法fd
//EPOLLET //边缘触发(Edge Triggered)，这是相对于水平触发(Level Triggered)来说的
//EPOLLONESHOT //只监听一次事件，当监听完这次事件之后，如果还需要继续监听，需要再次把fd加入到EPOLL队列里

static void repoll_reset_oneshot(int epollfd, int fd) {
    struct epoll_event event = {0, {0}};
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

#define repoll_set_event_in(val) (val) |= RIO_POLLIN
#define repoll_set_event_out(val) (val) |= RIO_POLLOUT
#define repoll_set_event_all(val) (val) |= RIO_POLLIN | RIO_POLLOUT | RIO_POLLPRI
#define repoll_unset_event_in(val) (val) &= (~RIO_POLLIN)
#define repoll_unset_event_out(val) (val) &= (~RIO_POLLOUT)
#define repoll_check_event_in(val) (((val) & (RIO_POLLIN | RIO_POLLPRI)) != 0)
#define repoll_check_event_out(val) (((val) & RIO_POLLOUT) != 0)
#define repoll_check_event_err(val) (((val) & (RIO_POLLERR | RIO_POLLNVAL | EPOLLHUP)) != 0)

static int16_t epoll_get_event_req(int16_t event) {
    int16_t rv = 0;

    rv |= (event & RIO_POLLIN) ? EPOLLIN : 0;
    rv |= (event & RIO_POLLPRI) ? EPOLLPRI : 0;
    rv |= (event & RIO_POLLOUT) ? EPOLLOUT : 0;

    return rv;
}

static int16_t repoll_get_event_rsp(int16_t event) {
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
    ripc_data_source_t* ds;//自定义数据
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
    container->event_list = rdata_new_type_array(struct epoll_event, size);
    container->dest_items = rdata_new_type_array(repoll_item_t, size);

    return rcode_ok;
}
static int repoll_destroy(repoll_container_t* container) {
    rdata_free(struct epoll_event, container->event_list);
    rdata_free(repoll_item_t, container->dest_items);
    close(container->epoll_fd);
}

static int repoll_add(repoll_container_t *container, const repoll_item_t *repoll_item) {
    int ret_code = rcode_ok;

    struct epoll_event ev = {0}; //linux内核版本小于 2.6.9 必须初始化

    ev.events = epoll_get_event_req(repoll_item->event_val_req);

    ev.data.ptr = (void *)repoll_item;

    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_ADD, repoll_item->fd, &ev);

    if (ret_code != 0) {
        ret_code = rerror_get_osnet_err();
    }

    return ret_code;
}
static int repoll_modify(repoll_container_t *container, const repoll_item_t *repoll_item) {
    int ret_code = rcode_ok;

    // if (data->eventFlag == flag) {
    //     return rcode_ok;
    // }

    struct epoll_event ev = {0}; // {flag, {0}} linux内核版本小于 2.6.9 必须初始化

    ev.events = epoll_get_event_req(repoll_item->event_val_req);

    ev.data.ptr = (void *)repoll_item;

    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_MOD, repoll_item->fd, &ev);

    if (ret_code != 0) {
        ret_code = rerror_get_osnet_err();
    }

    return ret_code;
}
static int repoll_remove(repoll_container_t *container, const repoll_item_t *repoll_item) {
    int ret_code = rcode_ok;

    struct epoll_event ev = {0}; //linux内核版本小于 2.6.9 必须初始化

    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_DEL, repoll_item->fd, &ev);
    if (ret_code < 0) {
        rerror("event not found, code = %d", ret_code);
        return ret_code; //not found
    }

    for (int i = 0; i < container->fd_dest_count; i++) {//删掉结果列表对象
        if (container->dest_items[i].fd == repoll_item->fd) {
            container->dest_items[i].fd = 0;
            break;
        }
    }

    return rcode_ok;
}
static int repoll_poll(repoll_container_t *container, int timeout) {
    int ret_code = rcode_ok;

    int poll_amount = epoll_wait(container->epoll_fd, container->event_list, container->fd_amount, timeout);//毫秒

    if (poll_amount < 0) {
        container->fd_dest_count = 0;
        ret_code = rerror_get_osnet_err();
    } else if (poll_amount == 0) {
        container->fd_dest_count = 0;
        ret_code = rcode_ok;//超时而已
    } else {
        const repoll_item_t *fdptr;

        for (int i = 0; i < poll_amount; i++) {
            fdptr = (repoll_item_t*)(container->event_list[i].data.ptr);
            //fdptr = &(((pfd_elem_t *) (container->event_list[i].data.ptr))->pfd);

            container->dest_items[i] = *fdptr;
            container->dest_items[i].event_val_rsp = repoll_get_event_rsp(container->event_list[i].events);
        }

        container->fd_dest_count = poll_amount;

        ret_code = rcode_ok;
    }

    return ret_code;
}


static repoll_container_t test_container_obj;
static repoll_container_t* test_container = &test_container_obj;

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

static int rsocket_create(rsocket_t* sock_item, int domain, int type, int protocol) {
    *sock_item = socket(domain, type, protocol);
    if (*sock_item != SOCKET_INVALID) {
        return IO_DONE;
    }
    else {
        return rerror_get_osnet_err();
    }
}

static int rsocket_bind(rsocket_t* sock_item, rsockaddr_t *addr, rsocket_len_t len) {
    int ret_code = IO_DONE;
    rsocket_setblocking(sock_item);
    if (bind(*sock_item, addr, len) < 0) ret_code = rerror_get_osnet_err();
    rsocket_setnonblocking(sock_item);
    return ret_code;
}

static int rsocket_listen(rsocket_t* sock_item, int backlog) {
    int ret_code = IO_DONE;
    if (listen(*sock_item, backlog)) {
        ret_code = rerror_get_osnet_err();
    }
    return ret_code;
}

static void socket_shutdown(rsocket_t* sock_item, int how) {
    shutdown(*sock_item, how);
}

static int rsocket_connect(rsocket_t* sock_item, rsockaddr_t *addr, rsocket_len_t len, rtimeout_t* tm) {
    int ret_code = 0;
    repoll_container_t* container = test_container;

    /* avoid calling on closed sockets */
    if (*sock_item == SOCKET_INVALID) {
        rerror("invalid socket. code = %d", ret_code);
        return IO_CLOSED;
    }
    /* call connect until done or failed without being interrupted */
    do {
        if (connect(*sock_item, addr, len) == 0) {
            return IO_DONE;
        }
    } while ((ret_code = rerror_get_osnet_err()) == EINTR);

    /* timeout */
    if (rtimeout_done(tm)) {
        rerror("connect timeout. code = %d", ret_code);
        return IO_TIMEOUT;
    }

    /* if connection failed immediately, return error code */
    if (ret_code != EINPROGRESS && ret_code != EAGAIN) {
        rerror("connect failed. code = %d", ret_code);
        return ret_code;
    }

    return rcode_ok;//epoll connect 返回 EINPROGRESS 也视为成功，监听write事件处理
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
    
    if (*sock_item == SOCKET_INVALID) {
        return IO_CLOSED;
    }
    
    for ( ;; ) {
        long put = (long) send(*sock_item, data, count, 0);
        /* if we sent anything, we are done */
        if (put >= 0) {
            *sent = put;
            return IO_DONE;
        }
        ret_code = rerror_get_osnet_err();
        /* EPIPE means the connection was closed */
        if (ret_code == EPIPE) return IO_CLOSED;
        /* EPROTOTYPE means the connection is being closed (on Yosemite!)*/
        if (ret_code == EPROTOTYPE) continue;
        /* we call was interrupted, just try again */
        if (ret_code == EINTR) continue;
        /* if failed fatal reason, report error */
        if (ret_code != EAGAIN) return ret_code;

        if (rtimeout_done(tm)) {
            rerror("send timeout. code = %d", ret_code);
            return IO_TIMEOUT;
        }
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
        ret_code = rerror_get_osnet_err();
        if (ret_code == EPIPE) return IO_CLOSED;
        if (ret_code == EPROTOTYPE) continue;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != IO_DONE) return ret_code;
    }
    return IO_UNKNOWN;
}

static int rsocket_recv(rsocket_t* sock_item, char *data, size_t count, size_t *got, rtimeout_t* tm) {
    int ret_code = 0;
    *got = 0;

    if (*sock_item == SOCKET_INVALID) {
        return IO_CLOSED;
    }

    for ( ;; ) {
        // flags
        // 如果flags为0,则recv/send和read/write一样的操作
        // MSG_DONTROUTE:是 send函数使用的标志.这个标志告诉IP.目的主机在本地网络上面,没有必要查找表.这个标志一般用网络诊断和路由程序里面.
        // MSG_OOB:表示可以接收和发送带外的数据.关于带外数据我们以后会解释的.
        // MSG_PEEK:是recv函数的使用标志, 表示只是从系统缓冲区中读取内容,而不清除系统缓冲区的内容.这样下次读仍然是一样的内容.在有多个进程读写数据时可以使用这个标志.
        // MSG_WAITALL 是recv函数的使用标志,表示等到所有的信息到达时才返回.使用这个标志的时候recv回一直阻塞,直到指定的条件满足,或者是发生了错误. 
        // 1)当读到了指定的字节时,函数正常返回.返回值等于len 
        // 2)当读到了文件的结尾时,函数正常返回.返回值小于len 
        // 3)当操作发生错误时,返回-1,且设置错误为相应的错误号(errno)
        // MSG_NOSIGNAL is a flag used by send() in some implementations of the Berkeley sockets API.
        // 对于服务器端，可以使用这个标志。目的是不让其发送SIG_PIPE信号，导致程序退出。
        // 还有其它的几个选项,不过实际上用的很少
        long taken = (long) recv(*sock_item, data, count, 0);
        if (taken >= 0) {
            *got = taken;
            return IO_DONE;
        }

        ret_code = rerror_get_osnet_err();
        // if (taken == 0) return IO_CLOSED;
        if (ret_code == EINTR) {
            continue;
        }
        if (ret_code != EAGAIN && ret_code != EINPROGRESS) {//INPROGRESS
            return ret_code;
        }
        rinfo("unexpected code = %d", ret_code);

        if (rtimeout_done(tm)) {
            rerror("send timeout. code = %d", ret_code);
            return IO_TIMEOUT;
        }
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
        ret_code = rerror_get_osnet_err();
        /* EPIPE means the connection was closed */
        if (ret_code == EPIPE) {
            return IO_CLOSED;
        }
        /* EPROTOTYPE means the connection is being closed (on Yosemite!)*/
        if (ret_code == EPROTOTYPE) {
            continue;
        }
        /* we call was interrupted, just try again */
        if (ret_code == EINTR) {
            continue;
        }
        /* if failed fatal reason, report error */
        if (ret_code != EAGAIN) {
            return ret_code;
        }
        /* wait until we can send something or we timeout */
        if ((ret_code = rsocket_waitfd(sock_item, WAITFD_W, tm)) != IO_DONE) {
            return ret_code;
        }

        if (rtimeout_done(tm)) {
            return IO_TIMEOUT;
        }
    }
    /* can't reach here */
    return IO_UNKNOWN;
}

static int rsocket_read(rsocket_t* sock_item, char *data, size_t count, size_t *got, rtimeout_t* tm) {
    int ret_code = 0;
    *got = 0;

    if (*sock_item == SOCKET_INVALID) {
        return IO_CLOSED;
    }

    for ( ;; ) {
        long taken = (long) read(*sock_item, data, count);
        if (taken > 0) {
            *got = taken;
            return IO_DONE;
        }

        ret_code = rerror_get_osnet_err();
        if (taken == 0) return IO_CLOSED;
        if (ret_code == EINTR) continue;
        if (ret_code != EAGAIN) return ret_code;

        if (rtimeout_done(tm)) {
            return IO_TIMEOUT;
        }
    }
    return IO_UNKNOWN;
}


static int ripc_init_c(void* ctx, const void* cfg_data) {
    /* 忽略信号SIGPIPE，避免导致崩溃 */
    signal(SIGPIPE, SIG_IGN);//SIG_DFL

    rinfo("socket client init.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->stream;
    repoll_container_t* container = test_container;
    int ret_code = 0;

    ret_code = repoll_create(container, 10);
    if (ret_code != rcode_ok) {
        rerror("failed on open, code = %d", ret_code);
        return ret_code;
    }

    ds_client->stream = NULL;
    
    return rcode_ok;
}
static int ripc_uninit_c(void* ctx) {
    rinfo("socket client uninit.");

    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds_client = rsocket_ctx->stream;
    repoll_container_t* container = test_container;
    int ret_code = 0;

    ret_code = repoll_destroy(container);
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
    repoll_container_t* container = test_container;
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
        rerror("failed on getaddrinfo, code = %d, msg = %s", ret_code, (char*)rsocket_gaistrerror(ret_code));
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
        if (ret_code == IO_DONE) {
            family = current_family;
            rinfo("success on connect, family = %d", family);
            break;
        }

        if (ret_code != rcode_ok) {
            rdebug("wait for next addr to connect, code = %d, msg = %s", ret_code, rsocket_strerror(ret_code));
        }

    }
    freeaddrinfo(addrinfo_result);

    if (ret_code == rcode_ok) {
        //添加到epoll对象
        repoll_item_t* repoll_item = rdata_new(repoll_item_t);
        repoll_item->fd = *rsock_item;
        repoll_item->ds = ds_client;
        repoll_item->event_val_req = 0;
        repoll_set_event_all(repoll_item->event_val_req);

        ret_code = repoll_add(container, repoll_item);
        if (ret_code != rcode_ok){
            //释放资源
            rerror("add to epoll failed. code = %d", ret_code);
            return ret_code;
        }

        ds_client->stream = repoll_item;//stream间接指向sock, ds_client->stream->fd
        rsocket_ctx->stream_state = ripc_state_ready_pending;
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
    repoll_container_t* container = test_container;

    rinfo("socket client close.");

    if (rsocket_ctx->stream_state == ripc_state_closed) {
        return rcode_ok;
    }

    rbuffer_release(ds_client->read_cache);
    rbuffer_release(ds_client->write_buff);

    //从epoll移除，销毁socket对象
    if (ds_client->stream != NULL) {
        repoll_remove(container, (repoll_item_t*)ds_client->stream);

        rdata_free(rsocket_t, ((repoll_item_t*)ds_client->stream)->fd);

        rdata_free(repoll_item_t, ds_client->stream);
    }

    rsocket_ctx->stream_state = ripc_state_closed;

    return rcode_ok;
}
static int ripc_start_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;
    ripc_data_source_t* ds = rsocket_ctx->stream;
    repoll_container_t* container = test_container;
    int ret_code = 0;

    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 3, -1);
    rtimeout_start(&tm);

    do {
        ret_code = repoll_poll(container, 1);//不要用边缘触发模式，可能会调用多次，单线程不会惊群
        if (ret_code != rcode_ok){
            rerror("epoll_wait failed. code = %d", ret_code);
            return ret_code;
        }

        if (container->fd_dest_count > 0) {
            repoll_item_t* dest_item = NULL;

            for (int i = 0; i < container->fd_dest_count; i++) {
                dest_item = &(container->dest_items[i]);
                if (dest_item->fd != ((repoll_item_t*)ds->stream)->fd) {
                    continue;
                }

                if (repoll_check_event_err(dest_item->event_val_rsp)) {
                    rerror("error of socket, fd = ", dest_item->fd);
                    ripc_close_c(rsocket_ctx);//直接关闭
                    continue;
                }

                if (repoll_check_event_out(dest_item->event_val_rsp)) {
                    if (rsocket_ctx->stream_state == ripc_state_ready_pending) {
                        ds->read_cache = NULL;
                        rbuffer_init(ds->read_cache, read_cache_size);
                        ds->write_buff = NULL;
                        rbuffer_init(ds->write_buff, write_buff_size);

                        // rsocket_ctx->stream_state = ripc_state_ready;
                        rsocket_ctx->stream_state = ripc_state_start;
                        rinfo("socket client start.");

                        return rcode_ok;
                    }
                }
            }
        }
    } while (!rtimeout_done(&tm));

    return rcode_err_sock_timeout;
}
static int ripc_stop_c(void* ctx) {
    rsocket_ctx_t* rsocket_ctx = (rsocket_ctx_t*)ctx;

    rsocket_ctx->stream_state = ripc_state_stop;
    rinfo("socket client stop.");


    return rcode_ok;
}
//写入自定义缓冲区
static int ripc_send_data_2buffer_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = IO_DONE;
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
        ret_code = IO_DONE;
    }
    rdebug("end send_data_2buffer, code: %d", ret_code);

    return rcode_ok;
}
static int ripc_send_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = IO_DONE;
    rsocket_ctx_t* rsocket_ctx = ds_client->ctx;
    //rsocket_cfg_t* cfg = rsocket_ctx->cfg;

    const char* data_buff = rbuffer_read_start_dest(ds_client->write_buff);
    int count = rbuffer_size(ds_client->write_buff);
    int sent_len = 0;//立即处理
    
    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 3, -1);
    rtimeout_start(&tm);

    ret_code = rsocket_send((rsocket_t*)(&((repoll_item_t*)ds_client->stream)->fd), data_buff, count, &sent_len, &tm);
    if (ret_code != IO_DONE) {
        rwarn("end send_data, code: %d, sent_len: %d, buff_size: %d", ret_code, sent_len, count);

        ripc_close_c(rsocket_ctx);//直接关闭
        if (ret_code == IO_TIMEOUT) {
            return rcode_err_sock_timeout;
        }
        else {
            return rcode_err_sock_disconnect;//所有未知错误都断开
        }
    }
    rdebug("end send_data, code: %d, sent_len: %d", ret_code, sent_len);

    rbuffer_skip(ds_client->write_buff, sent_len);
    return rcode_ok;
}

static int ripc_receive_data_c(ripc_data_source_t* ds_client, void* data) {
    int ret_code = 0;
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

    rtimeout_t tm;
    rtimeout_init_millisec(&tm, 1, -1);
    rtimeout_start(&tm);

    ret_code = rsocket_recv((rsocket_t*)(&((repoll_item_t*)ds_client->stream)->fd), data_buff, count, &received_len, &tm);

    if (ret_code == IO_TIMEOUT) {
        ret_code = IO_DONE;
    } else if (ret_code == IO_CLOSED) {//udp & tcp
        if (received_len > 0) {//有可能已经关闭了，下一次再触发会是0
            ret_code = IO_DONE;
        }
    }

    if (ret_code != IO_DONE) {
        rwarn("end client send_data, code: %d, received_len: %d, buff_size: %d", ret_code, received_len, count);

        ripc_close_c(rsocket_ctx);//直接关闭
        if (ret_code == IO_TIMEOUT) {
            return rcode_err_sock_timeout;
        }
        else {
            return rcode_err_sock_disconnect;//所有未知错误都断开
        }
    }

    if(received_len > 0 && rsocket_ctx->in_handler) {
        data_raw.len = received_len;

        ret_code = rsocket_ctx->in_handler->process(rsocket_ctx->in_handler, ds_client, &data_raw);
        if (ret_code != ripc_code_success) {
            rerror("error on handler process, code: %d", ret_code);
            return rcode_err_logic_decode;
        }
        ret_code = IO_DONE;
    }

    return rcode_ok;
}

int ripc_check_data_c(ripc_data_source_t* ds, void* data) {
    rsocket_ctx_t* rsocket_ctx = ds->ctx;
    repoll_container_t* container = test_container;
    int ret_code = 0;

    ret_code = repoll_poll(container, 1);//不要用边缘触发模式，可能会调用多次，单线程不会惊群
    if (ret_code != rcode_ok){
        rerror("epoll_wait failed. code = %d", ret_code);
        return ret_code;
    }

    if (container->fd_dest_count > 0) {
        repoll_item_t* dest_item = NULL;

        for (int i = 0; i < container->fd_dest_count; i++) {
            dest_item = &(container->dest_items[i]);
            if (dest_item->fd == 0) {//取回后调用了del
                continue;
            }

            if (repoll_check_event_err(dest_item->event_val_rsp)) {
                rerror("error of socket, fd = ", dest_item->fd);
                ripc_close_c(rsocket_ctx);//直接关闭
                continue;
            }

            if (repoll_check_event_out(dest_item->event_val_rsp)) {
                if likely(rsocket_ctx->stream_state == ripc_state_start) {
                    ripc_send_data_c(ds, data);
                }
            }

            if (repoll_check_event_in(dest_item->event_val_rsp)) {
                if likely(rsocket_ctx->stream_state == ripc_state_start) {
                    ripc_receive_data_c(ds, data);
                }
            }

        }
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
    (ripc_send_func)ripc_send_data_2buffer_c,// ripc_send_func send;
    (ripc_check_func)ripc_check_data_c,// ripc_check_func check;
    (ripc_receive_func)ripc_receive_data_c,// ripc_receive_func receive;
    NULL// ripc_error_func error;
};
const ripc_entry_t* rsocket_c = &impl_c;

const ripc_entry_t* rsocket_s = &impl_c;

#else

#endif //__linux__

#undef SOCKET_INVALID
#undef WAITFD_R
#undef WAITFD_W
#undef WAITFD_C
