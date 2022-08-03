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
    rbuffer_size_t count = 62;
    int j;
    int temp;
    char* buf_write = rstr_new(count);
    char buf_read[10];

    init_benchmark(1024, "test rbuffer - %"rbuffer_size_t_format, count);

    start_benchmark(0);
    rbuffer_t* buffer_ins = NULL;
    rbuffer_init(buffer_ins, count);
    assert_true(buffer_ins);
    end_benchmark("create buffer.");

    start_benchmark(0);
    for (j = 0; j < count; j++) { // '0'=48 'A'=65 (char)(65 + 0)
        buf_write[j] = (char)(0 + j);
    }
    temp = rbuffer_write(buffer_ins, buf_write, count);
    assert_true(temp == count);
    end_benchmark("write buffer.");

    start_benchmark(0);
    rbuffer_read(buffer_ins, buf_read, rstr_sizeof(buf_read));
    for (j = 0; j < rstr_sizeof(buf_read); j++) {
        assert_true(buf_read[j] == buf_write[j]);
    }
    end_benchmark("read buffer.");

    start_benchmark(0);
    temp = rbuffer_write(buffer_ins, buf_write, count);
    assert_true(temp == rstr_sizeof(buf_read));
    end_benchmark("rewind and write buffer.");

    start_benchmark(0);
    rbuffer_skip(buffer_ins, count - rstr_sizeof(buf_read));
    rbuffer_read(buffer_ins, buf_read, rstr_sizeof(buf_read));
    for (j = 0; j < rstr_sizeof(buf_read); j++) {
        assert_true(buf_read[j] == buf_write[j]);
    }
    end_benchmark("skip buffer.");

    rbuffer_release(buffer_ins);
    rstr_free(buf_write);

    uninit_benchmark();
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__