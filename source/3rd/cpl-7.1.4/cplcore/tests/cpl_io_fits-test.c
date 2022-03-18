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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_io_fits.h"

#include "cpl_test.h"

#include "cpl_fits.h"
#include "cpl_memory.h"
#include "cpl_table.h"

#include <stdlib.h>
#include <string.h>

/*-----------------------------------------------------------------------------
                                  Defines
 -----------------------------------------------------------------------------*/

#ifndef NEXT
#define NEXT 3
#endif

#ifndef NFILE
#define NFILE 4
#endif

#define BASENAME "cpl_io_fits_test"

/*----------------------------------------------------------------------------
                 Function prototypes
 ----------------------------------------------------------------------------*/

static void cpl_io_fits_test_many(int);
static void cpl_io_fits_test_fulldisk_table(const char*);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    cpl_boolean    do_bench;
    int            next = NEXT;
    int            j;



    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    if (do_bench)
        next *= next;

    /* Insert tests below */

    if (argc == 2) {
        cpl_io_fits_test_fulldisk_table(argv[1]);
    }

    cpl_io_fits_test_many(do_bench
                          ? 1 + CPL_IO_FITS_MAX_OPEN
                          : 1 + CPL_IO_FITS_MAX_OPEN/3);

#ifdef _OPENMP
#pragma omp parallel for private(j)
#endif
    for (j = 0; j < NFILE; j++) {
        char * filename = cpl_sprintf(BASENAME "-%d.fits", j + 1);
        int base = j * next * 10; /* Assume at most 10 load-loops */
        int i;

        cpl_msg_info(cpl_func, "Testing with file %s", filename);

        /* Creation + append only */
#ifdef _OPENMP
        /* Multi-threaded writing is limited to one thread writing at a time */
#pragma omp parallel for ordered private(i)
#endif
        for (i = 0; i <= next; i++) {
            cpl_error_code myerror;
            cpl_image * myimage = cpl_image_new(1, 1, CPL_TYPE_INT);
            cpl_propertylist * plist = cpl_propertylist_new();

            myerror = cpl_propertylist_append_int(plist, "MYINT", i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

            myerror = cpl_image_add_scalar(myimage, i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

#ifdef _OPENMP
#pragma omp ordered
#endif
            {
                myerror = cpl_image_save(myimage, filename, CPL_TYPE_INT, plist,
                                         i ? CPL_IO_EXTEND : CPL_IO_CREATE);
            }
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);
            cpl_image_delete(myimage);
            cpl_propertylist_delete(plist);

        }

        cpl_test_eq(cpl_fits_count_extensions(filename), next);

        /* Reading only */
#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
        for (i = 0; i <= next; i++) {
            int is_bad;
            cpl_image * myimage = cpl_image_load(filename, CPL_TYPE_INT, 0, i);
            cpl_propertylist * plist;

            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(myimage);
            cpl_test_eq(cpl_image_get(myimage, 1, 1, &is_bad), i + base);
            cpl_test_zero(is_bad);
            cpl_image_delete(myimage);

            /* Make sure to jump around between the extensions */
            cpl_test_eq(cpl_fits_count_extensions(filename), next);

            plist = cpl_propertylist_load(filename, i);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(plist);

            cpl_test_eq(cpl_propertylist_get_int(plist, "MYINT"), i + base);

            cpl_propertylist_delete(plist);
        }

        base += next;

        /* Alternating creation + append only */
#ifdef _OPENMP
        /* Multi-threaded writing is limited to one thread writing at a time */
#pragma omp parallel for ordered private(i)
#endif
        for (i = 0; i <= next; i++) {
            cpl_error_code myerror;
            cpl_image * myimage = cpl_image_new(1, 1, CPL_TYPE_INT);
            cpl_propertylist * plist = cpl_propertylist_new();

            myerror = cpl_propertylist_append_int(plist, "MYINT", i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

            myerror = cpl_image_add_scalar(myimage, i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

#ifdef _OPENMP
#pragma omp ordered
#endif
            {
                myerror = cpl_image_save(myimage, filename, CPL_TYPE_INT, plist,
                                         (i & 1) ? CPL_IO_EXTEND
                                         : CPL_IO_CREATE);
            }
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

            cpl_image_delete(myimage);
            cpl_propertylist_delete(plist);

        }

        cpl_test_eq(cpl_fits_count_extensions(filename), next & 1);

        /* Reading only */
#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
        for (i = 0; i <= (next & 1); i++) {
            int is_bad;
            cpl_image * myimage = cpl_image_load(filename, CPL_TYPE_INT, 0, i);
            cpl_propertylist * plist;


            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(myimage);
            cpl_test_eq(cpl_image_get(myimage, 1, 1, &is_bad), i + (next & ~1)
                                                               + base);
            cpl_test_zero(is_bad);
            cpl_image_delete(myimage);

            plist = cpl_propertylist_load(filename, i);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(plist);

            cpl_test_eq(cpl_propertylist_get_int(plist, "MYINT"),
                        i + (next & ~1) + base);

            cpl_propertylist_delete(plist);

        }

        base += next;

        /* Creation + append and reading */
#ifdef _OPENMP
        /* Multi-threaded writing is limited to one thread writing at a time */
#pragma omp parallel for ordered private(i)
#endif
        for (i = 0; i <= next; i++) {
            int is_bad;
            cpl_error_code myerror;
            cpl_image * myimage = cpl_image_new(1, 1, CPL_TYPE_INT);
            cpl_propertylist * plist = cpl_propertylist_new();

            myerror = cpl_propertylist_append_int(plist, "MYINT", i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

            myerror = cpl_image_add_scalar(myimage, i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

            cpl_test_eq(i, i);

#ifdef _OPENMP
#pragma omp ordered
#endif
            {
                myerror = cpl_image_save(myimage, filename, CPL_TYPE_INT, plist,
                                         i ? CPL_IO_EXTEND : CPL_IO_CREATE);
                cpl_test_eq_error(myerror, CPL_ERROR_NONE);
                cpl_image_delete(myimage);

                /* Make sure to jump around between the extensions */
                cpl_test_eq(cpl_fits_count_extensions(filename), i);

                myimage = cpl_image_load(filename, CPL_TYPE_INT, 0, i);
                cpl_test_error(CPL_ERROR_NONE);

                cpl_propertylist_delete(plist);
                plist = cpl_propertylist_load(filename, i);
                cpl_test_error(CPL_ERROR_NONE);

            }

            cpl_test_nonnull(myimage);
            cpl_test_eq(cpl_image_get(myimage, 1, 1, &is_bad), i + base);
            cpl_test_zero(is_bad);

            cpl_test_nonnull(plist);
            cpl_test_eq(cpl_propertylist_get_int(plist, "MYINT"), i + base);

            cpl_image_delete(myimage);
            cpl_propertylist_delete(plist);

        }

        cpl_test_eq(cpl_fits_count_extensions(filename), next);

        base += next;

        /* Creation + append from one thread, reading from from another*/
#ifdef _OPENMP
        /* Multi-threaded writing is limited to one thread writing at a time */
#pragma omp parallel for ordered private(i)
#endif
        for (i = 0; i <= 2 * next + 1; i++)
#ifdef _OPENMP
#pragma omp ordered
#endif
            {
                if (i & 1) {
                    if (i > 2) {
                        /* Read the previously written extension */
                        const int i2 = i / 2 - 1;
                        int is_bad;
                        cpl_propertylist * plist;
                        cpl_image * myimage = cpl_image_load(filename,
                                                             CPL_TYPE_INT,
                                                             0, i2);

                        cpl_test_error(CPL_ERROR_NONE);
                        cpl_test_nonnull(myimage);
                        cpl_test_eq(cpl_image_get(myimage, 1, 1, &is_bad),
                                    i2 + base);
                        cpl_test_zero(is_bad);
                        cpl_image_delete(myimage);

                        plist = cpl_propertylist_load(filename, i2);
                        cpl_test_error(CPL_ERROR_NONE);

                        cpl_test_nonnull(plist);
                        cpl_test_eq(cpl_propertylist_get_int(plist, "MYINT"),
                                    i2 + base);

                        cpl_propertylist_delete(plist);
                    }

                } else {
                    cpl_error_code myerror;
                    cpl_image * myimage = cpl_image_new(1, 1, CPL_TYPE_INT);
                    cpl_propertylist * plist = cpl_propertylist_new();

                    myerror = cpl_propertylist_append_int(plist, "MYINT",
                                                          i/2 + base);
                    cpl_test_eq_error(myerror, CPL_ERROR_NONE);

                    myerror = cpl_image_add_scalar(myimage, i/2 + base);
                    cpl_test_eq_error(myerror, CPL_ERROR_NONE);

                    cpl_test_eq(i, i);
                    myerror = cpl_image_save(myimage, filename,
                                             CPL_TYPE_INT, plist,
                                             i ? CPL_IO_EXTEND : CPL_IO_CREATE);

                    cpl_test_eq_error(myerror, CPL_ERROR_NONE);
                    cpl_image_delete(myimage);
                    cpl_propertylist_delete(plist);
                }
            }

        cpl_test_eq(cpl_fits_count_extensions(filename), next);

        base += next;

        /* Alternating creation + append and reading */
#ifdef _OPENMP
        /* Multi-threaded writing is limited to one thread writing at a time */
#pragma omp parallel for ordered private(i)
#endif
        for (i = 0; i <= next; i++) {
            int is_bad;
            cpl_error_code myerror;
            cpl_image * myimage = cpl_image_new(1, 1, CPL_TYPE_INT);
            cpl_propertylist * plist = cpl_propertylist_new();

            myerror = cpl_propertylist_append_int(plist, "MYINT", i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

            myerror = cpl_image_add_scalar(myimage, i + base);
            cpl_test_eq_error(myerror, CPL_ERROR_NONE);

#ifdef _OPENMP
#pragma omp ordered
#endif
            {
                myerror = cpl_image_save(myimage, filename, CPL_TYPE_INT, plist,
                                         (i & 1) ? CPL_IO_EXTEND
                                         : CPL_IO_CREATE);
                cpl_test_eq_error(myerror, CPL_ERROR_NONE);

                /* Make sure to jump around between the extensions */
                cpl_test_eq(cpl_fits_count_extensions(filename), i & 1);

                cpl_image_delete(myimage);
                myimage = cpl_image_load(filename, CPL_TYPE_INT, 0, i & 1);

                cpl_test_error(CPL_ERROR_NONE);

                cpl_propertylist_delete(plist);
                plist = cpl_propertylist_load(filename, i & 1);
                cpl_test_error(CPL_ERROR_NONE);

            }

            cpl_test_nonnull(myimage);
            cpl_test_eq(cpl_image_get(myimage, 1, 1, &is_bad), i + base);
            cpl_test_zero(is_bad);
            cpl_image_delete(myimage);

            cpl_test_nonnull(plist);
            cpl_test_eq(cpl_propertylist_get_int(plist, "MYINT"), i + base);

            cpl_propertylist_delete(plist);

        }

        cpl_test_eq(cpl_fits_count_extensions(filename), next & 1);

        cpl_free((void*)filename);
    }

    if (cpl_fits_get_mode() | CPL_FITS_START_CACHING) {
        cpl_error_code myerror;
        myerror = cpl_fits_set_mode(CPL_FITS_STOP_CACHING);
        cpl_test_eq_error(myerror, CPL_ERROR_NONE);
    }

    for (j = 0; j < NFILE; j++) {
        char * filename = cpl_sprintf(BASENAME "-%d.fits", j + 1);
        cpl_test_zero(remove(filename));
        cpl_free(filename);
    }

    /* End of tests */
    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Open multiple files
  @param n  The number of files to process
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_io_fits_test_many(int n)
{

    cpl_propertylist * plist = cpl_propertylist_new();
    const cpl_fits_mode iomode = cpl_fits_get_mode();

    cpl_msg_info(cpl_func, "Testing I/O-mode %d <=> %d with %d file(s)",
                 (int)iomode, (int)CPL_FITS_START_CACHING, n);

    for (int i = 0; i < n; i++) {
        char * filename = cpl_sprintf(BASENAME "-%d.fits", i + 1);
        cpl_error_code myerror;
        myerror = cpl_propertylist_update_int(plist, "MYINT", i);
        cpl_test_eq_error(myerror, CPL_ERROR_NONE);

        myerror = cpl_propertylist_save(plist, filename, CPL_IO_CREATE);
        cpl_test_eq_error(myerror, CPL_ERROR_NONE);

        cpl_propertylist_delete(plist);
        plist = cpl_propertylist_load(filename, 0);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(plist);

        cpl_test_eq(cpl_propertylist_get_int(plist, "MYINT"), i);

        cpl_free(filename);

    }

    cpl_propertylist_delete(plist);

    if (iomode | CPL_FITS_START_CACHING) {
        cpl_error_code myerror;
        myerror = cpl_fits_set_mode(CPL_FITS_STOP_CACHING);
        cpl_test_eq_error(myerror, CPL_ERROR_NONE);
    }

    for (int i = 0; i < n; i++) {
        char * filename = cpl_sprintf(BASENAME "-%d.fits", i + 1);
        cpl_test_zero(remove(filename));
        cpl_free(filename);
    }
    if (iomode | CPL_FITS_START_CACHING) {
        cpl_error_code myerror;
        myerror = cpl_fits_set_mode(iomode);
        cpl_test_eq_error(myerror, CPL_ERROR_NONE);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @brief       Test I/O on a user specified file system (of limited size)
  @param  root The name of the file system to fill up
  @return void
 */
/*----------------------------------------------------------------------------*/
static void cpl_io_fits_test_fulldisk_table(const char* root)
{
    cpl_table*     mytab = cpl_table_new(1000);
    char*          filename = cpl_sprintf("%s/" BASENAME ".fits", root);
    const cpl_fits_mode iomode = cpl_fits_get_mode();
    cpl_error_code code;

    cpl_test_nonnull(mytab);
    cpl_test_nonnull(filename);

    code = cpl_table_new_column(mytab, "COL1", CPL_TYPE_DOUBLE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_table_fill_invalid_double(mytab, "COL1", 42.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_table_save(mytab, NULL, NULL, filename, CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_fits(filename);

    do {
        code = cpl_table_save(mytab, NULL, NULL, filename, CPL_IO_EXTEND);
    } while (code == CPL_ERROR_NONE);

    cpl_test_eq_error(code, CPL_ERROR_FILE_NOT_CREATED);

    if (cpl_test_get_failed() > 0) {
        cpl_msg_warning(cpl_func, "Test failed, not deleting file:: %s",
                        filename);
    } else if (remove(filename) != 0) {
        cpl_msg_info(cpl_func, "The failure-test file has already been deleted: "
                     "%s", filename);
    }

    cpl_table_delete(mytab);

    if (iomode | CPL_FITS_START_CACHING) {
        cpl_error_code myerror;
        myerror = cpl_fits_set_mode(CPL_FITS_STOP_CACHING);
        cpl_test_eq_error(myerror, CPL_ERROR_NONE);
    }

    cpl_test_zero(remove(filename));

    if (iomode | CPL_FITS_START_CACHING) {
        cpl_error_code myerror;
        myerror = cpl_fits_set_mode(iomode);
        cpl_test_eq_error(myerror, CPL_ERROR_NONE);
    }

    cpl_free(filename);
}
