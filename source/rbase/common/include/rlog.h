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
#include "rthread.h"

#define rlog_filename_length 1024
//不能小于64
#define rlog_temp_data_size 5120
#define rlog_cache_data_size 40960

//#define print2file


#define rlog_level_2str(RLOG_E)    \
		((RLOG_E) == RLOG_VERB ? "VEBR" : \
		((RLOG_E) == RLOG_TRACE ? "TRACE" : \
		((RLOG_E) == RLOG_DEBUG ? "DEBUG" : \
		((RLOG_E) == RLOG_INFO ? "INFO" : \
		((RLOG_E) == RLOG_WARN ? "WARN" : \
		((RLOG_E) == RLOG_ERROR ? "ERROR" : \
		((RLOG_E) == RLOG_FATAL ? "FATAL" : \
		((RLOG_E) == RLOG_ALL ? "all" : \
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

    typedef struct rlog_t
    {
        rmutex_t_def mutex;
        bool inited;
        rlog_level_t level;
        rlog_info_t* log_items[RLOG_ALL];
    } rlog_t;

    R_API int rlog_init(const char* log_default_filename, const rlog_level_t log_default_level, const bool log_default_seperate_file);
    R_API int rlog_uninit();

    R_API int rlog_init_log(rlog_t* rlog, const char* filename, const rlog_level_t level, const bool seperate_file);
    R_API int rlog_uninit_log(rlog_t* rlog);

    R_API int rlog_printf_cached(rlog_t* rlog, rlog_level_t level, const char* fmt, ...);
    R_API int rlog_printf(rlog_t* rlog, rlog_level_t evel, const char* fmt, ...);
    R_API int rlog_flush_file(rlog_t* rlog, bool close_file);
    R_API int rlog_rolling_file(rlog_t* rlog);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_LOG_H