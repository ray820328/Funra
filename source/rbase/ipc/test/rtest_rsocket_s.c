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
#include "rcodec_default.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rsocket_ctx_uv_t rsocket_ctx;
static rthread socket_thread;

static void* run_server(void* arg) {
    rsocket_s.open(&rsocket_ctx);
    //while (true)
    //{
    //    rtools_wait_mills(10);
    //}
    rinfo("end, run_server: %s\n", (char *)arg);

    return arg;
}

static void rsocket_s_full_test(void **state) {
	(void)state;
	int count = 10000;
	init_benchmark(1024, "test rsocket_s (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&socket_thread, run_server, "socket_thread"); // 0;// 
    //run_server("socket_thread");
    assert_true(ret_code == 0);
    
	end_benchmark("start listening.");
		
    uninit_benchmark();
}

static int setup(void **state) {
    //初始化配置
    rsocket_ctx.id = 1;
    rsocket_ctx.server_type = ripc_type_tcp;
    rsocket_ctx.server = (uv_handle_t*)rnew_data(uv_tcp_t);
    rsocket_ctx.loop = uv_default_loop();
    rsocket_ctx.server_state = 1;

    ripc_data_source_t* ds = rnew_data(ripc_data_source_t);
    ds->ds_type = ripc_data_source_type_server;
    ds->ds_id = rsocket_ctx.id;
    ds->ds = &rsocket_ctx;

    ((uv_tcp_t*)(rsocket_ctx.server))->data = ds;

    rsocket_cfg_t* cfg = (rsocket_cfg_t*)rnew_data(rsocket_cfg_t);
    rsocket_ctx.cfg = cfg;
    cfg->id = 1;
    cfg->sid_min = 100000;
    cfg->sid_max = 200000;
    rstr_set(cfg->ip, "0.0.0.0");
    cfg->port = 23000;

    rdata_handler_t* handler = (rdata_handler_t*)rnew_data(rdata_handler_t);
    rsocket_ctx.handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_default.on_before;
    handler->process = rcodec_default.process;
    handler->on_after = rcodec_default.on_after;
    handler->on_next = rcodec_default.on_next;
    handler->on_notify = rcodec_default.on_notify;
    handler->notify = rcodec_default.notify;

    rsocket_s.init(&rsocket_ctx, rsocket_ctx.cfg);
    rthread_init(&socket_thread);

    return rcode_ok;
}
static int teardown(void **state) {

    void* param;
    int ret_code = rthread_join(&socket_thread, &param);
    assert_true(ret_code == 0);
    assert_true(rstr_eq((char *)param, "socket_thread"));

    rsocket_s.uninit(&rsocket_ctx);

    //todo Ray 释放ctx资源配置

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