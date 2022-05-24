﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <string.h>

#include "rlog.h"
#include "rbase/common/test/rtest.h"

#include "rlist.h"

static rlist_t *test_entries = NULL;

int rtest_add_test_entry(rtest_entry_type entry_func) {
    rlist_rpush(test_entries, entry_func);

	return rcode_ok;
}

static int init_platform();
static int run_tests(int output);

static int init_platform() {

    test_entries = rlist_new(raymalloc);
    assert_true(test_entries != NULL);

    rlist_init(test_entries);
    test_entries->malloc_node = raymalloc;
    test_entries->free_node = rayfree;
    test_entries->free_self = rayfree;

    return rcode_ok;
}

int main(int argc, char **argv) {
    rlog_init("RLog.txt", RLOG_ALL, false, "all");

    init_platform();

    run_tests(0);
    // switch (argc) {
    // case 1: return run_tests(0);
    // default:
    //   fprintf(stderr, "Too many arguments.\n");
    //   fflush(stderr);
    // }

    rlog_uninit();

#ifndef __SUNPRO_C
    return rcode_ok;
#endif
}

static int run_tests(int output) {
    int testResult;

    rtest_add_test_entry(run_rstring_tests);
    rtest_add_test_entry(run_rthread_tests);
    rtest_add_test_entry(run_rarray_tests);
	rtest_add_test_entry(run_rcommon_tests);
	rtest_add_test_entry(run_rdict_tests);
    rtest_add_test_entry(run_rlog_tests);
    rtest_add_test_entry(run_rfile_tests);
	

    testResult = 0;

    rlist_iterator_t it = rlist_it(test_entries, rlist_dir_tail);
    rlist_node_t *node = NULL;
    while ((node = rlist_next(&it))) {
		testResult = ((rtest_entry_type)(node->val))(output);
        if (testResult != rcode_ok) {
            break;
        }
    }

    return testResult;
}

