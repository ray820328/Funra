/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RAY_LOG_H
#define RAY_LOG_H

#include "string.h"

#include "rcommon.h"

#define rlog_filename_length 1024
//不能小于64
#define rlog_temp_data_size 51200
#define rlog_cache_data_size 409600

#define print2file


#define RLOG_TOSTR(RLOG_E)    \
			((RLOG_E) == RLOG_VERB ? "VEBR" : \
			((RLOG_E) == RLOG_TRACE ? "TRACE" : \
			((RLOG_E) == RLOG_DEBUG ? "DEBUG" : \
			((RLOG_E) == RLOG_INFO ? "INFO" : \
			((RLOG_E) == RLOG_WARN ? "WARN" : \
			((RLOG_E) == RLOG_ERROR ? "ERROR" : \
			((RLOG_E) == RLOG_FATAL ? "FATAL" : \
			((RLOG_E) == RLOG_ALL ? "ALL" : \
			"UNDEFINED"))))))))

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
    typedef enum {
        RLOG_VERB = 0,
        RLOG_TRACE,
        RLOG_DEBUG,
        RLOG_INFO,
        RLOG_WARN,
        RLOG_ERROR,
        RLOG_FATAL,
		RLOG_ALL,
    } rlog_level_t;

    typedef struct rlog_info_t
    {
        char* filename;
        FILE* file_ptr;
        rlog_level_t level;
        char* logItemData;
        char* logData;
        char* fmtDest;
    } rlog_info_t;

    R_API void init_rlog(const char* logFilename, const rlog_level_t logLevel, const bool seperateFile, const char* fileSuffix);
    R_API void uninit_rlog();

    R_API int rayprintf(rlog_level_t logLevel, const char* fmtStr, ...);
    R_API int flush_rlog_file();

#pragma region "其他"


#pragma endregion "其他"


#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_LOG_H