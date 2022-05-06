/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
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

#define rstr_new(size) rnew_data_array(char, (size))
#define rstr_init(rstr) ((char*)(rstr))[0] = rstr_end
#define rstr_uninit(rstr) ((char*)(rstr))[0] = rstr_end
#define rstr_free(str) rfree_data_array(char, (str))

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
    strcmp((str1), (str2)) == 0 ? 1 : 0
#define rstr_len(str1) strlen((str1))

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

R_API size_t rstr_cat(char* dest, const char* src, const size_t sizeofDest);

R_API char* rstr_fmt(char* dest, const char* fmt, const int maxLen, ...);

R_API char* rstr_cpy(const void *key);

R_API char* rstr_substr(const char *src, const size_t dest_size);

//有中文截断危险
R_API char* rstr_repl(char *src, char *destStr, int destLen, char *oldStr, char *newStr);


#ifdef __cplusplus
}
#endif

#endif //RSTRING_H
