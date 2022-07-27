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
//     return rcode_ok;
// }
// static int teardown(void **state) {
//     free(*state);
//     return rcode_ok;
// }
// const struct CMUnitTest test_group2[] = {
//     cmocka_unit_test_setup_teardown(int_test_success, setup, teardown),
// };
//int result = 0;
//result += cmocka_run_group_tests(test_group2, NULL, NULL);

static int init();
static int uninit();

static void rdict_int_test(void **state);
static void rdict_string_test(void **state);

const static struct CMUnitTest tests[] = {
    cmocka_unit_test(rdict_int_test),
    cmocka_unit_test(rdict_string_test),
};

static int init() {
    int total;

    rcount_array(tests, total);

    fprintf(stdout, "total: %d\n", total);
    fflush(stdout);

    return rcode_ok;
}

static int uninit() {

    return rcode_ok;
}

int run_rdict_tests(int benchmark_output) {
    init();

    int64_t timeNow = rtime_nanosec();

    int result = 0;
    result += cmocka_run_group_tests(tests, NULL, NULL);

    printf("run_rcommon_tests all time: %"PRId64" us\n", (rtime_nanosec() - timeNow));

    uninit();

    return result == 0 ? rcode_ok : -1;
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
    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_int, rdata_type_int, 0, 0);
    assert_true(dict_ins);
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

    uninit_benchmark();
}


static void rdict_string_test(void **state) {// string类型 k-v
    (void)state;
    int count = 10000;
    int j;
    char key_str_buffer[24] = { '\0' };
    char value_str_buffer[24] = { '\0' };

    init_benchmark(1024, "test string rdict_t(%d)", count);

    start_benchmark(0);
    rdict_t* dict_ins = NULL;
    rdict_init(dict_ins, rdata_type_string, rdata_type_string, count, 0);
    assert_true(dict_ins);
    end_benchmark("Create string map.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rformat_s(key_str_buffer, "%d", j);
        rformat_s(value_str_buffer, "%d", count + j);
        int ret = rdict_add(dict_ins, key_str_buffer, value_str_buffer);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == count);
    end_benchmark("Fill string map.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rformat_s(key_str_buffer, "%d", j);
        rdict_entry_t *de = rdict_find(dict_ins, key_str_buffer);
        assert_true(de != NULL);
        //rinfo("key: %s, val: %s\n", de->key.ptr, de->value.ptr);
        assert_true(de != NULL && rstr_2int(de->value.ptr) == count + j);
    }
    end_benchmark("Linear access existing elements");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int index = rand() % count;
        rformat_s(key_str_buffer, "%d", index);
        rdict_entry_t *de = rdict_find(dict_ins, key_str_buffer);
        assert_true(de != NULL && rstr_2int(de->value.ptr) == count + index);
    }
    end_benchmark("Random access existing elements");

    start_benchmark(0);
    for (j = count; j < 2 * count; j++) {
        rformat_s(key_str_buffer, "%d", j);
        rdict_entry_t *de = rdict_find(dict_ins, key_str_buffer);
        assert_true(de == NULL);
    }
    end_benchmark("Access missing keys");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rformat_s(key_str_buffer, "%d", j);
        int ret = rdict_remove(dict_ins, key_str_buffer);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == 0);
    end_benchmark("Remove all keys");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rformat_s(key_str_buffer, "%d", j);
        rdict_entry_t *de = rdict_find(dict_ins, key_str_buffer);
        assert_true(de == NULL);
    }
    end_benchmark("Linear access not existing elements");

    start_benchmark(0);
    for (j = 0; j < 2 * count; j++) {
        rformat_s(key_str_buffer, "%d", j);
        rformat_s(value_str_buffer, "%d", count + j);
        int ret = rdict_add(dict_ins, key_str_buffer, value_str_buffer);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == 2 * count);
    end_benchmark("Double elements and fill map.");

    start_benchmark(0);
    rdict_iterator_t it = rdict_it(dict_ins);
    for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        assert_true(rstr_2int(de->key.ptr) == rstr_2int(de->value.ptr) - count);
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


    uninit_benchmark();
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__