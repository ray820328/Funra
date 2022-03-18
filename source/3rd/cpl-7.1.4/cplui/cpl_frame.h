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

#ifndef CPL_FRAME_H
#define CPL_FRAME_H

#include <stdio.h>

#include <cpl_type.h>
#include <cpl_error.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_frame
 *
 * @brief
 *   The frame data type.
 */

typedef struct _cpl_frame_ cpl_frame;

/**
 * @ingroup cpl_frame
 *
 * @brief
 *   Frame comparison function.
 *
 * Type definition for functions which compares the frame @em other with the
 * frame @em self.
 *
 * All function of this type must return -1, 0, or 1 if @em self is considered
 * to be less than, equal or greater than @em other respectively.
 */

typedef int (*cpl_frame_compare_func)(const cpl_frame *self,
                                      const cpl_frame *other);


/**
 * @ingroup cpl_frame
 *
 * @brief
 *   Supported frame types
 *
 * Defines the possible values for the frame's type attribute.
 */

enum _cpl_frame_type_ {

    /**
     * Undefined frame type
     * @hideinitializer
     */
    CPL_FRAME_TYPE_NONE   = 1 << 0,

    /**
     * Image frame type identifier
     * @hideinitializer
     */

    CPL_FRAME_TYPE_IMAGE  = 1 << 1,

    /**
     * Matrix frame type identifier
     * @hideinitializer
     */

    CPL_FRAME_TYPE_MATRIX = 1 << 2,

    /**
     * Table frame type identifier
     * @hideinitializer
     */

    CPL_FRAME_TYPE_TABLE  = 1 << 3,

    /**
     * paf frame type identifier
     * @hideinitializer
     */

    CPL_FRAME_TYPE_PAF  = 1 << 4,

    /**
     * identifier for any other type
     * @hideinitializer
     */

    CPL_FRAME_TYPE_ANY  = 1 << 5
};


/**
 * @ingroup cpl_frame
 *
 * @brief
 *   The frame type data type.
 */

typedef enum _cpl_frame_type_ cpl_frame_type;


/**
 * @ingroup cpl_frame
 *
 * @brief
 *   Supported frame groups.
 *
 * Defines the possible values for the frame's group attribute.
 */

enum _cpl_frame_group_ {

    /**
     * The frame does not belong to any supported group.
     */

    CPL_FRAME_GROUP_NONE,

    /**
     * The frame is associated to unprocessed data.
     */

    CPL_FRAME_GROUP_RAW,

    /**
     * The frame is associated to calibration data.
     */

    CPL_FRAME_GROUP_CALIB,

    /**
     * The frame is associated to processed data.
     */

    CPL_FRAME_GROUP_PRODUCT
};


/**
 * @ingroup cpl_frame
 *
 * @brief
 *   The frame group data type.
 */

typedef enum _cpl_frame_group_ cpl_frame_group;


/**
 * @ingroup cpl_frame
 *
 * @brief
 *   Supported frame processing levels.
 *
 * @note
 *   The processing levels are just flags and it is left to the application
 *   to trigger the appropriate action for the different levels.
 */

enum _cpl_frame_level_ {

    /**
     * Undefined processing level
     */

    CPL_FRAME_LEVEL_NONE,

    /**
     * Temporary product. The corresponding file will be deleted when
     * the processing chain is completed.
     */

    CPL_FRAME_LEVEL_TEMPORARY,

    /**
     * Intermediate product. The corresponding file is only kept on request.
     * The default is to delete these products at the end of the processing
     * chain.
     */

    CPL_FRAME_LEVEL_INTERMEDIATE,

    /**
     * Final data product, which is always written to a file at the end of
     * the processing chain.
     */

    CPL_FRAME_LEVEL_FINAL

};


/**
 * @ingroup cpl_frame
 *
 * @brief
 *   The frame level data type.
 */

typedef enum _cpl_frame_level_ cpl_frame_level;


/*
 * Identifier strings for the different frame groups.
 */

/**
 * @ingroup cpl_frame
 *
 * @brief
 *   Frame group tag for unprocessed data.
 */

#define CPL_FRAME_GROUP_RAW_ID      "RAW"

/**
 * @ingroup cpl_frame
 *
 * @brief
 *   Frame group tag for calibration data.
 */

#define CPL_FRAME_GROUP_CALIB_ID    "CALIB"

/**
 * @ingroup cpl_frame
 *
 * @brief
 *   Frame group tag for processed data.
 */

#define CPL_FRAME_GROUP_PRODUCT_ID  "PRODUCT"


/*
 * Create, copy and destroy operations
 */

cpl_frame *cpl_frame_new(void) CPL_ATTR_ALLOC;
cpl_frame *cpl_frame_duplicate(const cpl_frame *other) CPL_ATTR_ALLOC;
void cpl_frame_delete(cpl_frame *self);

/*
 * Modifying and non-modifying operations
 */

const char *cpl_frame_get_filename(const cpl_frame *self);
const char *cpl_frame_get_tag(const cpl_frame *self);
cpl_frame_type cpl_frame_get_type(const cpl_frame *self);
cpl_frame_group cpl_frame_get_group(const cpl_frame *self);
cpl_frame_level cpl_frame_get_level(const cpl_frame *self);

/*
 * Assignment operations
 */

cpl_error_code cpl_frame_set_filename(cpl_frame *self, const char *filename);
cpl_error_code cpl_frame_set_tag(cpl_frame *self, const char *tag);
cpl_error_code cpl_frame_set_type(cpl_frame *self, cpl_frame_type type);
cpl_error_code cpl_frame_set_group(cpl_frame *self, cpl_frame_group group);
cpl_error_code cpl_frame_set_level(cpl_frame *self, cpl_frame_level level);

/*
 * Others
 */

cpl_size cpl_frame_get_nextensions(const cpl_frame *self);

void cpl_frame_dump(const cpl_frame *frame, FILE *stream);


CPL_END_DECLS

#endif /* CPL_FRAME_H */
