/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rtools.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__


    int rtools_endindex1(uint64_t val) {//1 end index (0x1-0x8000000000000000返回0-63, val==0返回-1)
        if (val == 0) {
            return -1;
        }
#ifdef WIN32
        int index = 0;
        _BitScanForward64(&index, val);
        return index;
#else
        return __builtin_ctzll(val);//__builtin_ffsll - 前导1 //__builtin_ctz - 后导0个数，__builtin_clz - 前导0个数
#endif
    }
    
    int rtools_startindex1(uint64_t val) {//1 start index (0x1-0x8000000000000000返回0-63, val==0返回-1)
        if (val == 0) {
            return -1;
        }
#ifdef WIN32
        int index = 0;
        _BitScanReverse64(&index, val);
        return index;
#else
        return 63 - __builtin_clzll(val);// __builtin_clzll(val);//前导的0的个数
#endif
    }

    int rtools_popcount1(uint64_t val) {//1bits
#ifdef WIN32
        return -1;
#else
        return __builtin_popcountll(val);
#endif
    }


#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__