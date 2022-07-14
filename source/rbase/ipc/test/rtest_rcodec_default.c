/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <string.h>

#include "rcommon.h"
#include "rtime.h"
#include "rlist.h"
#include "rstring.h"

#include "rcodec_default.h"

#include "rbase/ipc/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rcodec_full_test(void **state) {
    (void)state;
    int count = 10000;
    init_benchmark(1024, "test rcodec (%d)", count);

    start_benchmark(0);
    // char* file_path = rstr_cpy("d:\\temp\\\\test\\", 0);
    // rfile_format_path(file_path);
    // assert_true(rstr_eq(file_path, "d:/temp/test"));
    // rstr_free(file_path);
    end_benchmark("rcodec_full_test.");
        
    uninit_benchmark();
}


static int setup(void **state) {
    // int *answer = malloc(sizeof(int));
    // assert_non_null(answer);
    // *answer = 0;
    // *state = answer;

    return rcode_ok;
}
static int teardown(void **state) {
    // free(*state);
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rcodec_full_test, NULL, NULL),
};

int run_rcodec_default_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rcodec_default_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__