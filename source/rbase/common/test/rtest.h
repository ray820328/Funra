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

#define init_benchmark(bufferSize, fmtStr, ...) \
    char* benchmarkTitle = raymalloc(bufferSize); \
    assert_true(sprintf(benchmarkTitle, fmtStr, ##__VA_ARGS__) < bufferSize); \
    int64_t benchmarkStart = 0, benchmarkElapsed = 0

#define start_benchmark(benchmarkMsgTitle) do { \
    benchmarkStart = millisec_r(); \
    if (benchmarkMsgTitle) benchmarkTitle = benchmarkMsgTitle; \
} while(0)

#define end_benchmark(benchmarkMsgSuffix) do { \
    benchmarkElapsed = millisec_r() - benchmarkStart; \
    printf("%s: elapsed %"PRId64" ms, " benchmarkMsgSuffix "\n", benchmarkTitle, benchmarkElapsed); \
} while(0)


int run_rcommon_tests(int benchmark_output);
#endif /* RTEST_H */
