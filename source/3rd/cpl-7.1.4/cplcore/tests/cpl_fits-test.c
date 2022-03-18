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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_fits.h"
#include "cpl_memory.h"
#include "cpl_test.h"

/* Needed for CPL_IO_FITS_MAX_OPEN */
#include "cpl_io_fits.h"

#include <stdlib.h>
#include <string.h>

/*-----------------------------------------------------------------------------
                                  Defines
 -----------------------------------------------------------------------------*/

#define CPL_FITS_NAME "cpl_fits-test.fits"

#ifndef CPL_FITS_NEXT
#define CPL_FITS_NEXT 12
#endif

/*----------------------------------------------------------------------------
                 Private function prototypes
 ----------------------------------------------------------------------------*/

static void cpl_fits_mode_test(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_propertylist * pl;
    cpl_size           ivalue;
    cpl_error_code     error;
    cpl_size           iext;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    cpl_fits_mode_test();

    ivalue = cpl_fits_count_extensions(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(ivalue, -1);

    ivalue = cpl_fits_find_extension(CPL_FITS_NAME, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(ivalue, -1);

    ivalue = cpl_fits_find_extension(NULL, "EXT0");
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(ivalue, -1);

    ivalue = cpl_fits_count_extensions(".");
    cpl_test_error(CPL_ERROR_FILE_IO);
    cpl_test_eq(ivalue, -1);

    ivalue = cpl_fits_find_extension(".", "EXT0");
    cpl_test_error(CPL_ERROR_FILE_IO);
    cpl_test_eq(ivalue, -1);

    /* Create a test MEF */

    /* Empty primary unit */
    pl = cpl_propertylist_new();
    cpl_test_nonnull(pl);
    error = cpl_propertylist_save(pl, CPL_FITS_NAME, CPL_IO_CREATE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    ivalue = cpl_fits_count_extensions(CPL_FITS_NAME);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(ivalue);

    ivalue = cpl_fits_find_extension(CPL_FITS_NAME, "EXT0");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(ivalue);

    /* First extension is without the EXTNAME card */
    error = cpl_propertylist_save(pl, CPL_FITS_NAME, CPL_IO_EXTEND);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    ivalue = cpl_fits_count_extensions(CPL_FITS_NAME);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq(ivalue, 1);

    for (iext = 2; iext <= CPL_FITS_NEXT; iext++) {
        const char * extname = cpl_sprintf("EXT%" CPL_SIZE_FORMAT, iext);
        cpl_size jext;

        error = cpl_propertylist_update_string(pl, "EXTNAME", extname);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_propertylist_save(pl, CPL_FITS_NAME, CPL_IO_EXTEND);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        ivalue = cpl_fits_count_extensions(CPL_FITS_NAME);
        cpl_test_error(CPL_ERROR_NONE);

        cpl_test_eq(ivalue, iext);

        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
        cpl_free((void*)extname);
        CPL_DIAG_PRAGMA_POP;

        for (jext = 0; jext <= iext; jext++) {
            extname = cpl_sprintf("EXT%" CPL_SIZE_FORMAT, jext);

            ivalue = cpl_fits_find_extension(CPL_FITS_NAME, extname);
            cpl_test_error(CPL_ERROR_NONE);

            CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
            cpl_free((void*)extname);
            CPL_DIAG_PRAGMA_POP;

            if (jext > 1) {
                cpl_test_eq(ivalue, jext);
            } else {
                /* EXT0/1 not found */
                cpl_test_zero(ivalue);
            }
        }
    }

    cpl_propertylist_delete(pl);

    cpl_test_zero(remove(CPL_FITS_NAME));

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Test the CPL function(s)
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_fits_mode_test(void)
{
    cpl_error_code code;
    cpl_fits_mode mode, modec;

#ifdef CPL_IO_FITS
    cpl_fits_mode modeo;

    /* Copied from cpl_init() */
    const char * io_fits_mode_string = getenv("CPL_IO_MODE");
    const cpl_boolean use_io_fits = io_fits_mode_string != NULL &&
        strcmp("1", io_fits_mode_string) == 0;
#else
    const cpl_boolean use_io_fits = CPL_FALSE;
#endif

    /* Get the mode */
    mode = cpl_fits_get_mode();
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(mode, use_io_fits ? CPL_FITS_START_CACHING
                : CPL_FITS_STOP_CACHING);

    /* Set the mode to what it already is */
    code = cpl_fits_set_mode(mode);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Verify that it did not change */
    modec = cpl_fits_get_mode();
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(mode, modec);

#ifdef CPL_IO_FITS
    /* Change to the other mode */
    modeo = use_io_fits ? CPL_FITS_STOP_CACHING : CPL_FITS_START_CACHING;
    code = cpl_fits_set_mode(modeo);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Verify that it did change */
    modec = cpl_fits_get_mode();
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(modeo, modec);
#endif

    /* Change back */
    code = cpl_fits_set_mode(mode);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Verify that it did change */
    modec = cpl_fits_get_mode();
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(mode, modec);

    /* Test handling of incorrect mode combination */
    code = cpl_fits_set_mode(0); /* Result of binary and */
    cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);

    code = cpl_fits_set_mode(1); /* Result of logical and/or */
    cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

}
