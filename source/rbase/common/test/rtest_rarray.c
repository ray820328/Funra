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
#include "rarray.h"

#include "rbase/common/test/rtest.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif //__GNUC__

static void rarray_int_test(void **state);
static void rarray_string_test(void **state);
static void rarray_ptr_test(void **state);

static char* dir_path = NULL;

static int setup(void **state) {
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 0;
    *state = answer;

    dir_path = "./";

    return rcode_ok;
}
static int teardown(void **state) {
    dir_path = NULL;

    free(*state);
    return rcode_ok;
}
static struct CMUnitTest test_group2[] = {
    cmocka_unit_test_setup_teardown(rarray_int_test, setup, teardown),
    cmocka_unit_test_setup_teardown(rarray_string_test, setup, teardown),
    cmocka_unit_test_setup_teardown(rarray_ptr_test, setup, teardown),
};

int run_rarray_tests(int benchmark_output) {
    int result = 0;

    int64_t timeNow = nanosec_r();

    result += cmocka_run_group_tests(test_group2, NULL, NULL);

    printf("run_rarray_tests, failed: %d, all time: %"PRId64" us\n", result, (nanosec_r() - timeNow));

    return result == 0 ? rcode_ok : -1;
}

static void rarray_int_test(void **state) {
    (void)state;
    rarray_size_t count = 100;
    int j;
    int temp;

    init_benchmark(1024, "test rarray (%d)", count);

    start_benchmark(0);
    rarray_t* array_ins = NULL;
    rarray_init(array_ins, rdata_type_int, count);
    assert_true(array_ins);
    end_benchmark("create rarray.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rarray_add(array_ins, j + count);
        temp = rarray_at(array_ins, j);
        assert_true(temp == j + count);
    }
    temp = 0;
    assert_true(rarray_size(array_ins) == count);
    end_benchmark("Fill array.");

    start_benchmark(0);
    rarray_clear(array_ins);
    end_benchmark("Clear rarray.");

    start_benchmark(0);
    rarray_iterator_t it = rarray_it(array_ins);
    for (; temp = rarray_next(&it), rarray_has_next(&it); ) {
        assert_true(temp == 0);
    }
    end_benchmark("Iterator rarray.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rarray_add(array_ins, count - j);
    }

    j = 0;
    rarray_it_first(&it);
    for (; temp = rarray_next(&it), rarray_has_next(&it); ) {
        rinfo("Iterator rarray, value=%d, index="rarray_size_t_format"\n", temp, (&it)->index);
        assert_true(temp == count - j++);
    }
    temp = 0;
    assert_true(rarray_size(array_ins) == count);
    end_benchmark("Fill and check.");

    rarray_release(array_ins);

}

static void rarray_string_test(void **state) {
    (void)state;
    rarray_size_t count = 3;
    int j;
    char* temp;

    init_benchmark(1024, "test rarray (%d)", count);

    start_benchmark(0);
    rarray_t* array_ins = NULL;
    rarray_init(array_ins, rdata_type_string, count);
    assert_true(array_ins);
    end_benchmark("create rarray.");

    start_benchmark(0);
    char* temp_str = NULL;
    for (j = 0; j < count; j++) {
        rnum2str(temp_str, j + count, 0);
        rarray_add(array_ins, temp_str);
        temp = rarray_at(array_ins, j);
        assert_true(rstr_2int(temp) == j + count);
    }
    temp = NULL;
    assert_true(rarray_size(array_ins) == count);
    end_benchmark("Fill array.");

    start_benchmark(0);
    rarray_clear(array_ins);
    temp = rarray_at(array_ins, 0);
    assert_true(temp == NULL);
    end_benchmark("Clear rarray.");

    for (j = 0; j < count; j++) {
        rnum2str(temp_str, j + count, 0);
        rarray_add(array_ins, temp_str);
        temp = rarray_at(array_ins, j);
        assert_true(rstr_2int(temp) == j + count);
    }
    temp = NULL;

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        temp = rarray_at(array_ins, 0);
        assert_true(rstr_2int(temp) == j + count);
        rarray_remove_at(array_ins, 0);
    }
    assert_true(rarray_size(array_ins) == 0);
    temp = NULL;
    end_benchmark("Remove rarray.");

    start_benchmark(0);
    rarray_iterator_t it = rarray_it(array_ins);
    for (; temp = rarray_next(&it), rarray_has_next(&it); ) {
        assert_true(temp == 0);
    }
    end_benchmark("Iterator rarray.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rnum2str(temp_str, count - j, 0);
        rarray_add(array_ins, temp_str);
        temp = rarray_at(array_ins, j);
        assert_true(rstr_2int(temp) == count - j);
        rnum2str(temp_str, count - j, 0);
        assert_true(rstr_eq(temp, temp_str));
    }

    j = 0;
    rarray_it_first(&it);
    for (; temp = rarray_next(&it), rarray_has_next(&it); ) {
        rinfo("Iterator rarray, value=%d, index="rarray_size_t_format"\n", temp, (&it)->index);
        assert_true(rstr_2int(temp) == count - j);
        rnum2str(temp_str, count - j, 0);
        assert_true(rstr_eq(temp, temp_str));
        j++;
    }
    temp = NULL;
    assert_true(rarray_size(array_ins) == count);
    end_benchmark("Fill and check.");

    rarray_release(array_ins);

}

