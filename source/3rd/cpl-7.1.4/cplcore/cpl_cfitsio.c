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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpl_cfitsio.h"


/* The below doxygen has been inactivated by removing the '**' comment. */

/*----------------------------------------------------------------------------*/
/*
 * @defgroup cpl_cfitsio   Wrap around CFITSIO functions dropping const
 *                         modifiers that the functions should have.
 * 
 * The purpose is to limit all the const-correctness warnings to a single file.
 * 
 * @par Synopsis:
 * @code
 *   #include "cpl_cfitsio.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/

/**@{*/

/*-----------------------------------------------------------------------------
                              Function definitions
 -----------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Wrap around the CFISTIO function dropping const modifiers that the
 *        CFITSIO function should have.
 * @see fits_read_subset()
 */
int cpl_fits_read_subset(fitsfile *fptr, int datatype, const long *blc,
                         const long *trc, const long *inc, const void *nulval,
                         void *array, int *anynul, int *status)
{
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return fits_read_subset(fptr, datatype, (long*)blc, (long*)trc, (long*)inc,
                            (void*)nulval, array, anynul,status);
    CPL_DIAG_PRAGMA_POP;
}

/**
 * @internal
 * @brief Wrap around the CFISTIO function dropping const modifiers that the
 *        CFITSIO function should have.
 * @see fits_read_subset()
 */
int cpl_fits_write_pix(fitsfile *fptr, int datatype, const long *firstpix,
                       LONGLONG nelem, const void *array, int *status)
{
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return fits_write_pix(fptr, datatype, (long*)firstpix, nelem,
                          (void*)array, status);
    CPL_DIAG_PRAGMA_POP;
}

#ifdef fits_write_pixll
/**
 * @internal
 * @brief Wrap around the CFISTIO function dropping const modifiers that the
 *        CFITSIO function should have.
 * @see fits_read_subset()
 */
int cpl_fits_write_pixll(fitsfile *fptr, int datatype, const LONGLONG *firstpix,
                         LONGLONG nelem, const void *array, int *status)
{
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return fits_write_pixll(fptr, datatype, (LONGLONG*)firstpix, nelem,
                            (void*)array, status);
    CPL_DIAG_PRAGMA_POP;
}
#endif

/**
 * @internal
 * @brief Wrap around the CFISTIO function dropping const modifiers that the
 *        CFITSIO function should have.
 * @see fits_read_subset()
 */
int cpl_fits_create_img(fitsfile *fptr, int bitpix, int naxis,
                        const long *naxes, int *status)
{
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return fits_create_img(fptr, bitpix, naxis,
                           (long *)naxes, status);
    CPL_DIAG_PRAGMA_POP;
}

#ifdef fits_create_imgll
/**
 * @internal
 * @brief Wrap around the CFISTIO function dropping const modifiers that the
 *        CFITSIO function should have.
 * @see fits_read_subset()
 */
int cpl_fits_create_imgll(fitsfile *fptr, int bitpix, int naxis,
                          const LONGLONG *naxes, int *status)
{
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return fits_create_imgll(fptr, bitpix, naxis,
                             (LONGLONG *)naxes, status);
    CPL_DIAG_PRAGMA_POP;
}
#endif

/**
 * @internal
 * @brief Wrap around the CFISTIO function dropping const modifiers that the
 *        CFITSIO function should have.
 * @see fits_read_subset()
 */
int cpl_fits_read_pix(fitsfile *fptr, int datatype, const long *firstpix,
                      LONGLONG nelem, const void *nulval, void *array,
                      int *anynul, int *status)
{
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return fits_read_pix(fptr, datatype, (long*)firstpix, nelem,
                         (void*)nulval, array, anynul, status);
    CPL_DIAG_PRAGMA_POP;
}

#ifdef fits_read_pixll
/**
 * @internal
 * @brief Wrap around the CFISTIO function dropping const modifiers that the
 *        CFITSIO function should have.
 * @see fits_read_subset()
 */
int cpl_fits_read_pixll(fitsfile *fptr, int datatype, const LONGLONG *firstpix,
                        LONGLONG nelem, const void *nulval, void *array,
                        int *anynul, int *status)
{
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return fits_read_pixll(fptr, datatype, (LONGLONG*)firstpix, nelem,
                           (void*)nulval, array, anynul, status);
    CPL_DIAG_PRAGMA_POP;
}
#endif

/**@}*/
