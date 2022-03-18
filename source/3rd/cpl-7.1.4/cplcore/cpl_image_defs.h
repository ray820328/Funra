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

#ifndef CPL_IMAGE_DEFS_H
#define CPL_IMAGE_DEFS_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image.h"
#include "cpl_type.h"
#include "cpl_mask.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

#define CPL_CLASS_NONE           0
#define CPL_CLASS_DOUBLE         1
#define CPL_CLASS_FLOAT          2
#define CPL_CLASS_INT            3
#define CPL_CLASS_DOUBLE_COMPLEX 4
#define CPL_CLASS_FLOAT_COMPLEX  5

/*-----------------------------------------------------------------------------
                                New types
 -----------------------------------------------------------------------------*/

struct _cpl_image_ {
    /* Size of the image in x and y */
    cpl_size            nx, ny;
    /* Type of the pixels used for the cpl_image */
    cpl_type            type;
    /* Pointer to pixel buffer as a 1d buffer */
    void            *   pixels;
    /* Bad Pixels mask */
    cpl_mask        *   bpm;
};

CPL_END_DECLS

#endif

