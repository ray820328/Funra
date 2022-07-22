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

#define rcode_eq 1
#define rcode_neq 2
#define rcode_gt 3
#define rcode_gte 4
#define rcode_lt 5
#define rcode_lte 6

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

#define get_filename(x) strrchr(x, '\\') ? strrchr(x, '\\') + 1 : x
#define likely(x) x
#define unlikely(x) x

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

//析构宏，二级指针难看
#define rdestroy_object(obejectPrt, destroyFunc) \
    if ((obejectPrt)) { \
        (destroyFunc)((obejectPrt)); \
        (obejectPrt) = NULL; \
    }


//#define RAY_USE_POOL

#ifdef RAY_USE_POOL
#define rnew_data(T) rcheck_value(true, rpool_get(T, rget_pool(T)))
#define rfree_data(T, data) \
            do { \
                rpool_free(T, data, rget_pool(T)); \
                data = NULL; \
            } while(0)

#else //RAY_USE_POOL
#define rnew_data(T) rcheck_value(true, (T*)raymalloc(sizeof(T)))
#define rfree_data(T, data) \
            do { \
			    rayfree(data); \
			    data = NULL; \
            } while(0)

#endif //RAY_USE_POOL

#define rnew_data_array(size_elem, count) calloc((count), (size_elem))
#define rclear_data_array(data, size_block) memset((data), 0, (size_block))
#define rfree_data_array(data) \
            do { \
			    free(data); \
			    (data) = NULL; \
            } while(0)

#define rmin(a,b) ((a)<(b)?(a):(b))
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


typedef enum rdata_plain_type_t {
    rdata_type_unknown = 0,           /** no type */
    rdata_type_bool,                  /** bool */
    rdata_type_char,                  /** char */
    rdata_type_uchar,                 /** unsigned char */
    rdata_type_short,                 /** short int */
    rdata_type_ushort,                /** unsigned short int */
    rdata_type_int,                   /** int */
    rdata_type_uint,                  /** unsigned int */
    rdata_type_long,                  /** long int */
    rdata_type_ulong,                 /** unsigned long int */
    rdata_type_long_long,             /** long long */
    rdata_type_ulong_long,            /** unsigned long long */
    rdata_type_float,                 /** float */
    rdata_type_double,                /** double */
    rdata_type_long_double,           /** long double */
    rdata_type_string,                /** string */
    rdata_type_ptr                    /** generic pointer */
} rdata_plain_type_t;

typedef enum rdata_plain_type_size_t {
    rdata_type_unknown_size = 0,                          /** no type */
    rdata_type_bool_size = sizeof(bool),                  /** bool */
    rdata_type_char_size = sizeof(char),                  /** char */
    rdata_type_uchar_size = sizeof(unsigned char),        /** unsigned char */
    rdata_type_short_size = sizeof(short),                /** short int */
    rdata_type_ushort_size = sizeof(unsigned short),      /** unsigned short int */
    rdata_type_int_size = sizeof(int),                    /** int */
    rdata_type_uint_size = sizeof(unsigned int),          /** unsigned int */
    rdata_type_long_size = sizeof(long),                  /** long int */
    rdata_type_ulong_size = sizeof(unsigned long),        /** unsigned long int */
    rdata_type_long_long_size = sizeof(long long),        /** long long */
    rdata_type_ulong_long_size = sizeof(unsigned long long),           /** unsigned long long */
    rdata_type_float_size = sizeof(float),                /** float */
    rdata_type_double_size = sizeof(double),              /** double */
    rdata_type_long_double_size = sizeof(long double),    /** long double */
    rdata_type_string_size = sizeof(char*),               /** string */
    rdata_type_ptr_size = sizeof(void*)                   /** generic pointer */
} rdata_plain_type_size_t;

#define rdata_type_unknown_inner_type
#define rdata_type_bool_inner_type bool
#define rdata_type_char_inner_type char
#define rdata_type_uchar_inner_type unsigned char
#define rdata_type_short_inner_type short int
#define rdata_type_ushort_inner_type unsigned short int
#define rdata_type_int_inner_type int
#define rdata_type_uint_inner_type unsigned int
#define rdata_type_long_inner_type long int
#define rdata_type_ulong_inner_type unsigned long int
#define rdata_type_long_long_inner_type long long
#define rdata_type_ulong_long_inner_type unsigned long long
#define rdata_type_float_inner_type float
#define rdata_type_double_inner_type double
#define rdata_type_long_double_inner_type long double
#define rdata_type_string_inner_type char*
#define rdata_type_ptr_inner_type void**

#define rdata_type_unknown_copy_func NULL
#define rdata_type_bool_copy_func NULL
#define rdata_type_char_copy_func NULL
#define rdata_type_uchar_copy_func NULL
#define rdata_type_short_copy_func NULL
#define rdata_type_ushort_copy_func NULL
#define rdata_type_int_copy_func NULL
#define rdata_type_uint_copy_func NULL
#define rdata_type_long_copy_func NULL
#define rdata_type_ulong_copy_func NULL
#define rdata_type_long_long_copy_func NULL
#define rdata_type_ulong_long_copy_func NULL
#define rdata_type_float_copy_func NULL
#define rdata_type_double_copy_func NULL
#define rdata_type_long_double_copy_func NULL
#define rdata_type_string_copy_func rstr_cpy_full
#define rdata_type_ptr_copy_func NULL

#define rdata_type_unknown_free_func NULL
#define rdata_type_bool_free_func NULL
#define rdata_type_char_free_func NULL
#define rdata_type_uchar_free_func NULL
#define rdata_type_short_free_func NULL
#define rdata_type_ushort_free_func NULL
#define rdata_type_int_free_func NULL
#define rdata_type_uint_free_func NULL
#define rdata_type_long_free_func NULL
#define rdata_type_ulong_free_func NULL
#define rdata_type_long_long_free_func NULL
#define rdata_type_ulong_long_free_func NULL
#define rdata_type_float_free_func NULL
#define rdata_type_double_free_func NULL
#define rdata_type_long_double_free_func NULL
#define rdata_type_string_free_func rstr_free_func
#define rdata_type_ptr_free_func NULL

#define rdata_type_unknown_compare_func NULL
#define rdata_type_bool_compare_func NULL
#define rdata_type_char_compare_func NULL
#define rdata_type_uchar_compare_func NULL
#define rdata_type_short_compare_func NULL
#define rdata_type_ushort_compare_func NULL
#define rdata_type_int_compare_func NULL
#define rdata_type_uint_compare_func NULL
#define rdata_type_long_compare_func NULL
#define rdata_type_ulong_compare_func NULL
#define rdata_type_long_long_compare_func NULL
#define rdata_type_ulong_long_compare_func NULL
#define rdata_type_float_compare_func NULL
#define rdata_type_double_compare_func NULL
#define rdata_type_long_double_compare_func NULL
#define rdata_type_string_compare_func rstr_compare_func
#define rdata_type_ptr_compare_func NULL

#define rdata_type_declare_compare_func(T) \
    int T##_compare_func(const T##_inner_type obj1, const T##_inner_type obj2)

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
