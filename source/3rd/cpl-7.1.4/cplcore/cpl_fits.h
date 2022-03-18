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

#ifndef CPL_FITS_H
#define CPL_FITS_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_error.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

/**
 * @ingroup cpl_fits
 *
 * @brief The values of the CPL fits mode.
 * The values can be combined with bitwise or.
 */

enum _cpl_fits_mode_ {

/* No mode has the value 1, which makes the (mis)use of logical or detectable */

    /**
     * Stop the caching of open FITS files
     * @hideinitializer
     */
    CPL_FITS_STOP_CACHING =  1 << 1,
    /**
     * Start the caching of open FITS files
     * @hideinitializer
     */
    CPL_FITS_START_CACHING =  1 << 2,
    /**
     * Restart the caching of open FITS files
     * @hideinitializer
     */
    CPL_FITS_RESTART_CACHING = CPL_FITS_STOP_CACHING | CPL_FITS_START_CACHING,
    /**
     * Apply the mode change only to the current thread
     * @hideinitializer
     */
    CPL_FITS_ONE =  1 << 3
 
};

/**
 * @ingroup cpl_fits
 *
 * @brief
 *   The CPL fits mode. It is a bit field.
 */
typedef enum _cpl_fits_mode_ cpl_fits_mode;


/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

cpl_size cpl_fits_count_extensions(const char *);
cpl_size cpl_fits_find_extension(const char *, const char *);

cpl_error_code cpl_fits_set_mode(cpl_fits_mode);
cpl_fits_mode cpl_fits_get_mode(void);

int cpl_fits_get_nb_extensions(const char *) CPL_ATTR_DEPRECATED;
int cpl_fits_get_extension_nb(const char *, const char *) CPL_ATTR_DEPRECATED;

CPL_END_DECLS

#endif 
