/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rlist.h"

/*
 * Allocate a new rlist_t. NULL on failure.
 */
rlist_t* rlist_new(void* (*malloc_self)(size_t size)) {
    rlist_t *self;
    if (!malloc_self || !(self = malloc_self(sizeof(rlist_t)))) {
        return NULL;
    }

    return rlist_init(self);
}

rlist_t* rlist_init(rlist_t* self) {
    if (!self) {
        return NULL;
    }
    self->head = NULL;
    self->tail = NULL;
    self->malloc_node = NULL;
    self->malloc_it = NULL;
    self->free_node_val = NULL;
    self->free_node = NULL;
    self->free_self = NULL;
    self->free_it = NULL;
    self->match = NULL;
    self->len = 0;

    return self;
}

/*
 * Free the list.
 */
void rlist_clear(rlist_t *self) {
    if (!self || self->len <= 0) {
        return;
    }
    unsigned int len = self->len;
    rlist_node_t *next;
    rlist_node_t *curr = self->head;

    while (len--) {
        next = curr->next;
        if (self->free_node_val) {
            self->free_node_val(curr->val);
        }

        if (self->free_node) {
            self->free_node(curr);
        }
        curr = next;
    }

    self->tail = self->head = NULL;
    self->len = 0;
}

void rlist_destroy(rlist_t *self) {
    unsigned int len = self->len;
    rlist_node_t *next;
    rlist_node_t *curr = self->head;

    while (len--) {
        next = curr->next;
        if (self->free_node_val) {
            self->free_node_val(curr->val);
        }

        if (self->free_node) {
            self->free_node(curr);
        }
        curr = next;
    }

    if (self->free_self) self->free_self(self);
}

/*
 * Append the given node to the list
 * and return the node, NULL on failure.
 */
rlist_node_t* rlist_rpush(rlist_t *self, void* nodeValue) {
    if (!nodeValue) return NULL;//不支持空节点

    rlist_node_t* node = self->malloc_node(sizeof(rlist_node_t));
    if (!node) {
        return NULL;
    }
    node->val = nodeValue;


    if (self->len > 0) {
        node->prev = self->tail;
        node->next = NULL;
        self->tail->next = node;
        self->tail = node;
    }
    else {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;

    return node;
}

/*
 * Return / detach the last node in the list, or NULL.
 */
rlist_node_t* rlist_rpop(rlist_t *self) {
    if (!self->len) return NULL;

    rlist_node_t *node = self->tail;

    if (--self->len) {
        (self->tail = node->prev)->next = NULL;
    }
    else {
        self->tail = self->head = NULL;
    }

    node->next = node->prev = NULL;
    return node;
}

/*
 * Return / detach the first node in the list, or NULL.
 */
rlist_node_t* rlist_lpop(rlist_t *self) {
    if (!self->len) return NULL;

    rlist_node_t *node = self->head;

    if (--self->len) {
        (self->head = node->next)->prev = NULL;
    }
    else {
        self->head = self->tail = NULL;
    }

    node->next = node->prev = NULL;
    return node;
}

/*
 * Prepend the given node to the list
 * and return the node, NULL on failure.
 */
rlist_node_t* rlist_lpush(rlist_t *self, void* nodeValue) {
    if (!nodeValue) return NULL;//不支持空节点

    rlist_node_t* node = self->malloc_node(sizeof(rlist_node_t));
    if (!node) {
        return NULL;
    }
    node->val = nodeValue;

    if (self->len) {
        node->next = self->head;
        node->prev = NULL;
        self->head->prev = node;
        self->head = node;
    } else {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;
    return node;
}

/*
 * Return the node associated to val or NULL.
 */
rlist_node_t* rlist_find(rlist_t *self, void *val) {
    rlist_iterator_t it = rlist_it(self, rlist_dir_tail);
    rlist_node_t *node;

    while ((node = rlist_next(&it))) {
        if (self->match) {
            if (self->match(val, node->val)) {
                return node;
            }
        }
        else {
            if (val == node->val) {
                return node;
            }
        }
    }

    return NULL;
}

/*
 * Return the node at the given index or NULL.
 */

rlist_node_t* rlist_at(rlist_t *self, int index) {
    rlist_direction_t direction = rlist_dir_tail;

    if (index < 0) {
        direction = rlist_dir_head;
        index = ~index;
    }

    if ((unsigned)index < self->len) {
        rlist_iterator_t it = rlist_it(self, direction);
        rlist_node_t *node = rlist_next(&it);
        while (index--) {
            node = rlist_next(&it);
        }

        return node;
    }

    return NULL;
}

/*
 * Remove the given node from the list, freeing it and it's value.
 */
void rlist_remove(rlist_t *self, rlist_node_t *node) {
    if (!node) return;

    node->prev ? (node->prev->next = node->next) : (self->head = node->next);

    node->next ? (node->next->prev = node->prev) : (self->tail = node->prev);

    if (node->val && self->free_node_val) {
        self->free_node_val(node->val);
    }

    if (self->free_node) {
        self->free_node(node);
    }

    --self->len;
}

void rlist_remove_alone(rlist_t *self, rlist_node_t *node) {
    if (node->val && self->free_node_val) self->free_node_val(node->val);

    if (self->free_node) self->free_node(node);
}

static inline rlist_iterator_t* rlist_iterator_new_from_node(rlist_t *list, rlist_direction_t direction, rlist_node_t *node) {
    rlist_iterator_t *self;
    if (!(self = list->malloc_it(sizeof(rlist_iterator_t)))) {
        return NULL;
    }
    self->next = node;
    self->direction = direction;

    return self;
}

rlist_iterator_t* rlist_iterator_new(rlist_t *list, rlist_direction_t direction) {
    rlist_node_t *node = direction == rlist_dir_tail ? list->head : list->tail;

    return rlist_iterator_new_from_node(list, direction, node);
}

rlist_node_t* rlist_next(rlist_iterator_t *self) {
    rlist_node_t *curr = self->next;
    if (curr) {
        self->next = self->direction == rlist_dir_tail ? curr->next : curr->prev;
    }

    return curr;
}


