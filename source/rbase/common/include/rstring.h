/**
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

/* ------------------------------- Macros ------------------------------------*/

#define rstr_null "NULL"
#define rstr_empty ""
#define rstr_end '\0'
#define rstr_array_end NULL

#define rstr_new(size) raymalloc(size)
#define rstr_init(rstr) ((char*)(rstr))[0] = rstr_end
#define rstr_uninit(rstr) ((char*)(rstr))[0] = rstr_end
#define rstr_reset(rstr) rstr_init((rstr))
#define rstr_free(rstr) if ((rstr) != rstr_empty) rayfree(rstr)

#define rstr_array_new(size) rnew_data_array(sizeof(char*), (size) + 1)
#define rstr_array_count(rstr, count) \
    do { \
        count = 0; \
        while (rstr && *(rstr + count)) { \
            count += 1; \
        } \
    } while(0)
/** 确保非最后一个NULL都为rstr_empty，否则会提前结束遍历 **/
#define rstr_array_for(rstr, item) \
    for (size_t rstr##_index = 0; (char*)item = *((char**)rstr + rstr##_index), rstr != NULL && item != rstr_array_end; rstr##_index++)
/** 确保非最后一个NULL都为rstr_empty，否则会内存泄露 **/
#define rstr_array_free(rstr) \
    while(rstr) { \
        char* rstr##_free_item = NULL; \
        for (size_t rstr##_free_index = 0; rstr##_free_item = *((char**)rstr + rstr##_free_index), rstr##_free_item != rstr_array_end; rstr##_free_index++) { \
            rstr_free(rstr##_free_item); \
        } \
        rfree_data_array(rstr); \
    }
    

#define rnum2str(retNumStr, num, base) \
    do { \
        char retTempStr[32]; \
        int lenNumStr = sprintf((retTempStr), "%"PRId64, (int64_t)(num)); \
        (retTempStr)[lenNumStr] = '\0'; \
        (retNumStr) = (retTempStr); \
    } while(0)
#define rformat_s(strBuffer, fmt, ...) \
    do { \
        if (sprintf((strBuffer), (fmt), ##__VA_ARGS__) >= (int)sizeof((strBuffer))) { \
            rlog_printf(RLOG_ERROR, "error, data exceed of max len(%d).\n", (int)sizeof((strBuffer))); \
            (strBuffer)[(int)sizeof((strBuffer)) - 1] = '\0'; \
        } \
    } while(0)
#define rformat_time_s_full(timeStr, timeValue) \
    do { \
        int* timeNowDatas = rdate_from_time_millis((timeValue) ? (timeValue) : millisec_r()); \
        rformat_s((timeStr), "%.4d-%.2d-%.2d %.2d:%.2d:%.2d %.3d", \
            timeNowDatas[0], timeNowDatas[1], timeNowDatas[2], timeNowDatas[3], timeNowDatas[4], timeNowDatas[5], timeNowDatas[6]); \
    } while(0)
#define rformat_time_s_yyyymmddhhMMss(timeStr, timeValue) \
    do { \
        int* timeNowDatas = rdate_from_time_millis((timeValue) ? (timeValue) : millisec_r()); \
        rformat_s((timeStr), "%.4d%.2d%.2d%.2d%.2d%.2d", \
            timeNowDatas[0], timeNowDatas[1], timeNowDatas[2], timeNowDatas[3], timeNowDatas[4], timeNowDatas[5]); \
    } while(0)

#define rstr_eq(str1, str2) \
    strcmp((str1), (str2)) == 0 ? true : false
#define rstr_len(str1) (str1) == NULL ? 0 : strlen((str1))

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

R_API size_t rstr_cat(char* dest, const char* src, const size_t sizeof_dest);
R_API char* rstr_concat(const char** src, const char* delim, bool suffix);

R_API char* rstr_fmt(char* dest, const char* fmt, const int max_len, ...);
/** len为0时到src结尾 **/
R_API char* rstr_cpy(const void *key, size_t len);

R_API int rstr_index(const char* src, const char* key);
R_API int rstr_last_index(const char* src, const char* key);

/** ascii长度，如果new = false，整个from字符串，不仅仅dest **/
R_API char* rstr_sub(const char* src, const size_t from, const size_t dest_size, bool new);
R_API char* rstr_sub_str(const char* src, const char* key, bool new);

/** 支持utf8，非unicode16 **/
R_API char* rstr_repl(char *src, char *dest_str, int dest_len, char *old_str, char *new_str);
/** 无匹配返回null **/
R_API char** rstr_split(const char *src, const char *delim);


#ifdef __cplusplus
}
#endif

#endif //RSTRING_H
