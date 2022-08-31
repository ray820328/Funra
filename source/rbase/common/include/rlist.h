/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RLIST_H
#define RLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "rcommon.h"

// Memory management macros
#ifdef RLIST_CONFIG_H
#define _STR(x) #x
#define STR(x) _STR(x)
#include STR(RLIST_CONFIG_H)
#undef _STR
#undef STR
#endif

/* ------------------------------- Structs ------------------------------------*/
typedef void* (*rlist_fn_malloc_node_val)(void *val);
typedef void (*rlist_fnfree_node_val)(void *val);

typedef enum {
    rlist_dir_head,
    rlist_dir_tail
} rlist_direction_t;

typedef struct rlist_node_t {
    struct rlist_node_t *prev;
    struct rlist_node_t *next;
    void *val;
} rlist_node_t;

typedef struct rlist_t {
    rlist_node_t *head;
    rlist_node_t *tail;
    unsigned int len;
    rlist_fn_malloc_node_val malloc_node_val;
    rlist_fnfree_node_val free_node_val;
    rcom_compare_func_type compare_node_val;
} rlist_t;

typedef struct rlist_iterator_t {
    rlist_node_t *next;
    rlist_direction_t direction;
} rlist_iterator_t;


/* ------------------------------- Macros ------------------------------------*/

#define rlist_declare_set_value_func(T) \
    T##_inner_type rlist_copy_value_func_##T(T##_inner_type obj); \
    void rlist_free_value_func_##T(void* obj)

#define rlist_define_set_value_func(T) \
    T##_inner_type rlist_copy_value_func_##T(T##_inner_type obj)

#define rlist_init(self, T) \
    do { \
        if ((self) == NULL) { \
            (self) = rlist_create(); \
        } \
        rassert((self) != NULL, ""); \
        self->malloc_node_val = (rlist_fn_malloc_node_val)T##_copy_func; \
        self->free_node_val = (rlist_fnfree_node_val)T##_free_func; \
        self->compare_node_val = (rcom_compare_func_type)T##_compare_func; \
    } while(0)

#define rlist_free_node(self, node) \
    if ((node) != NULL) { \
        if((self) != NULL && (self)->free_node_val) (self)->free_node_val((node)->val); \
        rdata_free(rlist_node_t, (node)); \
    } \

#define rlist_size(self) (self)->len

#define rlist_destroy_self(rlist_ptr) \
do { \
    rlist_destroy((rlist_ptr)); \
    (rlist_ptr) = NULL; \
} while(0)

#define rlist_it(list, direction) \
    { \
        ((direction) == rlist_dir_tail) ? (list)->head : (list)->tail, (direction) \
    }

#define rlist_it_first(it) \
    do { \
        (it)->next = ((direction) == rlist_dir_tail) ? (list)->head : (list)->tail; \
        (it)->direction = (direction); \
    } while(0)

#define rlist_it_from(list, direction, from) \
    { \
        (from), (direction) \
    }


/* ------------------------------- APIs ------------------------------------*/

rlist_t* rlist_create();

rlist_node_t* rlist_rpush(rlist_t *self, void* nodeVal);

rlist_node_t* rlist_lpush(rlist_t *self, void* nodeVal);

rlist_node_t* rlist_find(rlist_t *self, void *val);

rlist_node_t* rlist_at(rlist_t *self, int index);

rlist_node_t* rlist_rpop(rlist_t *self);

rlist_node_t* rlist_lpop(rlist_t *self);

rlist_t* rlist_clone(rlist_t* src, rlist_direction_t dir);

int rlist_merge(rlist_t* dest, rlist_t* temp, rlist_direction_t dir);

void rlist_remove(rlist_t *self, rlist_node_t *node);

void rlist_clear(rlist_t *self);

void rlist_destroy(rlist_t *self);

rlist_node_t* rlist_next(rlist_iterator_t *self);

#ifdef __cplusplus
}
#endif

#endif /* RLIST_H */
