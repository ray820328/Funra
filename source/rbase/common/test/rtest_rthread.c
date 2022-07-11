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
#include "rthread.h"
#include "rtools.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rthread_full_test(void **state);

static int setup(void **state) {

    return rcode_ok;
}
static int teardown(void **state) {

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rthread_full_test, setup, teardown),
};

int run_rthread_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rthread_tests, pass: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

static void* rfunc_test(void* arg) {
    srand((unsigned int)(rtime_millisec()));
    rinfo("start, thread: %s\n", (char *) arg);
    printf("start, thread: %s\n", (char *) arg);

    int wait_time = rtools_rand_int(10, 100);
    rtools_wait_mills(wait_time);

    rinfo("stop, thread: %s, wait = %d\n", (char *) arg, wait_time);
    printf("stop, thread: %s, wait = %d\n", (char *) arg, wait_time);
    return arg;
}
static void rthread_full_test(void **state) {
    (void)state;
    int count = 10000;

    int ret_code;
    void *ret;
    rthread thread;
    rthread thread2;

    init_benchmark(1024, "test rthread (%d)", count);

    start_benchmark(0);
    rthread_init(&thread);
    ret_code = rthread_start(&thread, rfunc_test, "first");
    assert_true(ret_code == 0);
    end_benchmark("thread init && start.");

    start_benchmark(0);
    ret_code = rthread_join(&thread, &ret);
    assert_true(ret_code == 0);
    assert_true(rstr_eq((char *) ret, "first"));
    end_benchmark("thread join.");

    start_benchmark(0);
    ret_code = rthread_uninit(&thread);
    assert_true(ret_code == 0);
    ret_code = rthread_uninit(&thread);
    assert_true(ret_code == 0);
    end_benchmark("thread uninit.");

    start_benchmark(0);
    rthread_init(&thread);
    ret_code = rthread_start(&thread, rfunc_test, "first");
    assert_true(ret_code == 0);
    rtools_wait_mills(5);
    rthread_init(&thread2);
    ret_code = rthread_start(&thread2, rfunc_test, "second");
    assert_true(ret_code == 0);
    ret_code = rthread_uninit(&thread);
    assert_true(ret_code == 0);
    ret_code = rthread_uninit(&thread2);
    assert_true(ret_code == 0);
    end_benchmark("thread multi runing.");

	start_benchmark(0);
	
	end_benchmark("thread lock.");

    uninit_benchmark();
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__