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
#include "rstring.h"

#include "rbase/common/test/rtest.h"

static int init();
static int uninit();
// int run_tests(int benchmark_output);
//rattribute_unused(static int run_test(const char* test, int benchmark_output, int test_count));

static void rlist_test(void **state);

const static struct CMUnitTest tests[] = {
    cmocka_unit_test(rlist_test),
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

    int64_t timeNow = rtime_nanosec();

    int result = 0;
    result += cmocka_run_group_tests(tests, NULL, NULL);

    printf("run_rcommon_tests all time: %"PRId64" us\n", (rtime_nanosec() - timeNow));

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
        char* value = rstr_new(50);
        if (value) {
            sprintf(value, "listNode - %d", i);

            rlist_rpush(ret_list, value);
        }
        rstr_free(value);
    }

    assert_int_equal(ret_list->len, count);

    char* node_val = "listNode - 3";
    rlist_node_t *node_find = rlist_find(ret_list, node_val);

    assert_true(node_find);

    assert_true(node_find->val);

    assert_string_equal(node_find->val, node_val);

    rlist_remove(ret_list, node_find);

    assert_int_equal(ret_list->len, count - 1);

    assert_true(rlist_at(ret_list, 0));  // first
    assert_true(rlist_at(ret_list, 1));  // second
    assert_true(rlist_at(ret_list, -1)); // last
    assert_true(rlist_at(ret_list, -3)); // third last
    assert_false(rlist_at(ret_list, count - 1));

    node_find = rlist_rpop(ret_list);
    assert_false(rlist_at(ret_list, count - 2));
    rlist_free_node(ret_list, node_find);

    rlist_iterator_t it = rlist_it(ret_list, rlist_dir_tail);
    rlist_node_t *node = rlist_next(&it);
    while (node) {
        assert_true(node->val);
        node = rlist_next(&it);
    }

    rdata_destroy(ret_list, rlist_destroy);

    assert_false(ret_list);
}

