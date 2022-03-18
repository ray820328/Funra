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
#include "cpl_framedata.h"

int main(void)
{

    const char* tags[] = {"One", "Two"};
    const char* tag = NULL;

    cpl_error_code status;
    int            count;

    cpl_framedata* framedata = NULL;
    cpl_framedata* _framedata = NULL;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /*
     * Test 1: Create a framedata object, check its validity and destroy it
     *         again.
     */

    framedata = cpl_framedata_new();
    cpl_test_nonnull(framedata);

    cpl_test_null(cpl_framedata_get_tag(framedata));
    cpl_test_eq(cpl_framedata_get_min_count(framedata), -1);
    cpl_test_eq(cpl_framedata_get_max_count(framedata), -1);

    cpl_framedata_delete(framedata);
    framedata = NULL;


    /*
     * Test 2: Create a framedata object with given initializers, check
     *         its validity and destroy it again.
     */

    framedata = cpl_framedata_create(tags[0], 0, 5);
    cpl_test_nonnull(framedata);

    cpl_test_noneq_ptr(cpl_framedata_get_tag(framedata), tags[0]);
    cpl_test_eq_string(cpl_framedata_get_tag(framedata), tags[0]);
    cpl_test_zero(cpl_framedata_get_min_count(framedata));
    cpl_test_eq(cpl_framedata_get_max_count(framedata), 5);

    cpl_framedata_delete(framedata);
    framedata = NULL;


    /*
     * Test 3: Clone a framedata object and check that both objecta are
     *         identical.
     */

    framedata = cpl_framedata_create(tags[1],  5, -1);
    cpl_test_nonnull(framedata);

    _framedata = cpl_framedata_duplicate(framedata);
    cpl_test_nonnull(_framedata);


    cpl_test_noneq_ptr(cpl_framedata_get_tag(framedata),
                       cpl_framedata_get_tag(_framedata));
    cpl_test_eq_string(cpl_framedata_get_tag(framedata),
                       cpl_framedata_get_tag(_framedata));
    cpl_test_eq(cpl_framedata_get_min_count(framedata),
                cpl_framedata_get_min_count(_framedata));
    cpl_test_eq(cpl_framedata_get_max_count(framedata),
                cpl_framedata_get_max_count(_framedata));

    cpl_framedata_delete(_framedata);
    _framedata = NULL;

    cpl_framedata_delete(framedata);
    framedata = NULL;


    /*
     * Test 4: Clear a framedata object and check that it is in its
     *         default state.
     */

    framedata = cpl_framedata_create(tags[1], 5, -1);
    cpl_test_nonnull(framedata);

    cpl_framedata_clear(framedata);


    cpl_test_null(cpl_framedata_get_tag(framedata));
    cpl_test_eq(cpl_framedata_get_min_count(framedata), -1);
    cpl_test_eq(cpl_framedata_get_max_count(framedata), -1);

    cpl_framedata_delete(framedata);
    framedata = NULL;


    /*
     * Test 5: Create a framedata object modify its contents, and verify it.
     */

    framedata = cpl_framedata_create(tags[0], 0, -1);
    cpl_test_nonnull(framedata);

    cpl_framedata_set_tag(framedata, tags[1]);
    cpl_test_noneq_ptr(cpl_framedata_get_tag(framedata), tags[1]);
    cpl_test_eq_string(cpl_framedata_get_tag(framedata), tags[1]);

    cpl_framedata_set_min_count(framedata, 3);
    cpl_test_eq(cpl_framedata_get_min_count(framedata), 3);

    cpl_framedata_set_max_count(framedata, 5);
    cpl_test_eq(cpl_framedata_get_max_count(framedata), 5);

    cpl_framedata_set(framedata, tags[0], 0, -1);
    cpl_test_noneq_ptr(cpl_framedata_get_tag(framedata), tags[0]);
    cpl_test_eq_string(cpl_framedata_get_tag(framedata), tags[0]);
    cpl_test_zero(cpl_framedata_get_min_count(framedata));
    cpl_test_eq(cpl_framedata_get_max_count(framedata), -1);

    cpl_framedata_delete(framedata);
    framedata = NULL;


    /*
     * Test 5: Check error codes set by the member functions.
     */

    framedata = cpl_framedata_create(NULL, 0, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(cpl_framedata_get_tag(framedata));

    cpl_framedata_delete(framedata);
    framedata = NULL;

    framedata = cpl_framedata_duplicate(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(framedata);

    tag = cpl_framedata_get_tag(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(tag);

    count = cpl_framedata_get_min_count(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(count, -2);

    count = cpl_framedata_get_max_count(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(count, -2);

    framedata = cpl_framedata_new();
    cpl_test_nonnull(framedata);

    status = cpl_framedata_set_tag(NULL, tags[0]);
    cpl_test_eq_error(status, CPL_ERROR_NULL_INPUT);

    status = cpl_framedata_set_tag(framedata, NULL);
    cpl_test_eq_error(status, CPL_ERROR_ILLEGAL_INPUT);

    status = cpl_framedata_set_tag(framedata, "");
    cpl_test_eq_error(status, CPL_ERROR_ILLEGAL_INPUT);

    status = cpl_framedata_set_min_count(NULL, 0);
    cpl_test_eq_error(status, CPL_ERROR_NULL_INPUT);

    status = cpl_framedata_set_max_count(NULL, 0);
    cpl_test_eq_error(status, CPL_ERROR_NULL_INPUT);

    status = cpl_framedata_set(NULL, tags[0], 0, 5);
    cpl_test_eq_error(status, CPL_ERROR_NULL_INPUT);

    status = cpl_framedata_set(framedata, NULL, 0, 5);
    cpl_test_eq_error(status, CPL_ERROR_ILLEGAL_INPUT);

    status = cpl_framedata_set(framedata, "", 0, 5);
    cpl_test_eq_error(status, CPL_ERROR_ILLEGAL_INPUT);

    cpl_framedata_delete(framedata);

    /*
     * All tests succeeded
     */

    return cpl_test_end(0);

}
