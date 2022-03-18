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

#ifndef CPL_MULTIFRAME_H
#define CPL_MULTIFRAME_H

#include <cxmemory.h>

#include <cpl_error.h>
#include <cpl_frame.h>



CPL_BEGIN_DECLS

/**
 * @ingroup cpl_regex
 *
 * Regular expressions syntax options
 */

enum _cpl_regex_syntax_option_
{

    /**
     * Case insensitive searches.
     * @hideinitializer
     */

    CPL_REGEX_ICASE    = 1 << 0,

    /**
     * No sub-expressions.
     * @hideinitializer
     */

    CPL_REGEX_NOSUBS   = 1 << 1,

    /**
     * Basic POSIX grammer.
     * @hideinitializer
     */

    CPL_REGEX_BASIC    = 1 << 2,

    /**
     * Extended POSIX grammer.
     * @hideinitializer
     */

    CPL_REGEX_EXTENDED = 1 << 3

};


/**
 * @ingroup cpl_regex
 *
 * @brief
 *  Regular expression syntax options
 *
 */

typedef enum _cpl_regex_syntax_option_  cpl_regex_syntax_option;


/**
 * @ingroup cpl_regex
 *
 * @brief
 *  The opaque regular expression filter data type.
 */

typedef struct _cpl_regex_ cpl_regex;


cpl_regex *cpl_regex_new(const char *expression, int negated,
                                       cpl_regex_syntax_option flags);
void cpl_regex_delete(cpl_regex *self);

int cpl_regex_apply(const cpl_regex *self, const char *string);
int cpl_regex_is_negated(const cpl_regex *self);

void cpl_regex_negate(cpl_regex *self);



/**
 * @ingroup cpl_multiframe
 *
 * @brief
 *  The flags indicating the naming scheme to use for multi-frame datasets.
 */

enum _cpl_multiframe_id_mode_
{

    /**
     * Use the given identifier as dataset name
     * @hideinitializer
     */

    CPL_MULTIFRAME_ID_SET    = 1 << 0,

    /**
     * Create the dataset name from the given identifier by appending the name of the source
     * dataset.
     * @hideinitializer
     */

    CPL_MULTIFRAME_ID_PREFIX = 1 << 1,

    /**
     * Create the dataset name by concatenating the names of the involved source datasets.
     * The given identifier is used as separator.
     * @hideinitializer
     */

    CPL_MULTIFRAME_ID_JOIN   = 1 << 2

};


/**
 * @ingroup cpl_multiframe
 *
 * @brief
 *  The flags indicating the naming scheme to use for multi-frame datasets.
 */

typedef enum _cpl_multiframe_id_mode_ cpl_multiframe_id_mode;


/**
 * @ingroup cpl_multiframe
 *
 * @brief
 *  The opaque multi-frame data type.
 */

typedef struct _cpl_multiframe_ cpl_multiframe;


cpl_multiframe *cpl_multiframe_new(const cpl_frame *head, const char *id,
                                   cpl_regex *filter);
void cpl_multiframe_delete(cpl_multiframe *self);

cpl_size cpl_multiframe_get_size(const cpl_multiframe *self);

cpl_error_code cpl_multiframe_append_dataset(cpl_multiframe *self,
                                             const char *id,
                                             const cpl_frame *frame,
                                             const char *name,
                                             const cpl_regex *filter1,
                                             const cpl_regex *filter2,
                                             unsigned int flags);

cpl_error_code cpl_multiframe_append_dataset_from_position(cpl_multiframe *self,
                                                           const char *id,
                                                           const cpl_frame *frame,
                                                           cpl_size position,
                                                           const cpl_regex *filter1,
                                                           const cpl_regex *filter2,
                                                           unsigned int flags);

cpl_error_code cpl_multiframe_append_datagroup(cpl_multiframe *self, const char *id,
                                               const cpl_frame *frame,
                                               cpl_size nsets, const char **names,
                                               const cpl_regex **filter1,
                                               const cpl_regex **filter2,
                                               const char **properties,
                                               unsigned int flags);

cpl_error_code cpl_multiframe_append_datagroup_from_position(cpl_multiframe *self, const char *id,
                                                             const cpl_frame *frame,
                                                             cpl_size nsets, cpl_size *positions,
                                                             const cpl_regex **filter1,
                                                             const cpl_regex **filter2,
                                                             const char **properties,
                                                             unsigned int flags);

cpl_error_code cpl_multiframe_add_empty(cpl_multiframe *self, const char *id);

cpl_error_code cpl_multiframe_write(cpl_multiframe *self,
                                    const char *filename);

CPL_END_DECLS

#endif /* CPL_MULTIFRAME_H */
