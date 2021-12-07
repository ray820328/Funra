/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#pragma warning(disable : 4819)

#include "rcommon.h"
#include "rtime.h"
#include "rlist.h"
#include "dict.h"

#include "rbase/common/test/rtest.h"

#include "3rd/cmocka/include/cmocka.h"
#include "3rd/cmocka/include/cmocka_pbc.h"

int init();
int uninit();
// int run_tests(int benchmark_output);
int run_test(const char* test, int benchmark_output, int test_count);

void rdict_test(void **state);
void rlist_test(void **state);

static int init() {
    
	return 0;
}

static int uninit() {
    
  return 0;
}

int run_rcommon_tests(int benchmark_output) {
  int actual;
  int total;
  int passed;
  int failed;
  int skipped;
  int current;
  int testResult;
  // int skip;
  // task_entry_t* task;
  
  testResult = init();
  if(testResult != 0){
    return -1;
  }

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
  // 
  
  const struct CMUnitTest tests[] = {
        cmocka_unit_test(rdict_test),
        cmocka_unit_test(rlist_test),
    };
  testResult = cmocka_run_group_tests(tests, NULL, NULL);
  
  if(testResult != 0){
    return testResult;
  }

  testResult = uninit();
  if(testResult != 0){
    return -2;
  }

  return failed;
}

static int run_test(const char* test,
             int benchmark_output,
             int test_count) {
  int status = 0;

  return status;
}


static void rlist_test(void **state) {
    (void) state;
    int count = 10;
    count = count > 0 ? count : 1;
    rlist_t *mylist = rlist_new(malloc);
    //rlist_init(mylist);
    mylist->malloc_node = malloc;
    mylist->malloc_it = malloc;
    mylist->free_node_val = free;
    mylist->free_node = free;
    mylist->free_self = free;
    mylist->free_it = free;

    for (int i = 0; i < count; i++) {
        char* value = malloc(50);
        if (value) {
            sprintf(value, "listNode - %d", i);

            rlist_rpush(mylist, value);
            //list->tail->val;

            //rlist_lpush(mylist, node);
            //list->head->val;
        }
    }

    rlist_node_t *nodeFind = rlist_find(mylist, "listNode - 3");
    if (nodeFind) {
        printf("rlist find: %s\n", (char*)nodeFind->val);
        rlist_remove(mylist, nodeFind);
    }
    rlist_at(mylist, 0);  // first
    rlist_at(mylist, 1);  // second
    rlist_at(mylist, -1); // last
    rlist_at(mylist, -3); // third last

    rlist_iterator_t *it = rlist_iterator_new(mylist, RLIST_HEAD);
    while ((nodeFind = rlist_iterator_next(it))) {
        printf("rlist iter: %s\n", (char*)nodeFind->val);
    }
    rlist_iterator_destroy(mylist, it);

    rlist_destroy(mylist);
}


static uint64_t dic_hash_callback(const void *key) {
    return dictGenHashFunction((unsigned char*)key, (int)strlen((char*)key));
}
static int dic_compare_callback(void *privdata, const void *key1, const void *key2) {
    int l1, l2;
    DICT_NOTUSED(privdata);

    l1 = (int)strlen((char*)key1);
    l2 = (int)strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}
static void dic_free_callback(void *privdata, void *val) {
    DICT_NOTUSED(privdata);

    zfree(val);
}

static char *dic_longlong_2string(long long value) {
    char buf[32];
    int len;
    char *s;

    len = sprintf(buf, "%lld", value);
    s = zmalloc(len + 1);
    memcpy(s, buf, len);
    s[len] = '\0';
    return s;
}


#define start_benchmark() start = millisec_r()
#define end_benchmark(msg) do { \
    elapsed = millisec_r()-start; \
    printf(msg ": %ld items in %lld ms\n", count, elapsed); \
} while(0)

static void rdict_test(void **state) {
    (void) state;
    int count = 10000;
    count = count > 0 ? count : 10000;

    dictType BenchmarkDictType = {
        dic_hash_callback,
        NULL,
        NULL,
        dic_compare_callback,
        dic_free_callback,
        NULL,
        NULL
    };

    long j;
    long long start = 0, elapsed = 0;
    dict *dict = dictCreate(&BenchmarkDictType, NULL);

    for (j = 0; j < count; j++) {
        int retval = dictAdd(dict, dic_longlong_2string(j), (void*)j);
        assert(retval == DICT_OK);
    }
    assert((long)dictSize(dict) == count);

    /* Wait for rehashing. */
    while (dictIsRehashing(dict)) {
        dictRehashMilliseconds(dict, 100);
    }

    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(j);
        dictEntry *de = dictFind(dict, key);
        assert(de != NULL);
        zfree(key);
    }

    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(j);
        dictEntry *de = dictFind(dict, key);
        assert(de != NULL);
        zfree(key);
    }
    end_benchmark("Linear access of existing elements (2nd round)");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(rand() % count);
        dictEntry *de = dictFind(dict, key);
        assert(de != NULL);
        zfree(key);
    }
    end_benchmark("Random access of existing elements");

    start_benchmark();
    for (j = 0; j < count; j++) {
        dictEntry *de = dictGetRandomKey(dict);
        assert(de != NULL);
    }
    end_benchmark("Accessing random keys");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(rand() % count);
        key[0] = 'X';
        dictEntry *de = dictFind(dict, key);
        assert(de == NULL);
        zfree(key);
    }
    end_benchmark("Accessing missing");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = dic_longlong_2string(j);
        int retval = dictDelete(dict, key);
        assert(retval == DICT_OK);
        key[0] += 17; /* Change first number to letter. */
        retval = dictAdd(dict, key, (void*)j);
        assert(retval == DICT_OK);
    }
    end_benchmark("Removing and adding");
    dictRelease(dict);
}
