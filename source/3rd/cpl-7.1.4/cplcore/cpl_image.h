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

#ifndef CPL_IMAGE_H
#define CPL_IMAGE_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image_io.h"
#include "cpl_image_basic.h"
#include "cpl_image_iqe.h"
#include "cpl_image_bpm.h"
#include "cpl_image_resample.h"
#include "cpl_image_stats.h"
#include "cpl_image_filter.h"
#include "cpl_image_gen.h"

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_image Images
 *
 * This module provides functions to create, use, and destroy a @em cpl_image.
 * A @em cpl_image is a 2-dimensional data structure with a pixel type
 * (one of @c CPL_TYPE_INT, @c CPL_TYPE_FLOAT, @c CPL_TYPE_DOUBLE,
 * @c CPL_TYPE_FLOAT_COMPLEX or @c CPL_TYPE_DOUBLE_COMPLEX) and an
 * optional bad pixel map.
 * 
 * The pixel indexing follows the FITS convention in the sense that the
 * lower left pixel in a CPL image has index (1, 1). The pixel buffer is
 * stored row-wise so for optimum performance any pixel-wise access should
 * be done likewise.
 * 
 * Functionality include:
 * FITS I/O
 * Image arithmetic, casting, extraction, thresholding, filtering, resampling
 * Bad pixel handling
 * Image statistics
 * Generation of test images
 * Special functions, such as the image quality estimator
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_image.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/

#endif 
