/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rcodec_default.h"

static int rcodec_init(const void* cfg_data) {


    return rcode_ok;
}

static int rcodec_uninit() {

    return rcode_ok;
}

const rcodec rcodec_default = {
    rcodec_init,
    rcodec_uninit
};