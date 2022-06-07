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
	rlist_t* file_list = rdir_list(dir_path, true, true);
	rlist_iterator_t it = rlist_it(file_list, rlist_dir_tail);
	rlist_node_t *node = NULL;
	while ((node = rlist_next(&it))) {
		rinfo("rfile_full_test filename: %s\n", (char*)(node->val));
		//assert_true(test_entries != 0);
	}
	end_benchmark("print to file.");
	rlist_destroy(file_list);

	start_benchmark(0);

	end_benchmark("rolling files.");

}


static int setup(void **state) {
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 0;
    *state = answer;

    dir_path = "./logs/local";

	rfile_make_dir(dir_path, true);

    return rcode_ok;
}
static int teardown(void **state) {
	//rfile_remove_dir(dir_path);
    dir_path = NULL;

    free(*state);
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rfile_full_test, NULL, NULL),
};

int run_rfile_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = nanosec_r();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rfile_tests, failed: %d, all time: %"PRId64" us\n", result, (nanosec_r() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__