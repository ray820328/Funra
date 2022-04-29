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

static void rdict_int_test(void **state) {// 整数类型 k-v
    (void)state;
    int count = 10000;
    int j;

    rdict_entry_t de_temp = { .key.ptr = 0, .value.ptr = 0 };

    init_benchmark(1024, "test int rdict_t(%d) - %f", count, de_temp.key.d);

    //rdict_t dict_ins_stack = {
    //    NULL, NULL, 0, 0, 0, 0, 0.0f, NULL,

    //    //dic_hash_callback,
    //    NULL,
    //    NULL,
    //    NULL,
    //    NULL,
    //    NULL,
    //    NULL,
    //    NULL
    //};
    
    start_benchmark(0);
    rdict_t* dict_ins = rdict_create(count, 0, NULL);
    assert_true(dict_ins);
    dict_ins->hash_func = rhash_func_int;
    //dict_ins->set_key_func = set_key_func_default;
    //dict_ins->set_value_func = set_value_func_default;
    end_benchmark("Create map.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int ret = rdict_add(dict_ins, j, count + j);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == count);
    end_benchmark("Fill map.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rdict_entry_t *de = rdict_find(dict_ins, j);
        assert_true(de != NULL && de->value.s64 == count + j);
    }
    end_benchmark("Linear access existing elements");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int index = rand() % count;
        rdict_entry_t *de = rdict_find(dict_ins, index);
        assert_true(de != NULL && de->value.s64 == count + index);
    }
    end_benchmark("Random access existing elements");

    start_benchmark(0);
    for (j = count; j < 2 * count; j++) {
        rdict_entry_t *de = rdict_find(dict_ins, j);
        assert_true(de == NULL);
    }
    end_benchmark("Access missing keys");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int ret = rdict_remove(dict_ins, j);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == 0);
    end_benchmark("Remove all keys");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rdict_entry_t *de = rdict_find(dict_ins, j);
        assert_true(de == NULL);
    }
    end_benchmark("Linear access not existing elements");

    start_benchmark(0);
    for (j = 0; j < 2 * count; j++) {
        int ret = rdict_add(dict_ins, j, count + j);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == 2 * count);
    end_benchmark("Double elements and fill map.");

    start_benchmark(0);
    rdict_iterator_t it = rdict_it(dict_ins);
    for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        assert_true(de->key.s64 == de->value.s64 - count);
        //printf("rdict_iterator_t: %"PRId64" -> %"PRId64"\n", de->key.s64, de->value.s64);
    }
    end_benchmark("Iterator map.");

    start_benchmark(0);
    rdict_clear(dict_ins);
    assert_true(rdict_size(dict_ins) == 0);
    rdict_it_first(&it);
    for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        assert_true(false);
    }
    end_benchmark("Clear map.");

    start_benchmark(0);
    rdict_free(dict_ins);
    end_benchmark("Release map");
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__