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

#include <complex.h>

#include "cpl_init.h"
#include "cpl_image_io.h"
#include "cpl_error.h"
#include "cpl_propertylist_impl.h"
#include "cpl_table.h"
#include "cpl_msg.h"
#include "cpl_test.h"
#include "cpl_memory.h"

#include "cpl_fits.h"
#include "cpl_io_fits.h"
#include "cpl_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <math.h>
#include <fitsio.h>

#undef CPL_HAVE_LOCALE

#ifdef HAVE_LOCALE_H
#include <locale.h>
#define CPL_HAVE_LOCALE
#endif
#if defined HAVE_XLOCALE_H
#include <xlocale.h>
#define CPL_HAVE_LOCALE
#endif

/*----------------------------------------------------------------------------
                 Defines
 ----------------------------------------------------------------------------*/

#ifndef CPL_NON_POSIX_LOCALE
#define CPL_NON_POSIX_LOCALE "de_DE.UTF8"
#endif

#define BASE "cpl_propertylist-test"

#define FILENAME(N) BASE CPL_STRINGIFY(N) ".fits"

#define RMCARDQUOTE(KEYBASE, MYFILE)                    \
    "perl -pi -e 's/(" KEYBASE "\\d+ = )\\047([^\\047]*)\\047/$1 $2 /g' " MYFILE


#define KEYBASE "ESO KEY"
#define KEYFORM KEYBASE "%05d"

#define NOKEY "Non-existing key"

#define BADNUM "l.O"
/* This complex format is valid and supported (when enclosed in parenthesis) */
#define COMPLEXVAL "1D1,2E2"

#define LONGNAME80                              \
    "0123456789012345678901234567890123456789"  \
    "0123456789012345678901234567890123456789"


/*----------------------------------------------------------------------------
                 Functions prototypes
 ----------------------------------------------------------------------------*/

static int cpl_test_property_compare_name(const cpl_property  *,
                                          const cpl_property  *);

static void cpl_property_eq(const cpl_property *,
                            const cpl_property *)
    CPL_ATTR_NONNULL;

static void cpl_propertylist_test_numeric_type(void);

static void cpl_propertylist_test_numeric_load_ok(void);
static void cpl_propertylist_test_numeric_load_ok2(void);
static void cpl_propertylist_test_numeric_load_ok3(void);
static void cpl_propertylist_test_numeric_load_bad(void);

static void cpl_propertylist_compare(const cpl_propertylist *,
                                     const cpl_propertylist *)
    CPL_ATTR_NONNULL;

static void cpl_propertylist_test_file(const char *, cpl_size *)
    CPL_ATTR_NONNULL;

