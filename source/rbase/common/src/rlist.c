/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rlist.h"

rlist_t* rlist_create() {
    rlist_t* self = rnew_data(rlist_t);
    rassert(self != NULL, "");
    self->head = NULL;
    self->tail = NULL;
    self->len = 0;

    return self;
}

void rlist_clear(rlist_t *self) {
    if (!self || self->len <= 0) {
        return;
    }
    unsigned int len = self->len;
    rlist_node_t *next;
    rlist_node_t *curr = self->head;

    while (len--) {
        next = curr->next;

        rlist_free_node(self, curr);

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

        rlist_free_node(self, curr);

        curr = next;
    }

    rfree_data(rlist_t, self);
}

/*
 * Append the given node to the list
 * and return the node, NULL on failure.
 */
rlist_node_t* rlist_rpush(rlist_t *self, void* nodeValue) {
    if (!nodeValue) return NULL;//不支持空节点

    rlist_node_t* node = rnew_data(rlist_node_t);
    if (!node) {
        return NULL;
    }
    if (self->malloc_node_val) {
        node->val = self->malloc_node_val(nodeValue);
    }
    else {
        node->val = nodeValue;
    }

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

    rlist_node_t* node = rnew_data(rlist_node_t);
    if (!node) {
        return NULL;
    }

    if (self->malloc_node_val) {
        node->val = self->malloc_node_val(nodeValue);
    }
    else {
        node->val = nodeValue;
    }

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
        if (self->compare_node_val) {
            if (self->compare_node_val(val, node->val) == rcode_eq) {
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

    rlist_free_node(self, node);

    --self->len;
}

rlist_t* rlist_clone(rlist_t* src, rlist_direction_t dir) {
    if (src == NULL) {
        return NULL;
    }

    rlist_t* dest = rlist_create();
    dest->malloc_node_val = src->malloc_node_val;
    dest->free_node_val = src->free_node_val;
    dest->compare_node_val = src->compare_node_val;

    rlist_iterator_t it = rlist_it(src, dir);
    rlist_node_t *node = NULL;
    while ((node = rlist_next(&it))) {
        rlist_rpush(dest, node->val);
    }

    return dest;
}

int rlist_merge(rlist_t* dest, rlist_t* temp, rlist_direction_t dir) {
    int count = 0;
    if (dest == NULL || temp == NULL) {
        return count;
    }

    rlist_iterator_t it = rlist_it(temp, dir);
    rlist_node_t *node = NULL;
    while ((node = rlist_next(&it))) {
        rlist_rpush(dest, node->val);
        count++;
    }

    return count;
}

rlist_node_t* rlist_next(rlist_iterator_t *self) {
    rlist_node_t *curr = self->next;
    if (curr) {
        self->next = self->direction == rlist_dir_tail ? curr->next : curr->prev;
    }

    return curr;
}


