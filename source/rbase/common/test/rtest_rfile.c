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
#include "rfile.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static char* dir_path = NULL;

static void rfile_full_test(void **state) {
	(void)state;
	int count = 10000;
	int j;
	init_benchmark(1024, "test rlog (%d)", count);

    start_benchmark(0);
    char* file_path = rstr_cpy("d:\\temp\\\\test\\", 0);
	rfile_format_path(file_path);
	assert_true(rstr_eq(file_path, "d:/temp/test"));
	rstr_free(file_path);
	end_benchmark("rfile_format_path.");
		
    start_benchmark(0);
    char* path_name = rdir_get_path_dir("d:\\temp\\\\test\\log_05.log");
    assert_true(rstr_eq(path_name, "d:/temp/test"));
    rstr_free(path_name);

    char* file_name = rdir_get_path_filename("d:\\temp\\\\test\\log_05.log");
    assert_true(rstr_eq(file_name, "log_05.log"));
    rstr_free(file_name);
    end_benchmark("get path dir and filename.");

    start_benchmark(0);
    char* file_path_concat = rfile_get_filepath("d:\\temp\\\\test", "log_05.log");
    assert_true(rstr_eq(file_path_concat, "d:/temp/test/log_05.log"));
    rstr_free(file_path_concat);
    end_benchmark("get filepath.");

	start_benchmark(0);
    for (int file_list_count = 0; file_list_count < 3; file_list_count++) {
        rlist_t* file_list = rdir_list("./", true, true);//dir_path
        rlist_iterator_t it = rlist_it(file_list, rlist_dir_tail);
        rlist_node_t *node = NULL;
        while ((node = rlist_next(&it))) {
            rinfo("rfile_full_test filename: %s\n", (char*)(node->val));
            //assert_true(test_entries != 0);
        }
        rlist_destroy(file_list);
    }
	end_benchmark("list dir.");

	start_benchmark(0);

	end_benchmark("rolling files.");

    uninit_benchmark();
}


static int setup(void **state) {
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 0;
    *state = answer;

    dir_path = "./test_dir/local";

    rdir_make(dir_path, true);
    rfile_create_in(dir_path, "file01.txt");
    rfile_create_in(dir_path, "file02.data");

    return rcode_ok;
}
static int teardown(void **state) {
    //assert_true(rdir_remove(dir_path) == rcode_ok);
    dir_path = NULL;

    free(*state);
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rfile_full_test, NULL, NULL),
};

int run_rfile_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rfile_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__