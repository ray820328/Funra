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

static rsocket_ctx_t rsocket_ctx;//非线程安全
static rthread_t socket_thread;
static volatile int sent_times = 3;

static void* run_client(void* arg) {
    int ret_code = 0;

    rsocket_ctx_t* ctx = &rsocket_ctx;

    ctx->id = 3008;
    ctx->stream_type = ripc_type_tcp;

    ctx->ipc_entry = (ripc_entry_t*)rsocket_c;//windows默认sellect，linux默认poll

    ripc_data_source_t* ds = rdata_new(ripc_data_source_t);
    ds->ds_type = ripc_data_source_type_client;
    ds->ds_id = ctx->id;
    ds->ctx = &rsocket_ctx;

    ctx->ds = ds;

    rsocket_cfg_t* cfg = (rsocket_cfg_t*)rdata_new(rsocket_cfg_t);
    ctx->cfg = cfg;
    cfg->id = 1;
    rstr_set(cfg->ip, "127.0.0.1", 0);
    cfg->port = 23000;

    rdata_handler_t* handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    ctx->in_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_decode_default.on_before;
    handler->process = rcodec_decode_default.process;
    handler->on_error = rcodec_decode_default.on_error;
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
    handler->on_error = rcodec_encode_default.on_error;
    handler->on_after = rcodec_encode_default.on_after;
    handler->on_next = rcodec_encode_default.on_next;
    handler->on_notify = rcodec_encode_default.on_notify;
    handler->notify = rcodec_encode_default.notify;

    rtools_wait_mills(5000);

    ret_code = ctx->ipc_entry->init(&rsocket_ctx, ctx->cfg);
    assert_true(ret_code == rcode_ok);
    ret_code = ctx->ipc_entry->open(&rsocket_ctx);
    assert_true(ret_code == rcode_ok);

    ret_code = ctx->ipc_entry->start(&rsocket_ctx);
    assert_true(ret_code == rcode_ok);

	while (sent_times-- > 0) {
		ripc_data_default_t data;
		data.cmd = 11;
		data.data = rstr_cpy("client rsocket_c (win=select,linux=poll)  send test", 0);
		data.len = rstr_len(data.data);
		ctx->ipc_entry->send(ds, &data);

        rtools_wait_mills(1000);
        ctx->ipc_entry->check(ds, NULL);//send & recv

        rdata_free(char*, data.data);

        rtools_wait_mills(1000);
	}

    ctx->ipc_entry->stop(&rsocket_ctx);

    ctx->ipc_entry->close(&rsocket_ctx);
    ctx->ipc_entry->uninit(&rsocket_ctx);

    rdata_free(rdata_handler_t, ctx->in_handler);
    rdata_free(rdata_handler_t, ctx->out_handler);
    rdata_free(ripc_data_source_t, ds);
    rdata_free(rsocket_cfg_t, cfg);

    rinfo("end, run_client success: %s", (char *)arg);

    return arg;
}

static void rsocket_select_c_test(void **state) {
	(void)state;
	int count = 1;
	init_benchmark(1024, "test rsocket_c (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&socket_thread, run_client, "socket_thread"); // 0;// 
    //run_client("socket_thread");
    assert_true(ret_code == 0);
	end_benchmark("open connection.");

    rtools_wait_mills(1000);

    rinfo("client select started.");

    uninit_benchmark();
}


static int setup(void **state) {
    rthread_init(&socket_thread);

    return rcode_ok;
}
static int teardown(void **state) {
    void* param;
    // int ret_code = rthread_join(&socket_thread, &param);
    int ret_code = rthread_detach(&socket_thread, &param);
    assert_true(ret_code == 0);
    // assert_true(rstr_eq((char *)param, "socket_thread"));
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_select_c_test, NULL, NULL),
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