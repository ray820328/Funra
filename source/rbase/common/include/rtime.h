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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    int rtime_gettimeofdayfix(struct timeval *tp, void *tzp);
#endif

    extern int64_t rtime_nanosec_r();
    extern int64_t rtime_microsec_r();
    extern int64_t rtime_millisec_r();

    extern void rtime_set_time_zone(int timeZone);
    extern int rtime_get_time_zone();
    extern int* rtime_from_time_millis(int64_t timeMillis);
    extern int* rtime_from_time_millis_security(int64_t timeMillis, int* datas);

#ifdef __cplusplus
}
#endif

#endif
