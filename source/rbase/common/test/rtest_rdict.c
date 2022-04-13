/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <string.h>

#include "rlog.h"

#include "rcommon.h"
#include "rtime.h"
#include "rlist.h"
#include "rdict.h"
#include "dict.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

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

static uint64_t rhash_func_int(const void* key) {
    return (uint64_t)key;
}
//static void set_key_func_default(void* data_ext, rdict_entry* entry, const void* key) {
//    entry->key.s64 = key;
//}
//static void set_value_func_default(void* data_ext, rdict_entry* entry, const void* obj) {
//    entry->value.s64 = obj;
//}
static void rdict_int_test(void **state) {// 整数类型 k-v
    (void)state;
    int count = 10000;
    int j;

    rdict_entry de_temp = { .key.ptr = 0, .value.ptr = 0 };

    init_benchmark(1024, "test int rdict(%d) - %f", count, de_temp.key.d);

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
    rdict* dict_ins = rdict_create(count, 0, NULL);
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
    end_benchmark("Double elements and fill map.");

    start_benchmark(0);
    rdict_iterator it = rdict_it(dict_ins);
    for (rdict_entry *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        assert_true(de->key.s64 == de->value.s64 - count);
        //printf("rdict_iterator: %"PRId64" -> %"PRId64"\n", de->key.s64, de->value.s64);
    }
    end_benchmark("Iterator map.");

    start_benchmark(0);
    rdict_clear(dict_ins);
    assert_true(rdict_size(dict_ins) == 0);
    rdict_it_first(&it);
    for (rdict_entry *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        assert_true(false);
    }
    end_benchmark("Clear map.");

    start_benchmark(0);
    rdict_free(dict_ins);
    end_benchmark("Release map");
}



static uint64_t rhash_func_string(const void* key) {
    if (key) {
        return rhash_func_murmur((char*)key);
    }
    return 0;
}
static void* copy_key_func_string(void* data_ext, const void* key) {
    if (key) {
        return rstr_cpy(key);
    }
    return NULL;
}
static void* copy_value_func_string(void* data_ext, const void* obj) {
    if (obj) {
        return rstr_cpy(obj);
    }
    return NULL;
}
static void free_key_func_string(void* data_ext, void* key) {
    if (key) {
        rstr_free(key);
    }
}
static void free_value_func_string(void* data_ext, void* obj) {
    if (obj) {
        rstr_free(obj);
    }
}
static int compare_key_func_string(void* data_ext, const void* key1, const void* key2) {
    if (key1 && key2) {
        return rstr_eq(key1, key2);
    }
    return 0;
}
static void rdict_string_test(void **state) {// string类型 k-v
    (void)state;
    int count = 10000;
    int j;
    char key_str_buffer[24] = { '\0' };
    char value_str_buffer[24] = { '\0' };

    init_benchmark(1024, "test string rdict(%d)", count);

    start_benchmark(0);
    rdict* dict_ins = rdict_create(count, 32, NULL);
    assert_true(dict_ins);
    dict_ins->hash_func = rhash_func_string;
    dict_ins->copy_key_func = copy_key_func_string;
    dict_ins->copy_value_func = copy_value_func_string;
    dict_ins->free_key_func = free_key_func_string;
    dict_ins->free_value_func = free_value_func_string;
    dict_ins->compare_key_func = compare_key_func_string;
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
        rdict_entry *de = rdict_find(dict_ins, key_str_buffer);
        assert_true(de != NULL && rstr_2int(de->value.ptr) == count + j);
    }
    end_benchmark("Linear access existing elements");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int index = rand() % count;
        rformat_s(key_str_buffer, "%d", index);
        rdict_entry *de = rdict_find(dict_ins, key_str_buffer);
        assert_true(de != NULL && rstr_2int(de->value.ptr) == count + index);
    }
    end_benchmark("Random access existing elements");

    start_benchmark(0);
    for (j = count; j < 2 * count; j++) {
        rformat_s(key_str_buffer, "%d", j);
        rdict_entry *de = rdict_find(dict_ins, key_str_buffer);
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
        rdict_entry *de = rdict_find(dict_ins, key_str_buffer);
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
    rdict_iterator it = rdict_it(dict_ins);
    for (rdict_entry *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        assert_true(rstr_2int(de->key.ptr) == rstr_2int(de->value.ptr) - count);
        //printf("rdict_iterator: %"PRId64" -> %"PRId64"\n", de->key.s64, de->value.s64);
    }
    end_benchmark("Iterator map.");

    start_benchmark(0);
    rdict_clear(dict_ins);
    assert_true(rdict_size(dict_ins) == 0);
    rdict_it_first(&it);
    for (rdict_entry *de = NULL; (de = rdict_next(&it)) != NULL; ) {
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