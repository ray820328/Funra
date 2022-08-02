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
#include "rbuffer.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rbuffer_full_test(void **state);

static int setup(void **state) {
    return rcode_ok;
}
static int teardown(void **state) {
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rbuffer_full_test, setup, teardown),
};

int run_rbuffer_tests(int benchmark_output) {
    int result = 0;

    int64_t time_now = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rbuffer_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - time_now));

    return result == 0 ? rcode_ok : -1;
}

static void rbuffer_full_test(void **state) {
    (void)state;
    rbuffer_size_t count = 100;
    int j;
    int temp;

    init_benchmark(1024, "test rbuffer - %"rbuffer_size_t_format, count);

    start_benchmark(0);
    rbuffer_t* buffer_ins = NULL;
    rbuffer_init(buffer_ins, count);
    assert_true(buffer_ins);
    end_benchmark("create rbuffer.");

    start_benchmark(0);

    end_benchmark("Fill array.");

    rbuffer_release(buffer_ins);


    uninit_benchmark();
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__