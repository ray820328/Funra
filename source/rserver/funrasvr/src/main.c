/** 
 * Copyright (c) 2021 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4319 4819)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#include "rcommon.h"
#include "rtime.h"
#include "ripc.h"
#include "rlog.h"

int main(int argc, char **argv) {
    init_rlog("rserver_%s_%s.log", RLOG_ALL, false, "all");// NULL);// 
    printdbgr("starting rserver...\n");

    int64_t timeNowNano = nanosec_r();
    int64_t timeNowMicro = microsec_r();
    int64_t timeNowMill = millisec_r();

    ripc_init(NULL);

    while (true) {
        wait_millsec(50);

    }
    printdbgr("timeNow: %"PRId64" 毫秒, %"PRId64" 微秒, %"PRId64" 纳秒, %"PRId64" us\n",
        (millisec_r() - timeNowMill), (microsec_r() - timeNowMicro), (nanosec_r() - timeNowNano), timeNowNano);

    //char* dataStr = char[64];
    //rformat_time_s_full(dataStr, timeNow);
    //printf("dataStr: %s\n\n", dataStr);

  return 1;
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
