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

//int vasprintf(char **strp, const char *fmt, va_list ap) {
//    // _vscprintf tells you how big the buffer needs to be
//    int len = _vscprintf(fmt, ap);
//    if (len == -1) {
//        return -1;
//    }
//    size_t size = (size_t)len + 1;
//    char *str = malloc(size);
//    if (!str) {
//        return -1;
//    }
//    // _vsprintf_s is the "secure" version of vsprintf
//    int r = -1; // _vsprintf_s(str, len + 1, fmt, ap);
//    if (r == -1) {
//        free(str);
//        return -1;
//    }
//    *strp = str;
//    return r;
//}
//int asprintf(char **strp, const char *fmt, ...) {
//    va_list ap;
//    va_start(ap, fmt);
//    int r = vasprintf(strp, fmt, ap);
//    va_end(ap);
//    return r;
//}

void testRList(int count) {
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



static uint64_t hashCallbackTest(const void *key) {
    return dictGenHashFunction((unsigned char*)key, (int)strlen((char*)key));
}
static int compareCallbackTest(void *privdata, const void *key1, const void *key2) {
    int l1, l2;
    DICT_NOTUSED(privdata);

    l1 = (int)strlen((char*)key1);
    l2 = (int)strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}
static void freeCallbackTest(void *privdata, void *val) {
    DICT_NOTUSED(privdata);

    zfree(val);
}

static char *stringFromLongLong(long long value) {
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
void testRDict(int count) {
    count = count > 0 ? count : 10000;

    dictType BenchmarkDictType = {
        hashCallbackTest,
        NULL,
        NULL,
        compareCallbackTest,
        freeCallbackTest,
        NULL,
        NULL
    };

    long j;
    long long start = 0, elapsed = 0;
    dict *dict = dictCreate(&BenchmarkDictType, NULL);

    for (j = 0; j < count; j++) {
        int retval = dictAdd(dict, stringFromLongLong(j), (void*)j);
        assert(retval == DICT_OK);
    }
    assert((long)dictSize(dict) == count);

    /* Wait for rehashing. */
    while (dictIsRehashing(dict)) {
        dictRehashMilliseconds(dict, 100);
    }

    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictFind(dict, key);
        assert(de != NULL);
        zfree(key);
    }

    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictFind(dict, key);
        assert(de != NULL);
        zfree(key);
    }
    end_benchmark("Linear access of existing elements (2nd round)");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(rand() % count);
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
        char *key = stringFromLongLong(rand() % count);
        key[0] = 'X';
        dictEntry *de = dictFind(dict, key);
        assert(de == NULL);
        zfree(key);
    }
    end_benchmark("Accessing missing");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        int retval = dictDelete(dict, key);
        assert(retval == DICT_OK);
        key[0] += 17; /* Change first number to letter. */
        retval = dictAdd(dict, key, (void*)j);
        assert(retval == DICT_OK);
    }
    end_benchmark("Removing and adding");
    dictRelease(dict);
}

int main(int argc, char **argv) {
  printf("run testing...\n");

  int64_t timeNow = microsec_r();

  //testHashMap(10);
  //testRList(10);
  testRDict(10);

  printf("timeNow: %lld 毫秒, %lld 微秒, %lld 耗时微秒\n", millisec_r(), timeNow, (microsec_r() - timeNow));

  int* datas = rdate_from_time_millis(millisec_r());
  printf("%.4d-%.2d-%.2d %.2d:%.2d:%.2d %.3d\n\n", datas[0], datas[1], datas[2], datas[3], datas[4], datas[5], datas[6]);

  return 1;
}