static struct data_test
{
    int index;
    char* value;
} data_test;
static struct data_test* copy_value_func_obj(const struct data_test* obj) {
    struct data_test* dest = rnew_data(struct data_test);
    dest->index = obj->index;
    dest->value = rstr_cpy(obj->value);
    return dest;
}
static void free_value_func_obj(struct data_test* obj) {
    if (obj == NULL) {
        return;
    }
    rstr_free(obj->value);
    obj->value = NULL;
    rfree_data(struct data_test, obj);
}
static void rarray_ptr_test(void **state) {
    (void)state;
    rarray_size_t count = 100;
    int j;
    struct data_test* temp;

    init_benchmark(1024, "test rarray (%d)", count);

    start_benchmark(0);
    rarray_t* array_ins = NULL;
    rarray_init(array_ins, rdata_type_ptr, count);
    array_ins->copy_value_func = copy_value_func_obj;
    array_ins->free_value_func = free_value_func_obj;
    assert_true(array_ins);
    end_benchmark("create rarray.");

    start_benchmark(0);
    struct data_test temp_obj = { 0, NULL };
    struct data_test* temp_ptr = &temp_obj;
    char* temp_str = NULL;
    for (j = 0; j < count; j++) {
        rnum2str(temp_str, j + count, 0);
        temp_ptr->index = j;
        temp_ptr->value = temp_str;
        rarray_add(array_ins, temp_ptr);
        temp = rarray_at(array_ins, j);
        assert_true(temp->index == j);
        assert_true(rstr_2int(temp->value) == j + count);
    }
    temp = NULL;
    assert_true(rarray_size(array_ins) == count);
    end_benchmark("Fill array.");

    start_benchmark(0);
    rarray_clear(array_ins);
    end_benchmark("Clear rarray.");

    start_benchmark(0);
    rarray_iterator_t it = rarray_it(array_ins);
    for (; temp = rarray_next(&it), rarray_has_next(&it); ) {
        assert_true(temp == NULL);
    }
    end_benchmark("Iterator rarray.");

    start_benchmark(0);
    for (j = 0; j < count; j++) {
        rnum2str(temp_str, count - j, 0);
        temp_ptr->index = j;
        temp_ptr->value = temp_str;
        rarray_add(array_ins, temp_ptr);
        temp = rarray_at(array_ins, j);
        assert_true(temp->index == j);
        assert_true(rstr_2int(temp->value) == count - j);
        assert_true(rstr_eq(temp->value, temp_str));
    }

    j = 0;
    rarray_it_first(&it);
    for (; temp = rarray_next(&it), rarray_has_next(&it); ) {
        rinfo("Iterator rarray, value=%s, index="rarray_size_t_format"\n", temp->value, (&it)->index);
        assert_true(rstr_2int(temp->value) == count - j);
        rnum2str(temp_str, count - j, 0);
        assert_true(rstr_eq(temp->value, temp_str));
        j++;
    }
    temp = NULL;
    assert_true(rarray_size(array_ins) == count);
    end_benchmark("Fill and check.");

    rarray_release(array_ins);

}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__