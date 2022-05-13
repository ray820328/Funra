/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "ripc.h"
#include "rsocket.h"

int ripc_init(const void* cfg_data) {

    rsocket_init(cfg_data);

    return rcode_ok;
}

int ripc_uninit() {

    return rcode_ok;
}
