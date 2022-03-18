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

#include "cpl_test.h"
#include "cpl_parameter.h"

int main(void)
{

    const cxchar *sval1 = "abcdefghijklmnopqrstuvwxyz";
    const cxchar *sval2 = "zyxwvutsrqponmlkjihgfedcba";

    cxint ival = 0;

    cpl_error_code error = CPL_ERROR_NONE;

    cpl_parameter *p = NULL;
    cpl_parameter *q = NULL;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);


    /*
     * Test 1: Create a string parameter and check its validity
     */

    p = cpl_parameter_new_value("a", CPL_TYPE_STRING, "Test parameter",
                                "None", sval1);

    cpl_test_nonnull(p);

    cpl_test_eq_string(cpl_parameter_get_name(p), "a");
    cpl_test_eq_string(cpl_parameter_get_help(p), "Test parameter");
    cpl_test_eq_string(cpl_parameter_get_context(p), "None");
    cpl_test_null(cpl_parameter_get_tag(p));

    cpl_test_eq(cpl_parameter_get_type(p), CPL_TYPE_STRING);
    cpl_test_eq(cpl_parameter_get_class(p), CPL_PARAMETER_CLASS_VALUE);

    cpl_test_eq_string(cpl_parameter_get_string(p), sval1);
    cpl_test_noneq_ptr(cpl_parameter_get_string(p), sval1);


    /*
     * Test 2: Assign a new string value to the parameter and check
     *         its validity
     */

    error = cpl_parameter_set_string(p, sval2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_string(p), sval2);
    cpl_test_noneq_ptr(cpl_parameter_get_string(p), sval2);
    cpl_test_noneq_ptr(cpl_parameter_get_string(p), sval1);

    cpl_parameter_delete(p);


    /*
     * Test 3: Change the parameter's default value.
     */

    p = cpl_parameter_new_value("b", CPL_TYPE_BOOL, "A boolean value.",
                                "None", 0);

    cpl_test_zero(cpl_parameter_get_default_bool(p));
    cpl_test_eq(cpl_parameter_get_default_bool(p), cpl_parameter_get_bool(p));

    error = cpl_parameter_set_default_bool(p, 1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq(cpl_parameter_get_default_bool(p), 1);
    cpl_test_noneq(cpl_parameter_get_default_bool(p), cpl_parameter_get_bool(p));

    cpl_parameter_delete(p);


    p = cpl_parameter_new_value("c", CPL_TYPE_STRING, "A string value.",
                                "None", sval1);

    cpl_test_eq_string(cpl_parameter_get_default_string(p), sval1);
    cpl_test_eq_string(cpl_parameter_get_default_string(p),
                       cpl_parameter_get_string(p));

    error = cpl_parameter_set_default_string(p, sval2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_default_string(p), sval2);
    cpl_test_noneq_ptr(cpl_parameter_get_default_string(p), sval2);
    cpl_test_noneq_string(cpl_parameter_get_default_string(p),
                          cpl_parameter_get_string(p));

    cpl_parameter_delete(p);


    /*
     * Test 4.1: Copy an enum parameter
     */

    p = cpl_parameter_new_enum("a", CPL_TYPE_STRING, "A string enum.",
                               "None", sval1, 2, sval1, sval2);

    q = cpl_parameter_duplicate(p);

    cpl_test_nonnull(q);

    cpl_test_eq(cpl_parameter_get_class(q),
                cpl_parameter_get_class(p));
    cpl_test_eq(cpl_parameter_get_type(q),
                cpl_parameter_get_type(p));

    cpl_test_eq_string(cpl_parameter_get_name(q),
                       cpl_parameter_get_name(p));
    cpl_test_noneq_ptr(cpl_parameter_get_name(q),
                       cpl_parameter_get_name(p));

    cpl_test_eq_string(cpl_parameter_get_context(q),
                       cpl_parameter_get_context(p));
    cpl_test_noneq_ptr(cpl_parameter_get_context(q),
                       cpl_parameter_get_context(p));

    cpl_test_eq_string(cpl_parameter_get_help(q),
                       cpl_parameter_get_help(p));
    cpl_test_noneq_ptr(cpl_parameter_get_help(q),
                       cpl_parameter_get_help(p));

    cpl_test_eq_string(cpl_parameter_get_default_string(q),
                       cpl_parameter_get_default_string(p));
    cpl_test_noneq_ptr(cpl_parameter_get_default_string(q),
                       cpl_parameter_get_default_string(p));

    cpl_test_eq_string(cpl_parameter_get_string(q),
                       cpl_parameter_get_string(p));
    cpl_test_noneq_ptr(cpl_parameter_get_string(q),
                       cpl_parameter_get_string(p));

    cpl_test_eq(cpl_parameter_get_enum_size(q),
                cpl_parameter_get_enum_size(p));

    cpl_test_eq_string(cpl_parameter_get_enum_string(q, 0),
                       cpl_parameter_get_enum_string(p, 0));
    cpl_test_noneq_ptr(cpl_parameter_get_enum_string(q, 0),
                       cpl_parameter_get_enum_string(p, 0));
    cpl_test_eq_string(cpl_parameter_get_enum_string(q, 1),
                       cpl_parameter_get_enum_string(p, 1));
    cpl_test_noneq_ptr(cpl_parameter_get_enum_string(q, 1),
                       cpl_parameter_get_enum_string(p, 1));

    cpl_parameter_delete(q);
    cpl_parameter_delete(p);


    /*
     * Test 4.2: Copy an integer range parameter
     */

    p = cpl_parameter_new_range("a", CPL_TYPE_INT, "An int range.",
                                "None", (int)3, (int)1, (int)5);
    q = cpl_parameter_duplicate(p);

    cpl_test_nonnull(q);

    cpl_test_eq(cpl_parameter_get_class(q),
                cpl_parameter_get_class(p));
    cpl_test_eq(cpl_parameter_get_type(q),
                cpl_parameter_get_type(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_name(q),
                       cpl_parameter_get_name(p));
    cpl_test_noneq_ptr(cpl_parameter_get_name(q),
                       cpl_parameter_get_name(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_context(q),
                       cpl_parameter_get_context(p));
    cpl_test_noneq_ptr(cpl_parameter_get_context(q),
                       cpl_parameter_get_context(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_help(q),
                       cpl_parameter_get_help(p));
    cpl_test_noneq_ptr(cpl_parameter_get_help(q),
                       cpl_parameter_get_help(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq(cpl_parameter_get_default_int(q),
                cpl_parameter_get_default_int(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq(cpl_parameter_get_int(q),
                cpl_parameter_get_int(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq(cpl_parameter_get_range_min_int(q),
                cpl_parameter_get_range_min_int(p));
    cpl_test_eq(cpl_parameter_get_range_max_int(q),
                cpl_parameter_get_range_max_int(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_parameter_delete(q);
    cpl_parameter_delete(p);


    /*
     * Test 4.3: Copy an integer range parameter
     */

    p = cpl_parameter_new_range("a", CPL_TYPE_DOUBLE, "A double range.",
                                "None", (double)3.4, (double)1.2, (double)5.7);
    q = cpl_parameter_duplicate(p);

    cpl_test_nonnull(q);

    cpl_test_eq(cpl_parameter_get_class(q),
                cpl_parameter_get_class(p));
    cpl_test_eq(cpl_parameter_get_type(q),
                cpl_parameter_get_type(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_name(q),
                       cpl_parameter_get_name(p));
    cpl_test_noneq_ptr(cpl_parameter_get_name(q),
                       cpl_parameter_get_name(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_context(q),
                       cpl_parameter_get_context(p));
    cpl_test_noneq_ptr(cpl_parameter_get_context(q),
                       cpl_parameter_get_context(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_string(cpl_parameter_get_help(q),
                       cpl_parameter_get_help(p));
    cpl_test_noneq_ptr(cpl_parameter_get_help(q),
                       cpl_parameter_get_help(p));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_abs(cpl_parameter_get_default_double(q),
                 cpl_parameter_get_default_double(p),
                 DBL_EPSILON);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_abs(cpl_parameter_get_double(q),
                 cpl_parameter_get_double(p),
                 DBL_EPSILON);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_abs(cpl_parameter_get_range_min_double(q),
                 cpl_parameter_get_range_min_double(p),
                 DBL_EPSILON);
    cpl_test_abs(cpl_parameter_get_range_max_double(q),
                 cpl_parameter_get_range_max_double(p),
                 DBL_EPSILON);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_parameter_delete(q);
    cpl_parameter_delete(p);


    /*
     * Test 5: Test some error handling
     */

    p = cpl_parameter_new_value("a", CPL_TYPE_STRING, "A string value.",
                                "None", sval1);

    error = cpl_parameter_set_int(p, 0);
    cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);

    (void)cpl_parameter_get_int(p);
    cpl_test_error(CPL_ERROR_TYPE_MISMATCH);

    ival = cpl_parameter_get_enum_size(p);
    cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
    cpl_test_zero(ival);

    cpl_parameter_delete(p);


    p = cpl_parameter_new_range("a", CPL_TYPE_BOOL, "A boolean range.",
                                "None", FALSE, FALSE, TRUE);
    cpl_test_null(p);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);

    cpl_parameter_delete(p);

    p = cpl_parameter_new_range("a", CPL_TYPE_STRING, "A string range.",
                                "None", "aaa", "aaa", "zzz");
    cpl_test_null(p);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);

    cpl_parameter_delete(p);

    p = cpl_parameter_new_enum("a", CPL_TYPE_BOOL, "A boolean enumeration.",
                               "None", FALSE, 2, FALSE, TRUE);
    cpl_test_null(p);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);

    cpl_parameter_delete(p);

    p = cpl_parameter_new_enum("a", CPL_TYPE_INT, "An integer enumeration.",
                               "None", 0, 0, 0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(p);

    cpl_parameter_delete(p);

    /*
     * All tests finished
     */

    return cpl_test_end(0);

}
