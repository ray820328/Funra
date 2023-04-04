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

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/* ------------------------------- Macros ------------------------------------*/

#define rlog_filename_length 1024
//不能小于64
#define rlog_temp_data_size 2048
#define rlog_cache_data_size 5120

#define log_in_multi_thread
#define print2file
#define print2stdout

#define rlog_level_2str(log_level)    \
	((log_level) == rlog_level_verb ? "VEBR" : \
	((log_level) == rlog_level_trace ? "TRACE" : \
	((log_level) == rlog_level_debug ? "DEBUG" : \
	((log_level) == rlog_level_info ? "INFO" : \
	((log_level) == rlog_level_warn ? "WARN" : \
	((log_level) == rlog_level_error ? "ERROR" : \
	((log_level) == rlog_level_fatal ? "FATAL" : \
	((log_level) == rlog_level_all ? "ALL" : \
	"UNDEFINED"))))))))

#define rlog_declare_global() \
extern rlog_t** rlog_all

#define rlog_define_global() \
rlog_t** rlog_all = NULL

#define rlog_init_global(filename, level, separated_file, file_size) \
if (rlog_all == NULL) { \
    rlog_init((filename), (level), (separated_file), (file_size)); \
} else { \
    rinfo("Already inited."); \
}

#define rlog_uninit_global() \
if (rlog_all != NULL) { \
    rlog_uninit(); \
}

/* ------------------------------- Structs ------------------------------------*/

typedef enum {
    rlog_level_verb = 0,
    rlog_level_trace,
    rlog_level_debug,
    rlog_level_info,
    rlog_level_warn,
    rlog_level_error,
    rlog_level_fatal,
	rlog_level_all,
} rlog_level_t;

typedef enum {
    rlog_state_init = 0,
    rlog_state_working,
    rlog_state_roll_file,
    rlog_state_uninit,
} rlog_state_t;

typedef struct rlog_info_s {
    rlog_level_t level;
    int file_size;//volatile
    rmutex_t* item_mutex;
    char* filename;
    FILE* file_ptr;
    char* item_buffer;
    char* buffer;
    char* item_fmt;
} rlog_info_t;

typedef struct rlog_s {
    rlog_state_t state;
    bool file_separated;
    rlog_level_t level;
    int file_size_max;
    rmutex_t* mutex;
    char* filepath_template;
    rlog_info_t* log_items[rlog_level_all];
} rlog_t;

/* ------------------------------- APIs ------------------------------------*/

// extern rlog_t** rlog_all;

/** file_size: 单位 m **/
R_API int rlog_init(const char* log_default_filename, const rlog_level_t log_default_level, const bool log_default_seperate_file, int file_size);
R_API int rlog_uninit();

R_API int rlog_init_log(rlog_t* rlog, const char* filename, const rlog_level_t level, const bool seperate_file, int file_size);
R_API int rlog_reset(rlog_t* rlog, const rlog_level_t level, int file_size);
R_API int rlog_uninit_log(rlog_t* rlog);

R_API int rlog_printf_cached(rlog_t* rlog, rlog_level_t level, const char* fmt, ...);
R_API int rlog_printf(rlog_t* rlog, rlog_level_t evel, const char* fmt, ...);
R_API int rlog_flush_file(rlog_t* rlog, const rlog_level_t level, bool close_file);
R_API int rlog_rolling_file(rlog_t* rlog, const rlog_level_t level);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_LOG_H