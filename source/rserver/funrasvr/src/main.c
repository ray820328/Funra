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
    rlog_init("${date}/rserver_${index}.log", RLOG_ALL, false, 100);
    rinfo("starting rserver...");

    int64_t timeNowNano = rtime_nanosec();
    int64_t timeNowMicro = rtime_microsec();
    int64_t timeNowMill = rtime_millisec();

    ripc_init(NULL);

    while (true) {
        wait_millsec(50);

    }
    rinfo("timeNow: %"PRId64" 毫秒, %"PRId64" 微秒, %"PRId64" 纳秒, %"PRId64" us",
        (rtime_millisec() - timeNowMill), (rtime_microsec() - timeNowMicro), (rtime_nanosec() - timeNowNano), timeNowNano);

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
