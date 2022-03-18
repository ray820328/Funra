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

#ifndef CPL_GEOM_IMG_H
#define CPL_GEOM_IMG_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_image.h>
#include <cpl_vector.h>
#include <cpl_bivector.h>
#include <cpl_macros.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

/**@{*/

/**
 * @ingroup cpl_geom_img
 *
 * @brief
 *  The CPL Geometry combination modes
 */

typedef enum {

    /**
     * Combine using the intersection of the images.
     */
    CPL_GEOM_INTERSECT,

    /**
     * Combine using the union of the images.
     */
    CPL_GEOM_UNION,
    
    /**
     * Combine using the first image to aggregate the other ones.
     */
    CPL_GEOM_FIRST

} cpl_geom_combine;

/**@}*/

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Offsets detection and frames recombination */
cpl_bivector * cpl_geom_img_offset_fine(const cpl_imagelist *, 
                                        const cpl_bivector *,
                                        const cpl_bivector *,
                                        cpl_size, cpl_size, cpl_size, cpl_size, 
                                        cpl_vector *) CPL_ATTR_ALLOC;

cpl_image ** cpl_geom_img_offset_saa(const cpl_imagelist *,
                                     const cpl_bivector *,
                                     cpl_kernel, cpl_size,
                                     cpl_size, cpl_geom_combine,
                                     double *, double *) CPL_ATTR_ALLOC;

cpl_image ** cpl_geom_img_offset_combine(const cpl_imagelist *,
                                         const cpl_bivector *, int,
                                         const cpl_bivector *,
                                         const cpl_vector *, cpl_size *,
                                         cpl_size, cpl_size, cpl_size,
                                         cpl_size, cpl_size, cpl_size,
                                         cpl_geom_combine) CPL_ATTR_ALLOC;

CPL_END_DECLS

#endif 