static void cpl_propertylist_test_local(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{

    /* The number of allowed failures (for invalid CLI files) */
    int      nxfail = 0;
    cpl_size maxsz  = 0;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    if (argc == 1) {
#ifdef CPL_HAVE_LOCALE
        /* Set the locale to a non POSIX one: LC_NUMERIC=de_DE.UTF8 */
        locale_t nonposix_locale =
            newlocale(LC_NUMERIC_MASK, CPL_NON_POSIX_LOCALE, (locale_t) 0);
        if (nonposix_locale == (locale_t) 0) {
            cpl_msg_warning(cpl_func, "Skipping test with unavailable "
                            "non-POSIX locale: " CPL_NON_POSIX_LOCALE);
        } else {
            locale_t old_locale = uselocale(nonposix_locale);

            cpl_msg_info(cpl_func, "Testing with non-POSIX locale (float: %g): "
                         CPL_NON_POSIX_LOCALE, 1.23);

            cpl_propertylist_test_local();

            (void)uselocale(old_locale);
            freelocale(nonposix_locale);
        }

        cpl_msg_info(cpl_func, "Testing with default locale (float: %g)",
                     1.23);
#endif
        cpl_propertylist_test_local();
    }

    for (int ifile = 0; ifile < argc; ifile++) {
        if (ifile == 0) {
            const cpl_propertylist * nulllist
                = cpl_propertylist_load(argv[0], 0);

            cpl_test_error(CPL_ERROR_BAD_FILE_FORMAT);
            cpl_test_null(nulllist);
        } else {
            const cpl_size         nfailed   = cpl_test_get_failed();
            const cpl_msg_severity msg_level = cpl_msg_get_level();

            cpl_msg_set_level(CPL_MSG_OFF);
            cpl_test_fits(argv[ifile]);
            cpl_msg_set_level(msg_level);

            if (cpl_test_get_failed() == nfailed) {
                cpl_propertylist_test_file(argv[ifile], &maxsz);
            } else {
                cpl_msg_info(cpl_func, "Skipping tests using %s (%d/%d)",
                             argv[ifile], ifile+1, argc);
                nxfail++;
            }
        }
    }

    if (argc > 1) {
        cpl_msg_info(cpl_func, "Tested with %d file(s), max header size: %d",
                     argc - 1, (int)maxsz);
    }

    return cpl_test_end(-nxfail);

}

static void cpl_propertylist_test_local(void)
{

    const char *keys[] = {
        "a", "b", "c", "d", "e", "f", "g", "h", "i",
        "A", "B", "C", "D", "E", "F", "G", "H", "I",
    };

    const char *comments[] = {
        "A character value",
        "A boolean value",
        "A integer value",
        "A long integer value",
        "A floating point number",
        "A double precision number",
        "A string value (e.g. O'HARA, '/' is for a comment)",
        "A floating point complex number",
        "A double precision complex number"
    };

    cpl_type types[] = {
        CPL_TYPE_CHAR,
        CPL_TYPE_BOOL,
        CPL_TYPE_INT,
        CPL_TYPE_LONG,
        CPL_TYPE_FLOAT,
        CPL_TYPE_DOUBLE,
        CPL_TYPE_STRING,
        CPL_TYPE_FLOAT_COMPLEX,
        CPL_TYPE_DOUBLE_COMPLEX
    };

    int status, j;

    long i;
    long naxes[2] = {256,256};

    const float fval0 = -1.23456789;
    float fval1, fval2;
    const double dval0 = -1.23456789;
    double dval1, dval2;
    const float complex zf0 = fval0 + fval0 * fval0 * _Complex_I;
    float  complex zf1, zf2;
    const double complex zd0 = zf0;
    double complex zd1, zd2;
    float f0,f1,f2;
    double d0,d1,d2;
    const int nprops = sizeof(types)/sizeof(types[0]);
    char stringarray[FLEN_VALUE];

    const char *filename3 = BASE ".fits";
    /*const char regex[] = "^(DATE-OBS|ESO|COMMENT|HISTORY)$";*/
    const char to_rm[] = "^HIERARCH ESO |^NBXIS1$|^NBXIS2$|^HISTORY$";
    cpl_propertylist *plist, *_plist, *plist2;
    const cpl_propertylist * nulllist;
    cpl_property *pro, *pro2;
    const cpl_property *pro1;
    const cpl_property *nullprop;
    cpl_table *t;
    cpl_type t1;
    cpl_error_code code;
    int value;
    cpl_boolean   do_bench;
    int bench_size;
    const char * strval;
    int syscode;

    fitsfile *fptr = NULL;
    fitsfile *_fptr;

    FILE *stream;
    const char *longname = LONGNAME80;

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    /* Always test the _dump functions, but produce no output
       when the message level is above info */
    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull(stream);

    cpl_test_eq(sizeof(comments)/sizeof(comments[0]), nprops);
    cpl_test_eq(sizeof(keys)/sizeof(keys[0]), 2 * nprops);

    /*
     * Test 1: Create a property list and check its validity.
     */

    plist = cpl_propertylist_new();

    cpl_test_nonnull(plist);
    cpl_test(cpl_propertylist_is_empty(plist));
    cpl_test_zero(cpl_propertylist_get_size(plist));

    nullprop = cpl_propertylist_get_const(plist, 100);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(nullprop);

    /*
     * Test 2: Append properties to the list created in the previous test
     *         and verify the data.
     */

    cpl_propertylist_append_char(plist, keys[0], 'a');
    cpl_propertylist_set_comment(plist, keys[0], comments[0]);

    cpl_propertylist_append_bool(plist, keys[1], 1);
    cpl_propertylist_set_comment(plist, keys[1], comments[1]);

    cpl_propertylist_append_int(plist, keys[2], -1);
    cpl_propertylist_set_comment(plist, keys[2], comments[2]);

    cpl_propertylist_append_long(plist, keys[3], 32768);
    cpl_propertylist_set_comment(plist, keys[3], comments[3]);

    cpl_propertylist_append_float(plist, keys[4], fval0);
    cpl_propertylist_set_comment(plist, keys[4], comments[4]);

    cpl_propertylist_append_double(plist, keys[5], dval0);
    cpl_propertylist_set_comment(plist, keys[5], comments[5]);

    cpl_propertylist_append_string(plist, keys[6], comments[6]);
    cpl_propertylist_set_comment(plist, keys[6], comments[6]);

    cpl_propertylist_append_float_complex(plist, keys[7], zf0);
    cpl_propertylist_set_comment(plist, keys[7], comments[7]);

    cpl_propertylist_append_double_complex(plist, keys[8], zd0);
    cpl_propertylist_set_comment(plist, keys[8], comments[8]);

    cpl_test_zero(cpl_propertylist_is_empty(plist));

    cpl_test_eq(cpl_propertylist_get_size(plist), nprops);

    pro1 = cpl_propertylist_get_const(plist, nprops-1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(pro1);

    nullprop = cpl_propertylist_get_const(plist, nprops);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(nullprop);

    code = cpl_propertylist_save(plist, BASE "_2.fits", CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_2.fits");

    _plist = cpl_propertylist_load(BASE "_2.fits", 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(_plist);
    /* Some properties are added */
    cpl_test_leq(nprops, cpl_propertylist_get_size(_plist));

    for (i = 0; i < nprops; i++) {
        const cpl_property * p1 =
            cpl_propertylist_get_property_const(plist, keys[i]);
        const cpl_property * p2 =
            cpl_propertylist_get_property_const(_plist, keys[i + nprops]);

        cpl_test_eq(i, i);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(p1);
        cpl_test_nonnull(p2);

        if (types[i] == CPL_TYPE_CHAR) {
            /* FITS I/O promotes this type to a single character string */
            cpl_test_eq(cpl_property_get_type(p2), CPL_TYPE_STRING);
            cpl_test_eq(cpl_property_get_string(p2)[0], cpl_property_get_char(p1));
        } else if (types[i] == CPL_TYPE_LONG) {
            /* FITS I/O casts this type to int */
            cpl_test_eq(cpl_property_get_type(p2), CPL_TYPE_INT);
            cpl_test_eq(cpl_property_get_int(p2), cpl_property_get_long(p1));
        } else if (types[i] == CPL_TYPE_FLOAT) {
            /* FITS I/O promotes this type to double */
            cpl_test_eq(cpl_property_get_type(p2), CPL_TYPE_DOUBLE);
            cpl_test_abs(cpl_property_get_double(p2),
                         cpl_property_get_float(p1), 2.0 * FLT_EPSILON);
        } else if (types[i] == CPL_TYPE_FLOAT_COMPLEX) {
            /* FITS I/O promotes this type to double complex */
            cpl_test_eq(cpl_property_get_type(p2), CPL_TYPE_DOUBLE_COMPLEX);
            zd1 = cpl_property_get_float_complex(p1);
            zd2 = cpl_property_get_double_complex(p2);
            cpl_test_abs_complex(zd1, zd2, 2.0 * FLT_EPSILON);
        } else {
            cpl_test_eq(cpl_property_get_type(p2), types[i]);
            if (types[i] == CPL_TYPE_BOOL) {
                cpl_test_eq(cpl_property_get_bool(p2),
                            cpl_property_get_bool(p1));
            } else if (types[i] == CPL_TYPE_INT) {
                cpl_test_eq(cpl_property_get_int(p2), cpl_property_get_int(p1));
            } else if (types[i] == CPL_TYPE_DOUBLE) {
                cpl_test_abs(cpl_property_get_double(p2),
                             cpl_property_get_double(p1), DBL_EPSILON);
            } else if (types[i] == CPL_TYPE_STRING) {
                cpl_test_eq_string(cpl_property_get_string(p2),
                                   cpl_property_get_string(p1));
            } else if (types[i] == CPL_TYPE_DOUBLE_COMPLEX) {
                zd1 = cpl_property_get_double_complex(p1);
                zd2 = cpl_property_get_double_complex(p2);
                cpl_test_abs_complex(zd1, zd2, 32.0 * DBL_EPSILON);
            }
        }

        cpl_test_zero(strncmp(cpl_property_get_comment(p2), comments[i],
                              strlen(cpl_property_get_comment(p2))));

    }

    cpl_propertylist_delete(_plist);

    for (i = 0; i < nprops; i++) {
        const cpl_property *p = cpl_propertylist_get_const(plist, i);

        cpl_test_eq_string(cpl_property_get_name(p), keys[i]);
        cpl_test_eq_string(cpl_property_get_comment(p), comments[i]);
        cpl_test_eq(cpl_property_get_type(p), types[i]);

        cpl_test(cpl_propertylist_has(plist, keys[i]));
        cpl_test_eq_string(cpl_propertylist_get_comment(plist, keys[i]),
                           comments[i]);
        cpl_test_eq(cpl_propertylist_get_type(plist, keys[i]), types[i]);

    }

    cpl_test_eq(cpl_propertylist_get_char(plist, keys[0]), 'a');
    cpl_test_eq(cpl_propertylist_get_bool(plist, keys[1]), 1);
    cpl_test_eq(cpl_propertylist_get_int(plist, keys[2]), -1);
    cpl_test_eq(cpl_propertylist_get_long(plist, keys[3]), 32768);

    fval1 = cpl_propertylist_get_float(plist, keys[4]);
    cpl_test_abs(fval0, fval1, 0.0);

    dval1 = cpl_propertylist_get_double(plist, keys[5]);
    cpl_test_abs(dval0, dval1, 0.0);

    cpl_test_eq_string(cpl_propertylist_get_string(plist, keys[6]),
                       comments[6]);

    /*
     * Test 3: Modify the values of the property list entries
     *         and verify the data.
     */

    cpl_test_zero(cpl_propertylist_set_char(plist, keys[0], 'b'));
    cpl_test_eq(cpl_propertylist_get_char(plist, keys[0]), 'b');

    cpl_test_zero(cpl_propertylist_set_bool(plist, keys[1], 0));
    cpl_test_zero(cpl_propertylist_get_bool(plist, keys[1]));

    cpl_test_zero(cpl_propertylist_set_int(plist, keys[2], -1));
    cpl_test_eq(cpl_propertylist_get_int(plist, keys[2]), -1);

    cpl_test_zero(cpl_propertylist_set_long(plist, keys[3], 1));
    cpl_test_eq(cpl_propertylist_get_long(plist, keys[3]), 1);

    fval1 = 9.87654321;
    cpl_test_noneq(fval0, fval1);
    cpl_test_zero(cpl_propertylist_set_float(plist, keys[4], fval1));
    fval2 = cpl_propertylist_get_float(plist, keys[4]);
    cpl_test_abs(fval1, fval2, 0.0);

    dval1 = -9.87654321;
    cpl_test_noneq(dval0, dval1);
    cpl_test_zero(cpl_propertylist_set_double(plist, keys[5], dval1));
    dval2 = cpl_propertylist_get_double(plist, keys[5]);
    cpl_test_abs(dval1, dval2, 0.0);

    cpl_test_zero(cpl_propertylist_set_string(plist, keys[6], comments[0]));
    cpl_test_eq_string(cpl_propertylist_get_string(plist, keys[6]),
                       comments[0]);

    zf1 = 9.87654321 * zf0;
    cpl_test_noneq(zf0, zf1);
    cpl_test_zero(cpl_propertylist_set_float_complex(plist, keys[7], zf1));
    zf2 = cpl_propertylist_get_float_complex(plist, keys[7]);
    cpl_test_abs_complex(zf1, zf2, 0.0);

    zd1 = -9.87654321 * zd0;
    cpl_test_noneq(zd0, zd1);
    cpl_test_zero(cpl_propertylist_set_double_complex(plist, keys[8], zd1));
    zd2 = cpl_propertylist_get_double_complex(plist, keys[8]);
    cpl_test_abs_complex(zd1, zd2, 0.0);


    /*
     * Test 4: Check that trying to modify an entry with a different
     *         type is properly failing.
     */

    code = cpl_propertylist_set_char(plist, keys[1], 'a');
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_bool(plist, keys[2], 1);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_int(plist, keys[3], 1);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_long(plist, keys[4], 1);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_float(plist, keys[5], 1.);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_double(plist, keys[6], 1.);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_string(plist, keys[0], comments[0]);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_float(plist, keys[7], 1.);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);

    code = cpl_propertylist_set_double(plist, keys[8], 1.);
    cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);


    /*
     * Test 5: Verify that values are inserted correctly into the property
     *         list.
     */

    cpl_test_eq(cpl_propertylist_insert_char(plist, keys[0],
                                           keys[0 + nprops], 'a'), 0);

    cpl_test_eq(cpl_propertylist_insert_after_char(plist, keys[0],
                                                 keys[0 + nprops], 'c'), 0);

    cpl_test_eq(cpl_propertylist_insert_bool(plist, keys[1],
                                           keys[1 + nprops], 0), 0);

    cpl_test_eq(cpl_propertylist_insert_after_bool(plist, keys[1],
                                                 keys[1 + nprops], 1), 0);


    cpl_test_eq(cpl_propertylist_insert_int(plist, keys[2],
                                          keys[2 + nprops], 0), 0);

    cpl_test_eq(cpl_propertylist_insert_after_int(plist, keys[2],
                                                keys[2 + nprops], 1), 0);


    cpl_test_eq(cpl_propertylist_insert_long(plist, keys[3], keys[3 + nprops],
                                           123456789), 0);

    cpl_test_eq(cpl_propertylist_insert_after_long(plist, keys[3],
                                                   keys[3 + nprops],
                                                   123456789), 0);


    cpl_test_eq(cpl_propertylist_insert_float(plist, keys[4], keys[4 + nprops],
                                            fval0), 0);

    cpl_test_eq(cpl_propertylist_insert_after_float(plist, keys[4],
                                                    keys[4 + nprops],
                                                  -fval0), 0);


    cpl_test_eq(cpl_propertylist_insert_double(plist, keys[5], keys[5 + nprops],
                                             dval0), 0);

    cpl_test_eq(cpl_propertylist_insert_after_double(plist, keys[5],
                                                     keys[5 + nprops],
                                                   -dval0), 0);


    cpl_test_eq(cpl_propertylist_insert_string(plist, keys[6],
                                             keys[6 + nprops], ""), 0);

    cpl_test_eq(cpl_propertylist_insert_after_string(plist, keys[6],
                                                   keys[6 + nprops], ""), 0);

    cpl_test_eq(cpl_propertylist_insert_float(plist, keys[7], keys[7 + nprops],
                                            fval0), 0);

    cpl_test_eq(cpl_propertylist_insert_after_float(plist, keys[7],
                                                    keys[7 + nprops],
                                                  -fval0), 0);


    cpl_test_eq(cpl_propertylist_insert_double(plist, keys[8], keys[8 + nprops],
                                             dval0), 0);

    cpl_test_eq(cpl_propertylist_insert_after_double(plist, keys[8],
                                                     keys[8 + nprops],
                                                   -dval0), 0);


    for (i = 0; i < nprops; i++) {
        cpl_property *p0 = cpl_propertylist_get(plist, 3 * i);
        cpl_property *p1 = cpl_propertylist_get(plist, 3 * i + 1);
        cpl_property *p2 = cpl_propertylist_get(plist, 3 * i + 2);


        cpl_test_eq_string(cpl_property_get_name(p0), keys[i + nprops]);
        cpl_test_eq_string(cpl_property_get_name(p1), keys[i]);
        cpl_test_eq_string(cpl_property_get_name(p2), keys[i + nprops]);

        switch (cpl_property_get_type(p0)) {
        case CPL_TYPE_CHAR:
            cpl_test_eq(cpl_property_get_char(p0), 'a');
            cpl_test_eq(cpl_property_get_char(p2), 'c');
            break;

        case CPL_TYPE_BOOL:
            cpl_test_zero(cpl_property_get_bool(p0));
            cpl_test_eq(cpl_property_get_bool(p2), 1);
            break;

        case CPL_TYPE_INT:
            cpl_test_zero(cpl_property_get_int(p0));
            cpl_test_zero(cpl_property_get_int(p0));
            break;

        case CPL_TYPE_LONG:
            cpl_test_eq(cpl_property_get_long(p0), 123456789);
            cpl_test_eq(cpl_property_get_long(p0), 123456789);
            break;

        case CPL_TYPE_FLOAT:
            fval1 = cpl_property_get_float(p0);
            cpl_test_abs(fval0, fval1, 0.0);

            fval1 = -cpl_property_get_float(p2);
            cpl_test_abs(fval0, fval1, 0.0);
            break;

        case CPL_TYPE_DOUBLE:
            dval1 = cpl_property_get_double(p0);
            cpl_test_abs(dval0, dval1, 0.0);

            dval1 = -cpl_property_get_double(p2);
            cpl_test_abs(dval0, dval1, 0.0);
            break;

        case CPL_TYPE_STRING:
            cpl_test_eq_string(cpl_property_get_string(p0), "");
            cpl_test_eq_string(cpl_property_get_string(p2), "");
            break;

        default:
            /* This point should never be reached */
            cpl_test(0);
            break;
        }
    }


    /*
     * Test 6: Verify that modification of or insertion at/after a non
     *         existing elements is reported correctly.
     */

    cpl_test_zero(cpl_propertylist_has(plist, NOKEY));

    code = cpl_propertylist_set_char(plist, NOKEY, 'a');
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    code = cpl_propertylist_set_bool(plist, NOKEY, 1);
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    code = cpl_propertylist_set_int(plist, NOKEY, 1);
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    code = cpl_propertylist_set_long(plist, NOKEY, 1);
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    code = cpl_propertylist_set_float(plist, NOKEY, 1);
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    code = cpl_propertylist_set_double(plist, NOKEY, 1);
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    code = cpl_propertylist_set_string(plist, NOKEY, "");
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    code = cpl_propertylist_insert_char(plist, NOKEY, "h", 'a');
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_bool(plist, NOKEY, "h", 1);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_int(plist, NOKEY, "h", 1);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_long(plist, NOKEY, "h", 1);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_float(plist, NOKEY, "h", 1.0);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_double(plist, NOKEY, "h", 1.0);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_string(plist, NOKEY, "h", "");
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_after_char(plist, NOKEY, "h", 'a');
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_after_bool(plist, NOKEY, "h", 1);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_after_int(plist, NOKEY, "h", 1);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_after_long(plist, NOKEY, "h", 1);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_after_float(plist, NOKEY, "h", 1.0);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_after_double(plist, NOKEY, "h", 1.0);
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    code = cpl_propertylist_insert_after_string(plist, NOKEY, "h", "");
    cpl_test_eq_error(code, CPL_ERROR_UNSPECIFIED);

    /*
     * Test 7: Create a copy of the property list and verify that original
     *         and copy are identical but do not share any resources.
     */

    _plist = cpl_propertylist_duplicate(plist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(_plist);

    plist2 = cpl_propertylist_duplicate(plist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(plist2);

    cpl_test_noneq_ptr(_plist, plist);
    cpl_test_noneq_ptr(plist2, plist);

    cpl_test_eq(cpl_propertylist_get_size(plist),
                cpl_propertylist_get_size(_plist));
    cpl_test_eq(cpl_propertylist_get_size(plist),
                cpl_propertylist_get_size(plist2));

    for (i = 0; i < cpl_propertylist_get_size(plist); i++) {
        cpl_property *p = cpl_propertylist_get(plist, i);
        cpl_property *_p = cpl_propertylist_get(_plist, i);

        cpl_test_noneq_ptr(cpl_property_get_name(p), cpl_property_get_name(_p));
        cpl_test_eq_string(cpl_property_get_name(p), cpl_property_get_name(_p));

        cpl_test_eq(cpl_property_get_size(p), cpl_property_get_size(_p));
        cpl_test_eq(cpl_property_get_type(p), cpl_property_get_type(_p));

        if (cpl_property_get_comment(p) == NULL) {
            cpl_test_null(cpl_property_get_comment(_p));
        } else {
            cpl_test_noneq_ptr(cpl_property_get_comment(p),
                               cpl_property_get_comment(_p));
            cpl_test_eq_string(cpl_property_get_comment(p),
                               cpl_property_get_comment(_p));
        }

        switch (cpl_property_get_type(p)) {
        case CPL_TYPE_CHAR:
            cpl_test_eq(cpl_property_get_char(p),
                      cpl_property_get_char(_p));
            break;

        case CPL_TYPE_BOOL:
            cpl_test_eq(cpl_property_get_bool(p),
                      cpl_property_get_bool(_p));
            break;

        case CPL_TYPE_INT:
            cpl_test_eq(cpl_property_get_int(p),
                      cpl_property_get_int(_p));
            break;

        case CPL_TYPE_LONG:
            cpl_test_eq(cpl_property_get_long(p),
                      cpl_property_get_long(_p));
            break;

        case CPL_TYPE_FLOAT:
            fval1 = cpl_property_get_float(p);
            fval2 = cpl_property_get_float(_p);
            cpl_test_abs(fval1, fval2, 0.0);
            break;

        case CPL_TYPE_DOUBLE:
            dval1 = cpl_property_get_double(p);
            dval2 = cpl_property_get_double(_p);
            cpl_test_abs(dval1, dval2, 0.0);
            break;

        case CPL_TYPE_STRING:
            cpl_test_eq_string(cpl_property_get_string(p),
                               cpl_property_get_string(_p));
            break;

        case CPL_TYPE_FLOAT_COMPLEX:
            zf1 = cpl_property_get_float_complex(p);
            zf2 = cpl_property_get_float_complex(_p);
            cpl_test_abs_complex(zf1, zf2, 0.0);
            break;

        case CPL_TYPE_DOUBLE_COMPLEX:
            zd1 = cpl_property_get_double_complex(p);
            zd2 = cpl_property_get_double_complex(_p);
            cpl_test_abs_complex(zd1, zd2, 0.0);
            break;

        default:
            /* This point should never be reached */
            cpl_test(0);
            break;
        }
    }

    cpl_propertylist_delete(_plist);

    cpl_test_error(CPL_ERROR_NONE);

    /*
     * Test 8: Erase elements from the property list and verify the list
     *         structure and the data. Each key exists twice in capital
     *         letters and once in lowercase letters.
     */

    for (i = 0; i < nprops; i++) {
        cpl_propertylist_erase(plist, keys[i + nprops]);
        cpl_test_eq(cpl_propertylist_has(plist, keys[i + nprops]), 1);

        cpl_propertylist_erase(plist, keys[i + nprops]);
        cpl_test_zero(cpl_propertylist_has(plist, keys[i + nprops]));
    }
    cpl_test_eq(cpl_propertylist_get_size(plist), nprops);

    for (i = 0; i < nprops; i++) {
        cpl_property *p = cpl_propertylist_get(plist, i);
        cpl_test_eq_string(cpl_property_get_name(p), keys[i]);
    }

    cpl_test_eq(cpl_propertylist_get_char(plist, keys[0]), 'b');
    cpl_test_zero(cpl_propertylist_get_bool(plist, keys[1]));
    cpl_test_eq(cpl_propertylist_get_int(plist, keys[2]), -1);
    cpl_test_eq(cpl_propertylist_get_long(plist, keys[3]), 1);

    fval1 = 9.87654321;
    cpl_test_noneq(fval0, fval1);
    fval2 = cpl_propertylist_get_float(plist, keys[4]);
    cpl_test_abs(fval1, fval2, 0.0);

    dval1 = -9.87654321;
    cpl_test_noneq(dval0, dval1);
    dval2 = cpl_propertylist_get_double(plist, keys[5]);
    cpl_test_abs(dval1, dval2, 0.0);

    cpl_test_eq_string(cpl_propertylist_get_string(plist, keys[6]),
                       comments[0]);

    /*
     * Test 9: Erase all elements from the property list and verify that
     *         the list is empty.
     */

    cpl_propertylist_empty(plist);

//    cpl_test_assert(cpl_propertylist_is_empty(plist));
    cpl_test(cpl_propertylist_is_empty(plist));
   cpl_test_zero(cpl_propertylist_get_size(plist));

    cpl_propertylist_delete(plist);

     /*
     * Test 10: Write a propertylist to a FITS file
     *          and save it in disk
     */

    code = cpl_propertylist_update_string(plist2,"ESO OBS DID", "BLABLA");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_update_string(plist2,"ESO OBS OBSERVER", "NAME");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_update_string(plist2,"ESO OBS PI-COI NAME", "NAME");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_update_string(plist2,"ESO INS GRAT NAME", "DODO");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_update_string(plist2,"ESO PRO CATG", "PRODUCT");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_update_int(plist2,"ESO TPL NEXP", 4);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_update_int(plist2, "TOFITS", 6);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_set_comment(plist2, "TOFITS", "Test TOFITS function");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    (void)remove(filename3);
    status = 0;

    fits_create_diskfile(&fptr, filename3, &status);
    cpl_test_zero(status);

    /*
    * Create simple HDU
    */
    fits_create_img(fptr, 8, 0, naxes, &status);
    cpl_test_zero(status);

    /*Save a propertylist to a FITS file*/
    code = cpl_propertylist_to_fitsfile(fptr, plist2, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    fits_close_file(fptr, &status);
    cpl_test_zero(status);
    cpl_test_fits(filename3);

    cpl_propertylist_delete(plist2);

    /* Test 11: Load back the propertylist from a FITS file using CFITSIO
    *          and compare it with a plist loaded using CPL.
    *         Retrieve a property usign its name
    */

    plist2 = cpl_propertylist_load(filename3, 0);
    cpl_test_nonnull(plist2);

    /* Try to load a non-existent extension and
       check that the proper error code is set */
    nulllist = cpl_propertylist_load(filename3, 10);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(nulllist);

    pro2 = cpl_propertylist_get_property(plist2, "TOFITS");
    cpl_test_nonnull(pro2);
    cpl_test_eq_string("TOFITS", cpl_property_get_name(pro2));
    cpl_test_eq(cpl_property_get_int(pro2), 6);

    status = 0;
    fits_open_diskfile(&_fptr, filename3, READONLY, &status);
    cpl_test_zero(status);

    _plist = cpl_propertylist_from_fitsfile(_fptr);
    cpl_test_nonnull(_plist);

    /* Check that the property of both lists are the same */
    pro = cpl_propertylist_get_property(_plist, "TOFITS");
    cpl_test_nonnull(pro);

    cpl_test_eq(cpl_property_get_int(pro), cpl_property_get_int(pro2));
    //    cpl_propertylist_delete(plist2);

    fits_close_file(_fptr, &status);
    cpl_test_zero(status);
    cpl_test_fits(filename3);

    /*
    * Test 12: Compare the properties of both lists
    */

    cpl_test_eq(cpl_propertylist_get_size(plist2),
              cpl_propertylist_get_size(_plist));

    for (i = 0; i < cpl_propertylist_get_size(plist2); i++) {
        const cpl_property *p = cpl_propertylist_get_const(plist2, i);
        cpl_property *_p = cpl_propertylist_get(_plist, i);

        cpl_test_eq_string(cpl_property_get_name(p),
                           cpl_property_get_name(_p));
        cpl_test_eq_string(cpl_property_get_comment(p),
                           cpl_property_get_comment(_p));
        cpl_test_eq(cpl_property_get_type(p), cpl_property_get_type(_p));

        switch (cpl_property_get_type(p)) {
        case CPL_TYPE_BOOL:
            cpl_test_eq(cpl_property_get_bool(p), cpl_property_get_bool(_p));
            break;

        case CPL_TYPE_INT:
            cpl_test_eq(cpl_property_get_int(p), cpl_property_get_int(_p));
            break;

        case CPL_TYPE_DOUBLE:
            cpl_test_eq(cpl_property_get_double(p),
                      cpl_property_get_double(_p));
            break;

        case CPL_TYPE_STRING:
            cpl_test_eq_string(cpl_property_get_string(p),
                             cpl_property_get_string(_p));
            break;

        case CPL_TYPE_FLOAT_COMPLEX: {
            const float complex cval1 = cpl_property_get_float_complex(p);
            const float complex cval2 = cpl_property_get_float_complex(_p);
            const float         rval1 = crealf(cval1);
            const float         ival1 = cimagf(cval1);
            const float         rval2 = crealf(cval2);
            const float         ival2 = cimagf(cval2);

            cpl_test_error(CPL_ERROR_NONE);

            cpl_test_rel(rval1, rval2, FLT_EPSILON);
            cpl_test_rel(ival1, ival2, FLT_EPSILON);

            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX: {
            const double complex cval1 = cpl_property_get_double_complex(p);
            const double complex cval2 = cpl_property_get_double_complex(_p);
            const double         rval1 = creal(cval1);
            const double         ival1 = cimag(cval1);
            const double         rval2 = creal(cval2);
            const double         ival2 = cimag(cval2);

            cpl_test_error(CPL_ERROR_NONE);

            cpl_test_rel(rval1, rval2, DBL_EPSILON);
            cpl_test_rel(ival1, ival2, DBL_EPSILON);

            break;
        }
        default:
            /* This point should never be reached */

            cpl_test_eq(cpl_property_get_type(p), CPL_TYPE_INVALID);
            cpl_test_noneq(cpl_property_get_type(p), CPL_TYPE_INVALID);

            cpl_property_dump(p,  stderr);
            cpl_property_dump(_p, stderr);

            break;
        }
    }

    cpl_propertylist_delete(_plist);
    plist = cpl_propertylist_duplicate(plist2);

    cpl_propertylist_delete(plist2);

    /*
     * Test 13: Copy all properties matching a given pattern from one
     *          property list to another.
     */

    _plist = cpl_propertylist_new();

    cpl_propertylist_copy_property_regexp(_plist, plist, "^ESO .*", 0);

    cpl_test(cpl_propertylist_has(_plist, "ESO OBS DID"));
    cpl_test(cpl_propertylist_has(_plist, "ESO OBS OBSERVER"));
    cpl_test(cpl_propertylist_has(_plist, "ESO OBS PI-COI NAME"));
    cpl_test(cpl_propertylist_has(_plist, "ESO INS GRAT NAME"));
    cpl_test(cpl_propertylist_has(_plist, "ESO PRO CATG"));
    cpl_test(cpl_propertylist_has(_plist, "ESO TPL NEXP"));

    cpl_propertylist_empty(_plist);
    cpl_test(cpl_propertylist_is_empty(_plist));

    cpl_propertylist_copy_property_regexp(_plist, plist, "^ESO .*", 1);
    cpl_test_zero(cpl_propertylist_has(_plist, "ESO OBS DID"));


    /*
     * Test 14a: Erase all properties matching the given pattern from the
     *          property list.
     */

    cpl_propertylist_empty(_plist);
    cpl_test(cpl_propertylist_is_empty(_plist));

    code = cpl_propertylist_copy_property_regexp(NULL, plist, ".", 0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_propertylist_copy_property_regexp(_plist, NULL, ".", 0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_propertylist_copy_property_regexp(_plist, plist, NULL, 0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_propertylist_copy_property_regexp(_plist, plist, ")|(", 1);
    cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);

    code = cpl_propertylist_copy_property_regexp(_plist, plist, "^ESO .*", 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_eq(cpl_propertylist_get_size(_plist), 6);

    value  = cpl_propertylist_erase_regexp(_plist, "^ESO OBS .*", 0);
    cpl_test_eq(value, 3);
    cpl_test_eq(cpl_propertylist_get_size(_plist), 3);

    value = cpl_propertylist_erase_regexp(_plist, "ESO TPL NEXP", 0);
    cpl_test_eq(value, 1);
    cpl_test_eq(cpl_propertylist_get_size(_plist), 2);

    /*
    *  Test 14b: Erase a non-existing property and check return.
                 When regexp is null, the return should be -1
    */
    value = cpl_propertylist_erase_regexp(_plist, "ESO TPL NEXP", 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(value);

    value = cpl_propertylist_erase_regexp(_plist, ")|(", 1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(value, -1);

    value = cpl_propertylist_erase_regexp(NULL, ".", 1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(value, -1);

    value = cpl_propertylist_erase_regexp(_plist, NULL, 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(value, -1);

    cpl_propertylist_delete(_plist);
    cpl_propertylist_delete(plist);


    /*
     * Test 15: Create a property list from a file. Only properties matching
     *          the given pattern are loaded.
     */

    plist = cpl_propertylist_load_regexp(filename3, 0, "^ESO .*", 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(plist);
    cpl_test_zero(cpl_propertylist_is_empty(plist));
    cpl_test_eq(cpl_propertylist_get_size(plist), 6);
    cpl_test(cpl_propertylist_has(plist, "ESO OBS DID"));
    cpl_test(cpl_propertylist_has(plist, "ESO OBS OBSERVER"));
    cpl_test(cpl_propertylist_has(plist, "ESO OBS PI-COI NAME"));
    cpl_test(cpl_propertylist_has(plist, "ESO INS GRAT NAME"));
    cpl_test(cpl_propertylist_has(plist, "ESO PRO CATG"));
    cpl_test(cpl_propertylist_has(plist, "ESO TPL NEXP"));

    cpl_propertylist_delete(plist);

    /*
     * Test 15b: Failure tests of cpl_propertylist_load{,_regexp}().
     */

    remove("no.fits");

    value = cpl_fits_count_extensions(filename3);
    cpl_test_leq(0, value);

    nulllist = cpl_propertylist_load(NULL, 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp(NULL, 0, ".", 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp(filename3, 0, NULL, 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load("no.fits", 0);
    cpl_test_error(CPL_ERROR_FILE_IO);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp("no.fits", 0, ".", 0);
    cpl_test_error(CPL_ERROR_FILE_IO);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load("/dev/null", 0);
    cpl_test_error(CPL_ERROR_BAD_FILE_FORMAT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp("/dev/null", 0, ".", 0);
    cpl_test_error(CPL_ERROR_BAD_FILE_FORMAT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load(filename3, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp(filename3, -1, ".", 0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load(filename3, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp(filename3, -1, ".", 0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load(filename3, value + 1);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp(filename3, value + 1, ".", 0);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(nulllist);

    nulllist = cpl_propertylist_load_regexp(filename3, 0, "\\", 0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nulllist);


    /*
     * Test 16: Append a property list to another.
     */

    plist = cpl_propertylist_new();
    _plist = cpl_propertylist_new();

    cpl_propertylist_append_char(plist, keys[0], 'a');
    cpl_propertylist_set_comment(plist, keys[0], comments[0]);

    cpl_propertylist_append_bool(plist, keys[1], 1);
    cpl_propertylist_set_comment(plist, keys[1], comments[1]);

    cpl_propertylist_append_int(plist, keys[2], -1);
    cpl_propertylist_set_comment(plist, keys[2], comments[2]);

    cpl_propertylist_append_long(plist, keys[3], 32768);
    cpl_propertylist_set_comment(plist, keys[3], comments[3]);

    cpl_propertylist_append_float(_plist, keys[4], fval0);
    cpl_propertylist_set_comment(_plist, keys[4], comments[4]);

    cpl_propertylist_append_double(_plist, keys[5], dval0);
    cpl_propertylist_set_comment(_plist, keys[5], comments[5]);

    cpl_propertylist_append_string(_plist, keys[6], comments[6]);
    cpl_propertylist_set_comment(_plist, keys[6], comments[6]);

    cpl_propertylist_append_float_complex(_plist, keys[7], zf0);
    cpl_propertylist_set_comment(_plist, keys[7], comments[7]);

    cpl_propertylist_append_double_complex(_plist, keys[8], zd0);
    cpl_propertylist_set_comment(_plist, keys[8], comments[8]);

    cpl_test_zero(cpl_propertylist_is_empty(plist));
    cpl_test_eq(cpl_propertylist_get_size(plist), 4);

    cpl_test_zero(cpl_propertylist_is_empty(_plist));
    cpl_test_eq(cpl_propertylist_get_size(_plist), 5);

    code = cpl_propertylist_append(plist, _plist);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_propertylist_is_empty(plist));
    cpl_test_eq(cpl_propertylist_get_size(plist), nprops);

    cpl_test_zero(cpl_propertylist_is_empty(_plist));
    cpl_test_eq(cpl_propertylist_get_size(_plist), 5);

    for (i = 0; i < cpl_propertylist_get_size(plist); i++) {
        cpl_property *p = cpl_propertylist_get(plist, i);

        cpl_test_eq_string(cpl_property_get_name(p), keys[i]);
        cpl_test_eq_string(cpl_property_get_comment(p), comments[i]);
        cpl_test_eq(cpl_property_get_type(p), types[i]);

        cpl_test(cpl_propertylist_has(plist, keys[i]));
        cpl_test_eq_string(cpl_propertylist_get_comment(plist, keys[i]),
                           comments[i]);
        cpl_test_eq(cpl_propertylist_get_type(plist, keys[i]), types[i]);
    }

    /*
    * Test 17: Sequentially access the elements of a property list
    */

    /* First element */

    /*    pro = cpl_propertylist_get_first(plist);
    cpl_test_nonnull(pro);

    cpl_test_eq_string(cpl_property_get_name(pro), keys[0]);
    cpl_test_eq_string(cpl_property_get_comment(pro), comments[0]);
    cpl_test_eq(cpl_property_get_type(pro), types[0]);*/

    /*
    * Test 18: Loop through all elements except the first,
    *           and check the results.
    */
    /*    for (i = 1; i < cpl_propertylist_get_size(plist); i++) {

        cpl_property *p = cpl_propertylist_get_next(plist);

        cpl_test_eq_string(cpl_property_get_name(p), keys[i]);
        cpl_test_eq_string(cpl_property_get_comment(p), comments[i]);
        cpl_test_eq(cpl_property_get_type(p), types[i]);

        cpl_test_assert(cpl_propertylist_has(plist, keys[i]));
        cpl_test_zero(strcmp(cpl_propertylist_get_comment(plist, keys[i]),
                          comments[i]));
        cpl_test_eq(cpl_propertylist_get_type(plist, keys[i]), types[i]);
     }*/

     /*
    * Test 19: Try to access the next element and see
    *           if the end of list is handled properly.
    */

    /*    pro = cpl_propertylist_get_next(plist);
    cpl_test_eq(pro, NULL);*/


/*
     * Test 20: Create a FITS header using a list containing a property with
     *          a name of length 80 characters (the length of a FITS card)
     */

    cpl_propertylist_empty(plist);

    code = cpl_propertylist_append_string(plist, longname, comments[6]);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_propertylist_delete(_plist);
    _plist = NULL;

    cpl_propertylist_delete(plist);
    plist = NULL;

    /*
    * Test 21:  Sort a property list
    */

    plist = cpl_propertylist_new();

    cpl_test_nonnull(plist);
    cpl_test(cpl_propertylist_is_empty(plist));
    cpl_test_zero(cpl_propertylist_get_size(plist));


    for(i = 0; i < 1; i++) {

        cpl_propertylist_append_int(plist, "FDEBACGH", -1);
        cpl_propertylist_set_comment(plist, "FDEBACGH", comments[2]);

        cpl_propertylist_append_char(plist, "CDEFGB1", 'a');
        cpl_propertylist_set_comment(plist, "CDEFGB1", comments[0]);

        cpl_propertylist_append_bool(plist, "ABCDEFGH", 1);
        cpl_propertylist_set_comment(plist, "ABCDEFGH", comments[1]);

        cpl_propertylist_append_float(plist, "ZZZGDBCA", fval0);
        cpl_propertylist_set_comment(plist, "ZZZGDBCA", comments[4]);

        cpl_propertylist_append_string(plist, "BBBBB2", comments[6]);
        cpl_propertylist_set_comment(plist, "BBBBB2", comments[6]);

        cpl_propertylist_append_long(plist, "HISTORY", 32768);
        cpl_propertylist_set_comment(plist, "HISTORY", comments[3]);

        cpl_propertylist_append_int(plist, "HISTORY", 0);
        cpl_propertylist_set_comment(plist, "HISTORY", comments[3]);

        cpl_propertylist_append_double(plist, "HISTORY", dval0);
        cpl_propertylist_set_comment(plist, "HISTORY", comments[5]);

    }

    cpl_test_zero(cpl_propertylist_is_empty(plist));
    cpl_test_eq(cpl_propertylist_get_size(plist), 8);

    code = cpl_propertylist_sort(plist, cpl_test_property_compare_name);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_propertylist_delete(plist);

    /*
    * Test 21a:  Save a propertylist which contains a HISTORY
    *            field without a value
    */

    plist = cpl_propertylist_new();
    cpl_propertylist_append_string(plist, "HISTORY", "HELLO");
    cpl_propertylist_append_string(plist, "HISTORY", "WORLD");
    cpl_propertylist_append_string(plist, "HISTORY", "");
    cpl_propertylist_append_string(plist, "HISTORY", "HELLO");
    cpl_propertylist_append_string(plist, "HISTORY", "AGAIN");

    cpl_propertylist_append_string(plist, "COMMENT", "HELLO");
    cpl_propertylist_append_string(plist, "COMMENT", "WORLD");
    cpl_propertylist_append_string(plist, "COMMENT", "");
    cpl_propertylist_append_string(plist, "COMMENT", "HELLO");
    cpl_propertylist_append_string(plist, "COMMENT", "AGAIN");
    cpl_test_error(CPL_ERROR_NONE);

    t = cpl_table_new(1);
    code = cpl_table_save(t, plist, NULL, BASE "_21a.fits", CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_21a.fits");

    /* Two additional COMMENT cards will be added, so count only HISTORY */
    plist2 = cpl_propertylist_load_regexp(BASE "_21a.fits", 0,
                                          "HISTORY", 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(plist2);

    cpl_test_eq(cpl_propertylist_get_size(plist),
                cpl_propertylist_get_size(plist2) * 2);

    cpl_propertylist_delete(plist2);
    cpl_propertylist_delete(plist);
    cpl_table_delete(t);

    /*
     * Test 21b: load plist from an image and sort it.
     * This test was compared with result from cpl_propertylist_sort().
     * Results are expected to be the same.
     */

    plist = cpl_propertylist_load(BASE "_image.fits", 0);

    if (plist == NULL) {
        cpl_test_error(CPL_ERROR_FILE_IO);
    } else {

        /* Inactive per default */

        code = cpl_propertylist_sort(plist, cpl_test_property_compare_name);
        cpl_test_eq_error(code, CPL_ERROR_NONE);


        code = cpl_image_save(NULL, BASE "_sorted.fits", CPL_TYPE_UCHAR,
                              plist, CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_test_fits(BASE "_sorted.fits");
        cpl_test_zero(remove(BASE "_sorted.fits"));

        cpl_propertylist_delete(plist);
    }


    /*
     * Test 21c:  Save a very long property list into a FITS file
     */

    plist = cpl_propertylist_new();

    cpl_test_nonnull(plist);
    cpl_test(cpl_propertylist_is_empty(plist));
    cpl_test_zero(cpl_propertylist_get_size(plist));

    bench_size = do_bench ? 100000 : 10000;

    for(i = 0; i < bench_size; i++) {
        cpl_propertylist_append_float(plist, keys[4], fval0);
        cpl_propertylist_set_comment(plist, keys[4], comments[4]);

        cpl_propertylist_append_int(plist, keys[2], -1);
        cpl_propertylist_set_comment(plist, keys[2], comments[2]);

        cpl_propertylist_append_char(plist, keys[0], 'a');
        cpl_propertylist_set_comment(plist, keys[0], comments[0]);

        cpl_propertylist_append_bool(plist, keys[1], 1);
        cpl_propertylist_set_comment(plist, keys[1], comments[1]);

        cpl_propertylist_append_string(plist, keys[6], comments[6]);
        cpl_propertylist_set_comment(plist, keys[6], comments[6]);

        cpl_propertylist_append_long(plist, keys[3], 32768);
        cpl_propertylist_set_comment(plist, keys[3], comments[3]);

        cpl_propertylist_append_double(plist, keys[5], dval0);
        cpl_propertylist_set_comment(plist, keys[5], comments[5]);

        cpl_propertylist_append_float_complex(plist, keys[7], fval0);
        cpl_propertylist_set_comment(plist, keys[7], comments[7]);

        cpl_propertylist_append_double_complex(plist, keys[8], dval0);
        cpl_propertylist_set_comment(plist, keys[8], comments[8]);

    }

    cpl_test_zero(cpl_propertylist_is_empty(plist));
    cpl_test_eq(cpl_propertylist_get_size(plist), nprops * bench_size);

    remove(filename3);
    status = 0;

    fits_create_diskfile(&fptr, filename3, &status);
    cpl_test_zero(status);

    fits_create_img(fptr, 8, 0, naxes, &status);
    cpl_test_zero(status);

    code = cpl_propertylist_to_fitsfile(fptr, plist, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    fits_close_file(fptr, &status);
    cpl_test_zero(status);
    cpl_test_fits(filename3);

    cpl_propertylist_delete(plist);

    /*
     *   Test 22: Save a property list into a FITS file using
     *            cpl_propertylist_save()
     */

    plist = cpl_propertylist_new();
    cpl_propertylist_append_char(plist, keys[0], 'a');
    cpl_propertylist_set_comment(plist, keys[0], comments[0]);

    cpl_propertylist_append_bool(plist, keys[1], 1);
    cpl_propertylist_set_comment(plist, keys[1], comments[1]);

    cpl_propertylist_append_int(plist, keys[2], -1);
    cpl_propertylist_set_comment(plist, keys[2], comments[2]);

    cpl_propertylist_append_long(plist, keys[3], 32768);
    cpl_propertylist_set_comment(plist, keys[3], comments[3]);

    cpl_propertylist_append_float(plist, keys[4], fval0);
    cpl_propertylist_set_comment(plist, keys[4], comments[4]);

    cpl_propertylist_append_double(plist, keys[5], dval0);
    cpl_propertylist_set_comment(plist, keys[5], comments[5]);

    cpl_propertylist_append_string(plist, keys[6], comments[6]);
    cpl_propertylist_set_comment(plist, keys[6], comments[6]);

    cpl_propertylist_append_float_complex(plist, keys[7], zf0);
    cpl_propertylist_set_comment(plist, keys[7], comments[7]);

    cpl_propertylist_append_double_complex(plist, keys[8], zd0);
    cpl_propertylist_set_comment(plist, keys[8], comments[8]);

    cpl_test_zero(cpl_propertylist_is_empty(plist));
    cpl_test_eq(cpl_propertylist_get_size(plist), nprops);


    code = cpl_propertylist_save(plist, BASE "_22.fits", CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_22.fits");

    code = cpl_propertylist_save(plist, BASE "_22.fits", CPL_IO_APPEND);
    cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_fits(BASE "_22.fits");

    cpl_propertylist_update_string(plist, keys[6], "updated string");
    code = cpl_propertylist_save(plist, BASE "_22.fits", CPL_IO_EXTEND);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_22.fits");

    cpl_propertylist_delete(plist);

    /*
     * Save a NULL property list to an extension
     */

    remove(BASE "_null.fits");
    plist = cpl_propertylist_new();
    code = cpl_propertylist_save(plist, BASE "_null.fits", CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_null.fits");

    code = cpl_propertylist_save(NULL, BASE "_null.fits", CPL_IO_EXTEND);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_null.fits");
    cpl_propertylist_delete(plist);

    /*
     * Check that XTENSION is set to IMAGE
     */

    plist = cpl_propertylist_load(BASE "_null.fits",1);
    cpl_test_eq_string("IMAGE", cpl_propertylist_get_string(plist, "XTENSION"));
    cpl_propertylist_delete(plist);

    /*
    *   Test 23: Save a property list to a FITS file using
    *            a regexp to remove keys
    */
    remove(filename3);
    status = 0;

    fits_create_diskfile(&fptr, filename3, &status);
    cpl_test_zero(status);

    /*
    * Create simple HDU
    */
    fits_create_img(fptr, 8, 0, naxes, &status);
    cpl_test_zero(status);

    plist = cpl_propertylist_new();
    cpl_propertylist_append_char(plist, keys[0], 'a');
    cpl_propertylist_set_comment(plist, keys[0], comments[0]);

    cpl_propertylist_append_bool(plist, keys[1], 1);
    cpl_propertylist_set_comment(plist, keys[1], comments[1]);

    cpl_propertylist_append_int(plist, keys[2], -1);
    cpl_propertylist_set_comment(plist, keys[2], comments[2]);

    cpl_propertylist_append_long(plist, "NBXIS1", 32768);
    cpl_propertylist_set_comment(plist, "NBXIS1", comments[3]);

    cpl_propertylist_append_float(plist, keys[4], fval0);
    cpl_propertylist_set_comment(plist, keys[4], comments[4]);

    cpl_propertylist_append_double(plist, keys[5], dval0);
    cpl_propertylist_set_comment(plist, keys[5], comments[5]);

    cpl_propertylist_append_string(plist, "HIERARCH ESO TEST1", "One string without any comment");
    cpl_propertylist_append_string(plist, "HIERARCH ESO TEST2", "Two string without any comment");


    cpl_propertylist_append_float_complex(plist, keys[7], zf0);
    cpl_propertylist_set_comment(plist, keys[7], comments[7]);

    cpl_propertylist_append_double_complex(plist, keys[8], zd0);
    cpl_propertylist_set_comment(plist, keys[8], comments[8]);

    cpl_propertylist_append_long(plist, "NBXIS2", 32769);
    cpl_propertylist_set_comment(plist, "NBXIS2", comments[3]);

    cpl_propertylist_append_string(plist, "HISTORY", "One history string without any comment");
    cpl_propertylist_append_string(plist, "HISTORY", "Two history string without any comment");
    cpl_propertylist_append_string(plist, "HISTORY", "Three history string without any comment");

#ifdef CPL_FITS_TEST_NON_STRING_HISTORY_COMMENT
    /* This will produce a pretty strange FITS header */

    /* Check handling of HISTORY of non-string type */
    cpl_propertylist_append_float(plist, "HISTORY", 4.0);

    /* Check handling of COMMENT of non-string type */
    cpl_propertylist_append_float(plist, "COMMENT", 5.0);

    cpl_test_eq(cpl_propertylist_get_size(plist), 16);
#else
    cpl_test_eq(cpl_propertylist_get_size(plist), 14);
#endif
    cpl_test_zero(cpl_propertylist_is_empty(plist));

    /* Remove the following keys from the saving
       HIERARCH ESO |NBXIS1|NBXIS2|HISTORY
    */

    code = cpl_propertylist_to_fitsfile(fptr, plist, ")|(");
    cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);

    code = cpl_propertylist_to_fitsfile(fptr, plist, to_rm);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Pass a null regexp to the function*/
    code = cpl_propertylist_to_fitsfile(fptr, plist, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Pass an empty property list to the function */
    cpl_propertylist_empty(plist);
    code = cpl_propertylist_to_fitsfile(fptr, plist, to_rm);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_to_fitsfile(fptr, NULL, to_rm);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_propertylist_to_fitsfile(NULL, plist, to_rm);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    fits_close_file(fptr, &status);
    cpl_test_zero(status);
    cpl_test_fits(filename3);

    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(filename3, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(plist);

    /*
    *   Test 24: Create a property list with compressed
    *            keywords and save it in disk and in a table.
    *            Check if compressed
    *            keywords are removed after saving it.
    */

    remove(BASE "_24.fits");
    cpl_propertylist_empty(plist);
    cpl_propertylist_append_string(plist,"ZTENSION", "COMPRESSED_EMPTY");
    cpl_propertylist_append_long(plist, "ZNAXIS", 2);
    cpl_propertylist_set_comment(plist, "ZNAXIS", "compressed NAXIS");
    cpl_propertylist_append_long(plist, "ZNAXIS1", 2048);
    cpl_propertylist_set_comment(plist, "ZNAXIS1", "compressed NAXIS1");
    cpl_propertylist_append_long(plist, "ZNAXIS2", 1024);
    cpl_propertylist_set_comment(plist, "ZNAXIS2", "compressed NAXIS2");
    cpl_test_zero(cpl_propertylist_is_empty(plist));
    cpl_test_eq(cpl_propertylist_get_size(plist), 4);


    /* Save it in disk and check if compression keywords are removed */
    code = cpl_propertylist_save(plist, BASE "_24.fits", CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_24.fits");
    code = cpl_propertylist_save(plist, BASE "_24.fits", CPL_IO_EXTEND);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_24.fits");
//    cpl_propertylist_delete(plist);

    /* primary header */
    plist2 = cpl_propertylist_load(BASE "_24.fits", 0);
    cpl_test_zero(cpl_propertylist_has(plist2, "ZTENSION"));
    cpl_propertylist_delete(plist2);

    /* extension header */
    plist2 = cpl_propertylist_load(BASE "_24.fits", 1);
    cpl_test_zero(cpl_propertylist_has(plist2, "ZNAXIS"));
    cpl_propertylist_delete(plist2);


    (void)remove(BASE "_24.fits");
    t = cpl_table_new(1);

    /* Compressed keywords should be removed when saving table*/
    code = cpl_table_save(t, plist, NULL, BASE "_24.fits", CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_24.fits");
    code = cpl_table_save(t, NULL, plist, BASE "_24.fits", CPL_IO_EXTEND);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_24.fits");
    cpl_propertylist_delete(plist);

    /* Check if compressed keywords were removed from headers*/
    plist = cpl_propertylist_load(BASE "_24.fits",1);
    cpl_test_zero(cpl_propertylist_has(plist, "ZTENSION"));
    cpl_test_zero(cpl_propertylist_has(plist, "ZNAXIS"));
    cpl_test_zero(cpl_propertylist_has(plist, "ZNAXIS1"));
    cpl_test_zero(cpl_propertylist_has(plist, "ZNAXIS2"));
    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(BASE "_24.fits",2);
    cpl_test_zero(cpl_propertylist_has(plist, "ZTENSION"));
    cpl_test_zero(cpl_propertylist_has(plist, "ZNAXIS"));
    cpl_test_zero(cpl_propertylist_has(plist, "ZNAXIS1"));
    cpl_test_zero(cpl_propertylist_has(plist, "ZNAXIS2"));

    /* Test propertylist dump */
    cpl_propertylist_dump(plist, stream);

    cpl_table_delete(t);
    cpl_propertylist_delete(plist);

    /*
     *  Test 25: Test the new error message function
     */

    plist = cpl_propertylist_load(BASE "_null.fits", 2);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);

    /*
     *  Test 26: Verify loading of a blank card followed by a normal card
     *           - also, insert a couple of cards with an empty key
     */

    plist = cpl_propertylist_new();
    code = cpl_propertylist_append_string(plist, "", "emptykey: 0");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, " ", "emptykey: 1");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "  ", "emptykey: 2");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "   ", "emptykey: 3");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "    ", "emptykey: 4");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "     ", "emptykey: 5");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "      ", "emptykey: 6");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "       ", "emptykey: 7");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "        ", "emptykey: 8");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(plist, "DELKEY", "DELVALUE");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Add a normal card, otherwise CFITSIO may ignore the blank on load */
    code = cpl_propertylist_append_string(plist, "LASTKEY", "LASTKEY");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_save(plist, BASE "_26.fits", CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_fits(BASE "_26.fits");
    cpl_propertylist_delete(plist);

#ifdef CPL_NO_PERL
    plist = cpl_propertylist_load(BASE "_26.fits", 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(plist);

#else

    /* Need to close file due to non CPL access */
    cpl_test_zero(cpl_io_fits_close(BASE "_26.fits", &status));
    cpl_test_zero(status);

    /* Create a completely blank card */
    syscode = system("perl -pi -e 's/(DELKEY += +.DELVALUE.)/"
                     "sprintf(\"%\".length($1).\"s\", \" \")/e' "
                     BASE "_26.fits");
    cpl_test_zero(syscode);

    /* Load cards with no letter in key, i.e. nine cards */
    plist = cpl_propertylist_load_regexp(BASE "_26.fits", 0, "[A-Z]", 1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(plist);

    cpl_test_eq(cpl_propertylist_get_size(plist), 9);

#endif
    cpl_propertylist_delete(plist);


    /*
     * Test 27: Append a property to a property list
     */

    plist = cpl_propertylist_new();

    /* Catch error code */
    code = cpl_propertylist_append_property(plist, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    for(j = 0; j < 6; j++) {
        char *name = cpl_sprintf("HIERARCH ESO TEST%d APPEND", j);
        cpl_property *p = cpl_property_new(name, CPL_TYPE_INT);
        cpl_free(name);

        cpl_test_nonnull(p);
        cpl_test_zero(cpl_property_set_int(p, j+2));
        cpl_test_zero(cpl_propertylist_append_property(plist, p));
        cpl_property_delete(p);
    }

    cpl_test_eq(cpl_propertylist_get_size(plist),6);
    cpl_test_eq(cpl_propertylist_get_int(plist,"HIERARCH ESO TEST1 APPEND"),3);

     /*
      * Test 28: Prepend a property to a property list
      */


     /* Catch error code */
    code = cpl_propertylist_prepend_property(plist, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    for(j = 0; j < 3; j++) {
        char *name = cpl_sprintf("HIERARCH ESO TEST%d PREPEND",j);
        cpl_property *p = cpl_property_new(name,CPL_TYPE_STRING);
           cpl_free(name);
        cpl_test_nonnull(p);
        cpl_test_zero(cpl_property_set_string(p,"test prepend"));
        cpl_test_zero(cpl_propertylist_prepend_property(plist,p));
        cpl_property_delete(p);
    }

    cpl_test_eq(cpl_propertylist_get_size(plist),9);


     /* Test 29: Insert a property into a property list */

    code = cpl_propertylist_insert_property(plist, "HIERARCH ESO TEST0 APPEND",
                                            NULL);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    for(j = 0; j < 1; j++){
        const char *name = "HIERARCH ESO TEST0 APPEND";
        cpl_property *p = cpl_property_new("INSERT1", CPL_TYPE_FLOAT);
        cpl_property *p1 = cpl_property_new("INSERT2", CPL_TYPE_FLOAT);
        cpl_test_nonnull(p);
        cpl_test_nonnull(p1);
        cpl_test_zero(cpl_property_set_float(p, 1.0));
        cpl_test_zero(cpl_propertylist_insert_property(plist,name,p));
        cpl_test_zero(cpl_property_set_float(p1, 2.0));
        cpl_test_zero(cpl_propertylist_insert_after_property(plist,name,p1));
        cpl_property_delete(p);
        cpl_property_delete(p1);
    }

    cpl_test_eq(cpl_propertylist_get_size(plist),11);

//    cpl_propertylist_dump(plist,stdout);
    cpl_propertylist_delete(plist);


    /*
     * Test 30: Test the casting on the float and double accessors
     */

    plist = cpl_propertylist_new();
    cpl_test_zero(cpl_propertylist_update_string(plist,"C","String"));
    cpl_test_zero(cpl_propertylist_update_int(plist,"I",8));
    f0 = 235.89;
    cpl_test_zero(cpl_propertylist_update_float(plist,"F",f0));
    d0 = 1234.9994048;
    cpl_test_zero(cpl_propertylist_update_double(plist,"D",d0));
    cpl_test_zero(cpl_propertylist_save(plist,BASE "_30.fits",CPL_IO_CREATE));
    cpl_test_fits(BASE "_30.fits");

    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(BASE "_30.fits",0);
    cpl_test_nonnull(plist);

    /* The float keyword is casted to double when loaded from disk */
    t1 = cpl_propertylist_get_type(plist,"F");
    cpl_test_eq(t1,CPL_TYPE_DOUBLE);

    /* Test that the casting in cpl works */
    f1 = cpl_propertylist_get_float(plist, "F");
    cpl_test_error(CPL_ERROR_NONE);

    d1 = cpl_propertylist_get_double(plist, "F");
    cpl_test_error(CPL_ERROR_NONE);

    f2 = cpl_propertylist_get_float(plist,"D");
    cpl_test_error(CPL_ERROR_NONE);

    d2 = cpl_propertylist_get_double(plist, "D");
    cpl_test_error(CPL_ERROR_NONE);

    /* Test the values */
    cpl_test_abs(f0, f1, 0.0);
    cpl_test_rel(d0, d2, DBL_EPSILON);
    cpl_test_rel(d1, f1, FLT_EPSILON);
    cpl_test_rel(d2, f2, FLT_EPSILON);


    /*
     * Test 31: Verify the truncation of a long string in I/O
     */

    cpl_propertylist_empty(plist);

    code = cpl_propertylist_append_string(plist, "ESO PRO REC1 PIPE ID",
                                          LONGNAME80);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_save(plist, BASE "_31.fits", CPL_IO_CREATE);
    cpl_test_fits(BASE "_31.fits");

    plist2 = cpl_propertylist_load(BASE "_31.fits", 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_nonnull(plist2);

    strval = cpl_propertylist_get_string(plist2, "ESO PRO REC1 PIPE ID");
    cpl_test_nonnull(strval);

    if (strval != NULL) {
        value = strlen(strval);
        cpl_test_leq(value, 80);

        cpl_test_zero(strncmp(strval, LONGNAME80, value));
    }

    cpl_propertylist_delete(plist2);
    cpl_propertylist_empty(plist);

    /*
     * Test 32: Verify the handling of setting a string property to NULL
     */

    code = cpl_propertylist_update_string(plist, keys[6], NULL);
    cpl_test_eq_error(CPL_ERROR_NULL_INPUT, code);

    code = cpl_propertylist_append_string(plist, keys[6], comments[6]);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_set_string(plist, keys[6], NULL);
    cpl_test_eq_error(CPL_ERROR_NULL_INPUT, code);

    code = cpl_propertylist_insert_string(plist, keys[6], keys[6 + nprops],
                                          NULL);
    cpl_test_eq_error(CPL_ERROR_NULL_INPUT, code);

    code = cpl_propertylist_insert_after_string(plist, keys[6],
                                          keys[6 + nprops], NULL);
    cpl_test_eq_error(CPL_ERROR_NULL_INPUT, code);

    code = cpl_propertylist_prepend_string(plist, keys[6], NULL);
    cpl_test_eq_error(CPL_ERROR_NULL_INPUT, code);


    /*
     * Test 33: Verify the handling of various numerical cards
     */

    cpl_propertylist_test_numeric_type();

    cpl_propertylist_test_numeric_load_ok();
    cpl_propertylist_test_numeric_load_ok2();
    cpl_propertylist_test_numeric_load_ok3();

    /*
     * Test 34: Verify the handling of various special cards
     */

    cpl_propertylist_empty(plist);

    /* Create a card with an undefined value (FITS std. 4.2.1 1) */
    code = cpl_propertylist_append_string(plist, "DELKEY1", "DELVALUE");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_set_comment(plist, "DELKEY1", "Undefined value"
                                        " - also for the next card...");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Create a card with an undefined value - with no comment */
    code = cpl_propertylist_append_string(plist, "DELKEY2", "DELVALUE");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    memset(stringarray, '\'', FLEN_VALUE - 1);
    stringarray[FLEN_VALUE - 1] = 0;

    code = cpl_propertylist_append_string(plist, "MYQUOTE", stringarray);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_int(plist, "COMMENT", 77777);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_int(plist, "HISTORY", 77777);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_int(plist, "       ", 77777);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* This key is at its maximum length,  */
    code = cpl_propertylist_append_double(plist, "HIERARCH ESO "
                                          "ABCDEFGHIJ ABCDEFGHIJ ABCDEFGHIJ "
                                          "ABCDEFGHIJ ABCDEFGHIJ AB",
                                          1.23E-45);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_save(plist, BASE "_34.fits", CPL_IO_CREATE);
    cpl_test_fits(BASE "_34.fits");

    syscode = system("perl -pi -e 's/(DELKEY. +=)( +.DELVALUE.)/"
                     "sprintf(\"$1%\".length($2).\"s\", \" \")/eg;"
                     "s/1.23E-45/1.23D-45/i;' "
                     BASE "_34.fits");
    cpl_test_zero(syscode);

    plist2 = cpl_propertylist_load(BASE "_34.fits", 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_nonnull(plist2);

    strval = cpl_propertylist_get_string(plist2, "MYQUOTE");
    cpl_test_nonnull(strval);

    if (strval != NULL) {
        /* The FITS quote-encoding means that only half make it into the card */
        value = strlen(strval);
        cpl_test_eq(value, (FLEN_VALUE)/2 - 1);
        cpl_test_zero(memcmp(strval, stringarray, value));
    }

    t1 = cpl_propertylist_get_type(plist2, "HISTORY");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(t1, CPL_TYPE_STRING);

    strval = cpl_propertylist_get_string(plist2, "HISTORY");
    cpl_test_nonnull(strval);

    if (strval != NULL) {
        /* The FITS HISTORY card should keep its not-a-value indicator */
        const char * start;
        value = strlen(strval);
        cpl_test_leq(7, value);
        cpl_test_zero(memcmp(strval, "= ", 2));
        start = strstr(strval, "77777");
        cpl_test_nonnull(start);
        cpl_test_eq(*(start - 1), ' ');
        cpl_test_eq(*(start + 5), '\0');
    }

    dval1 = cpl_propertylist_get_double(plist, "HIERARCH ESO "
                                        "ABCDEFGHIJ ABCDEFGHIJ ABCDEFGHIJ "
                                        "ABCDEFGHIJ ABCDEFGHIJ AB");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_rel(dval1, 1.23E-45, 0.001);

    cpl_propertylist_test_numeric_load_bad();

#undef MYTEST
#define MYTEST 42

#undef MYFILE
#define MYFILE FILENAME(MYTEST)

    for (int isign = -1; isign < 2; isign++) {
        const double badval = (double)isign / 0.0;
        cpl_propertylist_empty(plist);
        /* This floating point value cannot be saved */
        code = cpl_propertylist_append_float(plist, "BADFLOAT", badval);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_fits(MYFILE);

        cpl_propertylist_empty(plist);
        /* This floating point value cannot be saved */
        code = cpl_propertylist_append_double(plist, "BADFLOAT", badval);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_fits(MYFILE);

        cpl_propertylist_empty(plist);
        /* This floating point value cannot be saved */
        code = cpl_propertylist_append_float_complex(plist, "BADFLOAT", badval);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_fits(MYFILE);

        cpl_propertylist_empty(plist);
        /* This floating point value cannot be saved */
        code = cpl_propertylist_append_double_complex(plist, "BADFLOAT",
                                                      badval);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_fits(MYFILE);
    }

    cpl_propertylist_delete(plist);
    cpl_propertylist_delete(plist2);

    /*
     * All tests done
     */

    if (stream != stdout) cpl_test_zero( fclose(stream) );

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compare each property in the 1st list against the same in the 2nd
  @param plist1 The 1st property list
  @param plist2 The 2nd property list
  @return Nothing
  @note This comparison is not commutative

 */
/*----------------------------------------------------------------------------*/
static void cpl_propertylist_compare(const cpl_propertylist * plist1,
                                     const cpl_propertylist * plist2)
{

    const cpl_size sz1 = cpl_propertylist_get_size(plist1);
    const cpl_size sz2 = cpl_propertylist_get_size(plist2);


    for (cpl_size j = 0; j < sz1; j++) {
        const cpl_size      nfailed = cpl_test_get_failed();
        const cpl_property *p1      = cpl_propertylist_get_const(plist1, j);
        const char         *name    = cpl_property_get_name(p1);
        const cpl_property *p2      =
            cpl_propertylist_get_property_const(plist2, name);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(p2);

        if (p2 != NULL) {
            cpl_property_eq(p1, p2);
        }

        if (cpl_test_get_failed() != nfailed) {
            cpl_msg_warning(cpl_func, "FAILED(%d/%d<=>%d): %s",
                            (int)j, (int)sz1, (int)sz2, name);
        }
    }
}


static void cpl_property_eq(const cpl_property * self,
                            const cpl_property * other)
{
    const cpl_size nfailed = cpl_test_get_failed();
    const char   * name1   = cpl_property_get_name(self); 
    const char   * name2   = cpl_property_get_name(other);

    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_nonnull(name1);
    cpl_test_nonnull(name2);

    if (cpl_test_get_failed() == nfailed) {

        cpl_test_eq_string(name1, name2);

        if (cpl_test_get_failed() == nfailed) {
            const cpl_type t1 = cpl_property_get_type(self);
            const cpl_type t2 = cpl_property_get_type(other);

            cpl_test_eq(t1, t2);

            if (cpl_test_get_failed() == nfailed) {
                switch (t1) {
                case CPL_TYPE_BOOL: {
                    const int v1 = cpl_property_get_bool(self);
                    const int v2 = cpl_property_get_bool(other);
                    cpl_test_eq(v1, v2);
                    break;
                }
                case CPL_TYPE_CHAR: {
                    const char v1 = cpl_property_get_char(self);
                    const char v2 = cpl_property_get_char(other);
                    cpl_test_eq(v1, v2);
                    break;
                }
                case CPL_TYPE_INT: {
                    const int v1 = cpl_property_get_int(self);
                    const int v2 = cpl_property_get_int(other);
                    cpl_test_eq(v1, v2);
                    break;
                }
                case CPL_TYPE_LONG: {
                    const long v1 = cpl_property_get_long(self);
                    const long v2 = cpl_property_get_long(other);
                    cpl_test_eq(v1, v2);
                    break;
                }
                case CPL_TYPE_LONG_LONG: {
                    const long long v1 = cpl_property_get_long_long(self);
                    const long long v2 = cpl_property_get_long_long(other);
                    cpl_test_eq(v1, v2);
                    break;
                }
                case CPL_TYPE_FLOAT: {
                    const float v1 = cpl_property_get_float(self);
                    const float v2 = cpl_property_get_float(other);
                    cpl_test_rel(v1, v2, FLT_EPSILON * 4.0);
                    break;
                }
                case CPL_TYPE_DOUBLE: {
                    const double v1 = cpl_property_get_double(self);
                    const double v2 = cpl_property_get_double(other);
                    cpl_test_rel(v1, v2, DBL_EPSILON * 12.0);
                    break;
                }
                case CPL_TYPE_STRING: {
                    const char * v1 = cpl_property_get_string(self);
                    const char * v2 = cpl_property_get_string(other);

                    cpl_test_eq_string(v1, v2);
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX: {
                    const double complex v1 =
                        cpl_property_get_double_complex(self);
                    const double complex v2 =
                        cpl_property_get_double_complex(other);
                    const double v1r = creal(v1);
                    const double v2r = creal(v2);
                    const double v1i = cimag(v1);
                    const double v2i = cimag(v2);

                    cpl_test_rel(v1r, v2r, DBL_EPSILON * 4.0);
                    cpl_test_rel(v1i, v2i, DBL_EPSILON * 4.0);
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX: {
                    const float complex v1 =
                        cpl_property_get_float_complex(self);
                    const float complex v2 =
                        cpl_property_get_float_complex(other);
                    const float v1r = crealf(v1);
                    const float v2r = crealf(v2);
                    const float v1i = cimagf(v1);
                    const float v2i = cimagf(v2);

                    cpl_test_rel(v1r, v2r, FLT_EPSILON * 4.0);
                    cpl_test_rel(v1i, v2i, FLT_EPSILON * 4.0);
                    break;
                }

                default:
                    break;
                }
            }
        }
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compare two properties
  @param    p1  First property to compare
  @param    p2  Second property to compare
  @return  An integer less than,  equal to, or greater than zero if
           the name of p1 is found, respectively, to be less than,
           to match, or be greater than that of p2.
  @see cpl_property_get_name(), strcmp()

 */
/*----------------------------------------------------------------------------*/
static int cpl_test_property_compare_name(
        const cpl_property  *   p1,
        const cpl_property  *   p2)
{

    /*
    * Compare the two properties
    */
    return strcmp(cpl_property_get_name(p1),
                  cpl_property_get_name(p2));
}

#undef MYTEST
#define MYTEST 0

#undef MYFILE
#define MYFILE FILENAME(MYTEST)

#define CPL_TEST_FITS_IGNORE \
    "^COMMENT$|^HISTORY|^MJD-OBS$|PSCAL[0-9]+|PZERO[0-9]+|PTYPE[0-9]+"

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Load the headers of the named file


 */
/*----------------------------------------------------------------------------*/
static void cpl_propertylist_test_file(const char * filename, cpl_size *pmaxsz)
{
    const cpl_size nhdu = cpl_fits_count_extensions(filename);

    cpl_msg_info(cpl_func, "Test using %d header(s) from %s",
                 (int)nhdu, filename);

    for (cpl_size ihdu = 0; ihdu < nhdu; ihdu++) {
        const cxchar *badkeys = ihdu ? /* Copied from cpl_propertylist_save() */
            CPL_FITS_BADKEYS_EXT  "|" CPL_FITS_COMPRKEYS "|" CPL_TEST_FITS_IGNORE:
            CPL_FITS_BADKEYS_PRIM "|" CPL_FITS_COMPRKEYS "|" CPL_TEST_FITS_IGNORE;

        /* Test that the header can be loaded */
        cpl_propertylist *plist1 = cpl_propertylist_load(filename, ihdu);
        cpl_propertylist *plist2;
        cpl_size          sz1, sz2;
        cpl_error_code    code;

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(plist1);

        sz1 = cpl_propertylist_get_size(plist1);
        cpl_test_error(CPL_ERROR_NONE);
        if (sz1 > *pmaxsz) {
            *pmaxsz = sz1;
        }


        /* Test that the header can be saved */
        code = cpl_propertylist_save(plist1, FILENAME(00),
                                     ihdu ? CPL_IO_EXTEND : CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        /* Remove cards that are related to a non-empty data unit */
        (void)cpl_propertylist_erase_regexp(plist1, badkeys, 0);
        cpl_test_error(CPL_ERROR_NONE);

        /* Test that the reduced header can be saved */
        code = cpl_propertylist_save(plist1, MYFILE,
                                     ihdu ? CPL_IO_EXTEND : CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_fits(MYFILE);

        /* Test that the reduced header can be loaded */
        plist2 = cpl_propertylist_load(MYFILE, ihdu);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(plist2);

        (void)cpl_propertylist_erase_regexp(plist2, badkeys, 0);
        cpl_test_error(CPL_ERROR_NONE);

        sz1 = cpl_propertylist_get_size(plist1);
        cpl_test_error(CPL_ERROR_NONE);
        sz2 = cpl_propertylist_get_size(plist2);
        cpl_test_error(CPL_ERROR_NONE);

        cpl_propertylist_compare(plist1, plist2);
        if (sz1 > sz2) {
            const cpl_size nfailed = cpl_test_get_failed();
            cpl_propertylist_compare(plist2, plist1);
            if (cpl_test_get_failed() == nfailed) {
                cpl_msg_warning(cpl_func, "Header %d seemingly has %d non-"
                                "unique card(s): %s", (int)ihdu,
                                (int)(sz1 - sz2), filename);
            }
        } else {
            cpl_test_eq(sz1, sz2);
        }

        code = cpl_propertylist_save(plist2, FILENAME(00),
                                     ihdu ? CPL_IO_EXTEND : CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_propertylist_delete(plist1);
        cpl_propertylist_delete(plist2);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Check integer vs floating point numeric cards

  These cards are defined as floating point, so save them as integer and
  check that they are loaded as floating point.


 */
/*----------------------------------------------------------------------------*/
static void cpl_propertylist_test_numeric_type(void)
{
    const char *keys[] = {
        "EQUINOX",
        "EPOCH",
        "MJD-OBS",
        "LONGPOLE",
        "LATPOLE",
        "CRPIX1",
        "CRPIX2",
        "CRPIX3",
        "CRVAL1",
        "CRVAL2",
        "CRVAL3",
        "CDELT1",
        "CDELT2",
        "CDELT3",
        "CRDER1",
        "CRDER2",
        "CRDER3",
        "CSYER1",
        "CSYER2",
        "CSYER3",
        "PC1_1",
        "PV1_1",
        "PC2_2",
        "PV2_2",
        "PV3_3",
        /* The below keys may not actually be valid,
           but fitsverify accepts them */
        "CRPIX001",
        "CRVAL002",
        "CDELT003",
        "PC01_1",
        "PV01_1",
        "PC1_01",
        "PV1_01",
        "PC01_01",
        "PV01_01",
        "PC1_01",
        "PV1_0001",
    };

    const cpl_size nkeys = (cpl_size)CX_N_ELEMENTS(keys);

    cpl_propertylist *plist = cpl_propertylist_new();
    cpl_size nkeys2;
    cpl_error_code code;
    cpl_imagelist * imlist = cpl_imagelist_new();
    cpl_image     * img    = cpl_image_new(1, 1, CPL_TYPE_INT);
    const long long neglong = -9223372036854775807LL; /* LLONG_MIN */
    const long long poslong =  9223372036854775807LL; /* LLONG_MAX */
    const int       bigint  = 2147483647; /*  (2<<31) - 1 */
    int ival;
    long long lval;
    int syscode;

    code = cpl_imagelist_set(imlist, img, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (cpl_size i = 0; i < nkeys; i++) {
        code = cpl_propertylist_append_int(plist, keys[i], 42 + i);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    nkeys2 = cpl_propertylist_get_size(plist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(nkeys2, nkeys);

    code = cpl_propertylist_append_string(plist, "CTYPE1", "PIXEL");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "CTYPE2", "PIXEL");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(plist, "CTYPE3", "PIXEL");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_long_long(plist, "NEGLONG", neglong);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_long_long(plist, "POSLONG", poslong);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_int(plist, "BIGINT", bigint);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_imagelist_save(imlist, BASE "_32.fits", CPL_TYPE_INT, plist,
                              CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_imagelist_delete(imlist);
    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(BASE "_32.fits", 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(plist);

    nkeys2 = cpl_propertylist_get_size(plist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_leq(nkeys, nkeys2);

    for (cpl_size i = 0; i < nkeys; i++) {
        const cpl_property * p = cpl_propertylist_get_property_const(plist,
                                                                     keys[i]);
        const cpl_type type = cpl_property_get_type(p);
        const char * name = cpl_property_get_name(p);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(p);

        cpl_test_nonnull(name);
        cpl_test_eq_string(name, keys[i]);

        cpl_test_eq(type, CPL_TYPE_DOUBLE);
    }

    lval = cpl_propertylist_get_long(plist, "NEGLONG");
    if (cpl_error_get_code()) {
        cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
    } else {
        cpl_test_eq(lval, neglong);
    }

    lval = cpl_propertylist_get_long_long(plist, "NEGLONG");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(lval, neglong);

    lval = cpl_propertylist_get_long(plist, "POSLONG");
    if (cpl_error_get_code()) {
        cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
    } else {
        cpl_test_eq(lval, poslong);
    }

    lval = cpl_propertylist_get_long_long(plist, "POSLONG");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(lval, poslong);

    ival = cpl_propertylist_get_char(plist, "BIGINT");
    cpl_test_error(CPL_ERROR_TYPE_MISMATCH);

    ival = cpl_propertylist_get_long_long(plist, "BIGINT");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(ival, bigint);

    ival = cpl_propertylist_get_long(plist, "BIGINT");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(ival, bigint);

    ival = cpl_propertylist_get_int(plist, "BIGINT");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(ival, bigint);

    cpl_propertylist_delete(plist);

    /* EPOCH is deprecated, so rename it before validating file */
    syscode = system("perl -pi -e 's/EPOCH   /ESOCH   /' " BASE "_32.fits");
    if (!syscode) {
        cpl_test_fits(BASE "_32.fits");
    }
}

#undef MYTEST
#define MYTEST 33

#undef MYFILE
#define MYFILE FILENAME(MYTEST)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Check loading of unusual numeric values

 */
/*----------------------------------------------------------------------------*/
static void cpl_propertylist_test_numeric_load_ok(void)
{
    struct numerical {
        const char *number;
        cpl_type type;
        double expected;
    };

    struct numerical okval[] = {
        {"+000000000000",   CPL_TYPE_INT, 0},
        {"-000000000000",   CPL_TYPE_INT, 0},
        {".000000000000",   CPL_TYPE_DOUBLE, 0},
        {"+0000000000000.", CPL_TYPE_DOUBLE, 0},
        {"-0000000000000.", CPL_TYPE_DOUBLE, 0},
        {"+0000000000000.00000000000000000000000000",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.00000000000000000000000000",    CPL_TYPE_DOUBLE,   0},

        /* Floating-point constants that are FITS and C89 compliant */
        {"+0000000000000.0000000000000000000000E000",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000E000",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000E-00",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000E-00",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000E+00",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000E+00",    CPL_TYPE_DOUBLE,   0},

        {"+0000000000000.0000000000000000000000E099",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000E099",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000E-99",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000E-99",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000E+99",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000E+99",    CPL_TYPE_DOUBLE,   0},

        /* Floating-point constants that are FITS, but not C89 compliant */
        {"+0000000000000.0000000000000000000000D000",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000D000",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000D-00",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000D-00",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000D+00",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000D+00",    CPL_TYPE_DOUBLE,   0},

        {"+0000000000000.0000000000000000000000D099",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000D099",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000D-99",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000D-99",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000D+99",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000D+99",    CPL_TYPE_DOUBLE,   0},

        /* Floating-point constants that are not FITS (!!!) but C89 compliant */
        {"+0000000000000.0000000000000000000000e000",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000e000",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000e-00",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000e-00",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000e+00",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000e+00",    CPL_TYPE_DOUBLE,   0},

        {"+0000000000000.0000000000000000000000e099",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000e099",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000e-99",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000e-99",    CPL_TYPE_DOUBLE,   0},
        {"+0000000000000.0000000000000000000000e+99",    CPL_TYPE_DOUBLE,   0},
        {"-0000000000000.0000000000000000000000e+99",    CPL_TYPE_DOUBLE,   0},

        {"( 0, 0)",        CPL_TYPE_DOUBLE_COMPLEX,  0},
        {"( +0, +0)",      CPL_TYPE_DOUBLE_COMPLEX,  0},
        {"( -0, -0)",      CPL_TYPE_DOUBLE_COMPLEX,  0},
        {"( -0E0, -0E0)",  CPL_TYPE_DOUBLE_COMPLEX,  0},
        {"( -0D0, -0E1)",  CPL_TYPE_DOUBLE_COMPLEX,  0},
        {"(-0000000D0, -0E1)",      CPL_TYPE_DOUBLE_COMPLEX,  0},
        {"(-0000000D0,+000000E1)",  CPL_TYPE_DOUBLE_COMPLEX,  0},

        /* Can only set up tests with an expected value of type double... */
        {"( 420, 0)",        CPL_TYPE_DOUBLE_COMPLEX,  420},
        {"(0420,0000)",    CPL_TYPE_DOUBLE_COMPLEX,  420},
        {"(042E1,+0000)",    CPL_TYPE_DOUBLE_COMPLEX,  420},
        {"(0.42E3,+0000E22)",   CPL_TYPE_DOUBLE_COMPLEX,  420},
        {"(+0.000042E7,+0000E22)",   CPL_TYPE_DOUBLE_COMPLEX,  420},
        {"(.000042E7,+0000E22)",  CPL_TYPE_DOUBLE_COMPLEX,  420},
        {"(-00042E1,+0000E22)",   CPL_TYPE_DOUBLE_COMPLEX, -420},

        /* Floating-point constants that are FITS and C89 compliant */
        {"00000000000000000000000000000000000000420",    CPL_TYPE_INT,    420},
        {"+0000000000000000000000000000000000000420",    CPL_TYPE_INT,    420},
        {"00000000000000000000000000000000000000420.",   CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420.",   CPL_TYPE_DOUBLE, 420},
        {"00000000000000000000000000000000000000420E0",  CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420E0",  CPL_TYPE_DOUBLE, 420},
        {"00000000000000000000000000000000000000420E-0", CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420E-0", CPL_TYPE_DOUBLE, 420},
        {"00000000000000000000000000000000000000420E+0", CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420E+0", CPL_TYPE_DOUBLE, 420},

        {"0000000000000000000000000000000000000042E1",  CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000000042E1",  CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000004200E-1", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000004200E-1", CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000000042E+1", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000000042E+1", CPL_TYPE_DOUBLE, 420},

        {"000000000000000000000000000004200000000E-7",  CPL_TYPE_DOUBLE, 420},
        {"+00000000000000000000000000004200000000E-7",  CPL_TYPE_DOUBLE, 420},
        {"000000000000000000000000000004200000000E-07",  CPL_TYPE_DOUBLE, 420},
        {"+00000000000000000000000000004200000000E-07",  CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000.000042E+7", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000.000042E+7", CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000.000042E07", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000.000042E07", CPL_TYPE_DOUBLE, 420},

        /* Floating-point constants that are FITS, but not C89 compliant */
        {"00000000000000000000000000000000000000420D0",  CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420D0",  CPL_TYPE_DOUBLE, 420},
        {"00000000000000000000000000000000000000420D-0", CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420D-0", CPL_TYPE_DOUBLE, 420},
        {"00000000000000000000000000000000000000420D+0", CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420D+0", CPL_TYPE_DOUBLE, 420},

        {"0000000000000000000000000000000000000042D1",  CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000000042D1",  CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000004200D-1", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000004200D-1", CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000000042D+1", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000000042D+1", CPL_TYPE_DOUBLE, 420},

        {"000000000000000000000000000004200000000D-7",  CPL_TYPE_DOUBLE, 420},
        {"+00000000000000000000000000004200000000D-7",  CPL_TYPE_DOUBLE, 420},
        {"000000000000000000000000000004200000000D-07",  CPL_TYPE_DOUBLE, 420},
        {"+00000000000000000000000000004200000000D-07",  CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000.000042D+7", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000.000042D+7", CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000.000042D07", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000.000042D07", CPL_TYPE_DOUBLE, 420},

        /* Floating-point constants that are not FITS (!!!) but C89 compliant */
        {"00000000000000000000000000000000000000420.",   CPL_TYPE_DOUBLE, 420},
        {"+0000000000000000000000000000000000000420.",   CPL_TYPE_DOUBLE, 420},

        {"0000000000000000000000000000000000000042.0e1",  CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000000042.0e1",  CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000004200.0e-1", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000004200.0e-1", CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000000042.e+1", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000000042.0e+1", CPL_TYPE_DOUBLE, 420},

        {"0000000000000000000000000000000000.000042e+7", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000.000042e+7", CPL_TYPE_DOUBLE, 420},
        {"0000000000000000000000000000000000.000042e07", CPL_TYPE_DOUBLE, 420},
        {"+000000000000000000000000000000000.000042e07", CPL_TYPE_DOUBLE, 420},

        /* These integers do not fit in a signed long long */
        {"000018446744073709551615", CPL_TYPE_DOUBLE, 18446744073709551615E0},
        {"+00018446744073709551615", CPL_TYPE_DOUBLE, 18446744073709551615E0},
        /* These do not fit in a long long and cause unsigned overflow */
        {"+00184467440737095516150", CPL_TYPE_DOUBLE, 18446744073709551615E+1},
        {"184467440737095516150000", CPL_TYPE_DOUBLE, 18446744073709551615E+4},
        {"-00184467440737095516150", CPL_TYPE_DOUBLE, -18446744073709551615E1},
        {"-18446744073709551615000", CPL_TYPE_DOUBLE, -18446744073709551615E+3}

    };

    const int          nokval = (int)CX_N_ELEMENTS(okval);
    cpl_propertylist * plist = cpl_propertylist_new();
    cpl_error_code     code;
    int                syscode;
    cpl_size           k;

    for (int i = 0; i < nokval; i++) {
        /* Prepend key with a test of the comment support */
        char * ckey = cpl_sprintf("'/' " KEYFORM, 1+i);
        const char * key = ckey + 4;

        cpl_propertylist_append_string(plist, key, okval[i].number);
        cpl_propertylist_set_comment(plist, key, ckey);
        cpl_free(ckey);
    }

    k = cpl_propertylist_get_size(plist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(k, nokval);

    code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_propertylist_delete(plist);

    /* For each card remove the quotes from the value, producing a number */
    syscode = system(RMCARDQUOTE(KEYBASE, MYFILE));

    if (!syscode) {
        int failcase = 0;
        cpl_test_fits(MYFILE);

        plist = cpl_propertylist_load_regexp(MYFILE, 0, "^" KEYBASE, 0);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(plist);

        k = cpl_propertylist_get_size(plist);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(k, nokval);

        for (int i = 0; i < nokval; i++) {
            char * key = cpl_sprintf(KEYFORM, 1+i);
            const cpl_property * pp;
            cpl_type type;
            const cpl_size nfailed = cpl_test_get_failed();

            cpl_msg_info(cpl_func, "Testing OK-case %d/%d: %s (%s)", 1+i,
                         (int)nokval, okval[i].number,
                         cpl_type_get_name(okval[i].type));

            pp = cpl_propertylist_get_property_const(plist, key);
            cpl_test_error(CPL_ERROR_NONE);

            type = cpl_property_get_type(pp);
            cpl_test_error(CPL_ERROR_NONE);

            cpl_test_eq(type, okval[i].type);

            if (type == CPL_TYPE_DOUBLE) {
                const double dval = cpl_property_get_double(pp);

                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_rel(dval, okval[i].expected, DBL_EPSILON);
            } else if (type == CPL_TYPE_DOUBLE_COMPLEX) {
                const double complex cval = cpl_property_get_double_complex(pp);
                const double         dval = creal(cval);
                const double         ival = cimag(cval);

                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_rel(dval, okval[i].expected, DBL_EPSILON);
                cpl_test_abs(ival, 0.0, 0.0);
            } else {
                const long long lval = cpl_property_get_long_long(pp);

                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_eq(lval, (long long)okval[i].expected);
            }

            if (cpl_test_get_failed() != nfailed) failcase++;

            cpl_free(key);
        }

        if (failcase > 0)
            cpl_msg_error(cpl_func, "Failed %d of %d test case(s)",
                          (int)failcase, nokval);

        cpl_propertylist_delete(plist);
    }
}

#undef MYTEST
#define MYTEST 35

#undef MYFILE
#define MYFILE FILENAME(MYTEST)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Check loading of unusual numeric (integer) values

 */
/*----------------------------------------------------------------------------*/
static void cpl_propertylist_test_numeric_load_ok2(void)
{
    struct numerical {
        const char *number;
        cpl_type type;
        long long expected;
    };

    struct numerical okval[] = {
        /* The below number is 2^63 */
        { "9223372036854775807", CPL_TYPE_LONG_LONG,  9223372036854775807LL},
        {"09223372036854775807", CPL_TYPE_LONG_LONG,  9223372036854775807LL},
        {"+9223372036854775807", CPL_TYPE_LONG_LONG,  9223372036854775807LL},
        /* 1/machine precision, 1<<52 - 1, cast from double with no roundoff */
        {"00004503599627370495", CPL_TYPE_LONG_LONG,  4503599627370495LL},
        {"+0004503599627370495", CPL_TYPE_LONG_LONG,  4503599627370495LL},
        {"-0004503599627370495", CPL_TYPE_LONG_LONG, -4503599627370495LL},
        /* 1/machine precision, (1<<52)  */
        {"00004503599627370496", CPL_TYPE_LONG_LONG,  4503599627370496LL},
        {"-0004503599627370496", CPL_TYPE_LONG_LONG,  -4503599627370496LL},
        {"01000000000000000000", CPL_TYPE_LONG_LONG,  1000000000000000000LL},
        {"+1000000000000000000", CPL_TYPE_LONG_LONG,  1000000000000000000LL},
        {"-1000000000000000000", CPL_TYPE_LONG_LONG, -1000000000000000000LL}
    };

    const int          nokval = (int)CX_N_ELEMENTS(okval);
    cpl_propertylist * plist = cpl_propertylist_new();
    cpl_error_code     code;
    int                syscode;
    cpl_size           k;

    for (int i = 0; i < nokval; i++) {
        /* Prepend key with a test of the comment support */
        char * ckey = cpl_sprintf("'/' " KEYFORM, 1+i);
        const char * key = ckey + 4;

        cpl_propertylist_append_string(plist, key, okval[i].number);
        cpl_propertylist_set_comment(plist, key, ckey);
        cpl_free(ckey);
    }

    k = cpl_propertylist_get_size(plist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(k, nokval);

    code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_propertylist_delete(plist);

    /* For each card remove the quotes from the value, producing a number */
    syscode = system(RMCARDQUOTE(KEYBASE, MYFILE));

    if (!syscode) {
        int failcase = 0;
        cpl_test_fits(MYFILE);

        for (int i = 0; i < nokval; i++) {
            char * key = cpl_sprintf(KEYFORM, 1+i);
            const cpl_property * pp;
            cpl_type type;
            const cpl_size nfailed = cpl_test_get_failed();

            cpl_msg_info(cpl_func, "Testing OK-case %d/%d: %s (%s)", 1+i,
                         (int)nokval, okval[i].number,
                         cpl_type_get_name(okval[i].type));

            plist = cpl_propertylist_load_regexp(MYFILE, 0, key, 0);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(plist);

            k = cpl_propertylist_get_size(plist);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(k, 1);

            pp = cpl_propertylist_get_property_const(plist, key);
            cpl_test_error(CPL_ERROR_NONE);

            type = cpl_property_get_type(pp);
            cpl_test_error(CPL_ERROR_NONE);

            cpl_test_eq(type, okval[i].type);

            if (type == CPL_TYPE_DOUBLE) {
                const double dval = cpl_property_get_double(pp);

                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_rel(dval, okval[i].expected, DBL_EPSILON);
            } else {
                const long long lval = cpl_property_get_long_long(pp);

                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_eq(lval, (long long)okval[i].expected);
            }

            cpl_free(key);
            cpl_propertylist_delete(plist);
            if (cpl_test_get_failed() != nfailed) failcase++;
        }
        if (failcase > 0)
            cpl_msg_error(cpl_func, "Failed %d of %d test case(s)",
                          (int)failcase, nokval);
    }
}

#undef MYTEST
#define MYTEST 36

#undef MYFILE
#define MYFILE FILENAME(MYTEST)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Check loading of bad (numeric) values

 */
/*----------------------------------------------------------------------------*/
static void cpl_propertylist_test_numeric_load_bad(void)
{
    const char* badval[] = {
        "0.F",
        "   0.F",
        "+0.F",
        "+0000.F",
        "    -0.F",
        ".E0",
        "   .E000",
        "+.E0",
        "       +.E0",
        "-.E0",
        "+-0.0",
        "  ++0.0",
        "    --0.0",
        "+-0.0",
        "TF",
        "E",
        "1EO",
        "1DO",
        "E",
        "E123"};

    const int          nbadval = (int)CX_N_ELEMENTS(badval);


    for (int idocom = 0; idocom < 2; idocom++) {
        cpl_propertylist * plist = cpl_propertylist_new();
        cpl_error_code     code;
        int                syscode;
        cpl_size           k;
        
        for (int i = 0; i < nbadval; i++) {
            /* Prepend key with a test of the comment support */
            char * ckey = cpl_sprintf("'/' " KEYFORM, 1+i);
            const char * key = ckey + 4;

            cpl_propertylist_append_string(plist, key, badval[i]);
            if (idocom) cpl_propertylist_set_comment(plist, key, ckey);
            cpl_free(ckey);
        }

        k = cpl_propertylist_get_size(plist);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(k, nbadval);

        code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_propertylist_delete(plist);

        /* For each card remove the quotes from the value, producing a number */
        syscode = system(RMCARDQUOTE(KEYBASE, MYFILE));

        if (!syscode) {
            int failcase = 0;
            cpl_test_fits(MYFILE);

            for (int i = 0; i < nbadval; i++) {
                cpl_propertylist * nulllist;
                char * key = cpl_sprintf(KEYFORM, 1+i);
                const cpl_size nfailed = cpl_test_get_failed();

                cpl_msg_info(cpl_func, "Testing BAD-case %d/%d: %s",
                             1+i, (int)nbadval, badval[i]);

                nulllist = cpl_propertylist_load_regexp(MYFILE, 0, key, 0);
                cpl_test_error(CPL_ERROR_BAD_FILE_FORMAT);
                cpl_test_null(nulllist);

                if (nfailed != cpl_test_get_failed()) {
                    cpl_propertylist_dump(nulllist, stderr);
                    cpl_propertylist_delete(nulllist);
                }

                cpl_free(key);
                if (cpl_test_get_failed() != nfailed) failcase++;
            }
            if (failcase > 0)
                cpl_msg_error(cpl_func, "Failed %d of %d test case(s)",
                              (int)failcase, nbadval);
        }
    }
}


#undef MYTEST
#define MYTEST 37

#undef MYFILE
#define MYFILE FILENAME(MYTEST)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Check loading of unusual numeric (complex) values

 */
/*----------------------------------------------------------------------------*/
static void cpl_propertylist_test_numeric_load_ok3(void)
{
    struct numerical {
        const char *number;
        cpl_type type;
        double   real_expect;
        double   imag_expect;
    };

    struct numerical okval[] = {
        { "(1, 0.0)",       CPL_TYPE_DOUBLE_COMPLEX, 1.0, 0.0},
        { "(1, 1)",         CPL_TYPE_DOUBLE_COMPLEX, 1.0, 1.0},
        { "(+1, +01)",      CPL_TYPE_DOUBLE_COMPLEX, 1.0, 1.0},
        { "(+1E0, +01D-0)", CPL_TYPE_DOUBLE_COMPLEX, 1.0, 1.0},
        { "(1D1,2E2)",      CPL_TYPE_DOUBLE_COMPLEX, 1E1, 2E2},
        { "(1D1,2E2)",      CPL_TYPE_DOUBLE_COMPLEX, 1E1, 2E2}
    };

    const int          nokval = (int)CX_N_ELEMENTS(okval);
    cpl_propertylist * plist = cpl_propertylist_new();
    cpl_error_code     code;
    int                syscode;
    cpl_size           k;

    for (int i = 0; i < nokval; i++) {
        /* Prepend key with a test of the comment support */
        char * ckey = cpl_sprintf("'/' " KEYFORM, 1+i);
        const char * key = ckey + 4;

        cpl_propertylist_append_string(plist, key, okval[i].number);
        cpl_propertylist_set_comment(plist, key, ckey);
        cpl_free(ckey);
    }

    k = cpl_propertylist_get_size(plist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(k, nokval);

    code = cpl_propertylist_save(plist, MYFILE, CPL_IO_CREATE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_propertylist_delete(plist);

    /* For each card remove the quotes from the value, producing a number */
    syscode = system(RMCARDQUOTE(KEYBASE, MYFILE));

    if (!syscode) {
        int failcase = 0;
        cpl_test_fits(MYFILE);

        for (int i = 0; i < nokval; i++) {
            char * key = cpl_sprintf(KEYFORM, 1+i);
            const cpl_property * pp;
            cpl_type type;
            const cpl_size nfailed = cpl_test_get_failed();

            cpl_msg_info(cpl_func, "Testing OK-case %d/%d: %s (%s)", 1+i,
                         (int)nokval, okval[i].number,
                         cpl_type_get_name(okval[i].type));

            plist = cpl_propertylist_load_regexp(MYFILE, 0, key, 0);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(plist);

            k = cpl_propertylist_get_size(plist);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(k, 1);

            pp = cpl_propertylist_get_property_const(plist, key);
            cpl_test_error(CPL_ERROR_NONE);

            type = cpl_property_get_type(pp);
            cpl_test_error(CPL_ERROR_NONE);

            cpl_test_eq(type, okval[i].type);

            if (type == CPL_TYPE_DOUBLE_COMPLEX) {

                const double complex cval = cpl_property_get_double_complex(pp);
                const double         dval = creal(cval);
                const double         ival = cimag(cval);

                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_rel(dval, okval[i].real_expect, DBL_EPSILON);
                cpl_test_rel(ival, okval[i].imag_expect, DBL_EPSILON);
            }

            cpl_free(key);
            cpl_propertylist_delete(plist);
            if (cpl_test_get_failed() != nfailed) failcase++;
        }
        if (failcase > 0)
            cpl_msg_error(cpl_func, "Failed %d of %d test case(s)",
                          (int)failcase, nokval);
    }
}
