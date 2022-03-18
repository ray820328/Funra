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

#ifndef CPL_CFITSIO_H
#define CPL_CFITSIO_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_macros.h"

#include <fitsio.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

int cpl_fits_read_subset(fitsfile *fptr, int datatype, const long *blc,
                         const long *trc, const long *inc,
                         const void *nulval, void *array, int *anynul,
                         int *status)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,3,4,5,7,9)))
#endif
    CPL_INTERNAL;

int cpl_fits_write_pix(fitsfile *fptr, int datatype,
                       const long *firstpix, LONGLONG  nelem,
                       const void *array, int *status)
    CPL_ATTR_NONNULL CPL_INTERNAL;

#ifdef fits_write_pixll
int cpl_fits_write_pixll(fitsfile *fptr, int datatype, const LONGLONG *firstpix,
                LONGLONG nelem, const void *array, int *status)
    CPL_ATTR_NONNULL CPL_INTERNAL;
#endif

int cpl_fits_create_img(fitsfile *fptr, int bitpix, int naxis,
                        const long *naxes, int *status)
    CPL_ATTR_NONNULL CPL_INTERNAL;

#ifdef fits_create_imgll
int cpl_fits_create_imgll(fitsfile *fptr, int bitpix, int naxis,
                          const LONGLONG *naxes, int *status)
    CPL_ATTR_NONNULL CPL_INTERNAL;
#endif

int cpl_fits_read_pix(fitsfile *fptr, int datatype, const long *firstpix,
                      LONGLONG nelem, const void *nulval, void *array,
                      int *anynul, int *status)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,3,6,8)))
#endif
    CPL_INTERNAL;

#ifdef fits_read_pixll
int cpl_fits_read_pixll(fitsfile *fptr, int datatype, const LONGLONG *firstpix,
                        LONGLONG nelem, const void *nulval, void *array,
                        int *anynul, int *status)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,3,6,8)))
#endif
    CPL_INTERNAL;
#endif


CPL_END_DECLS

#endif 
