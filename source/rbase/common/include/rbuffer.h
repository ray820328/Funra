/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RBUFFER_H
#define RBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"

/* ------------------------------- Macros ------------------------------------*/
#define rbuffer_size_t int32_t
#define rbuffer_size_max 0x7FFFFFFF
#define rbuffer_size_min 0
#define rbuffer_size_t_format "d"
#define rbuffer_init_capacity_default 1024

#define rbuffer_init(d, size) \
    do { \
        rassert((d) == NULL, ""); \
        (d) = rbuffer_create((size)); \
        rassert((d) != NULL, ""); \
    } while(0)

#define rbuffer_free(d) \
    if (d) { \
        rbuffer_release(d); \
        d = NULL; \
    }

#define rbuffer_capacity(d) ((d)->capacity)
#define rbuffer_size(d) ((d)->pos - (d)->offset)
#define rbuffer_start(d) ((d)->offset)
#define rbuffer_left(d) ((d)->capacity - (d)->pos)
#define rbuffer_full(d) (rbuffer_left(d) == 0)
#define rbuffer_empty(d) ((d)->pos == 0 && (d)->offset == 0)

/* ------------------------------- Structs ------------------------------------*/
    
typedef enum rbuffer_code_t {
    rbuffer_code_error = 1,
    rbuffer_code_index_out4_size,
    rbuffer_code_index_out4_capacity,
} rbuffer_code_t;

typedef struct rbuffer_s {
    char* data;
    int offset;
    int pos;
    int capacity;
} rbuffer_t;

/* ------------------------------- APIs ------------------------------------*/

rbuffer_t* rbuffer_create(rbuffer_size_t init_capacity);
int rbuffer_read(rbuffer_t* d, rbuffer_size_t size);
int rbuffer_write(rbuffer_t* d, char* val, rbuffer_size_t size);
int rbuffer_rewind(rbuffer_t* d);//数据移动到缓冲区头部
int rbuffer_skip(rbuffer_t* d, rbuffer_size_t size); // offset 往 pos 移动
int rbuffer_revert(rbuffer_t* d);//重置 pos 到 offset
void rbuffer_clear(rbuffer_t* d);
void rbuffer_release(rbuffer_t* d);
char* rbuffer_output(rbuffer_t* d);

#ifdef __cplusplus
}
#endif

#endif //RBUFFER_H
