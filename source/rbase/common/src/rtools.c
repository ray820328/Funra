/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rtools.h"
#include "rtime.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__


int rtools_init() {

    srand((unsigned int)(rtime_millisec()));

    return rcode_ok;
}

int rtools_rand_int(int start, int end) {
    // srand(rtime_millisec())
    return rand() % (end - start + 1) + start;
}

void rtools_wait_mills(int ms){
#if defined(_WIN32) || defined(_WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000); //延迟挂起进程。进程被挂起放到reday queue, 库函数中实现的，调用alarm()
    //sleep(seconds);//秒

    //static struct timespec req;
    //static struct timespec rem;
    //int res = -1;

    //req.tv_sec = ms / 1000;
    //req.tv_nsec = ms % 1000 * 1000000;

    //while (res < 0) {
    //    res = clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem); //多个系统时钟，需要使用相对于特定时钟的延迟
    //    if (res < 0) {
    //        if (errno == EINTR) {
    //            req = rem;
    //        }
    //        else {
    //            break;
    //        }
    //    }
    //}
#endif /* defined(_WIN32) || defined(_WIN64) */
}


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


uint64_t rhash_func_murmur(const char *key)
{
    const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
    const size_t len = strlen(key);
    const unsigned char* p = (const unsigned char*)key;
    const unsigned char *end = p + (len & ~(uint64_t)0x7);
    uint64_t h = (len * m);

    while (p != end) {
        uint64_t k;
        memcpy(&k, p, sizeof(k));

        k *= m;
        k ^= k >> 47u;
        k *= m;

        h ^= k;
        h *= m;
        p += 8;
    }

    switch (len & 7u) {
    case 7: h ^= (uint64_t)p[6] << 48ul; // fall through
    case 6: h ^= (uint64_t)p[5] << 40ul; // fall through
    case 5: h ^= (uint64_t)p[4] << 32ul; // fall through
    case 4: h ^= (uint64_t)p[3] << 24ul; // fall through
    case 3: h ^= (uint64_t)p[2] << 16ul; // fall through
    case 2: h ^= (uint64_t)p[1] << 8ul;  // fall through
    case 1: h ^= (uint64_t)p[0];         // fall through
        h *= m;
    default:
        break;
    };

    h ^= h >> 47u;
    h *= m;
    h ^= h >> 47u;

    return h; // (uint32_t)h;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__