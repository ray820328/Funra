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

#include "recs.h"

#include "rbase/ecs/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rentity_full_test(void **state);

static recs_context_t recs_context;

static int setup(void **state) {
    recs_context_t* ctx = &recs_context;

    ctx->sid_min = 10000;
    ctx->sid_max = 11000;
    // ctx->on_init = ;
    // ctx->on_uninit = ;
    ctx->create_cmp = rtest_recs_cmp_new;

    recs_init(ctx, NULL);

    return rcode_ok;
}
static int teardown(void **state) {
    recs_context_t* ctx = &recs_context;

    recs_uninit(ctx, NULL);

    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rentity_full_test, setup, teardown),
};

int run_rentity_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = rtime_nanosec();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rentity_tests, failed: %d, all time: %"PRId64" us\n", result, (rtime_nanosec() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

static void rentity_full_test(void **state) {
    (void)state;

    recs_context_t* ctx = &recs_context;
    int count = 3;
    int j;
    uint64_t temp_id = 0;

    init_benchmark(1024, "test entity (%d)", count);

    start_benchmark(0);
    recs_entity_t* entity = recs_entity_new(ctx, recs_etype_shared);
    assert_true(entity != NULL);
    temp_id = entity->id;
    recs_get_entity(ctx, temp_id, &entity);
    assert_true(entity != NULL);
    recs_entity_delete(ctx, entity, true);
    entity = NULL;
    recs_get_entity(ctx, temp_id, &entity);
    assert_true(entity == NULL);
    end_benchmark("create/destroy entity.");

    start_benchmark(0);
    rtest_cmp_t* cmp_item = NULL;

    char* temp_str = NULL;
    for (j = 0; j < count; j++) {
        cmp_item = (rtest_cmp_t*)recs_cmp_new(ctx, recs_ctype_rtest01);
        assert_true(cmp_item != NULL);
        rnum2str(temp_str, j + count, 0);
        cmp_item->index = j;
        cmp_item->value = temp_str;
        recs_get_cmp(ctx, cmp_item->id, &cmp_item);
        assert_true(cmp_item != NULL);
    }
    end_benchmark("Fill components.");


    uninit_benchmark();
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__