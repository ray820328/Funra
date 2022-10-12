/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RBASE_H
#define RBASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define rmem_eq(mem1, mem2, len) \
    (memcmp((mem1), (mem2), (len)) == 0 ? true : false)

#if defined(_WIN32) || defined(_WIN64)

#define ros_windows

#else //_WIN64

#define ros_linux

#endif //_WIN64

#define print2file
#ifdef print2file
#undef print2file
#endif

#define rtrace(format, ...) rlog_printf(NULL, RLOG_TRACE, "[%ld] %s:%s:%d "format"\n", rthread_cur_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rdebug(format, ...) rlog_printf(NULL, RLOG_DEBUG, "[%ld] %s:%s:%d "format"\n", rthread_cur_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rinfo(format, ...) rlog_printf(NULL, RLOG_INFO, "[%ld] %s:%s:%d "format"\n", rthread_cur_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rwarn(format, ...) rlog_printf(NULL, RLOG_WARN, "[%ld] %s:%s:%d "format"\n", rthread_cur_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rerror(format, ...) rlog_printf(NULL, RLOG_ERROR, "[%ld] %s:%s:%d "format"\n", rthread_cur_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rfatal(format, ...) \
do { \
	rlog_printf(NULL, RLOG_FATAL, "[%ld] %s:%s:%d "format"\n", rthread_cur_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	abort(); \
} while (0)

typedef struct rdata_userdata_s rdata_userdata_t;
struct rdata_userdata_s {
    rdata_userdata_t* next;
    const char* key;
    void* data;
};

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
    rdata_type_int64,                 /** long long */
    rdata_type_uint64,                /** unsigned long long */
    rdata_type_float,                 /** float */
    rdata_type_double,                /** double */
    rdata_type_long_double,           /** long double */
    rdata_type_string,                /** string */
    rdata_type_ptr,                   /** generic pointer */
    rdata_type_data                   /** struct */
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
    rdata_type_int64_size = sizeof(int64_t),              /** long long */
    rdata_type_uint64_size = sizeof(uint64_t),            /** unsigned long long */
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
#define rdata_type_uint_inner_type uint32_t
#define rdata_type_long_inner_type int32_t
#define rdata_type_ulong_inner_type unsigned long int
#define rdata_type_int64_inner_type int64_t
#define rdata_type_uint64_inner_type uint64_t
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
#define rdata_type_int64_copy_func NULL
#define rdata_type_uint64_copy_func NULL
#define rdata_type_float_copy_func NULL
#define rdata_type_double_copy_func NULL
#define rdata_type_long_double_copy_func NULL
#define rdata_type_string_copy_func rstr_cpy_full
#define rdata_type_ptr_copy_func NULL

#define rdata_type_unknown_copy(val)
#define rdata_type_bool_copy(val) (val)
#define rdata_type_char_copy(val) (val)
#define rdata_type_uchar_copy(val) (val)
#define rdata_type_short_copy(val) (val)
#define rdata_type_ushort_copy(val) (val)
#define rdata_type_int_copy(val) (val)
#define rdata_type_uint_copy(val) (val)
#define rdata_type_long_copy(val) (val)
#define rdata_type_ulong_copy(val) (val)
#define rdata_type_int64_copy(val) (val)
#define rdata_type_uint64_copy(val) (val)
#define rdata_type_float_copy(val) (val)
#define rdata_type_double_copy(val) (val)
#define rdata_type_long_double_copy(val) (val)
#define rdata_type_string_copy(val) rstr_cpy_full(val)
#define rdata_type_ptr_copy(val) (val)

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
#define rdata_type_int64_free_func NULL
#define rdata_type_uint64_free_func NULL
#define rdata_type_float_free_func NULL
#define rdata_type_double_free_func NULL
#define rdata_type_long_double_free_func NULL
#define rdata_type_string_free_func rstr_free_func
#define rdata_type_ptr_free_func NULL

#define rdata_type_unknown_free(val)
#define rdata_type_bool_free(val)
#define rdata_type_char_free(val)
#define rdata_type_uchar_free(val)
#define rdata_type_short_free(val)
#define rdata_type_ushort_free(val)
#define rdata_type_int_free(val)
#define rdata_type_uint_free(val)
#define rdata_type_long_free(val)
#define rdata_type_ulong_free(val)
#define rdata_type_int64_free(val)
#define rdata_type_uint64_free(val)
#define rdata_type_float_free(val)
#define rdata_type_double_free(val)
#define rdata_type_long_double_free(val)
#define rdata_type_string_free(val) rstr_free_func(val)
#define rdata_type_ptr_free(val)

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
#define rdata_type_int64_compare_func NULL
#define rdata_type_uint64_compare_func NULL
#define rdata_type_float_compare_func NULL
#define rdata_type_double_compare_func NULL
#define rdata_type_long_double_compare_func NULL
#define rdata_type_string_compare_func rstr_compare_func
#define rdata_type_ptr_compare_func NULL

#define rdata_type_unknown_compare(val1, val2) rcode_eq
#define rdata_type_bool_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_char_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_uchar_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_short_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_ushort_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_int_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_uint_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_long_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_ulong_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_int64_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_uint64_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_float_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_double_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_long_double_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)
#define rdata_type_string_compare(val1, val2) rstr_compare_func(val1, val2)
#define rdata_type_ptr_compare(val1, val2) (val1) == (val2) ? rcode_eq : ((val1) < (val2) ? rcode_lt : rcode_gt)

#define rdata_type_unknown_hash_func NULL
#define rdata_type_bool_hash_func NULL
#define rdata_type_char_hash_func NULL
#define rdata_type_uchar_hash_func NULL
#define rdata_type_short_hash_func NULL
#define rdata_type_ushort_hash_func NULL
#define rdata_type_int_hash_func NULL
#define rdata_type_uint_hash_func NULL
#define rdata_type_long_hash_func NULL
#define rdata_type_ulong_hash_func NULL
#define rdata_type_int64_hash_func NULL
#define rdata_type_uint64_hash_func NULL
#define rdata_type_float_hash_func NULL
#define rdata_type_double_hash_func NULL
#define rdata_type_long_double_hash_func NULL
#define rdata_type_string_hash_func rhash_func_murmur
#define rdata_type_ptr_hash_func NULL

#define rdata_type_unknown_hash(val)
#define rdata_type_bool_hash(val) (val)
#define rdata_type_char_hash(val) (val)
#define rdata_type_uchar_hash(val) (val)
#define rdata_type_short_hash(val) (val)
#define rdata_type_ushort_hash(val) (val)
#define rdata_type_int_hash(val) (val)
#define rdata_type_uint_hash(val) (val)
#define rdata_type_long_hash(val) (val)
#define rdata_type_ulong_hash(val) (val)
#define rdata_type_int64_hash(val) (val)
#define rdata_type_uint64_hash(val) (val)
#define rdata_type_float_hash(val) (val)
#define rdata_type_double_hash(val) (val)
#define rdata_type_long_double_hash(val) (val)
#define rdata_type_string_hash(val) rhash_func_murmur(val)
#define rdata_type_ptr_hash(val) (val)


#define rdata_copy_func_define(T, val) \
V##_inner_type rdict_copy_value_func_##V(void* data_ext, const V##_inner_type obj) { \
    return (V##_inner_type)V##_copy(obj); \
} \


#ifdef __cplusplus
}
#endif

#endif //RBASE_H
