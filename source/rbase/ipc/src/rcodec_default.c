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
#include "rsocket.h"
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
        rerror("error on handler before, is full, left: %d", rbuffer_left(datasource->write_buff));
        return rcode_err_ipc_cache_full;
    }

    return rcode_err_ok;
}

static int encode_process(rdata_handler_t* handler, void* ds, void* data) {
    int ret_code = rcode_err_ok;
    int payload_len = 0;
    int write_len = 0;
    // int require_len = 0;
    static int trans_len = ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;
    static int header_len = ripc_head_default_version_len + ripc_head_default_magic_len + ripc_head_default_len_len
        + ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;
	
    ret_code = handler->on_before(handler, ds, data);
    if (ret_code != rcode_err_ok) {
        rerror("error on handler before, code: %d", ret_code);
        return ret_code;
    }
	
    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    ripc_data_default_t* ipc_data = (ripc_data_default_t*)data;
    payload_len = ipc_data->len;
    rbuffer_t* buffer = datasource->write_buff;

    ipc_data->version = 0;
    rstr_set(ipc_data->magic, ripc_head_default_magic, ripc_head_default_magic_len);//0溢出，le覆盖
    ipc_data->len = htonl(trans_len + payload_len);
    ipc_data->cmd = htonl(ipc_data->cmd);
    ipc_data->sid = htonll(datasource->ds_id);
    ipc_data->crc = htonl(ipc_data->crc);
    ipc_data->reserve0 = htonl(ipc_data->reserve0);
	
    write_len = rbuffer_write(buffer, (char*)(ipc_data), header_len);
    if (write_len != header_len) {
        rbuffer_read_ext(buffer, -write_len);
        ipc_data->len = (uint32_t)ntohl(ipc_data->len);
        ipc_data->cmd = (int32_t)ntohl(ipc_data->cmd);
        ipc_data->sid = (uint64_t)ntohll(ipc_data->sid);
        ipc_data->crc = (int32_t)ntohl(ipc_data->crc);
        ipc_data->reserve0 = (uint64_t)ntohll(ipc_data->reserve0);
        rerror("error on handler process head: %d", write_len);
        return rcode_err_ipc_cache_full;
    }

    write_len = rbuffer_write(buffer, (char*)(ipc_data->data), payload_len);//data部分逻辑保证字节序，比如pb
    if (write_len != payload_len) {
        rbuffer_read_ext(buffer, -write_len - header_len);
        rerror("error on handler process payload: %d", write_len);
        return rcode_err_ipc_cache_full;
    }

    ret_code = handler->on_after(handler, ds, data);
    if (ret_code != rcode_err_ok) {
        rbuffer_read_ext(buffer, -write_len - header_len);
        rerror("error on handler after, code: %d", ret_code);
        return ret_code;
    }

    rdebug("encode, process buffer size: %d", rbuffer_size(buffer));

    return ret_code;
}

static int encode_on_after(rdata_handler_t* handler, void* ds, void* data) {
    //下一个handler处理
    if (handler->next != NULL) {
        return handler->next->process(handler->next, ds, data);
    }

    return rcode_err_ok;
}

static int encode_on_code(rdata_handler_t* handler, void* ds, void* data, int code) {
    rdebug("on code. code = %d", code);
    
    int code_ret = ripc_on_code(handler, ds, data, code);
    return code_ret;
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

    return rcode_err_ok;
}

