/*
 * This file is part of the ESO Common Pipeline Library
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

#ifndef CPL_FRAMESET_H
#define CPL_FRAMESET_H

#include <stdio.h>

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_frame.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_frameset
 *
 * @brief
 *   The frame set data type.
 *
 * This data type is opaque.
 */

typedef struct _cpl_frameset_ cpl_frameset;


/*
 * Create, copy and destroy operations
 */

cpl_frameset *cpl_frameset_new(void) CPL_ATTR_ALLOC;
cpl_frameset *cpl_frameset_duplicate(const cpl_frameset *other) CPL_ATTR_ALLOC;
void cpl_frameset_delete(cpl_frameset *self);

/*
 * Non modifying operations
 */

cpl_size cpl_frameset_get_size(const cpl_frameset *self);
int cpl_frameset_is_empty(const cpl_frameset *self);
int cpl_frameset_count_tags(const cpl_frameset *self, const char *tag);

/*
 * Search operations
 */

const cpl_frame *cpl_frameset_find_const(const cpl_frameset *self,
                                         const char *tag);
cpl_frame *cpl_frameset_find(cpl_frameset *self, const char *tag);

/*
 * Sequential access
 */

const cpl_frame *cpl_frameset_get_first_const(const cpl_frameset *self) CPL_ATTR_DEPRECATED;
cpl_frame *cpl_frameset_get_first(cpl_frameset *self) CPL_ATTR_DEPRECATED;

const cpl_frame *cpl_frameset_get_next_const(const cpl_frameset *self) CPL_ATTR_DEPRECATED;
cpl_frame *cpl_frameset_get_next(cpl_frameset *self) CPL_ATTR_DEPRECATED;

/*
 * Inserting and removing elements
 */

cpl_error_code cpl_frameset_insert(cpl_frameset *self, cpl_frame *frame);
cpl_size cpl_frameset_erase(cpl_frameset *self, const char *tag);
cpl_error_code cpl_frameset_erase_frame(cpl_frameset *self, cpl_frame *frame);

/*
 * Element access
 */

const cpl_frame *cpl_frameset_get_frame_const(const cpl_frameset *self,
                                              cpl_size position) CPL_ATTR_DEPRECATED;
cpl_frame *cpl_frameset_get_frame(cpl_frameset *self, cpl_size position) CPL_ATTR_DEPRECATED;


cpl_frame *cpl_frameset_get_position(cpl_frameset *self, cpl_size position);

const cpl_frame *cpl_frameset_get_position_const(const cpl_frameset *self,
                                                 cpl_size position);

/*
 * Miscellaneous functions
 */

cpl_error_code cpl_frameset_join(cpl_frameset *self, const cpl_frameset *other);
cpl_error_code cpl_frameset_sort(cpl_frameset *self,
                                 cpl_frame_compare_func compare);

cpl_frameset *cpl_frameset_extract(const cpl_frameset *self,
                                   const cpl_size *labels,
                                   cpl_size desired_label) CPL_ATTR_ALLOC;

cpl_size *cpl_frameset_labelise(const cpl_frameset *self,
                                int (*compare)(const cpl_frame *,
                                const cpl_frame *), cpl_size *nb_labels);

void cpl_frameset_dump(const cpl_frameset *self, FILE *stream);


/*
 * Iterator support for frame sets
 */


/**
 * @ingroup cpl_frameset_iterator
 *
 * @brief
 *   The frame set iterator data type.
 *
 * This data type is opaque.
 */

typedef struct _cpl_frameset_iterator_ cpl_frameset_iterator;


/*
 * Iterator create, copy and destroy operations
 */

cpl_frameset_iterator *
cpl_frameset_iterator_new(const cpl_frameset *parent) CPL_ATTR_ALLOC;
cpl_frameset_iterator *
cpl_frameset_iterator_duplicate(const cpl_frameset_iterator *other) CPL_ATTR_ALLOC;

void cpl_frameset_iterator_delete(cpl_frameset_iterator *self);

/*
 * Iterator assignment
 */

cpl_error_code
cpl_frameset_iterator_assign(cpl_frameset_iterator *self,
                             const cpl_frameset_iterator *other);

/*
 * Iterator positioning
 */

cpl_error_code cpl_frameset_iterator_advance(cpl_frameset_iterator *self,
                                             int distance);
int cpl_frameset_iterator_distance(const cpl_frameset_iterator *self,
                                   const cpl_frameset_iterator *other);

void cpl_frameset_iterator_reset(cpl_frameset_iterator *self);

/*
 * Iterator data access
 */

cpl_frame *cpl_frameset_iterator_get(cpl_frameset_iterator *self);

const cpl_frame *
cpl_frameset_iterator_get_const(const cpl_frameset_iterator *self);


CPL_END_DECLS

#endif /* CPL_FRAMESET_H */
