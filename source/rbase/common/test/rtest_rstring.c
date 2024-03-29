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
	char* delim = "z0";

    init_benchmark(1024, "test rstring (%d)", count);

    start_benchmark(0);
    char* str_join = rstr_join("test", "_", "join", " string", rstr_array_end);
    assert_true(rstr_eq(str_join, "test_join string"));
    rstr_free(str_join);
    end_benchmark("rstring join.");

    start_benchmark(0);
    char** str_array = rstr_make_array(2, "str", "array");
    char* str_concat_array = rstr_concat_array((const char**)str_array, "_", false);
    assert_true(rstr_eq(str_concat_array, "str_array"));
    rstr_free(str_concat_array);
    rstr_array_free(str_array);
    end_benchmark("rstring concat.");

    start_benchmark(0);
    char* sub_str = rstr_sub_str("123456789", "67", false);
    assert_true(rstr_eq(sub_str, "6789")); 
    sub_str = rstr_sub_str("123456789", "67", true);
    assert_true(rstr_eq(sub_str, "6789"));
    rstr_free(sub_str);
    end_benchmark("rstring sub string.");

    start_benchmark(0);
    int index = rstr_index(test_full_str, "z0");
    assert_true(index > 0);
    index = rstr_last_index(test_full_str, "/");
    assert_true(index > 0);
    index = rstr_last_index(test_full_str, "//");
    assert_true(index > 0);
    end_benchmark("rstring index.");

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

    char* str_concat_full = rstr_concat_array((const char**)tokens, delim, false);
    assert_true(rstr_eq(str_concat_full, test_full_str));

    rstr_free(str_concat_full);
    rstr_array_free(tokens);

    end_benchmark("rstring token.");

    start_benchmark(0);
    char* repl_str = rstr_repl("1234567896770000000000000000000000", "67", "_67_");
    assert_true(rstr_eq(repl_str, "12345_67_89_67_70000000000000000000000"));
    rstr_free(repl_str);
    end_benchmark("rstring replace.");


    uninit_benchmark();
}

static int setup(void **state) {
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 0;
    *state = answer;

    test_full_str = "abcderghijklmnopqrstuvwxyz0//恭喜123456789z0//发财ABCDEFJHIJKLMNOPQRSTUVWXYZ";

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

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rstring_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__