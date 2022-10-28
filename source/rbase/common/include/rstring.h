﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * 注意：目前不支持多线程
 *
 * @author: Ray
 */

#ifndef RSTRING_H
#define RSTRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"
#include "rarray.h"

extern char* rstr_empty_const;

/* ------------------------------- Macros ------------------------------------*/

#define rstr_t char

#define rstr_null "NULL"
#define rstr_empty rstr_empty_const
#define rstr_end '\0'
#define rstr_tab '\t'
#define rstr_blank ' '
#define rstr_array_end NULL

#define rstr_number_max_bytes 32

#define rstr_new(size) (char*)raymalloc((size) + 1u)
#define rstr_init(rstr) ((char*)(rstr))[0] = rstr_end
#define rstr_uninit(rstr) ((char*)(rstr))[0] = rstr_end
#define rstr_reset(rstr) rstr_init((rstr))
#define rstr_free(rstr) if ((rstr) != NULL && (rstr) != rstr_empty_const) rayfree(rstr)
#define rstr_is_empty(rstr) ((rstr) == NULL || (rstr) == rstr_empty)
#define rstr_sizeof(rstr) sizeof(rstr)

#define rstr_array_new(size) (char**)rdata_new_array(sizeof(char*), (size) + 1)
#define rstr_array_count(rstr, count) \
    do { \
        count = 0; \
        while (rstr && *(rstr + count)) { \
            count += 1; \
        } \
    } while(0)
#define rstr_array_make(rstr_arr, count) \
    char* rstr_arr[count] = { [count - 1] = rstr_array_end }
