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

#ifndef CPL_STATS_DEFS_H
#define CPL_STATS_DEFS_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_stats.h"
#include "cpl_type.h"

CPL_BEGIN_DECLS


/*-----------------------------------------------------------------------------
                            Type definition
 -----------------------------------------------------------------------------*/
struct _cpl_stats_
{
    double      min;
    double      max;
    double      mean;
    double      med;
    double      med_dev;
    double      mad;
    double      stdev;
    double      flux;
    double      absflux;
    double      sqflux;
    double      centroid_x;
    double      centroid_y;
    cpl_size    min_x;
    cpl_size    min_y;
    cpl_size    max_x;
    cpl_size    max_y;
    cpl_size    npix;
    cpl_stats_mode mode;
};

CPL_END_DECLS

#endif
