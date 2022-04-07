/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <string.h>

#include "rlog.h"
#include "rbase/common/test/rtest.h"

static int init_platform();
static int run_tests(int output);

static int init_platform() {

    return 0;
}

int main(int argc, char **argv) {
    init_rlog("RLog.txt", RLOG_ALL, false, "all");

    init_platform();

    run_tests(0);
    // switch (argc) {
    // case 1: return run_tests(0);
    // default:
    //   fprintf(stderr, "Too many arguments.\n");
    //   fflush(stderr);
    //   return 1;
    // }

#ifndef __SUNPRO_C
    return 0;
#endif
}

static int run_tests(int output) {
    int testResult;

    testResult = 0;

    testResult = run_rcommon_tests(output);

    run_rdict_tests(output);

    return testResult;
}

