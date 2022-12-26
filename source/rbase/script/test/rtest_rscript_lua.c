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

#include "rscript.h"
#include "rscript_lua.h"

#include "rbase/script/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static rscript_context_t rscript_context;

static void rscript_full_test(void **state) {
	(void)state;
	int count = 5;
	init_benchmark(1024, "test rscript (%d)", count);

    int ret_code = 0;
    rscript_context_t* ctx = &rscript_context;
    rscript_context_lua_t* ctx_lua = (rscript_context_lua_t*)ctx->ctx_script;

    start_benchmark(0);
    
    char* func_name = "LogErr";   
    lua_pushstring(ctx_lua->L, "from c calling...");
    lua_pushstring(ctx_lua->L, "from c 222");

    ret_code = rscript_lua->call_script(ctx, func_name, 2, 1);
    assert_true(ret_code == rcode_ok);
    lua_pop(ctx_lua->L, 1);

    func_name = "Util.LogErr";
    lua_pushstring(ctx_lua->L, "from c calling...");
    lua_pushstring(ctx_lua->L, "from c 222");

    ret_code = rscript_lua->call_script(ctx, func_name, 2, 2);
    assert_true(ret_code == rcode_ok);
    lua_pop(ctx_lua->L, 2);

    func_name = "Util.Log:LogErr";
    lua_pushstring(ctx_lua->L, "from c calling...");
    lua_pushstring(ctx_lua->L, "from c 222");

    ret_code = rscript_lua->call_script(ctx, func_name, 3, 2);//self
    assert_true(ret_code == rcode_ok);
    lua_pop(ctx_lua->L, 2);

    end_benchmark("test running rscript.");

    uninit_benchmark();
}


static int setup(void **state) {
    rscript_context_t* ctx = &rscript_context;
    rdata_init(ctx, sizeof(*ctx));

    assert_true(rscript_lua->init(ctx, NULL) == rcode_ok);

    return rcode_ok;
}
static int teardown(void **state) {
    rscript_context_t* ctx = &rscript_context;

    assert_true(rscript_lua->uninit(ctx, NULL) == rcode_ok);
    
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rscript_full_test, NULL, NULL),
};

int run_rscript_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, setup, teardown);

    printf("run_rscript_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__