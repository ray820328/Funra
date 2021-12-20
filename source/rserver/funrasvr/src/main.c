/** 
 * Copyright (c) 2021 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#pragma warning(disable : 4819)

#include "rcommon.h"

int main(int argc, char **argv) {
  printf("starting rserver...\n");

  int64_t timeNow = microsec_r();

  printf("timeNow: %lld 毫秒, %lld 微秒, %lld 耗时微秒\n", millisec_r(), timeNow, (microsec_r() - timeNow));

  //char* dataStr = char[64];
  //rformat_time_s_full(dataStr, timeNow);
  //printf("dataStr: %s\n\n", dataStr);

  return 1;
}
