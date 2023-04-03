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
	int count = 3;
	init_benchmark(1024, "test recs (%d)", count);

    int ret_code = 0;
    recs_context_t* ctx = &recs_context;

    assert_true(recs_start(ctx, NULL) == rcode_ok);

    start_benchmark(0);
    ret_code = recs_sys_add(ctx, (recs_system_t*)rtest_recs_test_system);
    assert_true(ret_code == rcode_ok);
    end_benchmark("test add recs_system.");

    start_benchmark(0);
    rtest_cmp_t* cmp_item = (rtest_cmp_t*)recs_cmp_new(ctx, recs_ctype_rtest01);
    assert_true(cmp_item->id > 0);
	end_benchmark("test create recs_component.");

    start_benchmark(0);
    for (int i = 0; i < count; i++) {
        recs_run(ctx, NULL);
        assert_true(ret_code == rcode_ok);
    }
    end_benchmark("test running recs.");

    start_benchmark(0);
    ret_code = recs_sys_remove(ctx, (recs_system_t*)rtest_recs_test_system);
    assert_true(ret_code == rcode_ok);
    end_benchmark("test remove recs_system.");

    uninit_benchmark();
}


static int setup(void **state) {
    recs_context_t* ctx = &recs_context;
    rdata_init(ctx, sizeof(*ctx));

    ctx->sid_min = 10000;
    ctx->sid_max = 11000;
    // ctx->on_init = ;
    // ctx->on_uninit = ;
    ctx->create_cmp = rtest_recs_cmp_new;

    assert_true(recs_init(ctx, NULL) == rcode_ok);

    return rcode_ok;
}
static int teardown(void **state) {
    recs_context_t* ctx = &recs_context;

    assert_true(recs_uninit(ctx, NULL) == rcode_ok);
    
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