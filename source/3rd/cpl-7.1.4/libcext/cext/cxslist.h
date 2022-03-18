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

#ifndef CX_SLIST_H
#define CX_SLIST_H

#include <cxmemory.h>

CX_BEGIN_DECLS

typedef struct _cx_slnode_ *cx_slist_iterator;
typedef const struct _cx_slnode_ *cx_slist_const_iterator;

typedef struct _cx_slist_ cx_slist;


/*
 * Create, copy and destroy operations
 */

cx_slist *cx_slist_new(void);
void cx_slist_delete(cx_slist *);
void cx_slist_destroy(cx_slist *, cx_free_func);

/*
 * Nonmodifying operations
 */

cxsize cx_slist_size(const cx_slist *);
cxbool cx_slist_empty(const cx_slist *);
cxsize cx_slist_max_size(const cx_slist *);

/*
 * Assignment operations
 */

void cx_slist_swap(cx_slist *, cx_slist *);
cxptr cx_slist_assign(cx_slist *, cx_slist_iterator, cxcptr);

/*
 * Element access
 */

cxptr cx_slist_front(const cx_slist *);
cxptr cx_slist_back(const cx_slist *);
cxptr cx_slist_get(const cx_slist *, cx_slist_const_iterator);

/*
 * Iterator functions
 */

cx_slist_iterator cx_slist_begin(const cx_slist *);
cx_slist_iterator cx_slist_end(const cx_slist *);
cx_slist_iterator cx_slist_next(const cx_slist *, cx_slist_const_iterator);

/*
 * Inserting and removing elements
 */

void cx_slist_push_front(cx_slist *, cxcptr);
cxptr cx_slist_pop_front(cx_slist *);
void cx_slist_push_back(cx_slist *, cxcptr);
cxptr cx_slist_pop_back(cx_slist *);

cx_slist_iterator cx_slist_insert(cx_slist *, cx_slist_iterator, cxcptr);
cx_slist_iterator cx_slist_erase(cx_slist *, cx_slist_iterator, cx_free_func);
cxptr cx_slist_extract(cx_slist *, cx_slist_iterator);
void cx_slist_remove(cx_slist *, cxcptr);
void cx_slist_clear(cx_slist *);

/*
 * Splice functions
 */

void cx_slist_unique(cx_slist *, cx_compare_func);
void cx_slist_splice(cx_slist *, cx_slist_iterator, cx_slist *,
                     cx_slist_iterator, cx_slist_iterator);
void cx_slist_merge(cx_slist *, cx_slist *, cx_compare_func);
void cx_slist_sort(cx_slist *, cx_compare_func);
void cx_slist_reverse(cx_slist *);

CX_END_DECLS

#endif /* CX_SLIST_H */
