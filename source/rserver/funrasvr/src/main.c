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

  int* datas = rdate_from_time_millis(millisec_r());
  printf("%.4d-%.2d-%.2d %.2d:%.2d:%.2d %.3d\n\n", datas[0], datas[1], datas[2], datas[3], datas[4], datas[5], datas[6]);

  return 1;
}
