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
#include "rcodec_default.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rsocket_ctx_uv_t rsocket_ctx;//非线程安全
static rthread socket_thread;//uv非线程安全，只能在loop线程启动和收发，否则用uv_async_send
static volatile int repeat_cb_count = 3;

static void repeat_cb(uv_timer_t* handle) {
    assert_true(handle != NULL);
    assert_true(1 == uv_is_active((uv_handle_t*)handle));
    if (rsocket_ctx.stream_state != 1) {
        rtools_wait_mills(5000);
        rsocket_c.open(&rsocket_ctx);
        return;
    }

    ripc_data_default_t data;
    data.cmd = 11;
    data.data = rstr_cpy("send test", 0);
    data.len = rstr_len(data.data);
    rsocket_c.send(((uv_tcp_t*)(rsocket_ctx.stream))->data, &data);//发送数据

    repeat_cb_count--;

    if (repeat_cb_count <= 0) {
        uv_close((uv_handle_t*)handle, NULL);
    }
}

static void* run_client(void* arg) {

    rsocket_ctx.id = 1;
    rsocket_ctx.stream_type = ripc_type_tcp;
    rsocket_ctx.stream = (uv_handle_t*)rdata_new(uv_tcp_t);
    rsocket_ctx.stream_state = 0; 
    uv_loop_t loop;
    assert_true(0 == uv_loop_init(&loop));
#ifdef _WIN32
    assert_true(UV_ENOSYS == uv_loop_configure(&loop, UV_LOOP_BLOCK_SIGNAL, 0));
#else
    assert_true(0 == uv_loop_configure(&loop, UV_LOOP_BLOCK_SIGNAL, SIGPROF));
#endif
    rsocket_ctx.loop = &loop;// uv_default_loop();

    ripc_data_source_t* ds = rdata_new(ripc_data_source_t);
    ds->ds_type = ripc_data_source_type_client;
    ds->ds_id = rsocket_ctx.id;
    ds->ctx = &rsocket_ctx;

    ((uv_tcp_t*)(rsocket_ctx.stream))->data = ds;

    rsocket_cfg_t* cfg = (rsocket_cfg_t*)rdata_new(rsocket_cfg_t);
    rsocket_ctx.cfg = cfg;
    cfg->id = 1;
    rstr_set(cfg->ip, "127.0.0.1", 0);
    cfg->port = 23000;

    rdata_handler_t* handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    rsocket_ctx.in_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_decode_default.on_before;
    handler->process = rcodec_decode_default.process;
    handler->on_after = rcodec_decode_default.on_after;
    handler->on_next = rcodec_decode_default.on_next;
    handler->on_notify = rcodec_decode_default.on_notify;
    handler->notify = rcodec_decode_default.notify;

    handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    rsocket_ctx.out_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_encode_default.on_before;
    handler->process = rcodec_encode_default.process;
    handler->on_after = rcodec_encode_default.on_after;
    handler->on_next = rcodec_encode_default.on_next;
    handler->on_notify = rcodec_encode_default.on_notify;
    handler->notify = rcodec_encode_default.notify;

    rsocket_c.init(&rsocket_ctx, rsocket_ctx.cfg);

    uv_timer_t timer_repeat;
    uv_timer_init(rsocket_ctx.loop, &timer_repeat);
    uv_timer_start(&timer_repeat, repeat_cb, 1000, 1000);

    rsocket_c.start(&rsocket_ctx);
    rinfo("end, run_client: %s", (char *)arg);

    rsocket_c.stop(&rsocket_ctx);
    rsocket_c.uninit(&rsocket_ctx);

    return arg;
}

static void rsocket_c_full_test(void **state) {
	(void)state;
	int count = 1;
	init_benchmark(1024, "test rsocket_c (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&socket_thread, run_client, "socket_thread"); // 0;// 
    //run_client("socket_thread");
	end_benchmark("open connection.");

    rtools_wait_mills(1000);

    rinfo("client uv started.");

    start_benchmark(0);
}


static int setup(void **state) {
    rthread_init(&socket_thread);

    return rcode_ok;
}
static int teardown(void **state) {
    //void* param;
    //int ret_code = rthread_join(&socket_thread, &param);
    //assert_true(ret_code == 0);
    //assert_true(rstr_eq((char *)param, "socket_thread"));
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_c_full_test, NULL, NULL),
};

int run_rsocket_c_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);
	//setup(0);
	//rsocket_c_full_test(0);
    printf("run_rsocket_c_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__