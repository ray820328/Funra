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

// Library version

#define RLIST_VERSION "0.2.0"

// Memory management macros
#ifdef RLIST_CONFIG_H
#define _STR(x) #x
#define STR(x) _STR(x)
#include STR(LIST_CONFIG_H)
#undef _STR
#undef STR
#endif

/*
 * list_t iterator direction.
 */

typedef enum {
    RLIST_HEAD
  , RLIST_TAIL
} rlist_direction_t;

/*
 * list_t node struct.
 */

typedef struct rlist_node {
  struct rlist_node *prev;
  struct rlist_node *next;
  void *val;
} rlist_node_t;

/*
 * list_t struct.
 */

typedef struct {
  rlist_node_t *head;
  rlist_node_t *tail;
  unsigned int len;
  void* (*malloc_node)(size_t size);
  void* (*malloc_it)(size_t size);
  void(*free_node_val)(void *val);
  void(*free_node)(void *val);
  void(*free_self)(void *val);
  void(*free_it)(void *val);
  int (*match)(void *a, void *b);
} rlist_t;

/*
 * list_t iterator struct.
 */

typedef struct {
    rlist_node_t *next;
    rlist_direction_t direction;
} rlist_iterator_t;

rlist_t* rlist_new(void* (*malloc_self)(size_t size));
rlist_t* rlist_init(rlist_t* self);

rlist_node_t* rlist_rpush(rlist_t *self, void* nodeVal);

rlist_node_t* rlist_lpush(rlist_t *self, void* nodeVal);

rlist_node_t* rlist_find(rlist_t *self, void *val);

rlist_node_t* rlist_at(rlist_t *self, int index);

rlist_node_t* rlist_rpop(rlist_t *self);

rlist_node_t* rlist_lpop(rlist_t *self);

void rlist_remove(rlist_t *self, rlist_node_t *node);
void rlist_remove_alone(rlist_t *self, rlist_node_t *node);

void rlist_clear(rlist_t *self);

void rlist_destroy(rlist_t *self);

#define rlist_free_node(self, node) \
    do { \
        if ((node) && (self)->free_node_val) (self)->free_node_val((node)->val); \
        if ((node) && (self)->free_node) (self)->free_node((node)); \
    } while(0)

#define rlist_size(self) self->len

#define rlist_destroy_self(rlist_ptr) \
do { \
    rlist_destroy((rlist_ptr)); \
    (rlist_ptr) = NULL; \
} while(0)

rlist_iterator_t* rlist_iterator_new(rlist_t *list, rlist_direction_t direction);

//rattribute_unused(static) rlist_iterator_t* rlist_iterator_new_from_node(rlist_t *list, rlist_node_t *node, rlist_direction_t direction);

rlist_node_t* rlist_iterator_next(rlist_iterator_t *self);

//static inline void rlist_iterator_destroy(rlist_t *list, rlist_iterator_t *self) {
//    list->free_it(self);
//    self = NULL;
//}
#define rlist_iterator_destroy(rlist_ptr, rlist_iterator_it_prt) \
do { \
    (rlist_ptr)->free_it((rlist_iterator_it_prt)); \
    (rlist_iterator_it_prt) = NULL; \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* RLIST_H */
