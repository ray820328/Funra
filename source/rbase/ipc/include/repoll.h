/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef REPOLL_H
#define REPOLL_H

#include "rcommon.h"
#include "rinterface.h"
#include "ripc.h"

#ifdef __cplusplus
extern "C" {
#endif

// #define repoll_trace(...) rtrace(__VA_ARGS__)
#define repoll_trace(...)

typedef struct sockaddr rsockaddr_t;

#if defined(__linux__)

#include <sys/epoll.h>

//自定义值一样，EPOLLIN, EPOLLPRI, EPOLLOUT, EPOLLERR, EPOLLHUP 对应 0x1|0x2|0x4|0x8|0x10
#define RIO_POLLIN    0x001     //Can read without blocking
#define RIO_POLLPRI   0x002     //Priority data available
#define RIO_POLLOUT   0x004     //Can write without blocking
#define RIO_POLLERR   0x010     //Pending error
#define RIO_POLLHUP   0x020     //Hangup POLLHUP永远不会被发送到一个普通的文件
#define RIO_POLLNVAL  0x040     //非法fd

#define repoll_set_event_in(val) (val) |= RIO_POLLIN
#define repoll_set_event_out(val) (val) |= RIO_POLLOUT
#define repoll_set_event_all(val) (val) |= RIO_POLLIN | RIO_POLLOUT | RIO_POLLPRI
#define repoll_unset_event_in(val) (val) &= (~RIO_POLLIN)
#define repoll_unset_event_out(val) (val) &= (~RIO_POLLOUT)
#define repoll_check_event_in(val) (((val) & (RIO_POLLIN | RIO_POLLPRI)) != 0)
#define repoll_check_event_out(val) (((val) & RIO_POLLOUT) != 0)
#define repoll_check_event_err(val) (((val) & (RIO_POLLERR | RIO_POLLNVAL | EPOLLHUP)) != 0)


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


int16_t repoll_get_event_req(int16_t event);
int16_t repoll_get_event_rsp(int16_t event);
int repoll_create(repoll_container_t* container, uint32_t size);
int repoll_destroy(repoll_container_t* container);
int repoll_add(repoll_container_t *container, const repoll_item_t *repoll_item);
int repoll_check(repoll_container_t* container, const repoll_item_t* repoll_item);
int repoll_modify(repoll_container_t *container, const repoll_item_t *repoll_item);
int repoll_remove(repoll_container_t *container, const repoll_item_t *repoll_item);
int repoll_poll(repoll_container_t *container, int timeout);
int repoll_reset_oneshot(repoll_container_t *container, int fd);

#endif //__linux__


#ifdef __cplusplus
}
#endif

#endif
