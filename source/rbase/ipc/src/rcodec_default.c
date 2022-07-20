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

#include "ripc.h"
#include "rcodec_default.h"

static int on_before(void* data) {

    return rcode_ok;
}

int process(void* data) {
    ripc_data_raw_t* data_raw = (ripc_data_raw_t*)data;

    rinfo("process...... %d\n", data_raw->len);

    return rcode_ok;
}

int on_after(void* data) {

    return rcode_ok;
}

void on_next(void* data) {

}

void on_notify(void* data) {

}

void notify(void* data) {

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