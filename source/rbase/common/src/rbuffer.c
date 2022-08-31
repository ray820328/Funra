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
    rbuffer_t* d = rdata_new(rbuffer_t);
    d->data = rdata_new_buffer(init_capacity);
    d->capacity = init_capacity;
    d->offset = d->pos = 0;

    return d;
}

/** 上层确保val不会溢出，返回read有效字节数 **/
int rbuffer_read(rbuffer_t* d, char* val, rbuffer_size_t read_size) {
    int size = 0;
    if (rstr_is_empty(val)) {
        return 0;
    }

    size = rbuffer_size(d);
    read_size = read_size > size ? size : read_size;
    memcpy(val, d->data + d->offset, read_size);
    d->offset += read_size;

    if (rbuffer_empty(d)) {
        d->offset = d->pos = 0;
    }

    return read_size;
}

/** 上层确保val不会溢出，返回write有效字节数，size == 0 默认val字符串长度，为0提前截断风险 **/
int rbuffer_write(rbuffer_t* d, const char* val, rbuffer_size_t size) {
    int free_size = 0;
    if (val == NULL) {
        return 0;
    }

    if (size <= 0) {
        size = rstr_len(val);
    }

    free_size = rbuffer_left(d);
    if (free_size < size) {
        rbuffer_rewind(d);
        free_size = rbuffer_left(d);
    }

    if (likely(free_size >= size)) {
        memcpy(d->data + d->pos, val, size);
        d->pos += size;
        return size;
    }
    else {
        memcpy(d->data + d->pos, val, free_size);
        d->pos += free_size;
        return free_size;
    }
}

int rbuffer_rewind(rbuffer_t* d) {
    int move_size = 0;
    int size = rbuffer_size(d);
    int block = 0;

    if (rbuffer_read_start_pos(d) > 0) {
        block = rbuffer_read_start_pos(d);

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

int rbuffer_seek(rbuffer_t* d, rbuffer_size_t pos) {
    if (likely(pos >= d->offset && pos < d->capacity)) {
        d->pos = pos;
    }else if (pos < d->offset) {
        d->pos = d->offset;
    }
    else {
        d->pos = d->capacity - 1;
    }

    return rcode_ok;
}

int rbuffer_skip(rbuffer_t* d, rbuffer_size_t size) {
    d->offset += size;
    if (d->offset > d->pos) {
        d->offset = d->pos;
    }
    if (d->offset < 0) {
        d->offset = 0;
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
    if (d != NULL) {
        rdata_free(rbuffer_t, d);
    }
}

int rbuffer_output(rbuffer_t* d, char* dest, rbuffer_size_t dest_size) {

    return dest_size;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__