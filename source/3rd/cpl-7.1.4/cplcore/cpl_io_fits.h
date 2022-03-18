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

#ifndef CPL_IO_FITS_H
#define CPL_IO_FITS_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_macros.h>

#include <cpl_type.h>
#include <cpl_error.h>

#include <fitsio2.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                               Defines
 -----------------------------------------------------------------------------*/

/* Need stat() for this to work */

#undef CPL_HAVE_STAT

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined HAVE_STAT && defined HAVE_DECL_STAT
#define CPL_HAVE_STAT
#endif

#undef CPL_IO_FITS
#ifndef CPL_HAVE_STAT
/* Without stat() the caching is disabled */
#elif defined CPL_IO_FITS_MAX_OPEN && CPL_IO_FITS_MAX_OPEN  == 0
/* May be defined to zero to disable the caching of file handles */
#else
#  define CPL_IO_FITS
#endif

#ifndef CPL_IO_FITS
#  undef CPL_IO_FITS_MAX_OPEN
#  define CPL_IO_FITS_MAX_OPEN 0
#elif !defined CPL_IO_FITS_MAX_OPEN || CPL_IO_FITS_MAX_OPEN > NMAXFILES
#  undef CPL_IO_FITS_MAX_OPEN
/* May be defined to zero to disable the caching of file handles */
/* Default value is the maximum allowed by CFITSIO, but can be defined
   (to less) if needed due to a limit imposed by e.g. setrlimit()
*/
#  define CPL_IO_FITS_MAX_OPEN NMAXFILES
#elif CPL_IO_FITS_MAX_OPEN < 0
/* Prevent trouble from a manually set value */
#  error "CPL_IO_FITS_MAX_OPEN has am illegal, negative value"
#endif

#define CPL_IO_FITS_ALL CPL_TRUE
#define CPL_IO_FITS_ONE CPL_FALSE

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

void cpl_io_fits_init(void);
cpl_error_code cpl_io_fits_end(void);

cpl_boolean cpl_io_fits_is_enabled(void);

int cpl_io_fits_create_file(fitsfile **, const char *, int *) CPL_ATTR_NONNULL;

int cpl_io_fits_open_diskfile(fitsfile **, const char *, int, int *)
    CPL_ATTR_NONNULL;

int cpl_io_fits_close_file(fitsfile *, int *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(2)))
#endif
    ;

int cpl_io_fits_delete_file(fitsfile *, int *)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(2)))
#endif
    ;

cpl_error_code cpl_io_fits_close_tid(cpl_boolean);

int cpl_io_fits_close(const char *, int *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(2)))
#endif
    ;

CPL_END_DECLS

#endif 
