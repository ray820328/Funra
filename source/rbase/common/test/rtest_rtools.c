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

static void rtools_rand_test(void **state) {
	(void)state;
	int count = 10;
	init_benchmark(1024, "test rtools rand (%d)", count);

    start_benchmark(0);
    int rand_val = 0;
    printf("rand_val = %d", rand_val);
    for (int i = 0; i < count; i++) {
        rand_val = rtools_rand_int(100, 200);
        printf("%d, ", rand_val);
        assert_true(rand_val >= 100 && rand_val <= 200);
    }
    printf("end rand\n");
	end_benchmark("rtools rand.");

    uninit_benchmark();
}

static void rtools_timeout_test(void **state) {
	(void)state;
	int count = 1;
	init_benchmark(1024, "test rtools rtimeout (%d)", count);

	start_benchmark(0);
	rtimeout_t tm;
	for (int i = 0; i < count; i++) {
		rtimeout_init_millisec(&tm, 2, 0);
		rtimeout_start(&tm);
		rtools_wait_mills(3);
		rinfo("t = %lld", rtimeout_get_block(&tm));
		assert_true(rtimeout_done(&tm));
	}
	end_benchmark("rtools rtimeout.");

	uninit_benchmark();
}


static int setup(void **state) {

    return rcode_ok;
}
static int teardown(void **state) {

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rtools_rand_test, NULL, NULL),
	cmocka_unit_test_setup_teardown(rtools_timeout_test, NULL, NULL),
};

int run_rtools_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rtools_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__