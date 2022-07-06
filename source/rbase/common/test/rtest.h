/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef RTEST_H
#define RTEST_H

#include "rcommon.h"

#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>

#include "3rd/cmocka/include/cmocka.h"
#include "3rd/cmocka/include/cmocka_pbc.h"

#define init_benchmark(buffer_size, fmt_str, ...) \
    char* benchmark_title = rstr_new(buffer_size); \
    assert_true(sprintf(benchmark_title, fmt_str, ##__VA_ARGS__) < buffer_size); \
    int64_t benchmark_start = 0, benchmark_elapsed = 0
#define uninit_benchmark() \
    rstr_free(benchmark_title); \
    benchmark_title = NULL

#define start_benchmark(benchmark_msg_title) do { \
    benchmark_start = rtime_millisec(); \
    if (benchmark_msg_title) benchmark_title = benchmark_msg_title; \
} while(0)

#define end_benchmark(benchmark_msg_suffix) do { \
    benchmark_elapsed = rtime_millisec() - benchmark_start; \
    printf("%s: elapsed %"PRId64" ms, " benchmark_msg_suffix "\n", benchmark_title, benchmark_elapsed); \
} while(0)


typedef int(*rtest_entry_type)(int benchmark_output);

int rtest_add_test_entry(rtest_entry_type entry_func);

int run_rpool_tests(int benchmark_output);
int run_rstring_tests(int benchmark_output);
int run_rthread_tests(int benchmark_output);
int run_rarray_tests(int benchmark_output);
int run_rdict_tests(int benchmark_output);
int run_rcommon_tests(int benchmark_output);
int run_rlog_tests(int benchmark_output);
int run_rfile_tests(int benchmark_output);
int run_rtools_tests(int benchmark_output);

#endif /* RTEST_H */
