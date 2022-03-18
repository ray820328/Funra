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

#ifndef CX_MULTIMAP_H
#define CX_MULTIMAP_H

#include <cxmemory.h>
#include <cxtree.h>

CX_BEGIN_DECLS

/**
 * @ingroup cxmultimap
 *
 * @brief
 *   The multimap datatype.
 *
 * The internal representation of a mutimap is, as for ordinary maps too, a
 * balanced binary tree. For this reason cx_multimap is just an alias for
 * cx_tree.
 */

typedef cx_tree cx_multimap;

/**
 * @ingroup cxmultimap
 *
 * @brief
 *   The multimap iterator datatype.
 *
 * The multimap iterator is just an alias for the cx_tree_iterator datatype.
 */

typedef cx_tree_iterator cx_multimap_iterator;

/**
 * @ingroup cxmultimap
 *
 * @brief
 *   The multimap constant iterator datatype.
 *
 * The multimap constant iterator is just an alias for the
 * cx_tree_const_iterator datatype.
 */

typedef cx_tree_const_iterator cx_multimap_const_iterator;

/**
 * @ingroup cxmultimap
 *
 * @brief
 *   The multimap's key comparison operator function.
 *
 * This type of function is used internally by a multimap when key
 * comparisons are necessary. It must return @c TRUE if the comparison
 * of its first argument with the second argument succeeds, and @c FALSE
 * otherwise. It is actually an alias for cx_tree_compare_func.
 *
 * @see cx_tree_compare_func
 */

typedef cx_tree_compare_func cx_multimap_compare_func;

/*
 * Create, copy and destroy operations
 */


cx_multimap *cx_multimap_new(cx_multimap_compare_func, cx_free_func,
                             cx_free_func);
void cx_multimap_delete(cx_multimap *);

/*
 * Nonmodifying operations
 */

cxsize cx_multimap_size(const cx_multimap *);
cxbool cx_multimap_empty(const cx_multimap *);
cxsize cx_multimap_max_size(const cx_multimap *);
cx_multimap_compare_func cx_multimap_key_comp(const cx_multimap *);

/*
 * Special search operations
 */

cxsize cx_multimap_count(const cx_multimap *, cxcptr);
cx_multimap_iterator cx_multimap_find(const cx_multimap *, cxcptr);
cx_multimap_iterator cx_multimap_lower_bound(const cx_multimap *, cxcptr);
cx_multimap_iterator cx_multimap_upper_bound(const cx_multimap *, cxcptr);
void cx_multimap_equal_range(const cx_multimap *, cxcptr,
                             cx_multimap_iterator *, cx_multimap_iterator *);

/*
 * Assignment operations
 */

void cx_multimap_swap(cx_multimap *, cx_multimap *);
cxptr cx_multimap_assign(cx_multimap *, cx_multimap_iterator, cxcptr);

/*
 * Element access
 */

cxptr cx_multimap_get_key(const cx_multimap *, cx_multimap_const_iterator);
cxptr cx_multimap_get_value(const cx_multimap *, cx_multimap_const_iterator);

/*
 * Iterator functions
 */

cx_multimap_iterator cx_multimap_begin(const cx_multimap *);
cx_multimap_iterator cx_multimap_end(const cx_multimap *);
cx_multimap_iterator cx_multimap_next(const cx_multimap *,
                                      cx_multimap_const_iterator);
cx_multimap_iterator cx_multimap_previous(const cx_multimap *,
                                          cx_multimap_const_iterator);


/*
 * Inserting and removing elements
 */

cx_multimap_iterator cx_multimap_insert(cx_multimap *, cxcptr, cxcptr);
void cx_multimap_erase_position(cx_multimap *, cx_multimap_iterator);
void cx_multimap_erase_range(cx_multimap *, cx_multimap_iterator,
                             cx_multimap_iterator);
cxsize cx_multimap_erase(cx_multimap *, cxcptr);
void cx_multimap_clear(cx_multimap *);

CX_END_DECLS

#endif /* CX_MULTIMAP_H */
