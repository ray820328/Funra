/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rlog.h"
#include "rcommon.h"
#include "rtime.h"
#include "rstring.h"
#include "rtools.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rtime_full_test(void **state) {
	(void)state;
    rtimeout_t tm;
    struct timeval tv, *tp = NULL;
    int64_t time_left = 0;
    int count = 0;

	init_benchmark(1024, "test rtime (%d)", count);

    start_benchmark(0);

    count = 5;
    rtimeout_init_millisec(&tm, -1, 500);
    rtimeout_start(&tm);
    while ((time_left = rtimeout_get_block(&tm)) > 0) {
        rtimeout_2timeval(tm, &tv, time_left);
        tp = &tv;
        rinfo("time block: %"PRId64", sec: %ld, micro: %ld", time_left, tp->tv_sec, tp->tv_usec);
        count--;
        rtools_wait_mills(100);
    }
    assert_true(count == 0 || count == 1);

    count = 5;
    rtimeout_init_millisec(&tm, 500, -1);
    rtimeout_start(&tm);
    while ((time_left = rtimeout_get_total(&tm)) > 0) {
        rtimeout_2timeval(tm, &tv, time_left);
        tp = &tv;
        rinfo("time total: %"PRId64", sec: %ld, micro: %ld", time_left, tp->tv_sec, tp->tv_usec);
        count--;
        rtools_wait_mills(100);
    }
    assert_true(count == 0 || count == 1);
    assert_true(rtimeout_done(&tm));

    count = 5;
    rtimeout_init_millisec(&tm, 300, 500);
    rtimeout_start(&tm);
    while ((time_left = rtimeout_get_block(&tm)) > 0) {//还是会按500停止，block值不变，最终为0的是total计算值
        rtimeout_2timeval(tm, &tv, time_left);
        tp = &tv;
        rinfo("time both: %"PRId64", sec: %ld, micro: %ld", time_left, tp->tv_sec, tp->tv_usec);
        count--;
        rtools_wait_mills(100);
    }
    assert_true(count == 0 || count == 1);
    assert_true(rtimeout_done(&tm));

    count = 5;
    rtimeout_init_millisec(&tm, 300, 500);
    rtimeout_start(&tm);
    while ((time_left = rtimeout_get_total(&tm)) > 0) {//还是会按500停止，block值不变，最终为0的是total计算值
        rtimeout_2timeval(tm, &tv, time_left);
        tp = &tv;
        rinfo("time both: %"PRId64", sec: %ld, micro: %ld", time_left, tp->tv_sec, tp->tv_usec);
        count--;
        rtools_wait_mills(100);
    }
    assert_true(count == 0 || count == 1);
    assert_true(rtimeout_done(&tm));

	end_benchmark("rtime timeout.");

    uninit_benchmark();
}


static int setup(void **state) {

    return rcode_ok;
}
static int teardown(void **state) {

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rtime_full_test, NULL, NULL),
};

int run_rtime_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rtime_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__