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
#include "rsocket_s.h"
#include "rsocket_c.h"
#include "rcodec_default.h"
#include "repoll.h"

#include "rbase/ipc/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rsocket_server_ctx_t rsocket_ctx;
static rthread_t epoll_server_thread;

static rsocket_ctx_t rsocket_client_ctx;//非线程安全
static rthread_t epoll_client_thread;
static volatile int sent_times = 3;


static void* run_server(void* arg) {
    int ret_code = 0;

    repoll_container_t epoll_obj;
    rsocket_ctx.user_data = &epoll_obj;
    ret_code = repoll_create(&epoll_obj, 16);
    assert_true(ret_code == rcode_ok);

    rsocket_ctx.id = 2008;
    rsocket_ctx.stream_type = ripc_type_tcp;

    rsocket_ctx.ipc_entry = (ripc_entry_t*)rsocket_s;

    ripc_data_source_t* ds = rdata_new(ripc_data_source_t);
    ds->ds_type = ripc_data_source_type_server;
    ds->ds_id = rsocket_ctx.id;
    ds->ctx = &rsocket_ctx;

    rsocket_ctx.ds = ds;

    rsocket_cfg_t* cfg = (rsocket_cfg_t*)rdata_new(rsocket_cfg_t);
    rsocket_ctx.cfg = cfg;
    cfg->id = 1;
    cfg->sid_min = 100000;
    cfg->sid_max = 2000000;
    rstr_set(cfg->ip, "0.0.0.0", 0);
    cfg->port = 23000;

    rsocket_ctx.sid_cur = cfg->sid_min;

    rdata_handler_t* handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    rsocket_ctx.in_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_decode_default.on_before;
    handler->process = rcodec_decode_default.process;
    handler->on_code = rcodec_decode_default.on_code;
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
    handler->on_code = rcodec_encode_default.on_code;
    handler->on_after = rcodec_encode_default.on_after;
    handler->on_next = rcodec_encode_default.on_next;
    handler->on_notify = rcodec_encode_default.on_notify;
    handler->notify = rcodec_encode_default.notify;

    ret_code = rsocket_ctx.ipc_entry->init(&rsocket_ctx, rsocket_ctx.cfg);
    assert_true(ret_code == rcode_ok);
    ret_code = rsocket_ctx.ipc_entry->open(&rsocket_ctx);
    assert_true(ret_code == rcode_ok);
    ret_code = rsocket_ctx.ipc_entry->start(&rsocket_ctx);//loop until to call stop
    assert_true(ret_code == rcode_ok);

    while (true) {
        ret_code = rsocket_ctx.ipc_entry->check(ds, NULL);//send & recv
        if (ret_code != rcode_ok) {
            rwarn("epoll wait stopped.");
            break;
        }

        rtools_wait_mills(100);
    }


    rsocket_ctx.ipc_entry->close(&rsocket_ctx);
    rsocket_ctx.ipc_entry->uninit(&rsocket_ctx);

    ret_code = repoll_destroy(&epoll_obj);
    assert_true(ret_code == rcode_ok);

    rdata_free(rdata_handler_t, rsocket_ctx.in_handler);
    rdata_free(rdata_handler_t, rsocket_ctx.out_handler);
    rdata_free(ripc_data_source_t, ds);
    rdata_free(rsocket_cfg_t, cfg);

    rtools_wait_mills(3000);
    rinfo("end, run_epoll_server: %s", (char *)arg);

    return arg;
}


