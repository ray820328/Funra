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

#include <complex.h>

#include "cpl_test.h"
#include "cpl_property_impl.h"

#include <stdio.h>
#include <string.h>

#define STRING100                                               \
    "01234567890123456789012345678901234567890123456789"        \
    "01234567890123456789012345678901234567890123456789"

#define CPL_NPROPS 9

int main(void)
{

    const char *names[] = {
        "char",
        "bool",
        "int",
        "long",
        "float",
        "double",
        "string",
        "float complex",
        "double complex"
    };

    const char *strings[] = {
        "Mary",
        "Mary had a little lamb",
        "Mary had a",
        STRING100,
        STRING100 STRING100,
        "Mary had"
    };

    const float fval0 = 1.23456789;
    float fval1, fval2;
    const double dval0 = fval0;
    double dval1, dval2;
    const float complex zf0 = fval0 + fval0 * fval0 * _Complex_I;
    float  complex zf1, zf2;
    const double complex zd0 = zf0;
    double complex zd1, zd2;
    cpl_error_code code;

    const cpl_type types[] = {
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

    cpl_property * property;
    cpl_property * plist[CPL_NPROPS];
    const size_t   nprops   = CX_N_ELEMENTS(names);
    const size_t   nstrings = CX_N_ELEMENTS(strings);
    cpl_property * nullprop;

    FILE         * stream;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull( stream );

    cpl_test_assert(nprops == CPL_NPROPS);

    /*
     * Test 1: Create a properties of different types, check their validity
     *         and destroy them again.
     */

    cpl_test_eq(sizeof(names)/sizeof(names[0]),
                sizeof(types)/sizeof(types[0]));


    nullprop = cpl_property_new(NULL, types[0]);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullprop);

    nullprop = cpl_property_new(names[0], CPL_TYPE_INVALID);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_null(nullprop);

    nullprop = cpl_property_new_array(NULL, types[0], 1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullprop);

    /* The below two tests fail without the container implementation */
    nullprop = cpl_property_new_array(names[0], CPL_TYPE_INVALID, 1);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_null(nullprop);

    nullprop = cpl_property_new_array(names[0], types[0], 0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullprop);

    for (size_t i = 0; i < nprops; i++) {
        plist[i] = cpl_property_new(names[i], types[i]);

        cpl_test_nonnull(plist[i]);
        cpl_test_eq_string(cpl_property_get_name(plist[i]), names[i]);
        cpl_test_null(cpl_property_get_comment(plist[i]));
        cpl_test_eq(cpl_property_get_type(plist[i]), types[i]);
        cpl_test_eq(cpl_property_get_size(plist[i]), 1);

        cpl_property_delete(plist[i]);
    }


    /*
     * Test 2: Create properties of different size and types, verify the
     *         properties and destroy them again. Note that only types
     *         having the array flag set will have sizes different from
     *         one.
     */

    for (size_t i = 0; i < nprops; i++) {
        for (size_t j = 1; j <= 100; j += 10) {
            property = cpl_property_new_array(names[i], types[i], j);

            cpl_test_nonnull(property);
            cpl_test_eq_string(cpl_property_get_name(property), names[i]);
            cpl_test_null(cpl_property_get_comment(property));
            cpl_test_eq(cpl_property_get_type(property), types[i]);
            if (types[i] & CPL_TYPE_FLAG_ARRAY) {
                cpl_test_eq(cpl_property_get_size(property), j);
            } else {
                cpl_test_eq(cpl_property_get_size(property), 1);
            }

            code = cpl_property_set_name(property, STRING100);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            cpl_test_eq_string(cpl_property_get_name(property), STRING100);

            code = cpl_property_set_comment(property, STRING100);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            cpl_test_eq_string(cpl_property_get_comment(property), STRING100);

            cpl_property_delete(property);
        }
    }

    /*
     * Test 3: Create a properties of different types, set their comments,
     *         modify their names and verify the new settings.
     */

    for (size_t i = 0; i < nprops; i++) {
        plist[i] = cpl_property_new(names[i], types[i]);
        cpl_test_nonnull(plist[i]);

        for (size_t j = 0; j < nstrings; j++) {
            code = cpl_property_set_comment(plist[i], strings[j]);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            cpl_test_eq_string(cpl_property_get_comment(plist[i]), strings[j]);

            code = cpl_property_set_name(plist[i], strings[j]);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            cpl_test_eq_string(cpl_property_get_name(plist[i]), strings[j]);

        }
    }

    for (size_t i = 0; i < nprops; i++) {
        cpl_property_set_name(plist[i], names[nprops - 1 - i]);
        cpl_test_eq_string(cpl_property_get_name(plist[i]),
                           names[nprops -1 - i]);

        cpl_property_delete(plist[i]);
    }


    /*
     * Test 4: Create properties for the supported types, set their
     *         values and verify them.
     */

    for (size_t i = 0; i < nprops; i++)
        plist[i] = cpl_property_new(names[i], types[i]);

    cpl_property_set_char(plist[0], 'a');
    cpl_test_eq(cpl_property_get_char(plist[0]), 'a');

    cpl_property_set_bool(plist[1], 1);
    cpl_test_eq(cpl_property_get_bool(plist[1]), 1);
    
    cpl_property_set_int(plist[2], 100);
    cpl_test_eq(cpl_property_get_int(plist[2]), 100);
    
    cpl_property_set_long(plist[3], 10L);
    cpl_test_eq(cpl_property_get_long(plist[3]), 10L);
    
    cpl_property_set_float(plist[4], fval0);
    fval1 = cpl_property_get_float(plist[4]);
    cpl_test_abs(fval0, fval1, 0.0);
    
    cpl_property_set_double(plist[5], dval0);
    dval1 = cpl_property_get_double(plist[5]);
    cpl_test_abs(dval0, dval1, 0.0);

    cpl_property_dump(plist[0], stream);
    cpl_property_dump(plist[1], stream);
    cpl_property_dump(plist[2], stream);
    cpl_property_dump(plist[3], stream);
    cpl_property_dump(plist[4], stream);
    cpl_property_dump(plist[5], stream);

    /*
     * Try strings with different length to verify that the value is
     * properly resized. Note that the size of a string property includes
     * the byte for the trailing zero.
     */

    for (size_t i = 0; i < nstrings; i++) {
        code = cpl_property_set_string(plist[6], strings[i]);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_eq(cpl_property_get_size(plist[6]),
                    ((cpl_size)strlen(strings[i]) + 1));

        cpl_test_eq_string(cpl_property_get_string(plist[6]), strings[i]);
    }

    code = cpl_property_set_float_complex(plist[7], zf0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    zf1 = cpl_property_get_float_complex(plist[7]);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs_complex(zf0, zf1, 0.0);

    cpl_test_eq(cpl_type_get_sizeof(CPL_TYPE_FLOAT_COMPLEX), sizeof(zf0));
    
    cpl_property_set_double_complex(plist[8], zd0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    zd1 = cpl_property_get_double_complex(plist[8]);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs_complex(zd0, zd1, 0.0);

    for (size_t i = 0; i < nprops; i++) {
        for (size_t j = 0; j < nstrings; j++) {

            code = cpl_property_set_comment(plist[i], strings[j]);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            for (size_t k = 0; k < nstrings; k++) {

                code = cpl_property_set_name(plist[i], strings[k]);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                cpl_test_eq_string(cpl_property_get_name(plist[i]),   
                                   strings[k]);
                cpl_test_eq_string(cpl_property_get_comment(plist[i]),
                                   strings[j]);

                for (size_t l = 0; l < nstrings; l++) {
                    code = cpl_property_set_string(plist[i], strings[l]);

                    if (types[i] != CPL_TYPE_STRING) {
                        cpl_test_eq_error(code, CPL_ERROR_TYPE_MISMATCH);
                    } else {
                        cpl_test_eq_error(code, CPL_ERROR_NONE);

                        cpl_test_eq_string(cpl_property_get_string(plist[i]),
                                           strings[l]);
                    }
                }
            }
        }
    }


    /*
     * Test 5: Use the properties from the previous test, copy them and
     *         and verify that original and copy are identical but don't
     *         share any resources.
     */

    for (size_t i = 0; i < nprops; i++) {
        cpl_property_set_comment(plist[i], strings[1]);
        property = cpl_property_duplicate(plist[i]);

        cpl_test_noneq_ptr(property, plist[i]);
        cpl_test_noneq_ptr(cpl_property_get_name(property),
                 cpl_property_get_name(plist[i]));
        cpl_test_noneq_ptr(cpl_property_get_comment(property),
                  cpl_property_get_comment(plist[i]));
        cpl_test_eq(cpl_property_get_size(property),
                    cpl_property_get_size(plist[i]));
        cpl_test_eq(cpl_property_get_type(property),
                    cpl_property_get_type(plist[i]));

        switch (cpl_property_get_type(property)) {
            case CPL_TYPE_CHAR:
                cpl_test_eq(cpl_property_get_char(property),
                            cpl_property_get_char(plist[i]));
                break;
                
            case CPL_TYPE_BOOL:
                cpl_test_eq(cpl_property_get_bool(property),
                            cpl_property_get_bool(plist[i]));
                break;

            case CPL_TYPE_INT:
                cpl_test_eq(cpl_property_get_int(property),
                            cpl_property_get_int(plist[i]));
                break;

            case CPL_TYPE_LONG:
                cpl_test_eq(cpl_property_get_long(property),
                            cpl_property_get_long(plist[i]));
                break;

            case CPL_TYPE_FLOAT:
                fval1 = cpl_property_get_float(property);
                fval2 = cpl_property_get_float(plist[i]);

                cpl_test_abs(fval1, fval2, 0.0);
                break;

            case CPL_TYPE_DOUBLE:
                dval1 = cpl_property_get_double(property);
                dval2 = cpl_property_get_double(plist[i]);

                cpl_test_abs(dval1, dval2, 0.0);
                break;

            case CPL_TYPE_STRING:
                cpl_test_eq_string(cpl_property_get_string(property),
                                   cpl_property_get_string(plist[i]));
                break;

            case CPL_TYPE_FLOAT_COMPLEX:
                zf1 = cpl_property_get_float_complex(property);
                zf2 = cpl_property_get_float_complex(plist[i]);

                cpl_test_abs_complex(zf1, zf2, 0.0);
                break;

            case CPL_TYPE_DOUBLE_COMPLEX:
                zd1 = cpl_property_get_double_complex(property);
                zd2 = cpl_property_get_double_complex(plist[i]);

                cpl_test_abs_complex(zd1, zd2, 0.0);
                break;

            default:

                /* This should never happen */
                cpl_test(0);
                break;
        }

        cpl_property_delete(plist[i]);
        cpl_property_delete(property);
    }


    /*
     * All tests finished
     */

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return cpl_test_end(0);

}
