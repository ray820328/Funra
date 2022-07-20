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

#include "rbase/ipc/test/rtest.h"
#include "rsocket_c.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rthread socket_thread;

static void* run_client(void* arg) {
    rsocket_c.open(NULL);
    rinfo("end, run_client: %s\n", (char *)arg);

    return arg;
}

static void rsocket_c_full_test(void **state) {
	(void)state;
	int count = 10000;
	init_benchmark(1024, "test rsocket_c (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&socket_thread, run_client, "socket_thread"); // 0;// 
    run_client("socket_thread");
	end_benchmark("open connection.");

    rtools_wait_mills(1000);

    start_benchmark(0);
    ripc_data_t data;
    data.data = rstr_cpy("send test", 0);
    data.len = rstr_len(data.data);
    rsocket_c.send(NULL, &data);//发送数据
    end_benchmark("send data.");

    rtools_wait_mills(1000);
		
    uninit_benchmark();
}


static int setup(void **state) {
    rsocket_c.init(NULL, NULL);

    return rcode_ok;
}
static int teardown(void **state) {
    void* param;
    int ret_code = rthread_join(&socket_thread, &param);
    assert_true(ret_code == 0);
    assert_true(rstr_eq((char *)param, "socket_thread"));
    
    rsocket_c.uninit(NULL);

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_c_full_test, NULL, NULL),
};

int run_rsocket_c_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rsocket_c_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__