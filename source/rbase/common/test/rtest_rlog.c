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
#include "rdict.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

// static int setup(void **state) {
//     int *answer = malloc(sizeof(int));
//     assert_non_null(answer);
//     *answer = 42;
//     *state = answer;
//     return 0;
// }
// static int teardown(void **state) {
//     free(*state);
//     return 0;
// }
// const struct CMUnitTest test_group2[] = {
//     cmocka_unit_test_setup_teardown(int_test_success, setup, teardown),
// };
//int result = 0;
//result += cmocka_run_group_tests(test_group2, NULL, NULL);

static int init();
static int uninit();

static void rlog_full_test(void **state);

const static struct CMUnitTest tests[] = {
    cmocka_unit_test(rlog_full_test),
};

static int init() {
    int total;

    rcount_array(tests, total);

    fprintf(stdout, "total: %d\n", total);
    fflush(stdout);

    return 0;
}

static int uninit() {

    return 0;
}

int run_rlog_tests(int benchmark_output) {
    init();

    int64_t timeNow = nanosec_r();

    cmocka_run_group_tests(tests, NULL, NULL);

    printf("run_rlog_tests all time: %"PRId64" us\n", (nanosec_r() - timeNow));

    uninit();

    return 0;
}

static void rlog_full_test(void **state) {
    (void)state;
    int count = 10000;
    int j;

    rdict_entry_t de_temp = { .key.ptr = 0, .value.ptr = 0 };

    init_benchmark(1024, "test int rdict_t(%d) - %f", count, de_temp.key.d);

    //start_benchmark(0);
    //for (j = 0; j < count; j++) {
    //    int ret = rdict_add(dict_ins, j, count + j);
    //    assert_true(ret == rdict_code_ok);
    //}
    //assert_true(rdict_size(dict_ins) == count);
    //end_benchmark("Fill map.");

}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__