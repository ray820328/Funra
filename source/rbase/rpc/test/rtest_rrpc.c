/**
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

#include "rbase/rpc/test/rtest.h"
#include "rrpc.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rrpc_full_test(void **state) {
	(void)state;
	int count = 1;
	init_benchmark(1024, "test rrpc (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    assert_true(ret_code == 0);
	end_benchmark("full test rpc.");

    rtools_wait_mills(1000);

    uninit_benchmark();
}


static int setup(void **state) {
    return rcode_ok;
}
static int teardown(void **state) {
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rrpc_full_test, NULL, NULL),
};

int run_rrpc_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);
    printf("run_rrpc_c_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__