/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RTOOLS_H
#define RTOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"

/* ------------------------------- Macros ------------------------------------*/

/* ------------------------------- APIs ------------------------------------*/

int rtools_init();
int rtools_uninit();

/** [start, end] */
int rtools_rand_int(int start, int end);

void rtools_wait_mills(int ms);

/** 1 end index (0x1-0x8000000000000000返回0-63, val==0返回-1) */
int rtools_endindex1(uint64_t val);

/** 1 start index (0x1-0x8000000000000000返回0-63, val==0返回-1) */
int rtools_startindex1(uint64_t val);

/** 1bits */
int rtools_popcount1(uint64_t val);


uint64_t rhash_func_murmur(const char *key);

#ifdef __cplusplus
}
#endif

#endif //RTOOLS_H
