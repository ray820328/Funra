/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rpool.h"

#include "rlog.h"

#include "rcommon.h"
#include "rtime.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

typedef struct rtest_pool_struct {
	int index;
	double value;
} rtest_pool_struct;

//rdefine_pool(rtest_pool_struct, 1000, 100);

static void rpool_full_test(void **state) {
    (void)state;

    int count = 100;
    int j;

    init_benchmark(1024, "test rpool (%d)", count);

    start_benchmark(0);
    // rstr_array_make(str_array, 3);
    // str_array[0] = "str";
    // str_array[1] = "array";
    // char* str_concat_array = rstr_concat(str_array, "_", false);
    // assert_true(rstr_eq(str_concat_array, "str_array"));
    // rstr_free(str_concat_array);
    end_benchmark("rstring concat.");

}

static int setup(void **state) {

    return rcode_ok;
}
static int teardown(void **state) {

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rpool_full_test, setup, teardown),
};

int run_rpool_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = nanosec_r();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rpool_tests, failed: %d, all time: %"PRId64" us\n", result, (nanosec_r() - timeNow));

    return result == 0 ? rcode_ok : -1;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__