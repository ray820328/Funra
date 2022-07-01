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

static void rlog_full_test(void **state) {
    (void)state;
    int count = 1000;
    int j;

    init_benchmark(1024, "test rlog (%d)", count);

    start_benchmark(0);
    for (j = 0; j < count; j++) {
		//rdebug("范德萨发三个\n");
		//rinfo("噶士大夫胜多负少\n");
    }
    end_benchmark("print to file.");

	start_benchmark(0);
	//rlog_rolling_file();
	end_benchmark("rolling files.");

    uninit_benchmark();
}

static char* dir_path;
static int setup(void **state) {
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 0;
    *state = answer;

    dir_path = "./logs/local";

    rdir_make(dir_path, true);

    return rcode_ok;
}
static int teardown(void **state) {
    //rdir_remove(dir_path);
    dir_path = NULL;

    free(*state);
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rlog_full_test, NULL, NULL),
};

int run_rlog_tests(int benchmark_output) {
    int64_t timeNow = rtime_nanosec();

    int result = 0;
    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rlog_tests all time: %"PRId64" us\n", (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__