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

#ifndef CX_TREE_H
#define CX_TREE_H

#include <cxmemory.h>

CX_BEGIN_DECLS

typedef struct _cx_tnode_ *cx_tree_iterator;
typedef const struct _cx_tnode_ *cx_tree_const_iterator;

typedef struct _cx_tree_ cx_tree;

/**
 * @ingroup cxtree
 *
 * @brief
 *   The tree's key comparison operator function.
 *
 * This type of function is used by a tree internally to compare the
 * keys of its elements. A key comparison operator returns @c TRUE
 * if the comparison of its first argument with the second argument
 * succeeds, and @c FALSE otherwise, as, for instance, the logical
 * operators <, >, ==, and != do.
 *
 * Examples:
 * - A less than operator for integer values
 *   @code
 *     #include <cxtree.h>
 *
 *     cxbool less_int(cxcptr i1, cxcptr i2)
 *     {
 *         return *i1 < *i2;
 *     }
 *   @endcode
 *
 * - A less than and an equal operator for strings
 *   @code
 *     #include <string.h>
 *     #include <cxtree.h>
 *
 *     cxbool less_string(cxcptr s1, cxcptr s2)
 *     {
 *         return strcmp(s1, s2) < 0;
 *     }
 *
 *     cxbool equal_string(cxptr s1, cxptr s2)
 *     {
 *         return strcmp(s1, s2) == 0;
 *     }
 *   @endcode
 */

typedef cxbool (*cx_tree_compare_func)(cxcptr, cxcptr);

/*
 * Create, copy and destroy operations
 */

cx_tree *cx_tree_new(cx_tree_compare_func, cx_free_func, cx_free_func);
void cx_tree_delete(cx_tree *);

/*
 * Nonmodifying operations
 */

cxsize cx_tree_size(const cx_tree *);
cxbool cx_tree_empty(const cx_tree *);
cxsize cx_tree_max_size(const cx_tree *);
cx_tree_compare_func cx_tree_key_comp(const cx_tree *);

/*
 * Special search operations
 */

cxsize cx_tree_count(const cx_tree *, cxcptr);
cx_tree_iterator cx_tree_find(const cx_tree *, cxcptr);
cx_tree_iterator cx_tree_lower_bound(const cx_tree *, cxcptr);
cx_tree_iterator cx_tree_upper_bound(const cx_tree *, cxcptr);
void cx_tree_equal_range(const cx_tree *, cxcptr, cx_tree_iterator *,
                         cx_tree_iterator *);

/*
 * Assignment operations
 */

void cx_tree_swap(cx_tree *, cx_tree *);
cxptr cx_tree_assign(cx_tree *, cx_tree_iterator, cxcptr);

/*
 * Element access
 */

cxptr cx_tree_get_key(const cx_tree *, cx_tree_const_iterator);
cxptr cx_tree_get_value(const cx_tree *, cx_tree_const_iterator);

/*
 * Iterator functions
 */

cx_tree_iterator cx_tree_begin(const cx_tree *);
cx_tree_iterator cx_tree_end(const cx_tree *);
cx_tree_iterator cx_tree_next(const cx_tree *, cx_tree_const_iterator);
cx_tree_iterator cx_tree_previous(const cx_tree *, cx_tree_const_iterator);


/*
 * Inserting and removing elements
 */

cx_tree_iterator cx_tree_insert_unique(cx_tree *, cxcptr, cxcptr);
cx_tree_iterator cx_tree_insert_equal(cx_tree *, cxcptr, cxcptr);
void cx_tree_erase_position(cx_tree *, cx_tree_iterator);
void cx_tree_erase_range(cx_tree *, cx_tree_iterator, cx_tree_iterator);
cxsize cx_tree_erase(cx_tree *, cxcptr);
void cx_tree_clear(cx_tree *);

/*
 * Debugging
 */

cxbool cx_tree_verify(const cx_tree *);

CX_END_DECLS

#endif /* CX_TREE_H */
