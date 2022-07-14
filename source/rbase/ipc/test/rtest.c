/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <locale.h>
#include <string.h>

#include "rlog.h"
#include "rbase/ipc/test/rtest.h"

#include "rlist.h"
#include "rstring.h"
#include "rfile.h"
#include "rtools.h"

static rlist_t *test_entries = NULL;

int rtest_add_test_entry(rtest_entry_type entry_func) {
    rlist_rpush(test_entries, entry_func);

	return rcode_ok;
}

static int init_platform();
static int run_tests(int output);

static int init_platform() {

    rlist_init(test_entries, rdata_type_ptr);

    return rcode_ok;
}

int main(int argc, char **argv) {
    rtools_init();

    rlog_init("${date}/rtest_ipc_${index}.log", RLOG_ALL, false, 100);

    char* exe_root = rdir_get_exe_root();
    rinfo("当前路径: %s\n", exe_root);
    rstr_free(exe_root);

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

    rtest_add_test_entry(run_rsocket_c_tests);
    rtest_add_test_entry(run_rcodec_default_tests);

    testResult = rcode_ok;

    testResult = run_rsocket_s_tests(output);
    if (testResult != rcode_ok) {
        return testResult;
    }

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

