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

#ifndef CPL_IMAGELIST_H
#define CPL_IMAGELIST_H

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef struct _cpl_imagelist_ cpl_imagelist;

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_imagelist_io.h"
#include "cpl_imagelist_basic.h"

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_imagelist Imagelists
 *
 * This module provides functions to create, use, and destroy a
 *   @em cpl_imagelist.
 * A @em cpl_imagelist is an ordered list of cpl_images. All images in a list
 * must have the same pixel-type and the same dimensions. It is allowed to
 * insert the same image into different positions in the list. Different images
 * in the list are allowed to share the same bad pixel map.
 * 
 * @par Synopsis:
 * @code
 *   #include "cpl_imagelist.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/

#endif 
