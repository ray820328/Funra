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

#include "rbase/ipc/test/rtest.h"
#include "rsocket_c.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rsocket_c_full_test(void **state) {
	(void)state;
	int count = 10000;
	init_benchmark(1024, "test rsocket_c (%d)", count);

    start_benchmark(0);
    rsocket_c.open();
	end_benchmark("open connection.");
		
    uninit_benchmark();
}


static int setup(void **state) {
    rsocket_c.init(NULL);

    return rcode_ok;
}
static int teardown(void **state) {
    rsocket_c.uninit();

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_c_full_test, NULL, NULL),
};

int run_rsocket_c_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rsocket_c_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__