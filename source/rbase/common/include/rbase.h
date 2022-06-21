﻿/**
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

#define print2file
#ifdef print2file
//#undef print2file
#endif

#define rtrace(format, ...) rlog_printf(RLOG_TRACE, "%s:%s:%d %ld "format"", get_filename(__FILE__), __FUNCTION__, __LINE__, get_cur_thread_id(), ##__VA_ARGS__)
#define rdebug(format, ...) rlog_printf(RLOG_DEBUG, "%s:%s:%d %ld "format"", get_filename(__FILE__), __FUNCTION__, __LINE__, get_cur_thread_id(), ##__VA_ARGS__)
#define rinfo(format, ...) rlog_printf(RLOG_INFO, "%s:%s:%d %ld "format"", get_filename(__FILE__), __FUNCTION__, __LINE__, get_cur_thread_id(), ##__VA_ARGS__)
#define rwarn(format, ...) rlog_printf(RLOG_WARN, "%s:%s:%d %ld "format"", get_filename(__FILE__), __FUNCTION__, __LINE__, get_cur_thread_id(), ##__VA_ARGS__)
#define rerror(format, ...) rlog_printf(RLOG_ERROR, "%s:%s:%d %ld "format"", get_filename(__FILE__), __FUNCTION__, __LINE__, get_cur_thread_id(), ##__VA_ARGS__)
#define rfatal(format, ...) \
	do { \
		rlog_printf(RLOG_FATAL, "%s:%s:%d %ld "format"", get_filename(__FILE__), __FUNCTION__, __LINE__, get_cur_thread_id(), ##__VA_ARGS__); \
		abort(); \
	} while (0)

#define raymalloc malloc
#define rayfree free

#define fn_raymalloc(x) raymalloc((x))
#define fn_raycmalloc(x, elem_type) (elem_type*) calloc((x), sizeof(elem_type))
#define fn_rayfree(x) \
    do { \
        rayfree((x)); \
        (x) = NULL; \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif //RBASE_H
