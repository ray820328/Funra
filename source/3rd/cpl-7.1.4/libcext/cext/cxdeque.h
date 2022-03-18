/*
 * This file is part of the ESO C Extension Library
 * Copyright (C) 2001-2017 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef CX_DEQUE_H
#define CX_DEQUE_H

#include <cxtypes.h>
#include <cxmemory.h>

CX_BEGIN_DECLS

typedef struct _cx_deque_ cx_deque;

typedef cxsize cx_deque_const_iterator;
typedef cxsize cx_deque_iterator;


/*
 * Create, copy and destroy operations
 */

cx_deque *cx_deque_new(void);
void cx_deque_delete(cx_deque* deque);
void cx_deque_destroy(cx_deque *deque, cx_free_func deallocate);

/*
 * Non-modifying operations
 */

cxsize cx_deque_size(const cx_deque *deque);
cxbool cx_deque_empty(const cx_deque *deque);
cxsize cx_deque_max_size(const cx_deque *deque);

/*
 * Assignment operations
 */

void cx_deque_swap(cx_deque *deque, cx_deque *other);
cxptr cx_deque_assign(cx_deque *deque, cx_deque_iterator position, cxptr data);

/*
 * Element access
 */

cxptr cx_deque_front(const cx_deque *deque);
cxptr cx_deque_back(const cx_deque *deque);
cxptr cx_deque_get(const cx_deque *deque, cx_deque_const_iterator position);

/*
 * Iterator functions
 */

cx_deque_iterator cx_deque_begin(const cx_deque *deque);
cx_deque_iterator cx_deque_end(const cx_deque *deque);
cx_deque_iterator cx_deque_next(const cx_deque *deque,
                                cx_deque_const_iterator position);
cx_deque_iterator cx_deque_previous(const cx_deque *deque,
                                    cx_deque_const_iterator position);

/*
 * Inserting and removing elements
 */

void cx_deque_push_front(cx_deque *deque, cxcptr data);
cxptr cx_deque_pop_front(cx_deque *deque);
void cx_deque_push_back(cx_deque *deque, cxcptr data);
cxptr cx_deque_pop_back(cx_deque *deque);

cx_deque_iterator cx_deque_insert(cx_deque *deque, cx_deque_iterator position,
                                  cxcptr data);
cx_deque_iterator cx_deque_erase(cx_deque *deque, cx_deque_iterator position,
                                 cx_free_func deallocate);
void cx_deque_clear(cx_deque *deque);


/*
 * Extra deque interfaces
 */

/*
 * Removing elements
 */

cxptr cx_deque_extract(cx_deque *deque, cx_deque_iterator position);
void cx_deque_remove(cx_deque *deque, cxcptr data);

/*
 * Splice functions
 */

void cx_deque_unique(cx_deque *deque, cx_compare_func compare);
void cx_deque_splice(cx_deque *deque, cx_deque_iterator position,
                     cx_deque *other, cx_deque_iterator start,
                     cx_deque_iterator end);
void cx_deque_merge(cx_deque *deque, cx_deque *other, cx_compare_func compare);
void cx_deque_sort(cx_deque *deque, cx_compare_func compare);
void cx_deque_reverse(cx_deque *deque);

CX_END_DECLS

#endif /* CX_DEQUE_H */
