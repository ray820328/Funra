﻿/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"
#include "rlog.h"

#include "ripc.h"
#include "rcodec_default.h"

static int on_before(rdata_handler_t* handler, void* ds, void* data) {
    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    ripc_data_raw_t* data_raw = (ripc_data_raw_t*)data;

    rbuffer_write_ext(datasource->read_cache, data_raw->len);//数据已经写进去了，设置正确的pos即可

    return ripc_code_success;
}

int process(rdata_handler_t* handler, void* ds, void* data) {
    int ret_code = ripc_code_success;
    int read_len = 0;
    int require_len = 0;

    ret_code = handler->on_before(handler, ds, data);
    if (ret_code != ripc_code_success) {
        rerror("error on handler before, code: %d\n", ret_code);
        return ret_code;
    }

    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    rbuffer_t* buffer = datasource->read_cache;

    //char read_temp[8];
    //require_len = ripc_head_version_len;
    //read_len = rbuffer_read(buffer, read_temp, require_len);
    //if (read_len != require_len) {
    //    return ret_code;
    //}
    //int8_t version = (int8_t)read_temp[0];

    rdebug("process buffer...... %d\n", rbuffer_size(buffer));

    ripc_data_default_t ipc_data;
    require_len = ripc_head_default_version_len + ripc_head_default_magic_len + ripc_head_default_len_len;
        //+ ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;
    read_len = rbuffer_read(buffer, (char*)(&ipc_data), require_len);
    if (read_len != require_len) {
        return ret_code;
    }

    ret_code = handler->on_after(handler, ds, data);
    if (ret_code != ripc_code_success) {
        rerror("error on handler after, code: %d\n", ret_code);
        return ret_code;
    }

    return ret_code;
}

int on_after(rdata_handler_t* handler, void* ds, void* data) {
    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    rbuffer_t* buffer = datasource->read_cache;
    int left_size = rbuffer_left(buffer);

    if (left_size < rbuffer_capacity(buffer) / 2) {
        rbuffer_rewind(buffer);
    }

    left_size = rbuffer_left(buffer);
    if (left_size < 1 || left_size > rbuffer_capacity(buffer)) {
        return ripc_code_cache_full;
    }

    //下一个handler处理
    if (handler->next != NULL) {
        return handler->next->process(handler->next, datasource, data);
    }

    return ripc_code_success;
}

void on_next(rdata_handler_t* handler, void* ds, void* data) {

}

void on_notify(rdata_handler_t* handler, void* ds, void* data) {

}

void notify(rdata_handler_t* handler, void* ds, void* data) {

}

const rdata_handler_t rcodec_default = {
    NULL, //struct rdata_handler_s* prev;
    NULL, //struct rdata_handler_s* next;

    on_before, //rdata_handler_on_before_func on_before;
    process, //rdata_handler_process_func process;
    on_after, //rdata_handler_on_after_func on_after;
    on_next, //rdata_handler_on_next_func on_next;

    on_notify, //rdata_handler_on_notify on_notify;// 被通知
    notify, //rdata_handler_notify notify; // 通知其他
};