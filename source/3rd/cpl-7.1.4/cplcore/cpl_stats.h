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

#ifndef CPL_STATS_H
#define CPL_STATS_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpl_image.h"

#include <stdio.h>


CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

/**
 * @ingroup cpl_stats
 *
 * @brief The values of the CPL stats mode.
 * The values can be combined with bitwise or.
 */

enum _cpl_stats_mode_ {

/* No mode has the value 1, which makes the (mis)use of logical or detectable */

    /**
     * The minimum
     * @hideinitializer
     */
    CPL_STATS_MIN        =  1 << 1,
    /**
     * The maximum
     * @hideinitializer
     */
    CPL_STATS_MAX        =  1 <<  2,
    /**
     * The mean
     * @hideinitializer
     */
    CPL_STATS_MEAN       =  1 <<  3,
    /**
     * The median
     * @hideinitializer
     */
    CPL_STATS_MEDIAN     =  1 <<  4,
    /**
     * The standard deviation
     * @hideinitializer
     */
    CPL_STATS_STDEV      =  1 <<  5,
    /**
     * The flux
     * @hideinitializer
     */
    CPL_STATS_FLUX       =  1 <<  6,
    /**
     * The absolute flux
     * @hideinitializer
     */
    CPL_STATS_ABSFLUX    =  1 <<  7,
    /**
     * The square flux
     * @hideinitializer
     */
    CPL_STATS_SQFLUX     =  1 <<  8,
    /**
     * The position of the minimum
     * @hideinitializer
     */
    CPL_STATS_MINPOS     =  1 <<  9,
    /**
     * The position of the maximum
     * @hideinitializer
     */
    CPL_STATS_MAXPOS     =  1 << 10,
    /**
     * The centroid position
     * @hideinitializer
     */
    CPL_STATS_CENTROID   =  1 << 11,
    /**
     * The mean of the absolute median deviation
     * @hideinitializer
     */
    CPL_STATS_MEDIAN_DEV =  1 << 12,
    /**
     * The median of the absolute median deviation
     * @hideinitializer
     */
    CPL_STATS_MAD =  1 << 13,
    /**
     * All of the above
     * @hideinitializer
     */
    CPL_STATS_ALL        = (1 << 14)-2
};


/**
 * @ingroup cpl_stats
 *
 * @brief
 *   The CPL stats mode. It is a bit field.
 */
typedef enum _cpl_stats_mode_ cpl_stats_mode;

/**
 * @ingroup cpl_stats
 * @brief The opaque CPL stats data type.
 */
typedef struct _cpl_stats_ cpl_stats;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Accessor functions */
double cpl_stats_get_min(const cpl_stats *);
double cpl_stats_get_max(const cpl_stats *);
double cpl_stats_get_mean(const cpl_stats *);
double cpl_stats_get_median(const cpl_stats *);
double cpl_stats_get_median_dev(const cpl_stats *);
double cpl_stats_get_mad(const cpl_stats *);
double cpl_stats_get_stdev(const cpl_stats *);
double cpl_stats_get_flux(const cpl_stats *);
double cpl_stats_get_absflux(const cpl_stats *);
double cpl_stats_get_sqflux(const cpl_stats *);
double cpl_stats_get_centroid_x(const cpl_stats *);
double cpl_stats_get_centroid_y(const cpl_stats *);
cpl_size cpl_stats_get_min_x(const cpl_stats *);
cpl_size cpl_stats_get_min_y(const cpl_stats *);
cpl_size cpl_stats_get_max_x(const cpl_stats *);
cpl_size cpl_stats_get_max_y(const cpl_stats *);
cpl_size cpl_stats_get_npix(const cpl_stats *);

void cpl_stats_delete(cpl_stats *);

/* Statistics computations */
cpl_stats * cpl_stats_new_from_image(const cpl_image *,
                                     cpl_stats_mode) CPL_ATTR_ALLOC;
cpl_stats * cpl_stats_new_from_image_window(const cpl_image *, cpl_stats_mode,
                                            cpl_size, cpl_size, cpl_size,
                                            cpl_size) CPL_ATTR_ALLOC;
cpl_error_code cpl_stats_dump(const cpl_stats *, cpl_stats_mode, FILE *);

CPL_END_DECLS

#endif 
