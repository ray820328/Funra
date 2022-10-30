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
#include "rtools.h"

#include "rbase/ecs/test/rtest.h"
#include "recs.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static recs_context_t recs_context;

static void recs_full_test(void **state) {
	(void)state;
	int count = 1;
	init_benchmark(1024, "test recs (%d)", count);

    int ret_code = 0;
    recs_context_t* ctx = &recs_context;

    start_benchmark(0);

    rtest_cmp_t* cmp_item = (rtest_cmp_t*)recs_cmp_new(ctx, recs_ctype_rtest01);

    assert_true(cmp_item->id > 0);
	end_benchmark("test ecs.");

    uninit_benchmark();
}


static int setup(void **state) {
    recs_context_t* ctx = &recs_context;

    ctx->sid_min = 10000;
    ctx->sid_max = 11000;
    // ctx->on_init = ;
    // ctx->on_uninit = ;
    ctx->create_cmp = rtest_recs_cmp_new;

    recs_init(ctx, NULL);

    return rcode_ok;
}
static int teardown(void **state) {
    recs_context_t* ctx = &recs_context;

    recs_uninit(ctx, NULL);
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(recs_full_test, NULL, NULL),
};

int run_recs_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_recs_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__