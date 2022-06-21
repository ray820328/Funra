/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rpool.h"

#include "rlog.h"

#include "rcommon.h"
#include "rtime.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

rpool_define_global();

typedef struct rtest_pool_struct_t {
	int index;
	double value;
} rtest_pool_struct_t;

rpool_declare(rtest_pool_struct_t);// .h
rdefine_pool(rtest_pool_struct_t, 100, 20); // .c

static void rpool_full_test(void **state) {
    (void)state;

    int count = 1000;
    rtest_pool_struct_t* datas[1000] = { NULL };

    init_benchmark(1024, "test rpool (%d)", count);

    start_benchmark(0);
    if (rget_pool(rtest_pool_struct_t) == NULL) {
        rget_pool(rtest_pool_struct_t) = rcreate_pool(rtest_pool_struct_t);
        assert_true(rget_pool(rtest_pool_struct_t) != NULL);
    }
    end_benchmark("rpool init.");

    start_benchmark(0);
    //rpool_get(rtest_pool_struct_t, rget_pool(rtest_pool_struct_t));
    rtest_pool_struct_t* dd = rpool_rtest_pool_struct_t_get(data_rtest_pool_struct_t_pool);
    for (int i = 0; i < count; i++) {
        datas[i] = rpool_new_data(rtest_pool_struct_t);
        datas[i]->index = i;
        datas[i]->value = count + i;
    }
    assert_true(datas[count / 2]->value == count + count / 2);
    end_benchmark("rpool new data.");

    start_benchmark(0);
    if (rpool_chain) {
        rpool_chain_node_t* tempChainNode = rpool_chain;
        while ((tempChainNode = tempChainNode->next) != NULL) {
            if (tempChainNode->rpool_travel_block_func) {
                tempChainNode->rpool_travel_block_func(NULL);
            }
        }
    }
    end_benchmark("rpool travel all pools.");

    start_benchmark(0);
    rdestroy_pool(rtest_pool_struct_t);
    assert_true(rget_pool(rtest_pool_struct_t) == NULL);
    end_benchmark("rpool destroy pool.");
}

static int setup(void **state) {
    rpool_init_global();

    return rcode_ok;
}
static int teardown(void **state) {
    rpool_uninit_global();

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rpool_full_test, setup, teardown),
};

int run_rpool_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = nanosec_r();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rpool_tests, failed: %d, all time: %"PRId64" us\n", result, (nanosec_r() - timeNow));

    return result == 0 ? rcode_ok : -1;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__