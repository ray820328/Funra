/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RCOMMON_H
#define RCOMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#define   macro_print_macro_helper(x)   #x  
#define   macro_print_macro(x)   #x"="macro_print_macro_helper(x)

//#define rassert(expr, rv) \
//	if(!(expr)) { \
//		rerror(#expr" is null or 0"); \
//		return rv; \
//	}

#define UNUSED __attribute__(unused)

#if defined(_WIN32) || defined(_WIN64)
#pragma message("Platform info: "macro_print_macro(_WIN64))

#pragma setlocale("chs")

#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mstcpip.h>
#include <iphlpapi.h>
#include <mswsock.h>

#define get_filename(x) strrchr(x, '\\') ? strrchr(x, '\\') + 1 : x
#define likely(x) x
#define unlikely(x) x
#define __attribute__(unused) 

#define access(param1, param2) _access(param1, param2)

#if defined(RAY_BUILD_AS_DLL)

#if defined(RAY_CORE) || defined(RAY_LIB)
#define R_API __declspec(dllexport)
#else
#define R_API __declspec(dllimport)
#endif

#else //RAY_BUILD_AS_DLL

#define R_API		extern

#endif //RAY_BUILD_AS_DLL

#else //WIN32

#include "execinfo.h"
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <libgen.h>

#define get_filename(x) basename(x)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define R_API		extern

#endif //WIN32

#define raymalloc malloc
#define rayfree free

#define RAY_USE_POOL

#ifdef RAY_USE_POOL
#define rnew_data(T) rcheck_value(true, rpool_get(T, rget_pool(T)))
#define rfree_data(T, data) \
            do { \
                rpool_free(T, data, rget_pool(T)); \
                data = NULL; \
            } while(0)

#define rnew_data_s(T, data)  data = rnew_data(T)

#else //RAY_USE_POOL
#define rnew_data(T) rcheck_value(true, raymalloc(sizeof(T)))
#define rfree_data(T, data) \
            do { \
			    rayfree(data); \
			    data = NULL; \
            } while(0)
#endif //RAY_USE_POOL

#define rmin(a,b) ((a)<(b)?(a):(b))
#define rmax(a, b) ((a)>(b)?(a):(b))

#define rarray_count(ptr, count) \
          do { \
            if (ptr) \
                count = sizeof(ptr) / sizeof((ptr)[0]) \
            else \
                count = 0; \
          } while(0)


#define rnum2str(retNumStr, num, base) \
          do { \
            char retTempStr[32]; \
            int lenNumStr = sprintf((retTempStr), "%"PRId64, (int64_t)(num)); \
            retTempStr[lenNumStr] = '\0'; \
            (retNumStr) = retTempStr; \
          } while(0)
#define rformat_s(str_buffer, fmt, ...) \
          do { \
            if (sprintf(str_buffer, fmt, ##__VA_ARGS__) >= (int)sizeof(str_buffer)) { \
                rayprintf(RLOG_ERROR, "error, data exceed of max len(%d).\n", (int)sizeof(str_buffer)); \
                str_buffer[(int)sizeof(str_buffer) - 1] = '\0'; \
            } \
          } while(0)
#define rformat_time_s_full(time_str, time_val) \
          do { \
            int* time_now_datas = rdate_from_time_millis(time_val ? time_val : millisec_r()); \
            rformat_s(time_str, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d %.3d", \
                time_now_datas[0], time_now_datas[1], time_now_datas[2], time_now_datas[3], time_now_datas[4], time_now_datas[5], time_now_datas[6]); \
          } while(0)
#define rformat_time_s_yyyymmddhhMMss(time_str, time_val) \
          do { \
            int* time_now_datas = rdate_from_time_millis(time_val ? time_val : millisec_r()); \
            rformat_s(time_str, "%.4d%.2d%.2d%.2d%.2d%.2d", \
                time_now_datas[0], time_now_datas[1], time_now_datas[2], time_now_datas[3], time_now_datas[4], time_now_datas[5]); \
          } while(0)

#define rassert(expr)                                     \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)


static inline size_t rstr_cat(char* dest, const char* src, const size_t sizeofDest) {
    size_t position = strlen(dest);
    size_t srcLen = strlen(src);
    size_t copyLen = rmin(srcLen, sizeofDest - strlen(dest) - 1);
    memcpy(dest + position, src, copyLen);
    position += copyLen;

    dest[position] = '\0';
    return copyLen - srcLen;
}

static inline char* rstr_cpy(const void *key) {
    if (!key) {
        return NULL;
    }
    unsigned int keyLen = (unsigned int)strlen((char*)key);
    char* keyCopy = raymalloc((int64_t)(keyLen + 1u));
    if (!keyCopy) {
        return NULL;
    }
    memcpy(keyCopy, key, keyLen);
    keyCopy[keyLen] = '\0';
    return keyCopy;
}
static inline void rstr_free(const void *key) {
    rayfree((void*)key);
}

#if defined(_WIN32) || defined(_WIN64)
#define rmutex_t_def CRITICAL_SECTION;
#else
#define rmutex_t_def pthread_mutex_t;
#endif /* defined(_WIN32) || defined(_WIN64) */

static inline void rmutex_init(void* ray_mutex)
{
#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(ray_mutex);
#else
    pthread_mutex_init(ray_mutex, NULL);
#endif /* defined(_WIN32) || defined(_WIN64) */
}

static inline void rmutex_lock(void* ray_mutex)
{
#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(ray_mutex);
#else
    pthread_mutex_lock(ray_mutex);
#endif /* defined(_WIN32) || defined(_WIN64) */
}

static inline void rmutex_unlock(void* ray_mutex)
{
#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(ray_mutex);
#else
    pthread_mutex_unlock(ray_mutex);
#endif /* defined(_WIN32) || defined(_WIN64) */
}

static inline long get_cur_thread_id(void)
{
#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentThreadId();
#elif __linux__
    return (long)syscall(SYS_gettid);
#elif defined(__APPLE__) && defined(__MACH__)
    return (long)syscall(SYS_thread_selfid);
#else
    return (long)pthread_self();
#endif /* defined(_WIN32) || defined(_WIN64) */
}

static inline void wait_seconds(int seconds)
{   // Pretty crossplatform, both ALL POSIX compliant systems AND Windows
#if defined(_WIN32) || defined(_WIN64)
    Sleep(1000 * seconds);
#else
    sleep(seconds);
#endif /* defined(_WIN32) || defined(_WIN64) */
}

#ifdef __cplusplus
}
#endif

#endif //RCOMMON_H
