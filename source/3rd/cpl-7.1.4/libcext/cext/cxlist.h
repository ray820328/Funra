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

#ifndef CX_LIST_H
#define CX_LIST_H

#include <cxmemory.h>

CX_BEGIN_DECLS

typedef struct _cx_lnode_ *cx_list_iterator;
typedef const struct _cx_lnode_ *cx_list_const_iterator;

typedef struct _cx_list_ cx_list;


/*
 * Create, copy and destroy operations
 */

cx_list *cx_list_new(void);
void cx_list_delete(cx_list *);
void cx_list_destroy(cx_list *, cx_free_func);

/*
 * Non-modifying operations
 */

cxsize cx_list_size(const cx_list *);
cxbool cx_list_empty(const cx_list *);
cxsize cx_list_max_size(const cx_list *);

/*
 * Assignment operations
 */

void cx_list_swap(cx_list *, cx_list *);
cxptr cx_list_assign(cx_list *, cx_list_iterator, cxcptr);

/*
 * Element access
 */

cxptr cx_list_front(const cx_list *);
cxptr cx_list_back(const cx_list *);
cxptr cx_list_get(const cx_list *, cx_list_const_iterator);

/*
 * Iterator functions
 */

cx_list_iterator cx_list_begin(const cx_list *);
cx_list_iterator cx_list_end(const cx_list *);
cx_list_iterator cx_list_next(const cx_list *, cx_list_const_iterator);
cx_list_iterator cx_list_previous(const cx_list *, cx_list_const_iterator);

/*
 * Inserting and removing elements
 */

void cx_list_push_front(cx_list *, cxcptr);
cxptr cx_list_pop_front(cx_list *);
void cx_list_push_back(cx_list *, cxcptr);
cxptr cx_list_pop_back(cx_list *);

cx_list_iterator cx_list_insert(cx_list *, cx_list_iterator, cxcptr);
cx_list_iterator cx_list_erase(cx_list *, cx_list_iterator, cx_free_func);
cxptr cx_list_extract(cx_list *, cx_list_iterator);
void cx_list_remove(cx_list *, cxcptr);
void cx_list_clear(cx_list *);

/*
 * Splice functions
 */

void cx_list_unique(cx_list *, cx_compare_func);
void cx_list_splice(cx_list *, cx_list_iterator, cx_list *,
                    cx_list_iterator, cx_list_iterator);
void cx_list_merge(cx_list *, cx_list *, cx_compare_func);
void cx_list_sort(cx_list *, cx_compare_func);
void cx_list_reverse(cx_list *);

CX_END_DECLS

#endif /* CX_LIST_H */
