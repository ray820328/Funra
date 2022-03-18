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

#ifndef CPL_FRAMEDATA_H
#define CPL_FRAMEDATA_H

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_error.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_framedata
 *
 * @brief
 *  The frame data object type.
 */

typedef struct _cpl_framedata_ cpl_framedata;


/**
 * @ingroup cpl_framedata
 *
 * @brief
 *  The public frame data object.
 *
 * The frame data object stores a frame identifier, the frame tag, and
 * the minimum and maximum number of frames needed.
 *
 * The data members of this structure are public to allow for a static
 * initialization. Any other access of the public data members should
 * still be done using the member functions.
 */

struct _cpl_framedata_ {

    /**
     * The frame tag. A unique string identifier for a particular kind of
     * frame.
     */

    const char* tag;

    /**
     * The minimum number of frames of the kind given by the @em tag,
     * the recipe requires in input. A value of @c -1 means that the
     * minimum number of frames is unspecified.
     */

    cpl_size min_count;

    /**
     * The maximum number of frames of the kind given by the @em tag,
     * the recipe requires in input. A value of @c -1 means that the
     * maximum number of frames is unspecified.
     */

    cpl_size max_count;

};


/*
 * Create, copy and destroy operations
 */

cpl_framedata* cpl_framedata_new(void) CPL_ATTR_ALLOC;
cpl_framedata* cpl_framedata_create(const char* tag, cpl_size min_count,
                                    cpl_size max_count) CPL_ATTR_ALLOC;
cpl_framedata* cpl_framedata_duplicate(const cpl_framedata* other)
    CPL_ATTR_ALLOC;
void cpl_framedata_clear(cpl_framedata* self);
void cpl_framedata_delete(cpl_framedata* self);

/*
 * Non modifying operations
 */

const char* cpl_framedata_get_tag(const cpl_framedata* self);

cpl_size cpl_framedata_get_min_count(const cpl_framedata* self);
cpl_size cpl_framedata_get_max_count(const cpl_framedata* self);


/*
 * Assignment operations
 */

cpl_error_code cpl_framedata_set_tag(cpl_framedata* self, const char* tag);

cpl_error_code cpl_framedata_set_min_count(cpl_framedata* self,
                                           cpl_size min_count);
cpl_error_code cpl_framedata_set_max_count(cpl_framedata* self,
                                           cpl_size max_count);

cpl_error_code cpl_framedata_set(cpl_framedata* self, const char* tag,
                                 cpl_size min_count, cpl_size max_count);

CPL_END_DECLS

#endif /* CPL_FRAMEDATA_H */
