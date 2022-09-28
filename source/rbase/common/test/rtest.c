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

#include "rmemory.h"
#include "rlog.h"
#include "rbase/common/test/rtest.h"

#include "rtime.h"
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
    setlocale(LC_ALL, "zh_CN.UTF-8");
    setlocale(LC_NUMERIC, "zh_CN");
    setlocale(LC_TIME, "zh_CN");

    rmem_init();
    rtools_init();

#ifdef ros_windows
    wchar_t local_time[100];
    time_t t = time(NULL);
    wcsftime(local_time, 100, L"%A %c", localtime(&t));
    wprintf(L"PI: %.2f\n当前时间: %Ls\n", 3.14, local_time);
#endif //_WIN64

    rlog_init("${date}/rtest_common_${index}.log", RLOG_ALL, false, 100);

    char* exe_root = rdir_get_exe_root();
    rinfo("当前路径: %s", exe_root);
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
    rtools_uninit();
    rmem_uninit();

#ifndef __SUNPRO_C
    return rcode_ok;
#endif
}

static int run_tests(int output) {
    int ret_code;

    rtest_add_test_entry(run_rtime_tests);
    rtest_add_test_entry(run_rbuffer_tests);
    rtest_add_test_entry(run_rpool_tests);
    rtest_add_test_entry(run_rstring_tests);
    rtest_add_test_entry(run_rthread_tests);
    rtest_add_test_entry(run_rarray_tests);
	rtest_add_test_entry(run_rcommon_tests);
	rtest_add_test_entry(run_rdict_tests);
    rtest_add_test_entry(run_rlog_tests);
    rtest_add_test_entry(run_rfile_tests);
    rtest_add_test_entry(run_rtools_tests);

    ret_code = 0;

    rlist_iterator_t it = rlist_it(test_entries, rlist_dir_tail);
    rlist_node_t *node = NULL;
    while ((node = rlist_next(&it))) {
        ret_code = ((rtest_entry_type)(node->val))(output);
        if (ret_code != rcode_ok) {
            break;
        }
    }

    rdata_destroy(test_entries, rlist_destroy);

    return ret_code;
}

