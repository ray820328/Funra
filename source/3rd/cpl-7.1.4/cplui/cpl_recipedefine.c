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
#  include <config.h>
#endif
#include "cpl_recipedefine.h"

/* For check of CPL version number inconsistency */
#include <cpl_msg.h>
#include <cpl_error_impl.h>
#include <cpl_memory.h>

/**
 * @defgroup cpl_recipedefine Recipe Definition
 *
 * This module implements the support for recipe defition.
 * 
 * @par Synopsis:
 * @code
 *   #include <cpl_recipedefine.h>
 * @endcode
 */

/**@{*/


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Create the plugin of type recipe
  @param list         The pluginlist to append the plugin to
  @param cplversion   The CPL version number at recipe compile time.
  @param version      The plugin's version number.
  @param name         The plugin's unique name.
  @param synopsis     The plugin's short description (purpose, synopsis ...).
  @param description  The plugin's detailed description.
  @param author       The plugin's author name.
  @param email        The plugin author's e-mail address.
  @param copyright    The plugin's copyright and licensing information.
  @param create       The function used to create the plugin.
  @param execute      The function used to execute the plugin.
  @param destroy      The function used to destroy the plugin.
  @return  CPL_ERROR_NONE iff successful
  @see cpl_recipe_define()
  @note Should never be called directly, only via cpl_recipe_define().
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_recipedefine_init(cpl_pluginlist * list,
                                     unsigned long cplversion,
                                     unsigned long version,
                                     const char *name, const char *synopsis,
                                     const char *description, const char *author,
                                     const char *email, const char *copyright,
                                     cpl_plugin_func create,
                                     cpl_plugin_func execute,
                                     cpl_plugin_func destroy)
{

    cpl_recipe * recipe;
    cpl_plugin * plugin;
    const char * msgname = name ? name : "<NULL>";
    /* Verify that the compile-time and run-time versions of CPL match */
    /* - this will work for run-time versions going back to CPL 3.0    */
    /* - earlier versions will cause an exit with an unresolved symbol */
    const unsigned vruntime = CPL_VERSION(cpl_version_get_major(),
                                          cpl_version_get_minor(),
                                          cpl_version_get_micro());
    /* The version number of the first major version */
    const unsigned vmruntime = CPL_VERSION(cpl_version_get_major(), 0, 0);


    if (vruntime < cplversion) {
        cpl_msg_warning(cpl_func, "Run-time version %s of CPL is lower than "
                        "the version (%lX) used to compile %s",
                        cpl_version_get_version(), cplversion, msgname);
    } else if (vmruntime > cplversion) {
        cpl_msg_warning(cpl_func, "Run-time version %s of CPL has a newer major "
                        "version than the version (%lX) used to compile %s",
                        cpl_version_get_version(), cplversion, msgname);
    } else if (vruntime > cplversion) {
        cpl_msg_info(cpl_func, "Run-time version %s of CPL is higher than "
                     "the version (%lX) used to compile %s",
                     cpl_version_get_version(), cplversion, msgname);
    }

    recipe = cpl_calloc(1, sizeof(*recipe));
    if (recipe == NULL) {
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_OUTPUT,
                                     "Recipe allocation failed");
    }

    plugin = &recipe->interface;

    if (cpl_plugin_init(plugin,
                    CPL_PLUGIN_API,
                    version,
                    CPL_PLUGIN_TYPE_RECIPE,
                    name,
                    synopsis,
                    description,
                    author,
                    email,
                    copyright,
                    create,
                    execute,
                    destroy)) {
        cpl_plugin_delete(plugin);
        return cpl_error_set_message_(cpl_error_get_code(),
                                     "Plugin initialization failed");
    }

    if (cpl_pluginlist_append(list, plugin)) {
        cpl_plugin_delete(plugin);
        return cpl_error_set_message_(cpl_error_get_code(),
                                     "Error adding plugin to list");
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Create the plugin of type recipe
  @param    plugin   The recipe to create
  @return   CPL_ERROR_NONE iff successful
  @see cpl_recipe_define()
  @note Should never be called directly, only via cpl_recipe_define().
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_recipedefine_create(cpl_plugin * plugin)
{
    cpl_recipe * recipe;

    /* Return immediately if an error code is already set */
    if (cpl_error_get_code() != CPL_ERROR_NONE) {
        return cpl_error_set_message_(cpl_error_get_code(),
                                     "An error is already set");
    }

    if (plugin == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Null plugin");
    }

    /* Verify plugin type */
    if (cpl_plugin_get_type(plugin) != CPL_PLUGIN_TYPE_RECIPE) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                     "Plugin is not a recipe: %lu <=> %d",
                                     cpl_plugin_get_type(plugin),
                                     CPL_PLUGIN_TYPE_RECIPE);
    }

    /* Get the recipe */
    recipe = (cpl_recipe *)plugin;

    /* Create the parameters list in the cpl_recipe object */                  
    recipe->parameters = cpl_parameterlist_new();
    if (recipe->parameters == NULL) {
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_OUTPUT,
                                     "Parameter list allocation failed");
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Check the return status of the parameter fill function call
  @param    prestate    The CPL errorstate prior to the call
  @param    fill_error  The return value to check
  @return   CPL_ERROR_NONE iff successful
  @see cpl_recipe_define()
  @note Should never be called directly, only via cpl_recipe_define().
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_recipedefine_create_is_ok(cpl_errorstate prestate,
                                             cpl_error_code fill_error)
{
    /* Propagate any error and set a CPL error
       if the filler failed without setting one */
    return fill_error || !cpl_errorstate_is_equal(prestate)
        ? cpl_error_set_message_(fill_error ? fill_error : cpl_error_get_code(),
                                 "Parameter list fill failed")
        : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Execute the plugin of type recipe using the given function
  @param    plugin   The recipe to execute
  @param    function The function to call
  @return   CPL_ERROR_NONE iff successful
  @see cpl_recipe_define()
  @note Should never be called directly, only via cpl_recipe_define().
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_recipedefine_exec(cpl_plugin * plugin,
                                     int (*function)(cpl_frameset *,
                                                     const cpl_parameterlist *))
{
    cpl_recipe * recipe;
    int recipe_status;
    cpl_errorstate initial_errorstate = cpl_errorstate_get();

    /* Return immediately if an error code is already set */
    if (cpl_error_get_code() != CPL_ERROR_NONE) {
        return cpl_error_set_message_(cpl_error_get_code(),
                                     "An error is already set");
    }

    if (plugin == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Null plugin");
    }

    /* Verify plugin type */
    if (cpl_plugin_get_type(plugin) != CPL_PLUGIN_TYPE_RECIPE) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "Plugin is not a recipe: %lu <=> %d",
                                      cpl_plugin_get_type(plugin),
                                      CPL_PLUGIN_TYPE_RECIPE);
    }

    /* Get the recipe */
    recipe = (cpl_recipe *)plugin;

    /* Verify parameter and frame lists */
    if (recipe->parameters == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Recipe invoked with NULL parameter list");
    }
    if (recipe->frames == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Recipe invoked with NULL frame set");
    }

    if (function == NULL) {
        /* Reaching this point indicates either an error in
           cpl_recipe_define() or that the caller is non-standard. */
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Recipe invoked with NULL function");
    }

    /* Invoke the recipe */
    recipe_status = function(recipe->frames, recipe->parameters);

    if (!cpl_errorstate_is_equal(initial_errorstate)) {
        /* Dump the error history since recipe execution start.
           At this point the recipe cannot recover from the error */
        cpl_errorstate_dump(initial_errorstate, CPL_FALSE, NULL);
    }

    return (cpl_error_code)recipe_status;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Deallocate the parameterlist of a plugin of type recipe
  @param    plugin   The recipe to modify
  @return   CPL_ERROR_NONE iff successful
  @see cpl_recipe_define()
  @note Should never be called directly, only via cpl_recipe_define().
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_recipedefine_destroy(cpl_plugin * plugin)
{
    cpl_recipe * recipe;                                                       
                                                                               
    if (plugin == NULL) {                                                      
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,      
                                     "Null plugin");                   
    }                                                                       
                                                                            
    /* Verify plugin type */                                                
    if (cpl_plugin_get_type(plugin) != CPL_PLUGIN_TYPE_RECIPE) {            
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "Plugin is not a recipe: %lu <=> %d",
                                      cpl_plugin_get_type(plugin),
                                      CPL_PLUGIN_TYPE_RECIPE);
    }                                                                       
                                                                            
    /* Get the recipe */                                                    
    recipe = (cpl_recipe *)plugin;                                          

    cpl_parameterlist_delete(recipe->parameters); /* May be NULL */         
                                                                            
    return CPL_ERROR_NONE;
}

/**@}*/
