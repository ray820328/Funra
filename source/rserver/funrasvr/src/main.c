/** 
 * Copyright (c) 2021 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#pragma warning(disable : 4819)

#include "rcommon.h"

int main(int argc, char **argv) {
    printf("starting rserver...\n");

    int64_t timeNowNano = nanosec_r();
    int64_t timeNowMicro = microsec_r();
    int64_t timeNowMill = millisec_r();
    wait_seconds(1);

    printf("timeNow: %"PRId64" 毫秒, %"PRId64" 微秒, %"PRId64" 纳秒, %"PRId64" us\n", 
        (millisec_r() - timeNowMill), (microsec_r() - timeNowMicro), (nanosec_r() - timeNowNano), timeNowNano);

    //char* dataStr = char[64];
    //rformat_time_s_full(dataStr, timeNow);
    //printf("dataStr: %s\n\n", dataStr);

  return 1;
}
