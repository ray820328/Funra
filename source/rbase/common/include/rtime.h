/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RTIME_H
#define RTIME_H

#include "rcommon.h"

#if defined(ros_darwin)
#include <sys/time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
int rtime_gettimeofdayfix(struct timeval *tp, void *tzp);
#endif

R_API int64_t rtime_nanosec();
R_API int64_t rtime_microsec();
R_API int64_t rtime_millisec();

R_API void rtime_set_inc(int64_t millisec, int64_t microsec, int64_t nanosec);
R_API int64_t rtime_nanosec_inc();
R_API int64_t rtime_microsec_inc();
R_API int64_t rtime_millisec_inc();

R_API void rtime_set_time_zone(int time_zone);
R_API int rtime_get_time_zone();
R_API int* rtime_from_time_millis(int64_t time_millis);
R_API int* rtime_from_time_millis_security(int64_t time_millis, int* datas);


/***************************** timeout ****************************/

typedef struct rtimeout_s {
    int64_t block;          /* max time for blocking calls，给阻塞调用用 */
    int64_t total;          /* total milliseconds for operation */
    int64_t start;          /* time of start */
} rtimeout_t;

#define rtimeout_init_microsec(tm, block_val, total_val) \
    do { \
        (tm)->block = (block_val); \
        (tm)->total = (total_val); \
    } while (0)

#define rtimeout_init_millisec(tm, block_val, total_val) \
    rtimeout_init_microsec(tm, (block_val) * 1000, (total_val) * 1000)

#define rtimeout_init_sec(tm, block_val, total_val) \
    rtimeout_init_microsec(tm, (block_val) * 1000000, (total_val) * 1000000)

#define rtimeout_get_start(tm) \
    (tm)->start

#define rtimeout_start(tm) \
    (tm)->start = rtime_microsec()

#define rtimeout_set_done(tm) ((tm)->block = 0)

#define rtimeout_done(tm) (rtimeout_get_total(tm) == 0)

#define rtimeout_2timeval(tm, tval, time_left) \
    do { \
        (tval)->tv_usec = (time_left) % 1000000; \
        (tval)->tv_sec = (time_left) - (tval)->tv_usec; \
    } while (0)

/**
  * 单位微秒
  * (block >= 0 && total < 0)返回block；
  * (block < 0 && total >= 0)返回total剩余值；
  * (block >= 0 && total >= 0)返回 min(block 和 total剩余值)；
  * 错误返回 -1 
  */
R_API int64_t rtimeout_get_block(rtimeout_t* tm);

/**
  * 单位微秒
  * (block >= 0 && total < 0)返回block剩余值；
  * (block < 0 && total >= 0)返回total剩余值；
  * (block >= 0 && total >= 0)返回 min(block 和 total剩余值)；
  * 错误返回 -1
  */
R_API int64_t rtimeout_get_total(rtimeout_t* tm);

#ifdef __cplusplus
}
#endif

#endif
