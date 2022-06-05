/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <rstring.h>

#include "rlog.h"

#include "rcommon.h"
#include "rtime.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static char* test_full_str = NULL;

static void rstring_index_test(void **state) {
    (void)state;

    int count = 100;
    int j;
	char* delim = "z0";

    init_benchmark(1024, "test rstring (%d)", count);

    //start_benchmark(0);
    //rstr_index(test_full_str, "cd");
    //end_benchmark("rstring index.");

    start_benchmark(0);
    char** tokens = rstr_split(test_full_str, delim);
    int count_token;
    rstr_array_count(tokens, count_token);
    assert_true(count_token == 3);

    char* token_cur = NULL;
    rstr_array_for(tokens, token_cur) {
        int cur_len = rstr_len(token_cur);
        assert_true(cur_len > 0);
    } 

	char* str_concat_full = rstr_concat(tokens, delim, false);
	assert_true(rstr_eq(str_concat_full, test_full_str));

	rstr_free(str_concat_full);
    rstr_array_free(tokens);

    end_benchmark("rstring token.");


}

static int setup(void **state) {
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 0;
    *state = answer;

    test_full_str = "abcderghijklmnopqrstuvwxyz0恭喜123456789z0发财ABCDEFJHIJKLMNOPQRSTUVWXYZ";

    return rcode_ok;
}
static int teardown(void **state) {
    test_full_str = NULL;

    free(*state);
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rstring_index_test, setup, teardown),
};

int run_rstring_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = nanosec_r();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rstring_tests, failed: %d, all time: %"PRId64" us\n", result, (nanosec_r() - timeNow));

    return result == 0 ? rcode_ok : -1;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__