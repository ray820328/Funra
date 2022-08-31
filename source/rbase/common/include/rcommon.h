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
#include <string.h>
#include <assert.h>

#include "rbase.h"
#include "rerror.h"
#include "rmemory.h"

#define rcode_ok 0

#define rcode_lt -1
#define rcode_eq 0
#define rcode_gt 1
#define rcode_neq 2
#define rcode_gte 3
#define rcode_lte 4

#define sizeptr 8
#define LPRId64 L"lld"

#define Li "\n"

#define rcheck_value(b, value) (value)

#define macro_print_macro_helper(x)   #x  
#define macro_print_macro(x)          #x"="macro_print_macro_helper(x)

#define rvoid(x) (void)(x)

    //#define rassert(expr, rv) 
    //	if(!(expr)) { 
    //		rerror(#expr" is null or 0"); 
    //		return rv; 
    //	}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4319 4819)
#endif //_MSC_VER

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif //__GNUC__

#define file_system_unicode         1

#if defined(_WIN32) || defined(_WIN64)
#pragma message("Platform info: "macro_print_macro(_WIN64))

#if defined(_WIN32_WCE) || defined(WINNT)

#define file_system_ansi            0

#else

#define file_system_ansi            1

#endif

#define rattribute_unused(declaration) declaration

#pragma setlocale("chs")

#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mstcpip.h>
#include <iphlpapi.h>
#include <mswsock.h>
#include <climits>

#define get_filename(x) strrchr((x), '\\') ? strrchr((x), '\\') + 1 : (x)
#define likely(x) (x)
#define unlikely(x) (x)

#define access(param1, param2) _access(param1, param2)

#if defined(RAY_BUILD_AS_DLL)

#if defined(RAY_CORE) || defined(RAY_LIB)
#define R_API __declspec(dllexport)
#else
#define R_API __declspec(dllimport)
#endif

#else //RAY_BUILD_AS_DLL

#define R_API extern

#endif //RAY_BUILD_AS_DLL

#else // ! WIN32

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

#define rattribute_unused(declaration) __attribute__((unused)) declaration

#define get_filename(x) basename(x)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define R_API extern

#endif //WIN32

//#define RAY_USE_POOL

#ifdef RAY_USE_POOL
#define rdata_new(T) rcheck_value(true, rpool_get(T, rget_pool(T)))
#define rdata_new_buffer(size) raymalloc((size))
#define rdata_free(T, data) \
    do { \
        rpool_free(T, data, rget_pool(T)); \
        data = NULL; \
    } while(0)

#else //RAY_USE_POOL
#define rdata_new(T) (T*)raymalloc(sizeof(T))
#define rdata_new_buffer(size) raymalloc((size))
#define rdata_free(T, data) rayfree(data)

#endif //RAY_USE_POOL

#define rdata_destroy(ptr, destroy_func) \
    if ((ptr) && (destroy_func)) { \
        (destroy_func)((ptr)); \
        (ptr) = NULL; \
    }
#define rdata_new_array(elem_size, count) raycmalloc((count), (elem_size))
#define rdata_new_type_array(elem_type, count) (elem_type*)raycmalloc_type((count), elem_type)
#define rdata_clear_array(data, size_block) memset((data), 0, (size_block))
#define rdata_free_array(data) \
            do { \
			    free(data); \
			    (data) = NULL; \
            } while(0)

#define rmin(a, b) ((a)<(b)?(a):(b))
#define rmax(a, b) ((a)>(b)?(a):(b))

/** 只支持带显式类型的原生数组，如：int[] **/
#define rcount_array(ptr, count) \
          do { \
            if ((ptr) != NULL) \
                (count) = sizeof((ptr)) / sizeof((ptr)[0]); \
            else \
                (count) = 0; \
          } while(0)


#define rassert(expr, msg)                                \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in [ %s:%d (%s) ], [ %s ]\n", \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr,                                        \
            (msg));                                       \
    abort();                                              \
  }                                                       \
 } while (0)

#define rassert_goto(expr, msg, code_int)                 \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in [ %s:%d (%s) ], [%d - %s ]\n", \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr,                                        \
            (code_int), (msg) ? (msg) : "");              \
    goto exit##code_int;                                  \
  }                                                       \
 } while (0)

#define rgoto(code_int) goto exit##code_int


/* ------------------------------- APIs ------------------------------------*/
    
    /** 1 - 相等; 具体参见上文定义 rcode_eq 等 **/
    typedef int(*rcom_compare_func_type)(const void* obj1, const void* obj2);



#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif //RCOMMON_H
