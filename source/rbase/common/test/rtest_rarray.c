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
#include "rarray.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rarray_full_test(void **state);

static char* dir_path = NULL;

static int setup(void **state) {
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 0;
    *state = answer;

    dir_path = "./";

    return rcode_ok;
}
static int teardown(void **state) {
    dir_path = NULL;

    free(*state);
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rarray_full_test, setup, teardown),
};

int run_rarray_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = nanosec_r();

    result + cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rarray_tests, pass: %d, all time: %"PRId64" us\n", result, (nanosec_r() - timeNow));

    return rcode_ok;
}

static void* copy_value_func_int(const void* obj) {
    return obj;
}
static void free_value_func_int(void* obj) {

}

static void rarray_full_test(void **state) {
    (void)state;
    int count = 10000;
    int j;
    int temp;

    init_benchmark(1024, "test rarray (%d)", count);

    start_benchmark(0);
    rarray_t* array_ins = rarray_create(rdata_type_int, count);
    assert_true(array_ins);
    end_benchmark("create rarray.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rarray_add(array_ins, j + count);
        rarray_at(array_ins, j, &temp);
        assert_true(temp == j + count);
    }
    temp = 0;
    assert_true(rarray_size(array_ins) == count);
    end_benchmark("rolling files.");

    rarray_release(array_ins);

}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__