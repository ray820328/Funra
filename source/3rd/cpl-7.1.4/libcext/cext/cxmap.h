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

#ifndef CX_MAP_H
#define CX_MAP_H

#include <cxmemory.h>
#include <cxtree.h>

CX_BEGIN_DECLS

/**
 * @ingroup cxmap
 *
 * @brief
 *   The map datatype.
 *
 * The internal representation of a map is a balanced binary tree. For
 * this reason cx_map is just an alias for cx_tree.
 */

typedef cx_tree cx_map;

/**
 * @ingroup cxmap
 *
 * @brief
 *   The map iterator datatype.
 *
 * The map iterator is just an alias for the cx_tree_iterator datatype.
 */

typedef cx_tree_iterator cx_map_iterator;

/**
 * @ingroup cxmap
 *
 * @brief
 *   The map constant iterator datatype.
 *
 * The map constant iterator is just an alias for the cx_tree_const_iterator
 * datatype.
 */

typedef cx_tree_const_iterator cx_map_const_iterator;

/**
 * @ingroup cxmap
 *
 * @brief
 *   The map's key comparison operator function.
 *
 * This type of function is used internally by a map when key comparisons
 * are necessary. It must return @c TRUE if the comparison of its first
 * argument with the second argument succeeds, and @c FALSE otherwise.
 * It is actually an alias for cx_tree_compare_func.
 *
 * @see cx_tree_compare_func
 */

typedef cx_tree_compare_func cx_map_compare_func;

/*
 * Create, copy and destroy operations
 */


cx_map *cx_map_new(cx_compare_func, cx_free_func, cx_free_func);
void cx_map_delete(cx_map *);

/*
 * Nonmodifying operations
 */

cxsize cx_map_size(const cx_map *);
cxbool cx_map_empty(const cx_map *);
cxsize cx_map_max_size(const cx_map *);
cx_map_compare_func cx_map_key_comp(const cx_map *);

/*
 * Special search operations
 */

cxsize cx_map_count(const cx_map *, cxcptr);
cx_map_iterator cx_map_find(const cx_map *, cxcptr);
cx_map_iterator cx_map_lower_bound(const cx_map *, cxcptr);
cx_map_iterator cx_map_upper_bound(const cx_map *, cxcptr);
void cx_map_equal_range(const cx_map *, cxcptr, cx_map_iterator *,
                        cx_map_iterator *);

/*
 * Assignment operations
 */

void cx_map_swap(cx_map *, cx_map *);
cxptr cx_map_assign(cx_map *, cx_map_iterator, cxcptr);
cxptr cx_map_put(cx_map *, cxcptr, cxcptr);

/*
 * Element access
 */

cxptr cx_map_get_key(const cx_map *, cx_map_const_iterator);
cxptr cx_map_get_value(const cx_map *, cx_map_const_iterator);
cxptr cx_map_get(cx_map *, cxcptr);

/*
 * Iterator functions
 */

cx_map_iterator cx_map_begin(const cx_map *);
cx_map_iterator cx_map_end(const cx_map *);
cx_map_iterator cx_map_next(const cx_map *, cx_map_const_iterator);
cx_map_iterator cx_map_previous(const cx_map *, cx_map_const_iterator);


/*
 * Inserting and removing elements
 */

cx_map_iterator cx_map_insert(cx_map *, cxcptr, cxcptr);
void cx_map_erase_position(cx_map *, cx_map_iterator);
void cx_map_erase_range(cx_map *, cx_map_iterator, cx_map_iterator);
cxsize cx_map_erase(cx_map *, cxcptr);
void cx_map_clear(cx_map *);

CX_END_DECLS

#endif /* CX_MAP_H */
