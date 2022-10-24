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
#include "rsocket_uv_c.h"
#include "rcodec_default.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rsocket_ctx_uv_t rsocket_ctx;//非线程安全
static rthread_t socket_thread;//uv非线程安全，只能在loop线程启动和收发，否则用uv_async_send
static volatile int repeat_cb_count = 3;

static void repeat_cb(uv_timer_t* handle) {
    assert_true(handle != NULL);
    assert_true(1 == uv_is_active((uv_handle_t*)handle));

    rsocket_ctx_uv_t* ctx = &rsocket_ctx;

    ripc_data_source_t* ds = (ripc_data_source_t*)ctx->ds;
    rinfo("code = %d", ds->state);
    if (ds->state == ripc_state_start) {
        rtools_wait_mills(3000);
        ctx->ipc_entry->open(ctx);
        return;
    }
    if (ds->state != ripc_state_ready) {
        rerror("invalid state = %d", ds->state);
        return;
    }

   ripc_data_default_t data;
   data.cmd = 11;
   data.data = rstr_cpy("client uv send test", 0);
   data.len = rstr_len(data.data);
   ctx->ipc_entry->send(ds, &data);//发送数据
   rdata_free(char*, data.data);

   if (--repeat_cb_count <= 0) {
        rtools_wait_mills(1000);

        data.cmd = 999;
        data.data = rstr_cpy("client uv_close test", 0);
        data.len = rstr_len(data.data);
        ctx->ipc_entry->send(ds, &data);//发送数据
        rdata_free(char*, data.data);

        //uv_close((uv_handle_t*)handle, NULL);
   }
}

static void* run_client(void* arg) {
    int ret_code = rcode_ok;

    rsocket_ctx_uv_t* ctx = &rsocket_ctx;

    ctx->id = 3001;
    ctx->stream_type = ripc_type_tcp;
    uv_loop_t loop;
    assert_true(0 == uv_loop_init(&loop));
#ifdef _WIN32
    assert_true(UV_ENOSYS == uv_loop_configure(&loop, UV_LOOP_BLOCK_SIGNAL, 0));
#else
    assert_true(0 == uv_loop_configure(&loop, UV_LOOP_BLOCK_SIGNAL, SIGPROF));
#endif
    ctx->loop = &loop;// uv_default_loop();

    ctx->ipc_entry = (ripc_entry_t*)&rsocket_uv_c;

    ripc_data_source_t* ds = rdata_new(ripc_data_source_t);
    ds->ds_type = ripc_data_source_type_client;
    ds->ds_id = ctx->id;
    ds->ctx = ctx;

    ctx->ds = ds;

    uv_handle_t* uv_handle = (uv_handle_t*)rdata_new(uv_tcp_t);
    uv_handle->data = ds;

    ds->stream = uv_handle;

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

    ret_code = ctx->ipc_entry->init(ctx, ctx->cfg);
    assert_true(ret_code == rcode_ok);

    uv_timer_t timer_repeat;
    uv_timer_init(ctx->loop, &timer_repeat);
    uv_timer_start(&timer_repeat, repeat_cb, 2000, 2000);

    ret_code = ctx->ipc_entry->start(ctx);
    assert_true(ret_code == rcode_ok);

    ret_code = ctx->ipc_entry->close(ctx);
    assert_true(ret_code == rcode_ok);

    ret_code = ctx->ipc_entry->stop(ctx);
    assert_true(ret_code == rcode_ok);
    ret_code = ctx->ipc_entry->uninit(ctx);
    assert_true(ret_code == rcode_ok);

    rdata_free(rdata_handler_t, ctx->in_handler);
    rdata_free(rdata_handler_t, ctx->out_handler);
    rdata_free(uv_tcp_t, ds->stream);
    rdata_free(ripc_data_source_t, ds);
    rdata_free(rsocket_cfg_t, cfg);

    rinfo("end, run_uv_client success: %s", (char *)arg);

    return arg;
}

static void rsocket_uv_c_full_test(void **state) {
	(void)state;
	int count = 1;
	init_benchmark(1024, "test rsocket_uv_c (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&socket_thread, run_client, "socket_uv_thread"); // 0;// 
    //run_client("socket_uv_thread");
    assert_true(ret_code == 0);
	end_benchmark("open uv connection.");

    rtools_wait_mills(1000);

    rinfo("client uv started.");

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
    // assert_true(rstr_eq((char *)param, "socket_uv_thread"));
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_uv_c_full_test, NULL, NULL),
};

int run_rsocket_uv_c_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);
	//setup(0);
	//rsocket_uv_c_full_test(0);
    printf("run_rsocket_uv_c_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__