static void* run_client(void* arg) {
    int ret_code = 0;

    repoll_container_t epoll_obj;
    rsocket_client_ctx.user_data = &epoll_obj;
    ret_code = repoll_create(&epoll_obj, 10);
    assert_true(ret_code == rcode_ok);

    rsocket_client_ctx.id = 3008;
    rsocket_client_ctx.stream_type = ripc_type_tcp;

    rsocket_client_ctx.ipc_entry = (ripc_entry_t*)rsocket_epoll_c;

    ripc_data_source_t* ds = rdata_new(ripc_data_source_t);
    ds->ds_type = ripc_data_source_type_client;
    ds->ds_id = rsocket_client_ctx.id;
    ds->ctx = &rsocket_client_ctx;

    rsocket_client_ctx.ds = ds;

    rsocket_cfg_t* cfg = (rsocket_cfg_t*)rdata_new(rsocket_cfg_t);
    rsocket_client_ctx.cfg = cfg;
    cfg->id = 1;
    rstr_set(cfg->ip, "127.0.0.1", 0);
    cfg->port = 23000;

    rdata_handler_t* handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    rsocket_client_ctx.in_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_decode_default.on_before;
    handler->process = rcodec_decode_default.process;
    handler->on_code = rcodec_decode_default.on_code;
    handler->on_after = rcodec_decode_default.on_after;
    handler->on_next = rcodec_decode_default.on_next;
    handler->on_notify = rcodec_decode_default.on_notify;
    handler->notify = rcodec_decode_default.notify;

    handler = (rdata_handler_t*)rdata_new(rdata_handler_t);
    rsocket_client_ctx.out_handler = handler;
    handler->prev = NULL;
    handler->next = NULL;
    handler->on_before = rcodec_encode_default.on_before;
    handler->process = rcodec_encode_default.process;
    handler->on_code = rcodec_encode_default.on_code;
    handler->on_after = rcodec_encode_default.on_after;
    handler->on_next = rcodec_encode_default.on_next;
    handler->on_notify = rcodec_encode_default.on_notify;
    handler->notify = rcodec_encode_default.notify;

    rtools_wait_mills(5000);

    ret_code = rsocket_client_ctx.ipc_entry->init(&rsocket_client_ctx, rsocket_client_ctx.cfg);
    assert_true(ret_code == rcode_ok);
    ret_code = rsocket_client_ctx.ipc_entry->open(&rsocket_client_ctx);
    assert_true(ret_code == rcode_ok);

    ret_code = rsocket_client_ctx.ipc_entry->start(&rsocket_client_ctx);
    assert_true(ret_code == rcode_ok);

    while (sent_times-- > 0) {
        ripc_data_default_t data;
        data.cmd = 11;
        data.data = rstr_cpy("client epoll_send test", 0);
        data.len = rstr_len(data.data);
        rsocket_client_ctx.ipc_entry->send(ds, &data);

        rtools_wait_mills(1000);
        rsocket_client_ctx.ipc_entry->check(ds, NULL);//send & recv

        rdata_free(char*, data.data);

        rtools_wait_mills(1000);
    }

    rsocket_client_ctx.ipc_entry->stop(&rsocket_client_ctx);

    rsocket_client_ctx.ipc_entry->close(&rsocket_client_ctx);
    rsocket_client_ctx.ipc_entry->uninit(&rsocket_client_ctx);

    ret_code = repoll_destroy(&epoll_obj);
    assert_true(ret_code == rcode_ok);

    rdata_free(rdata_handler_t, rsocket_client_ctx.in_handler);
    rdata_free(rdata_handler_t, rsocket_client_ctx.out_handler);
    rdata_free(ripc_data_source_t, ds);
    rdata_free(rsocket_cfg_t, cfg);

    rinfo("end, run_client success: %s", (char *)arg);

    return arg;
}

static void rsocket_epoll_server_test(void **state) {
    (void)state;
    int count = 1;
    init_benchmark(1024, "test rsocket_epoll_server_test (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&epoll_server_thread, run_server, "epoll_server_thread"); // 0;// 
    assert_true(ret_code == 0);
    end_benchmark("listen.");

    rinfo("server listen started.");

    uninit_benchmark();
}

static void rsocket_epoll_client_test(void **state) {
    (void)state;
    int count = 1;
    init_benchmark(1024, "test rsocket_epoll_client_test (%d)", count);

    int ret_code = 0;

    start_benchmark(0);
    ret_code = rthread_start(&epoll_client_thread, run_client, "epoll_client_thread"); // 0;// 
    //run_client("epoll_client_thread");
    assert_true(ret_code == 0);
    end_benchmark("open connection.");

    rtools_wait_mills(1000);

    rinfo("client epoll started.");

    uninit_benchmark();
}


static int setup(void **state) {
    rthread_init(&epoll_server_thread);
    rthread_init(&epoll_client_thread);

    return rcode_ok;
}
static int teardown(void **state) {
    int ret_code = 0;
    void* param;
    rinfo("detach epoll_client_thread.");
    // ret_code = rthread_join(&epoll_client_thread, &param);
    ret_code = rthread_detach(&epoll_client_thread, &param);
    assert_true(ret_code == 0);

    rinfo("join epoll_server_thread.");
    ret_code = rthread_join(&epoll_server_thread, &param);
    assert_true(ret_code == 0);
    assert_true(rstr_eq((char *)param, "epoll_server_thread"));
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rsocket_epoll_client_test, NULL, NULL),
    cmocka_unit_test_setup_teardown(rsocket_epoll_server_test, NULL, NULL),
};

int run_rsocket_epoll_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);
    //setup(0);
    //rsocket_c_full_test(0);
    printf("run_rsocket_epoll_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__