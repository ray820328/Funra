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
#include "rsocket_uv_s.h"
#include "rcodec_default.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rsocket_server_ctx_uv_t rsocket_ctx;
static rthread_t socket_thread;//uv非线程安全，只能在loop线程启动和收发，否则用uv_async_send

static void* run_server(void* arg) {

    //初始化配置
    rsocket_ctx_uv_t* ctx = (rsocket_ctx_uv_t*)&rsocket_ctx;
    ctx->id = 1;
    ctx->stream_type = ripc_type_tcp;
    ctx->stream = (uv_handle_t*)rdata_new(uv_tcp_t);
    ctx->stream_state = 1;
    //ctx->loop = uv_default_loop();
    uv_loop_t loop;
    assert_true(0 == uv_loop_init(&loop));
#ifdef _WIN32
    assert_true(UV_ENOSYS == uv_loop_configure(&loop, UV_LOOP_BLOCK_SIGNAL, 0));
#else
    assert_true(0 == uv_loop_configure(&loop, UV_LOOP_BLOCK_SIGNAL, SIGPROF));
#endif
    ctx->loop = &loop;

	rsocket_ctx.ipc_entry = &rsocket_uv_s;

    ripc_data_source_t* ds = rdata_new(ripc_data_source_t);
    ds->ds_type = ripc_data_source_type_server;
    ds->ds_id = ctx->id;
    ds->ctx = &rsocket_ctx;

    ((uv_tcp_t*)(ctx->stream))->data = ds;

    rsocket_cfg_t* cfg = (rsocket_cfg_t*)rdata_new(rsocket_cfg_t);
    ctx->cfg = cfg;
    cfg->id = 1;
    cfg->sid_min = 100000;
    cfg->sid_max = 2000000;
    rstr_set(cfg->ip, "0.0.0.0", 0);
    cfg->port = 23000;

    rsocket_ctx.sid_cur = cfg->sid_min;

    rdata_handler_t* handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    ctx->in_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_decode_default.on_before;
    handler->process = rcodec_decode_default.process;
    handler->on_after = rcodec_decode_default.on_after;
    handler->on_next = rcodec_decode_default.on_next;
    handler->on_notify = rcodec_decode_default.on_notify;
    handler->notify = rcodec_decode_default.notify;

    handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    ctx->out_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_encode_default.on_before;
    handler->process = rcodec_encode_default.process;
    handler->on_after = rcodec_encode_default.on_after;
    handler->on_next = rcodec_encode_default.on_next;
    handler->on_notify = rcodec_encode_default.on_notify;
    handler->notify = rcodec_encode_default.notify;

	rsocket_ctx.ipc_entry->init(&rsocket_ctx, ctx->cfg);
	rsocket_ctx.ipc_entry->open(&rsocket_ctx);
    rsocket_ctx.ipc_entry->start(&rsocket_ctx);//loop until to call stop

    rsocket_ctx.ipc_entry->close(&rsocket_ctx);
    rsocket_ctx.ipc_entry->uninit(&rsocket_ctx);

    rdata_free(rdata_handler_t, ctx->in_handler);
    rdata_free(rdata_handler_t, ctx->out_handler);
    rdata_free(ripc_data_source_t, ds);
    rdata_free(rsocket_cfg_t, cfg);
    rdata_free(uv_tcp_t, ctx->stream);

    rinfo("end, run_uv_server: %s", (char *)arg);

    return arg;
}

static void rsocket_uv_s_full_test(void **state) {
	(void)state;
	int count = 10000;
	init_benchmark(1024, "test rsocket_uv_s (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&socket_thread, run_server, "socket_uv_thread"); // 0;// 
    //run_server("socket_thread");
    assert_true(ret_code == 0);
    
	end_benchmark("start listening.");
		
    uninit_benchmark();
}

static int setup(void **state) {
    rthread_init(&socket_thread);

    return rcode_ok;
}
static int teardown(void **state) {
    rinfo("join socket_uv_thread.");
    void* param;
    int ret_code = rthread_join(&socket_thread, &param);
    assert_true(ret_code == 0);
    assert_true(rstr_eq((char *)param, "socket_uv_thread"));

    //todo Ray 释放ctx资源配置

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_uv_s_full_test, NULL, NULL),
};

int run_rsocket_uv_s_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rsocket_uv_s_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__