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
#include "rthread.h"
#include "rtools.h"

#include "rbase/ipc/test/rtest.h"
#include "rsocket_s.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rthread socket_thread;

static void* run_server(void* arg) {
    rsocket_s.open();
    //while (true)
    //{
    //    rtools_wait_mills(10);
    //}
    printf("stop, run_server: %s\n", (char *)arg);

    return arg;
}

static void rsocket_s_full_test(void **state) {
	(void)state;
	int count = 10000;
	init_benchmark(1024, "test rsocket_s (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = 0;// rthread_start(&socket_thread, run_server, "socket_thread");
    run_server("socket_thread");
    assert_true(ret_code == 0);
    
	end_benchmark("start listening.");
		
    uninit_benchmark();
}


static int setup(void **state) {
    rsocket_s.init(NULL);
    rthread_init(&socket_thread);

    return rcode_ok;
}
static int teardown(void **state) {
    //rsocket_s.uninit();

    //void* param;
    //int ret_code = rthread_join(&socket_thread, &param);
    //assert_true(ret_code == 0);
    //assert_true(rstr_eq((char *)param, "socket_thread"));

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_s_full_test, NULL, NULL),
};

int run_rsocket_s_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rsocket_s_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__