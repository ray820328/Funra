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

#include "repoll.h"

int16_t repoll_get_event_req(int16_t event) {
    int16_t rv = 0;

    rv |= (event & RIO_POLLIN) ? EPOLLIN : 0;
    rv |= (event & RIO_POLLPRI) ? EPOLLPRI : 0;
    rv |= (event & RIO_POLLOUT) ? EPOLLOUT : 0;

    return rv;
}

int16_t repoll_get_event_rsp(int16_t event) {
    int16_t rv = 0;

    rv |= (event & EPOLLIN) ? RIO_POLLIN : 0;
    rv |= (event & EPOLLPRI) ? RIO_POLLPRI : 0;
    rv |= (event & EPOLLOUT) ? RIO_POLLOUT : 0;
    rv |= (event & EPOLLERR) ? RIO_POLLERR : 0;
    rv |= (event & EPOLLHUP) ? RIO_POLLHUP : 0;

    return rv;
}

int repoll_create(repoll_container_t* container, uint32_t size) {
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

int repoll_destroy(repoll_container_t* container) {
    rdata_free(struct epoll_event, container->event_list);
    rdata_free(repoll_item_t, container->dest_items);
    close(container->epoll_fd);
}

int repoll_add(repoll_container_t* container, const repoll_item_t* repoll_item) {
    int ret_code = rcode_ok;

    struct epoll_event ev = {0}; //linux内核版本小于 2.6.9 必须初始化

    ev.events = repoll_get_event_req(repoll_item->event_val_req);

    ev.data.ptr = (void *)repoll_item;

    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_ADD, repoll_item->fd, &ev);

    if (ret_code != 0) {
        ret_code = rerror_get_osnet_err();
    }

    return ret_code;
}

int repoll_modify(repoll_container_t* container, const repoll_item_t* repoll_item) {
    int ret_code = rcode_ok;

    // if (data->event_flag == flag) {
    //     return rcode_ok;
    // }

    struct epoll_event ev = {0}; // {flag, {0}} linux内核版本小于 2.6.9 必须初始化

    ev.events = repoll_get_event_req(repoll_item->event_val_req);

    ev.data.ptr = (void *)repoll_item;

    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_MOD, repoll_item->fd, &ev);

    if (ret_code != 0) {
        ret_code = rerror_get_osnet_err();
    }

    return ret_code;
}

int repoll_remove(repoll_container_t* container, const repoll_item_t* repoll_item) {
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

int repoll_poll(repoll_container_t* container, int timeout) {
    int ret_code = rcode_ok;

    int poll_amount = epoll_wait(container->epoll_fd, container->event_list, container->fd_amount, timeout);//毫秒

    if (poll_amount < 0) {
        container->fd_dest_count = 0;
        ret_code = rerror_get_osnet_err();
    } else if (poll_amount == 0) {
        container->fd_dest_count = 0;
        ret_code = rcode_ok;//超时而已
    } else {
        const repoll_item_t* fdptr;

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

//EPOLLET //边缘触发(Edge Triggered)事件改变仅第一次返回内核事件，水平触发(Level Triggered)有事件每次wait都返回
//EPOLLONESHOT //只监听一次事件，当监听完这次事件之后，如果还需要继续监听，需要再次把fd加入到EPOLL参数里
int repoll_reset_oneshot(repoll_container_t* container, int fd) {
    int ret_code = rcode_ok;

    struct epoll_event event = {0, {0}};
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;
    ret_code = epoll_ctl(container->epoll_fd, EPOLL_CTL_MOD, fd, &event);

    if (ret_code != 0) {
        ret_code = rerror_get_osnet_err();
    }

    return ret_code;
}


#else //__linux__

#endif //__linux__
