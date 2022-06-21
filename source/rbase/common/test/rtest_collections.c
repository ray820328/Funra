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
#include "dict.h"
#include "rstring.h"

#include "rbase/common/test/rtest.h"

static int init();
static int uninit();
// int run_tests(int benchmark_output);
//rattribute_unused(static int run_test(const char* test, int benchmark_output, int test_count));

static void rlist_test(void **state);
static void dict_test(void **state);

const static struct CMUnitTest tests[] = {
    cmocka_unit_test(rlist_test),
    cmocka_unit_test(dict_test),
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

int run_rcommon_tests(int benchmark_output) {
    init();

    int64_t timeNow = nanosec_r();

    int result = 0;
    result += cmocka_run_group_tests(tests, NULL, NULL);

    printf("run_rcommon_tests all time: %"PRId64" us\n", (nanosec_r() - timeNow));

    uninit();

    return result == 0 ? rcode_ok : -1;
}

static void rlist_test(void **state) {
    (void)state;
    int count = 10;
    count = count > 4 ? count : 5;

    rlist_t *ret_list = NULL;
    rlist_init(ret_list, rdata_type_string);

    assert_int_equal(ret_list->len, 0);

    for (int i = 0; i < count; i++) {
        char* value = raymalloc(50);
        if (value) {
            sprintf(value, "listNode - %d", i);

            rlist_rpush(ret_list, value);
        }
    }

    assert_int_equal(ret_list->len, count);

    char* nodeValue = "listNode - 3";
    rlist_node_t *nodeFind = rlist_find(ret_list, nodeValue);

    assert_true(nodeFind);

    assert_true(nodeFind->val);

    assert_string_equal(nodeFind->val, nodeValue);

    rlist_remove(ret_list, nodeFind);

    assert_int_equal(ret_list->len, count - 1);

    assert_true(rlist_at(ret_list, 0));  // first
    assert_true(rlist_at(ret_list, 1));  // second
    assert_true(rlist_at(ret_list, -1)); // last
    assert_true(rlist_at(ret_list, -3)); // third last
    assert_false(rlist_at(ret_list, count - 1));

    rlist_iterator_t it = rlist_it(ret_list, rlist_dir_tail);
    rlist_node_t *node = rlist_next(&it);
    while (node) {
        assert_true(node->val);
        node = rlist_next(&it);
    }

    rdestroy_object(ret_list, rlist_destroy);

    assert_false(ret_list);
}

static uint64_t dic_hash_callback(const void *key) {
    return dictGenHashFunction((unsigned char*)key, (int)strlen((char*)key));
}
static int dic_compare_callback(void *privdata, const void *key1, const void *key2) {
    int l1, l2;
    rvoid(privdata);

    l1 = (int)strlen((char*)key1);
    l2 = (int)strlen((char*)key2);
    if (l1 != l2) return rcode_ok;
    return memcmp(key1, key2, l1) == 0;
}
static void dic_free_callback(void *privdata, void *val) {
    rvoid(privdata);

    zfree(val);
}

static char *dic_longlong_2string(long long value) {
    char buf[32];
    int len;
    char *s;

    len = sprintf(buf, "%lld", value);
    s = zmalloc(len + 1);
    memcpy(s, buf, len);
    s[len] = '\0';
    return s;
}


static void dict_test(void **state) {
    (void)state;
    int count = 10000;
    count = count > 0 ? count : 10000;
    long j;

    init_benchmark(1024, "test dict(%d)", count);

    dictType BenchmarkDictType = {
        dic_hash_callback,
        NULL,
        NULL,
        dic_compare_callback,
        dic_free_callback,
        NULL,
        NULL
    };

    start_benchmark(0);
    dict *dict = dictCreate(&BenchmarkDictType, NULL);
    assert_true(dict);
    end_benchmark("Linear access of existing elements (2nd round)");


    start_benchmark(0);
    for (j = 0; j < count; j++) {
        int retval = dictAdd(dict, dic_longlong_2string(j), (void*)j);
        assert_true(retval == DICT_OK);
    }
    assert_true((long)dictSize(dict) == count);

    /* Wait for rehashing. */
    while (dictIsRehashing(dict)) {
        dictRehashMilliseconds(dict, 100);
    }

    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(j);
        dictEntry *de = dictFind(dict, key);
        assert_true(de != NULL);
        zfree(key);
    }

    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(j);
        dictEntry *de = dictFind(dict, key);
        assert_true(de != NULL);
        zfree(key);
    }
    end_benchmark("Linear access of existing elements (2nd round)");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(rand() % count);
        dictEntry *de = dictFind(dict, key);
        assert_true(de != NULL);
        zfree(key);
    }
    end_benchmark("Random access of existing elements");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        dictEntry *de = dictGetRandomKey(dict);
        assert_true(de != NULL);
    }
    end_benchmark("Accessing random keys");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(rand() % count);
        key[0] = 'X';
        dictEntry *de = dictFind(dict, key);
        assert_true(de == NULL);
        zfree(key);
    }
    end_benchmark("Accessing missing");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(j);
        int retval = dictDelete(dict, key);
        assert_true(retval == DICT_OK);
        key[0] += 17; /* Change first number to letter. */
        retval = dictAdd(dict, key, (void*)j);
        assert_true(retval == DICT_OK);
    }
    end_benchmark("Removing and adding");
    dictRelease(dict);

    uninit_benchmark();
}
