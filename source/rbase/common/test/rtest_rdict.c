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
#include "rdict.h"
#include "dict.h"

#include "rbase/common/test/rtest.h"

static int init();
static int uninit();

static void rdict_int_test(void **state);

const static struct CMUnitTest tests[] = {
    cmocka_unit_test(rdict_int_test),
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

int run_rdict_tests(int benchmark_output) {
    init();

    int64_t timeNow = nanosec_r();

    cmocka_run_group_tests(tests, NULL, NULL);

    printf("run_rcommon_tests all time: %"PRId64" us\n", (nanosec_r() - timeNow));

    uninit();

    return 0;
}

static void set_key_func_default(void* data_ext, rdict_entry* entry, const void* key) {
    entry->key.s64 = key;
}
static void set_value_func_default(void* data_ext, rdict_entry* entry, const void* obj) {
    entry->value.s64 = obj;
}
static void rdict_int_test(void **state) {
    (void)state;
    int count = 10000;
    long j;

    rdict_entry de_temp = { .key.ptr = 0, .value.ptr = 0 };

    init_benchmark(1024, "test rdict(%d) - %d", count, de_temp.key.d);

    //rdict dict_ins_stack = {
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
    rdict* dict_ins = rdict_create(count, NULL);
    assert_true(dict_ins);
    dict_ins->set_key_func = set_key_func_default;
    dict_ins->set_value_func = set_value_func_default;
    end_benchmark("Create map.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int ret = rdict_add(dict_ins, j, count + j);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == count);
    end_benchmark("fill map.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rdict_entry *de = rdict_find(dict_ins, j);
        assert_true(de != NULL && de->value.s64 == count + j);
    }
    end_benchmark("Linear access existing elements");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int index = rand() % count;
        rdict_entry *de = rdict_find(dict_ins, index);
        assert_true(de != NULL && de->value.s64 == count + index);
    }
    end_benchmark("Random access existing elements");

    start_benchmark(0);
    for (j = count; j < 2 * count; j++) {
        rdict_entry *de = rdict_find(dict_ins, j);
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
        rdict_entry *de = rdict_find(dict_ins, j);
        assert_true(de == NULL);
    }
    end_benchmark("Linear access not existing elements");

    start_benchmark(0);
    for (j = 0; j < 2 * count; j++) {
        int ret = rdict_add(dict_ins, j, count + j);
        assert_true(ret == rdict_code_ok);
    }
    assert_true(rdict_size(dict_ins) == 2 * count);
    end_benchmark("fill map.");

    start_benchmark(0);
    rdict_iterator it = rdict_it(dict_ins);
    for (rdict_entry *de = NULL; de = rdict_next(&it); ) {
        assert_true(de->key.s64 == de->value.s64 - count);
        //printf("rdict_iterator: %"PRId64" -> %"PRId64"\n", de->key.s64, de->value.s64);
    }
    end_benchmark("iterator map.");

    start_benchmark(0);
    rdict_clear(dict_ins);
    assert_true(rdict_size(dict_ins) == 0);
    rdict_it_first(&it);
    for (rdict_entry *de = NULL; de = rdict_next(&it); ) {
        assert_true(false);
    }
    end_benchmark("clear map.");

    start_benchmark(0);
    rdict_free(dict_ins);
    end_benchmark("Release map");
}