/** 确保非最后一个NULL都为rstr_empty，否则会提前结束遍历 **/
#define rstr_array_for(rstr, item) \
    for (size_t rstr##_index = 0; (item) = ((rstr) == NULL ? NULL : ((char**)(rstr))[rstr##_index]), ((rstr) != NULL && (item) != rstr_array_end); rstr##_index ++)
/** 确保非最后一个NULL都为rstr_empty，否则会内存泄露 **/
#define rstr_array_free(rstr) \
    while(rstr) { \
        char* rstr##_free_item = NULL; \
        for (size_t rstr##_free_index = 0; rstr##_free_item = *((char**)rstr + rstr##_free_index), rstr##_free_item != rstr_array_end; rstr##_free_index++) { \
            rstr_free(rstr##_free_item); \
        } \
        rdata_free_array(rstr); \
    }

/**    
#define rnum2str(ret_num_str, num, fmt_str) \
    do { \
        char _num_temp_str_[rstr_number_max_bytes]; \
        rstr_fmt_num((_num_temp_str_), (num), (const char*)(fmt_str)); \
        (ret_num_str) = _num_temp_str_; \
    } while(0) **/

#define rnum2str(ret_num_str, num, fmt_str) \
    do { \
        char _num_temp_str_[rstr_number_max_bytes]; \
		int _len_num_str_ = 0; \
		if ((fmt_str) != NULL) { \
			_len_num_str_ = sprintf((_num_temp_str_), (fmt_str) ? (fmt_str) : "%X", (num)); /**警告不会执行*/ \
		} else {  \
			int64_t _num_temp_value_ = (num); \
			_len_num_str_ = sprintf(_num_temp_str_, "%"PRId64, _num_temp_value_); \
		} \
        rassert((_len_num_str_ < rstr_number_max_bytes), "rnum2str"); \
        (ret_num_str) = _num_temp_str_; \
    } while(0)
#define rformat_s(buffer_str, fmt_str, ...) \
    do { \
        if (sprintf((buffer_str), (fmt_str), ##__VA_ARGS__) >= (int)sizeof((buffer_str))) { \
            rerror("error, data exceed of max len(%d)", (int)sizeof((buffer_str))); \
            (buffer_str)[(int)sizeof((buffer_str)) - 1] = '\0'; \
        } \
    } while(0)
#define rformat_time_s_full(time_str, time_value, fmt_str) \
    do { \
        int* time_now_datas = rtime_from_time_millis((time_value) ? (time_value) : rtime_millisec()); \
        rformat_s((time_str), (fmt_str) ? (fmt_str) : "%.4d-%.2d-%.2d %.2d:%.2d:%.2d %.3d", \
            time_now_datas[0], time_now_datas[1], time_now_datas[2], time_now_datas[3], time_now_datas[4], time_now_datas[5], time_now_datas[6]); \
    } while(0)
#define rformat_time_s_yyyymmddhhMMss(time_str, time_value, fmt_str) \
    do { \
        int* _datetime_datas_ = rtime_from_time_millis((time_value) ? (time_value) : rtime_millisec()); \
        rformat_s((time_str), (fmt_str) ? (fmt_str) : "%.4d%.2d%.2d%.2d%.2d%.2d", \
            _datetime_datas_[0], _datetime_datas_[1], _datetime_datas_[2], _datetime_datas_[3], _datetime_datas_[4], _datetime_datas_[5]); \
    } while(0)
#define rformat_time_s_yyyymmdd(time_str, time_value, fmt_str) \
    do { \
        int* _date_datas_ = rtime_from_time_millis((time_value) ? (time_value) : rtime_millisec()); \
        rformat_s((time_str), (fmt_str) ? (fmt_str) : "%.4d%.2d%.2d", \
            _date_datas_[0], _date_datas_[1], _date_datas_[2]); \
    } while(0)
#define rformat_time_s_hhMMss(time_str, time_value, fmt_str) \
    do { \
        int* _time_datas_ = rtime_from_time_millis((time_value) ? (time_value) : rtime_millisec()); \
        rformat_s((time_str), (fmt_str) ? (fmt_str) : "%.2d%.2d%.2d", \
            _time_datas_[3], _time_datas_[4], _time_datas_[5]); \
    } while(0)

#define rstr_set(dest, src, len) \
    memcpy(dest, src, (len == 0) ? (rstr_len(src) + 1) : len);

#define rstr_compare(str1, str2) \
    rstr_compare_func((str1), (str2))
#define rstr_eq(str1, str2) \
    (strcmp((str1), (str2)) == 0 ? true : false)
#define rstr_len(str1) \
    ((str1) == NULL ? 0 : strlen((str1)))

#define rstr_2int(val) \
    atoi((val))
#define rstr_2long(val) \
    atol((val))
#define rstr_2ul(val) \
    strtoul((val))
#define rstr_2ll(val) \
    strtoll((val))
#define rstr_2ull(val) \
    strtoull((val))
#define rstr_2float(val) \
    strtof((val))
#define rstr_2double(val) \
    strtod((val))
#define rstr_2ld(val) \
    strtold((val))

/* ------------------------------- APIs ------------------------------------*/

R_API void rstr_free_func(char* dest);
R_API int rstr_compare_func(const char* obj1, const char* obj2);

/** rstr_array_end结尾 **/
R_API char** rstr_make_array(const int count, ...);

R_API size_t rstr_cat(char* dest, const char* src, const size_t sizeof_dest);
R_API char* rstr_concat_array(const char** src, const char* delim, bool suffix);
/** rstr_array_end结尾 **/
R_API char* rstr_join(const char* src, ...);

R_API char* rstr_fmt(char* dest, const char* fmt, int max_len, ...);
R_API int rstr_fmt_num(char* ret_num_str, void* num, const char* fmt);
/** len为0时到src结尾 **/
R_API char* rstr_cpy(const void *src, size_t len);
R_API char* rstr_cpy_full(const void *key);

/** -1: 无子串 **/
R_API int rstr_index(const char* src, const char* key);
/** -1: 无子串 **/
R_API int rstr_last_index(const char* src, const char* key);

/** ascii长度，如果new = false，整个from字符串，不仅仅dest，释放要特别注意 **/
R_API char* rstr_sub(const char* src, size_t from, size_t dest_size, bool new);
R_API char* rstr_sub_str(const char* src, const char* key, bool new);

/** 支持utf8，非unicode16 **/
R_API char* rstr_repl(const char* src, const char* old_str, const char* new_str);
/** 无匹配返回null **/
R_API char** rstr_split(const char* src, const char* delim);

R_API bool rstr_is_digit(const char* src, int end_index);

R_API int rstr_trim(char* src);
R_API int rstr_trim_begin(char* src);
R_API int rstr_trim_end(char* src);


#ifdef rmemory_enable_tracer

//static char* rstr_cpy_warp(char* data, int len, long thread_id, char* filename, char* func, int line) {
//
//    if (data == NULL || data == rstr_empty) {
//        return rstr_empty;
//}
//
//    unsigned int data_len = (unsigned int)strlen((char*)data);
//    data_len = len > 0 && len < data_len ? len : data_len;
//    char* data_copy = rmem_malloc_trace(data_len + 1, thread_id, filename, func, line);
//    if (data_copy == NULL) {
//        return rstr_empty;
//    }
//
//    //rstr_init(data_copy);
//    memcpy(data_copy, data, data_len);
//    data_copy[data_len] = rstr_end;
//    return data_copy;
//}
//#define rstr_cpy(src, len) rstr_cpy_warp(src, len, rthread_cur_id(), get_filename(__FILE__), __FUNCTION__, __LINE__) 

#endif //rmemory_enable_tracer

#ifdef __cplusplus
}
#endif

#endif //RSTRING_H
