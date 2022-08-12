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

 /** ------------------------编码---------------------- **/

static int encode_on_before(rdata_handler_t* handler, void* ds, void* data) {
    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    ripc_data_default_t* ipc_data = (ripc_data_default_t*)data;

    static int header_len = ripc_head_default_version_len + ripc_head_default_magic_len + ripc_head_default_len_len
        + ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;

    if (rbuffer_size(datasource->write_buff) == 0) {
        rbuffer_rewind(datasource->write_buff);//只有全写完才能rewind
    }
    
    if (rbuffer_left(datasource->write_buff) < (ipc_data->len + header_len)) {
        rerror("error on handler before, is full, left: %d\n", rbuffer_left(datasource->write_buff));
        return ripc_code_cache_full;
    }

    return ripc_code_success;
}

static int encode_process(rdata_handler_t* handler, void* ds, void* data) {
    int ret_code = ripc_code_success;
    int payload_len = 0;
    int write_len = 0;
    int require_len = 0;
    static int trans_len = ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;
    static int header_len = ripc_head_default_version_len + ripc_head_default_magic_len + ripc_head_default_len_len
        + ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;

    ret_code = handler->on_before(handler, ds, data);
    if (ret_code != ripc_code_success) {
        rerror("error on handler before, code: %d\n", ret_code);
        return ret_code;
    }

    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    ripc_data_default_t* ipc_data = (ripc_data_default_t*)data;
    payload_len = ipc_data->len;
    rbuffer_t* buffer = datasource->write_buff;

    ipc_data->version = 0;
    rstr_set(ipc_data->magic, ripc_head_default_magic, ripc_head_default_magic_len);//0溢出，le覆盖
    ipc_data->len = trans_len + payload_len;
    ipc_data->sid = datasource->ds_id;
    ipc_data->crc = 0;
    ipc_data->reserve0 = 0;

    write_len = rbuffer_write(buffer, (char*)(ipc_data), header_len);
    if (write_len != header_len) {
        rbuffer_read_ext(buffer, -write_len);
        rerror("error on handler process head: %d\n", write_len);
        return ripc_code_cache_full;
    }

    write_len = rbuffer_write(buffer, (char*)(ipc_data->data), payload_len);
    if (write_len != payload_len) {
        rbuffer_read_ext(buffer, -write_len - header_len);
        rerror("error on handler process payload: %d\n", write_len);
        return ripc_code_cache_full;
    }

    ret_code = handler->on_after(handler, ds, data);
    if (ret_code != ripc_code_success) {
        rbuffer_read_ext(buffer, -write_len - header_len);
        rerror("error on handler after, code: %d\n", ret_code);
        return ret_code;
    }

    rdebug("encode, process buffer size: %d\n", rbuffer_size(buffer));

    return ret_code;
}

static int encode_on_after(rdata_handler_t* handler, void* ds, void* data) {
    //下一个handler处理
    if (handler->next != NULL) {
        return handler->next->process(handler->next, ds, data);
    }

    return ripc_code_success;
}

static void encode_on_next(rdata_handler_t* handler, void* ds, void* data) {

}

static void encode_on_notify(rdata_handler_t* handler, void* ds, void* data) {

}

static void encode_notify(rdata_handler_t* handler, void* ds, void* data) {

}


/** ------------------------解码---------------------- **/

static int decode_on_before(rdata_handler_t* handler, void* ds, void* data) {
    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    ripc_data_raw_t* data_raw = (ripc_data_raw_t*)data;

    rbuffer_write_ext(datasource->read_cache, data_raw->len);//数据已经写进去了，设置正确的pos即可

    return ripc_code_success;
}

static int decode_process(rdata_handler_t* handler, void* ds, void* data) {
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
        rbuffer_read_ext(buffer, -read_len);
        return ripc_code_success;//长度不够，返回去继续读
    }

    if (!rmem_eq(ipc_data.magic, ripc_head_default_magic, ripc_head_default_magic_len)) {
        rerror("error on handler process, magic error: %d\n", ripc_code_error_magic);
        return ripc_code_error_magic;
    }

    ret_code = handler->on_after(handler, ds, data);
    if (ret_code != ripc_code_success) {
        rerror("error on handler after, code: %d\n", ret_code);
        return ret_code;
    }

    return ret_code;
}

static int decode_on_after(rdata_handler_t* handler, void* ds, void* data) {
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

static void decode_on_next(rdata_handler_t* handler, void* ds, void* data) {

}

static void decode_on_notify(rdata_handler_t* handler, void* ds, void* data) {

}

static void decode_notify(rdata_handler_t* handler, void* ds, void* data) {

}


const rdata_handler_t rcodec_encode_default = {
    NULL, //struct rdata_handler_s* prev;
    NULL, //struct rdata_handler_s* next;

    encode_on_before, //rdata_handler_on_before_func on_before;
    encode_process, //rdata_handler_process_func process;
    encode_on_after, //rdata_handler_on_after_func on_after;
    encode_on_next, //rdata_handler_on_next_func on_next;

    encode_on_notify, //rdata_handler_on_notify on_notify;// 被通知
    encode_notify, //rdata_handler_notify notify; // 通知其他
};

const rdata_handler_t rcodec_decode_default = {
    NULL, //struct rdata_handler_s* prev;
    NULL, //struct rdata_handler_s* next;

    decode_on_before, //rdata_handler_on_before_func on_before;
    decode_process, //rdata_handler_process_func process;
    decode_on_after, //rdata_handler_on_after_func on_after;
    decode_on_next, //rdata_handler_on_next_func on_next;

    decode_on_notify, //rdata_handler_on_notify on_notify;// 被通知
    decode_notify, //rdata_handler_notify notify; // 通知其他
};
