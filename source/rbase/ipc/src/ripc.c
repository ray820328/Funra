/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rcommon.h"
#include "rlog.h"
#include "ripc.h"
#include "rsocket.h"

int ripc_init(const void* cfg_data) {

    return rcode_ok;
}

int ripc_uninit() {

    return rcode_ok;
}

int ripc_on_code(rdata_handler_t* handler, void* ds, void* data, int code) {
    int code_ret = rcode_err_ok;

    if (code <= rcode_err_ipc_begin || code >= rcode_err_ipc_end) {
        code_ret = code;
        rinfo("invalid code = %d", code);

        return code_ret;
    }

    switch(code){
        case rcode_err_ipc_disconnect:

            break;
        default:
            code_ret = code;
            rinfo("undefined code = %d", code);
            break;
    }

    return code_ret;
}
