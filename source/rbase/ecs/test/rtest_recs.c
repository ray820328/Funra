﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"
#include "rlog.h"
#include "rcommon.h"
#include "rtime.h"
#include "rlist.h"
#include "rfile.h"
#include "rtools.h"

#include "rbase/ecs/test/rtest.h"
#include "recs.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void recs_full_test(void **state) {
	(void)state;
	int count = 1;
	init_benchmark(1024, "test recs (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    assert_true(ret_code == 0);
	end_benchmark("test ecs.");

    uninit_benchmark();
}


static int setup(void **state) {

    return rcode_ok;
}
static int teardown(void **state) {
    //void* param;
    //int ret_code = rthread_join(&socket_thread, &param);
    //assert_true(ret_code == 0);
    //assert_true(rstr_eq((char *)param, "socket_thread"));
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(recs_full_test, NULL, NULL),
};

int run_recs_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_recs_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__