/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RFILE_H
#define RFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"

/* ------------------------------- Macros ------------------------------------*/

/* ------------------------------- APIs ------------------------------------*/

/** 1 end index (0x1-0x8000000000000000返回0-63, val==0返回-1) */
int rtools_endindex1(uint64_t val);

/** 1 start index (0x1-0x8000000000000000返回0-63, val==0返回-1) */
int rtools_startindex1(uint64_t val);

/** 1bits */
int rtools_popcount1(uint64_t val);

#ifdef __cplusplus
}
#endif

#endif //RFILE_H