static int decode_process(rdata_handler_t* handler, void* ds, void* data) {
    int ret_code = rcode_err_ok;
    int read_len = 0;
    int require_len = 0;
	static int full_head_len = ripc_head_default_version_len + ripc_head_default_magic_len + ripc_head_default_len_len
		+ ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;
	static int data_head_len = 
		ripc_head_default_cmd_len + ripc_head_default_sid_len + ripc_head_default_crc_len + ripc_head_default_reserve0_len;
	
    ret_code = handler->on_before(handler, ds, data);
    if (ret_code != rcode_err_ok) {
        rerror("error on handler before, code: %d", ret_code);
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

    rdebug("process buffer...... %d", rbuffer_size(buffer));

    ripc_data_default_t ipc_data;
	while (true) {
		require_len = full_head_len;
		read_len = rbuffer_read(buffer, (char*)(&ipc_data), require_len);
		if (read_len != require_len) {
			rbuffer_read_ext(buffer, -read_len);
			break;//长度不够，返回去继续读
		}

		if (!rmem_eq(ipc_data.magic, ripc_head_default_magic, ripc_head_default_magic_len)) {
			rerror("error on handler process, magic error: %d", rcode_err_ipc_magic);
			rbuffer_clear(buffer);
			return rcode_err_ipc_magic;
		}

        ipc_data.len = (uint32_t)ntohl(ipc_data.len);
        ipc_data.cmd = (int32_t)ntohl(ipc_data.cmd);
        ipc_data.sid = (uint64_t)ntohll(ipc_data.sid);
        ipc_data.crc = (int32_t)ntohl(ipc_data.crc);
        ipc_data.reserve0 = (uint64_t)ntohll(ipc_data.reserve0);

		require_len = ipc_data.len - data_head_len;
		ipc_data.data = rdata_new_size(require_len + 1);
		read_len = rbuffer_read(buffer, (char*)(ipc_data.data), require_len);

		rdebug("received msg: %d - %d - %d", require_len, read_len, rbuffer_size(buffer));
		if (read_len != require_len) {
			rbuffer_read_ext(buffer, -read_len);
			break;//长度不够，返回去继续读
		}

		ipc_data.data[require_len] = rstr_end;
        rdebug("received(ds_type=%d, cmd=%d) msg(len=%d): %s", datasource->ds_type, ipc_data.cmd, require_len, ipc_data.data);
        
        if (datasource->ds_type == ripc_data_source_type_session) {
			rsocket_ctx_t* rsocket_ctx = datasource->ctx;
            ripc_data_default_t data_send;
			data_send.cmd = 101;
			data_send.data = rstr_join(ipc_data.data, " - server response.", rstr_array_end);//未释放
            data_send.len = rstr_len(data_send.data);
            rsocket_ctx->ipc_entry->send(datasource, &data_send);
            rdata_free(char*, data_send.data);

            if (ipc_data.cmd == 999) {
                rsocket_ctx->ipc_entry->stop(rsocket_ctx);
			}
		}

        rdata_free(char*, ipc_data.data);
	}

    ret_code = handler->on_after(handler, ds, data);
    if (ret_code != rcode_err_ok) {
        rerror("error on handler after, code: %d", ret_code);
        return ret_code;
    }

    return ret_code;
}

static int decode_on_after(rdata_handler_t* handler, void* ds, void* data) {
    ripc_data_source_t* datasource = (ripc_data_source_t*)(ds);
    rbuffer_t* buffer = datasource->read_cache;
    int left_size = rbuffer_left(buffer);

    if (rbuffer_size(buffer) == 0 || left_size < rbuffer_capacity(buffer) / 2) {
        rbuffer_rewind(buffer);
    }

    left_size = rbuffer_left(buffer);
    if (left_size < 1 || left_size > rbuffer_capacity(buffer)) {
        return rcode_err_ipc_cache_full;
    }

    //下一个handler处理
    if (handler->next != NULL) {
        return handler->next->process(handler->next, datasource, data);
    }

    return rcode_err_ok;
}


static int decode_on_code(rdata_handler_t* handler, void* ds, void* data, int code) {
    rdebug("on code. code = %d", code);
    
    int code_ret = ripc_on_code(handler, ds, data, code);
    return code_ret;
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
    encode_on_code,//rdata_handler_on_code_func;
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
    decode_on_code,//rdata_handler_on_code_func;
    decode_on_after, //rdata_handler_on_after_func on_after;
    decode_on_next, //rdata_handler_on_next_func on_next;

    decode_on_notify, //rdata_handler_on_notify on_notify;// 被通知
    decode_notify, //rdata_handler_notify notify; // 通知其他
};
