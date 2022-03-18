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
#include "cpl_parameterlist.h"

int main(void)
{

    const cpl_parameter *cq = NULL;

    cpl_parameter *p[4];
    cpl_parameter *q = NULL;

    cpl_parameterlist *list = NULL;
    cpl_error_code     error;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /*
     * Test 1: Create a parameter list
     */

    list = cpl_parameterlist_new();

    cpl_test_nonnull(list);
    cpl_test_zero(cpl_parameterlist_get_size(list));


    /*
     * Test 2: Append parameters to the list
     */

    p[0] = cpl_parameter_new_value("a", CPL_TYPE_STRING, "parameter1",
                                   "None", "value1");
    error = cpl_parameterlist_append(list, p[0]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_parameterlist_get_size(list), 1);

    p[1] = cpl_parameter_new_value("b", CPL_TYPE_STRING, "parameter2",
                                   "None", "value2");
    error = cpl_parameterlist_append(list, p[1]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_parameterlist_get_size(list), 2);

    p[2] = cpl_parameter_new_value("c", CPL_TYPE_STRING, "parameter3",
                                   "None", "value3");
    error = cpl_parameterlist_append(list, p[2]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_parameterlist_get_size(list), 3);

    p[3] = cpl_parameter_new_value("d", CPL_TYPE_STRING, "parameter4",
                                  "None", "value4");
    error = cpl_parameterlist_append(list, p[3]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq(cpl_parameterlist_get_size(list), 4);


    /*
     * Test 3: Verify the sequential access to the list elements.
     */

    cpl_test_eq_ptr(p[0], cpl_parameterlist_get_first(list));
    cpl_test_eq_ptr(p[1], cpl_parameterlist_get_next(list));
    cpl_test_eq_ptr(p[2], cpl_parameterlist_get_next(list));
    cpl_test_eq_ptr(p[3], cpl_parameterlist_get_next(list));
    cpl_test_eq_ptr(p[3], cpl_parameterlist_get_last(list));
    cpl_test_null(cpl_parameterlist_get_next(list));


    /*
     * Test 4: Add a user tag to one of the parameters and check whether it
     *         can be found by looking for the tag.
     */

    error = cpl_parameter_set_tag(p[2], "mytag");
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    q = cpl_parameterlist_find_tag(list, "mytag");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq_ptr(q, p[2]);

    q = cpl_parameterlist_find_tag(list, "notag");
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(q);

    cpl_parameterlist_delete(list);


    /*
     * Test 5: Check cpl_parameterlist_find_tag_const handles invalid
     *         tags (passed as argument or as member of the list to search)
     *         gracefully.
     */

    list = cpl_parameterlist_new();
    cpl_test_nonnull(list);
    p[0] = cpl_parameter_new_value("a", CPL_TYPE_STRING, "parameter1",
                                   "None", "value1");
    cpl_test_nonnull(p[0]);
    error = cpl_parameter_set_tag(p[0], "mytag");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_parameterlist_append(list, p[0]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    cq = cpl_parameterlist_find_tag_const(list, NULL);
    cpl_test_null(cq);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    p[1] = cpl_parameter_new_value("b", CPL_TYPE_STRING, "parameter2",
                                   "None", "value2");
    cpl_test_nonnull(p[1]);
    error = cpl_parameterlist_append(list, p[1]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    cq = cpl_parameterlist_find_tag_const(list, NULL);
    cpl_test_eq_ptr(cq, p[1]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    cpl_parameterlist_delete(list);


    /*
     * Test 6: Check cpl_parameterlist_find_context_const handles invalid
     *         contexts gracefully (passed as argument or member of the list).
     */

    list = cpl_parameterlist_new();
    cpl_test_nonnull(list);
    p[0] = cpl_parameter_new_value("a", CPL_TYPE_STRING, "parameter1",
                                   "Context1", "value1");
    cpl_test_nonnull(p[0]);
    error = cpl_parameterlist_append(list, p[0]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    p[1] = cpl_parameter_new_value("b", CPL_TYPE_STRING, "parameter2",
                                   NULL, "value2");
    cpl_test_nonnull(p[1]);
    error = cpl_parameterlist_append(list, p[1]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    cq = cpl_parameterlist_find_context_const(list, "Context1");
    cpl_test_eq_ptr(cq, p[0]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    cq = cpl_parameterlist_find_context_const(list, "Context2");
    cpl_test_null(cq);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    cq = cpl_parameterlist_find_context_const(list, NULL);
    cpl_test_eq_ptr(cq, p[1]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    cpl_parameterlist_delete(list);

    /*
     * All tests succeeded
     */

    return cpl_test_end(0);
}
