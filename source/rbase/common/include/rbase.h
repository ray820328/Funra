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

#if defined(_WIN32) || defined(_WIN64)

#define ros_windows

#else //_WIN64

#define ros_linux

#endif //_WIN64

#define print2file
#ifdef print2file
#undef print2file
#endif

#define rtrace(format, ...) rlog_printf(NULL, RLOG_TRACE, "[%ld] %s:%s:%d "format"", get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rdebug(format, ...) rlog_printf(NULL, RLOG_DEBUG, "[%ld] %s:%s:%d "format"", get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rinfo(format, ...) rlog_printf(NULL, RLOG_INFO, "[%ld] %s:%s:%d "format"", get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rwarn(format, ...) rlog_printf(NULL, RLOG_WARN, "[%ld] %s:%s:%d "format"", get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rerror(format, ...) rlog_printf(NULL, RLOG_ERROR, "[%ld] %s:%s:%d "format"", get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define rfatal(format, ...) \
	do { \
		rlog_printf(NULL, RLOG_FATAL, "[%ld] %s:%s:%d "format"", get_cur_thread_id(), get_filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		abort(); \
	} while (0)


#ifdef __cplusplus
}
#endif

#endif //RBASE_H
