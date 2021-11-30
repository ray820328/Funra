/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "assert.h"

int init_platform();
int run_tests(int benchmark_output);
int run_test(const char* test, int benchmark_output, int test_count);

static int init_platform() {
    
	return 0;
}

int main(int argc, char **argv) {
  init_platform();

  switch (argc) {
  case 1: return run_tests(0);
  default:
    fprintf(stderr, "Too many arguments.\n");
    fflush(stderr);
    return 1;
  }

#ifndef __SUNPRO_C
  return 0;
#endif
}

static int run_tests(int benchmark_output) {
  int actual;
  int total;
  int passed;
  int failed;
  int skipped;
  int current;
  // int test_result;
  // int skip;
  // task_entry_t* task;

  /* Count the number of tests. */
  actual = 0;
  total = 0;
  // for (task = TASKS; task->main; task++, actual++) {
  //   if (!task->is_helper) {
  //     total++;
  //   }
  // }

  /* Keep platform_output first. */
  // skip = (actual > 0 && 0 == strcmp(TASKS[0].task_name, "platform_output"));
  // qsort(TASKS + skip, actual - skip, sizeof(TASKS[0]), compare_task);

  fprintf(stdout, "1..%d\n", total);
  fflush(stdout);

  /* Run all tests. */
  passed = 0;
  failed = 0;
  skipped = 0;
  current = 1;
  // for (task = TASKS; task->main; task++) {
  //   if (task->is_helper) {
  //     continue;
  //   }

  //   test_result = run_test(task->task_name, benchmark_output, current);
  //   switch (test_result) {
  //   case TEST_OK: passed++; break;
  //   case TEST_SKIP: skipped++; break;
  //   default: failed++;
  //   }
  //   current++;
  // }

  return failed;
}


static int run_test(const char* test,
             int benchmark_output,
             int test_count) {
  int status = 0;

  return status;
}
