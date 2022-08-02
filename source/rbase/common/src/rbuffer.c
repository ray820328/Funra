/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"
#include "rlog.h"
#include "rbuffer.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

rbuffer_t* rbuffer_create(rbuffer_size_t init_capacity) {
    rbuffer_t* d = rnew_data(rbuffer_t);
    d->data = rstr_new(init_capacity);
    d->capacity = init_capacity;
    d->offset = 0;
    d->pos = 0;

    return d;
}

int rbuffer_read(rbuffer_t* d, rbuffer_size_t size) {

    return rcode_ok;
}

int rbuffer_write(rbuffer_t* d, char* val, rbuffer_size_t size) {

    return rcode_ok;
}

int rbuffer_rewind(rbuffer_t* d, bool force) {
    int move_size = 0;
    int size = rbuffer_size(d);
    int block = 0;

    if (rbuffer_start(d) > 0) {
        block = rbuffer_start(d);

        while (move_size < size) {
            if (move_size + block > size) {
                block = size - move_size;
            }

            memcpy(d->data + move_size, d->data + d->offset + move_size, block);
            move_size += block;
        }

        d->offset = 0;
        d->pos = size;
    }

    return move_size;
}

int rbuffer_skip(rbuffer_t* d, rbuffer_size_t size) {
    d->offset += size;
    if (d->offset > d->pos) {
        d->offset = d->pos;
    }
    return rcode_ok;
}

int rbuffer_revert(rbuffer_t* d) {
    d->offset = d->pos;
    return rcode_ok;
}

void rbuffer_clear(rbuffer_t* d) {
    d->offset = 0;
    d->pos = 0;
}

void rbuffer_release(rbuffer_t* d) {
    rfree_data(rbuffer_t, d);
}

char* rbuffer_output(rbuffer_t* d) {

    return NULL;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__