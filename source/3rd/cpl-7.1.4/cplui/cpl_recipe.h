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

#ifndef CPL_RECIPE_H
#define CPL_RECIPE_H

#include <cpl_macros.h>
#include <cpl_plugin.h>
#include <cpl_frameset.h>
#include <cpl_parameterlist.h>
#include <cpl_recipeconfig.h>


CPL_BEGIN_DECLS

/**
 * @defgroup cpl_recipe  Recipes
 *
 * @brief
 *   Recipe plugin interface definition
 *
 * This defines the interface in order to implement recipes as Pluggable
 * Data Reduction Modules (PDRMs). It extends the generic plugin interface
 * with a parameter list and a frame set containing the recipe's setup
 * information (parameters to run with) and the data frames to process.
 *
 * This interface is constructed in such a way, that a pointer to an object
 * of type @c cpl_recipe can be cast into a pointer to @c cpl_plugin (see
 * @ref cpl_plugin).
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_recipe.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *   The recipe plugin data type
 */

typedef struct _cpl_recipe_ cpl_recipe;


/**
 * @brief
 *   The type representation of the recipe plugin interface.
 */

struct _cpl_recipe_ {

    /**
     * @brief
     *   Generic plugin interface
     *
     * See the @ref cpl_plugin documentation for a detailed description.
     */

    cpl_plugin interface;    /* Must be the first member! */

    /**
     * @brief
     *   Pointer to the recipes parameter list, or @c NULL if the recipe
     *   does not provide/accept any parameters.
     *
     * This member points to a @c cpl_parameterlist, containing all
     * parameters the recipe accepts, or @c NULL if the recipe does not
     * need any parameters for execution.
     *
     * An application which wants to execute the recipe may update this list
     * with new parameter values, obtained from the command line for instance.
     */

    cpl_parameterlist *parameters;

    /**
     * @brief
     *   Pointer to a frame set, or @c NULL if no frame set is available.
     *
     * This member points to the frame set (see \ref cpl_frameset) the recipe
     * should process. The frame set to process has to be provided by the
     * application which is going to execute this recipe, i.e. this member
     * has to be set by the application.
     *
     * The recipe can rely on the availability of the frame set at the time
     * the application executes the recipe by calling
     * cpl_plugin::execute. The recipe is free to ignore a provided
     * frame set if it does not need any input frames.
     */

    cpl_frameset *frames;

};


typedef struct _cpl_recipe2_ cpl_recipe2;

struct _cpl_recipe2_ {

    cpl_recipe base;

    cpl_recipeconfig *config;

};

/**@}*/

CPL_END_DECLS

#endif /* CPL_RECIPE_H */
