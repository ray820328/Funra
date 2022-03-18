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
#include "cpl_recipeconfig.h"

/* Needed for cx_strfreev() */
#include "cxstrutils.h"

int main(void)
{

    char** tags = NULL;

    int status = 0;

    cpl_recipeconfig* config = NULL;

    const cpl_framedata frames[] = {
        {"frameA", 1, 5},
        {"frameB", 0, -1},
        {NULL, 0, 0}
    };

    const cpl_framedata inputs[] = {
        {"inputA", 1, -1},
        {"inputB", 0, -1},
        {"inputC",  -1, -1},
        {NULL, 0, 0}
    };

    const cpl_framedata _inputs[] = {
        {"inputD", 0, 3},
        {"inputE", 1, 1},
        {"inputF",  1, -1},
        {NULL, 0, 0}
    };

    const char* outputs[] = {"outputA", "outputB", "outputC", NULL};

    const cpl_framedata bad[] = {
        {"", 1, 3},
        {NULL, 0, 0}
    };

    const char* _bad[] = {"outputA", "", "outputC", NULL};


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /*
     * Test 1: Create a recipe configuration object, check its
     *         validity and destroy it again.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    tags = cpl_recipeconfig_get_tags(config);
    cpl_test_nonnull(tags);
    cpl_test_null(tags[0]);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 2: Create a recipe configuration object, add a configuration
     *         tag check its validity and destroy it again.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_tag(config, frames[0].tag, 1, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    tags = cpl_recipeconfig_get_tags(config);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], frames[0].tag);
    cpl_test_null(tags[1]);
    cpl_test_eq(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                               frames[0].tag), 1);
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                               frames[0].tag), -1);
    cpl_test_eq(cpl_recipeconfig_is_required(config, frames[0].tag,
                                             frames[0].tag), 1);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 3: Create a recipe configuration object, add a configuration
     *         tag, clear it and destroy it again.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_tag(config, frames[0].tag, 1, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    cpl_recipeconfig_clear(config);

    tags = cpl_recipeconfig_get_tags(config);
    cpl_test_nonnull(tags);
    cpl_test_null(tags[0]);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 4: Create a recipe configuration object, add a configuration
     *         tag, an input configuration, and an output configuration.
     *         Verify the object and destroy it again.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_tag(config, frames[0].tag, 1, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_input(config, frames[0].tag,
                                        inputs[0].tag, 2, 5);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_input(config, frames[0].tag,
                                        inputs[1].tag, 0, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_output(config, frames[0].tag, outputs[0]);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_output(config, frames[0].tag, outputs[1]);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_output(config, frames[0].tag, outputs[2]);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    tags = cpl_recipeconfig_get_inputs(config, frames[0].tag);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], inputs[0].tag);
    cpl_test_eq(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                               inputs[0].tag), 2);
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                               inputs[0].tag), 5);
    cpl_test_eq(cpl_recipeconfig_is_required(config, frames[0].tag,
                                             inputs[0].tag), 1);

    cpl_test_eq_string(tags[1], inputs[1].tag);
    cpl_test_zero(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                             inputs[1].tag));
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                             inputs[1].tag), -1);
    cpl_test_zero(cpl_recipeconfig_is_required(config, frames[0].tag,
                                           inputs[1].tag));

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    tags = cpl_recipeconfig_get_outputs(config, frames[0].tag);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], outputs[0]);
    cpl_test_eq_string(tags[1], outputs[1]);
    cpl_test_eq_string(tags[2], outputs[2]);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 5: Create a recipe configuration object, add a configuration
     *         tag, and check that adding the same tag properly replaces
     *         the previous settings.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_tag(config, frames[0].tag, 1, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    cpl_test_eq(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                             frames[0].tag), 1);
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                             frames[0].tag), -1);
    cpl_test_eq(cpl_recipeconfig_is_required(config, frames[0].tag,
                                           frames[0].tag), 1);

    status = cpl_recipeconfig_set_input(config, frames[0].tag,
                                        inputs[0].tag, 0, 5);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    cpl_test_zero(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                             inputs[0].tag));
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                             inputs[0].tag), 5);
    cpl_test_zero(cpl_recipeconfig_is_required(config, frames[0].tag,
                                           inputs[0].tag));

    status = cpl_recipeconfig_set_tag(config, frames[0].tag, 2, 5);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    cpl_test_eq(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                             frames[0].tag), 2);
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                             frames[0].tag), 5);
    cpl_test_eq(cpl_recipeconfig_is_required(config, frames[0].tag,
                                           frames[0].tag), 1);

    status = cpl_recipeconfig_set_input(config, frames[0].tag,
                                        inputs[0].tag, 1, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    cpl_test_eq(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                             inputs[0].tag), 1);
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                             inputs[0].tag), -1);
    cpl_test_eq(cpl_recipeconfig_is_required(config, frames[0].tag,
                                           inputs[0].tag), 1);

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 6: Create a recipe configuration object, add multiple
     *         configurations. Check the object.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_tags(config, frames);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    tags = cpl_recipeconfig_get_tags(config);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], frames[0].tag);
    cpl_test_eq_string(tags[1], frames[1].tag);
    cpl_test_null(tags[2]);
    cpl_test_eq(cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                               frames[0].tag),
                frames[0].min_count);
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                               frames[0].tag),
                frames[0].max_count);
    cpl_test_eq(cpl_recipeconfig_is_required(config, frames[0].tag,
                                             frames[0].tag),
                (frames[0].min_count > 0));
    cpl_test_eq(cpl_recipeconfig_get_min_count(config, frames[1].tag,
                                               frames[1].tag),
                frames[1].min_count);
    cpl_test_eq(cpl_recipeconfig_get_max_count(config, frames[1].tag,
                                               frames[1].tag),
                frames[1].max_count);
    cpl_test_eq(cpl_recipeconfig_is_required(config, frames[1].tag,
                                             frames[1].tag),
                (frames[1].min_count > 0));

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    status = cpl_recipeconfig_set_inputs(config, frames[0].tag, inputs);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    tags = cpl_recipeconfig_get_inputs(config, frames[0].tag);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], inputs[0].tag);
    cpl_test_eq_string(tags[1], inputs[1].tag);
    cpl_test_eq_string(tags[2], inputs[2].tag);
    cpl_test_null(tags[3]);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    status = cpl_recipeconfig_set_outputs(config, frames[0].tag, outputs);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    tags = cpl_recipeconfig_get_outputs(config, frames[0].tag);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], outputs[0]);
    cpl_test_eq_string(tags[1], outputs[1]);
    cpl_test_eq_string(tags[2], outputs[2]);
    cpl_test_null(tags[3]);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    status = cpl_recipeconfig_set_inputs(config, frames[1].tag, _inputs);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    tags = cpl_recipeconfig_get_inputs(config, frames[1].tag);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], _inputs[0].tag);
    cpl_test_eq_string(tags[1], _inputs[1].tag);
    cpl_test_eq_string(tags[2], _inputs[2].tag);
    cpl_test_null(tags[3]);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    status = cpl_recipeconfig_set_outputs(config, frames[1].tag, outputs);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);
    tags = cpl_recipeconfig_get_outputs(config, frames[1].tag);
    cpl_test_nonnull(tags);
    cpl_test_eq_string(tags[0], outputs[0]);
    cpl_test_eq_string(tags[1], outputs[1]);
    cpl_test_eq_string(tags[2], outputs[2]);
    cpl_test_null(tags[3]);

    cx_strfreev((cxchar**)tags);
    tags = NULL;

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 7: Create a recipe configuration object and check that
     *         input and output configurations cannot be added without
     *         adding the configuration tags before.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_input(config, frames[0].tag, inputs[0].tag,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, 1);

    status = cpl_recipeconfig_set_inputs(config, frames[0].tag, inputs);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, 1);

    status = cpl_recipeconfig_set_output(config, frames[0].tag, outputs[0]);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, 1);

    status = cpl_recipeconfig_set_outputs(config, frames[0].tag, outputs);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, 1);

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 8: Create a recipe configuration object and check that
     *         a configuration tag cannot be reused as input frame
     *         tag.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_tag(config, frames[0].tag, -1, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_input(config, frames[0].tag, frames[0].tag,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * Test 9: Check error conditions of the member functions.
     */

    config = cpl_recipeconfig_new();
    cpl_test_nonnull(config);

    status = cpl_recipeconfig_set_tag(config, frames[0].tag, -1, -1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    tags = cpl_recipeconfig_get_tags(NULL);
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    tags = cpl_recipeconfig_get_inputs(NULL, frames[0].tag);
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    tags = cpl_recipeconfig_get_inputs(config, NULL);
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    tags = cpl_recipeconfig_get_inputs(config, "");
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);

    tags = cpl_recipeconfig_get_inputs(config, frames[1].tag);
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);


    tags = cpl_recipeconfig_get_outputs(NULL, frames[0].tag);
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    tags = cpl_recipeconfig_get_outputs(config, NULL);
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    tags = cpl_recipeconfig_get_outputs(config, "");
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);

    tags = cpl_recipeconfig_get_outputs(config, frames[1].tag);
    cpl_test_null(tags);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);


    status = cpl_recipeconfig_get_min_count(NULL, frames[0].tag,
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_min_count(config, NULL,
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_min_count(config, "",
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_min_count(config, frames[1].tag,
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                            NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                            "");
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_min_count(config, frames[0].tag,
                                            inputs[1].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);


    status = cpl_recipeconfig_get_max_count(NULL, frames[0].tag,
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_max_count(config, NULL,
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_max_count(config, "",
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_max_count(config, frames[1].tag,
                                            inputs[0].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                            NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                            "");
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_get_max_count(config, frames[0].tag,
                                            inputs[1].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_is_required(NULL, frames[0].tag,
                                          inputs[0].tag);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_is_required(config, NULL,
                                          inputs[0].tag);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_is_required(config, "",
                                          inputs[0].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_is_required(config, frames[1].tag,
                                          inputs[0].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_is_required(config, frames[0].tag,
                                          NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_is_required(config, frames[0].tag,
                                          "");
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_is_required(config, frames[0].tag,
                                          inputs[1].tag);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, -1);


    status = cpl_recipeconfig_set_tag(NULL, frames[1].tag, -1, -1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_tag(config, NULL, -1, -1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_tag(config, "", -1, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);


    status = cpl_recipeconfig_set_input(NULL, frames[0].tag, inputs[0].tag,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_input(config, NULL, inputs[0].tag,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_input(config, "", inputs[0].tag,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_input(config, frames[1].tag, inputs[0].tag,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, 2);

    status = cpl_recipeconfig_set_input(config, frames[0].tag, NULL,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_input(config, frames[0].tag, "",
                                        -1, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_input(config, frames[0].tag, frames[0].tag,
                                        -1, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);


    status = cpl_recipeconfig_set_output(NULL, frames[0].tag, outputs[0]);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_output(config, NULL, outputs[0]);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_output(config, "", outputs[0]);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_output(config, frames[0].tag, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_output(config, frames[0].tag, "");
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);


    status = cpl_recipeconfig_set_tags(NULL, frames);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_tags(config, NULL);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_tags(config, bad);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, 1);


    status = cpl_recipeconfig_set_inputs(NULL, frames[0].tag, inputs);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_inputs(config, NULL, inputs);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_inputs(config, "", inputs);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_inputs(config, frames[1].tag, inputs);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, 2);

    status = cpl_recipeconfig_set_inputs(config, frames[0].tag, NULL);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_inputs(config, frames[0].tag, bad);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, 3);


    status = cpl_recipeconfig_set_outputs(NULL, frames[0].tag, outputs);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_outputs(config, NULL, outputs);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_outputs(config, "", outputs);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, -1);

    status = cpl_recipeconfig_set_outputs(config, frames[1].tag, outputs);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_eq(status, 2);

    status = cpl_recipeconfig_set_outputs(config, frames[0].tag, NULL);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(status);

    status = cpl_recipeconfig_set_outputs(config, frames[0].tag, _bad);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq(status, 3);

    cpl_recipeconfig_delete(config);
    config = NULL;


    /*
     * All tests finished
     */

    return cpl_test_end(0);

}
