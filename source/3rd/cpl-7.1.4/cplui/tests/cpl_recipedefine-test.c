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

#include "cpl_test.h"
#include "cpl_recipedefine.h"

/*----------------------------------------------------------------------------
                 Functions prototypes
 ----------------------------------------------------------------------------*/
static void cpl_recipedefine_test(cpl_pluginlist *);

cpl_recipe_define(my_recipe, 10101,
                  "Fname Lname", PACKAGE_BUGREPORT, "2008", 
                  "Test recipe", "Description of test recipe");

/* Enable my_recipe_fill_parameterlist() to either pass or fail */
static cpl_error_code fill_status = CPL_ERROR_NONE;

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/

int main(void)
{

    cpl_pluginlist * pluginlist;
    int error;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    error = cpl_plugin_get_info(NULL);

    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(1, error);

    pluginlist = cpl_pluginlist_new();

    cpl_test_zero(cpl_plugin_get_info(pluginlist));

    cpl_recipedefine_test(pluginlist);

    cpl_pluginlist_delete(pluginlist);

    return cpl_test_end(0);
}

static cpl_error_code my_recipe_fill_parameterlist(cpl_parameterlist * self)
{

    cpl_test_nonnull(self);

    return fill_status;

}


/*----------------------------------------------------------------------------*/
/**
  @brief    This recipe does nothing
  @param    frameset  A frameset
  @param    plist  A parameterlist
  @return   0 iff successful

 */
/*----------------------------------------------------------------------------*/
static int my_recipe(cpl_frameset * frameset, const cpl_parameterlist * plist)
{

    /* The recipe definition ensures that the actual recipe execution is done
       with non-NULL parameters */

    cpl_test_nonnull(frameset);
    cpl_test_nonnull(plist);

    return 0;

}


/*----------------------------------------------------------------------------*/
/**
  @brief    Find a plugin and submit it to some tests
  @param    self   A non-empty pluginlist
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_recipedefine_test(cpl_pluginlist * self)
{

    cpl_plugin     * plugin;
    cpl_recipe     * recipe;
    int            (*recipe_create) (cpl_plugin *);
    int            (*recipe_exec  ) (cpl_plugin *);
    int            (*recipe_deinit) (cpl_plugin *);
    cpl_errorstate prestate = cpl_errorstate_get();
    int            error;
    FILE           * stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    plugin = cpl_pluginlist_get_first(self);
    recipe = (cpl_recipe *) plugin;

    cpl_test_nonnull(plugin);

    cpl_plugin_dump(plugin, stream);

    recipe_create = cpl_plugin_get_init(plugin);
    recipe_exec   = cpl_plugin_get_exec(plugin);
    recipe_deinit = cpl_plugin_get_deinit(plugin);

    /* Only plugins of type recipe are defined */
    cpl_test_eq(cpl_plugin_get_type(plugin), CPL_PLUGIN_TYPE_RECIPE);

    cpl_test_eq(recipe_create(plugin), CPL_ERROR_NONE);

    cpl_test_nonnull(recipe->parameters);

    cpl_parameterlist_dump(recipe->parameters, stream);


    /* Test 1: Check handling of NULL frameset */
    recipe->frames = NULL;

    /* Execute recipe */
    cpl_test( recipe_exec(plugin) );

    /* Expect the errorstate to change */
    cpl_test_zero( cpl_errorstate_is_equal(prestate) );

    /* ... and the error code to not change */
    cpl_test_error( CPL_ERROR_NULL_INPUT );


    /* Test 2: Check handling of pre-existing error code */
    recipe->frames = cpl_frameset_new();

    cpl_error_set(cpl_func, CPL_ERROR_EOL);
    prestate = cpl_errorstate_get();
    /* Execute recipe */
    cpl_test( recipe_exec(plugin) );

    /* Expect the errorstate to change */
    cpl_test_zero( cpl_errorstate_is_equal(prestate) );

    /* ... and the error code to not change */
    cpl_test_error( CPL_ERROR_EOL );


    /* Test 3: Check success on empty frameset */

    /* Execute recipe */
    cpl_test_zero( recipe_exec(plugin) );

    /* Expect the error code to not change */
    cpl_test_error( CPL_ERROR_NONE );

    cpl_frameset_delete(recipe->frames);

    cpl_test_zero(recipe_deinit(plugin) );

    /* Test 4: Check failure on failed parameterlist fill */

    fill_status = CPL_ERROR_EOL;

    error = recipe_create(plugin);

    cpl_test_error( error );

    cpl_test_zero(recipe_deinit(plugin) );

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return;

}
