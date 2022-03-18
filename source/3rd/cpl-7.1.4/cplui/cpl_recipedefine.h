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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef CPL_RECIPEDEFINE_H
#define CPL_RECIPEDEFINE_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_recipe.h>
#include <cpl_plugininfo.h>
#include <cpl_version.h>
#include <cpl_errorstate.h>

CPL_BEGIN_DECLS

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_recipedefine
   @brief   Generate the recipe copyright and license text (GPL v.2)
   @param   PACKAGE_NAME  The name as a string literal, e.g. from config.h
   @param   YEAR          The year(s) as a string literal
   @return  The recipe copyright and license text as a string literal

   Example:
   @code
     const char * eso_gpl_license = cpl_get_license(PACKAGE_NAME, "2005, 2008");
   @endcode

*/
/*----------------------------------------------------------------------------*/

#define cpl_get_license(PACKAGE_NAME, YEAR)                                    \
    "This file is part of the " PACKAGE_NAME "\n"                              \
    "Copyright (C) " YEAR " European Southern Observatory\n"                   \
    "\n"                                                                       \
    "This program is free software; you can redistribute it and/or modify\n"   \
    "it under the terms of the GNU General Public License as published by\n"   \
    "the Free Software Foundation; either version 2 of the License, or\n"      \
    "(at your option) any later version.\n"                                    \
    "\n"                                                                       \
    "This program is distributed in the hope that it will be useful,\n"        \
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"         \
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"          \
    "GNU General Public License for more details.\n"                           \
    "\n"                                                                       \
    "You should have received a copy of the GNU General Public License\n"      \
    "along with this program; if not, write to the Free Software\n"            \
    "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, \n"                \
    "MA  02111-1307  USA"

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_recipedefine
   @brief  Define a standard CPL recipe
   @param  RECIPE_NAME         The name as an identifier
   @param  RECIPE_VERSION      The binary version number
   @param  RECIPE_AUTHOR       The author as a string literal
   @param  RECIPE_EMAIL        The contact email as a string literal
   @param  RECIPE_YEAR         The copyright year as a string literal
   @param  RECIPE_SYNOPSIS     The synopsis as a string literal
   @param  RECIPE_DESCRIPTION  The man-page as a string literal

   A CPL-based recipe may use this macro to define its four mandatory
   functions: cpl_plugin_get_info(), \<recipe\>_create(), \<recipe\>_exec() and
   \<recipe\>_destroy(), as well as declaring the actual data reduction
   function, \<recipe\>() as
   @code

     static int <recipe>(cpl_frameset *, const cpl_parameterlist *);

   @endcode

   The macro also declares the recipe-specific function that fills the
   recipe parameterlist with the supported parameters as
   @code

     static cpl_error_code <recipe>_fill_parameterlist(cpl_parameterlist *self);

   @endcode
   A recipe that invokes cpl_recipe_define() must define this function.

   The macro cpl_recipe_define() may be used by defining a macro, e.g. in
   my_recipe.h:

   @code

   #define MY_RECIPE_DEFINE(NAME, SYNOPSIS, DESCRIPTION)                    \
     cpl_recipe_define(NAME, MY_BINARY_VERSION,                             \
     "Firstname Lastname", "2006, 2008", SYNOPSIS, DESCRIPTION)

   @endcode

   - and then by invoking this macro in each recipe:

   @code

   #include "my_recipe.h"

   MY_RECIPE_DEFINE(instrume_img_dark,
                    "Dark recipe",
                    "instrume_img_dark -- imaging dark recipe.\n"
                    " ... recipe man-page\n");

   static
   cpl_error_code instrume_img_dark_fill_parameterlist(cpl_parameterlist *self);
   {

      // Fill the parameterlist with the parameters supported by the recipe.

      retun CPL_ERROR_NONE;
   }
   @endcode

*/
/*----------------------------------------------------------------------------*/

#define cpl_recipe_define(RECIPE_NAME, RECIPE_VERSION, RECIPE_AUTHOR,          \
                          RECIPE_EMAIL, RECIPE_YEAR,                           \
                          RECIPE_SYNOPSIS, RECIPE_DESCRIPTION)                 \
                                                                               \
    /* The prototypes of the recipe create, exec and destroy functions */      \
static int CPL_CONCAT2X(RECIPE_NAME,create) (cpl_plugin * plugin);             \
static int CPL_CONCAT2X(RECIPE_NAME,exec)   (cpl_plugin * plugin);             \
static int CPL_CONCAT2X(RECIPE_NAME,destroy)(cpl_plugin * plugin);             \
                                                                               \
   /* The prototype of the function called by the recipe exec function */      \
static int RECIPE_NAME(cpl_frameset *, const cpl_parameterlist *);             \
                                                                               \
    /* The prototype of the parameterlist filler function */                   \
static cpl_error_code CPL_CONCAT2X(RECIPE_NAME,fill_parameterlist)             \
    (cpl_parameterlist *);                                                     \
                                                                               \
int cpl_plugin_get_info(cpl_pluginlist * list)                                 \
{                                                                              \
    /* Propagate error, if any. Return 1 on failure */                         \
    return cpl_recipedefine_init(list, CPL_VERSION_CODE,                       \
                                RECIPE_VERSION,                                \
                                #RECIPE_NAME,                                  \
                                RECIPE_SYNOPSIS,                               \
                                RECIPE_DESCRIPTION,                            \
                                RECIPE_AUTHOR,                                 \
                                RECIPE_EMAIL,                                  \
                                cpl_get_license(PACKAGE_NAME, RECIPE_YEAR),    \
                                CPL_CONCAT2X(RECIPE_NAME,create),              \
                                CPL_CONCAT2X(RECIPE_NAME,exec),                \
                                CPL_CONCAT2X(RECIPE_NAME,destroy))             \
        ? ((void)cpl_error_set_where(cpl_func), 1) : 0;                        \
}                                                                              \
                                                                               \
   /* The definition of the recipe create function */                          \
static int CPL_CONCAT2X(RECIPE_NAME,create)(cpl_plugin * plugin)               \
{                                                                              \
    cpl_recipe   * recipe   = (cpl_recipe *)plugin; /* Needed for the fill */  \
    cpl_errorstate prestate = cpl_errorstate_get(); /* To check the fill */    \
                                                                               \
    /* Propagate error, if any */                                              \
    /* - Need two function calls to ensure plugin validity before the fill */  \
    return cpl_recipedefine_create(plugin)                                     \
        || cpl_recipedefine_create_is_ok(prestate,                             \
             CPL_CONCAT2X(RECIPE_NAME,fill_parameterlist)(recipe->parameters)) \
        ? (int)cpl_error_set_where(cpl_func) : 0;                              \
}                                                                              \
                                                                               \
   /* The definition of the recipe exec function */                            \
static int CPL_CONCAT2X(RECIPE_NAME,exec)(cpl_plugin * plugin)                 \
{                                                                              \
    /* Propagate error, if any */                                              \
    return cpl_recipedefine_exec(plugin, RECIPE_NAME)                          \
        ? (int)cpl_error_set_where(cpl_func) : 0;                              \
}                                                                              \
                                                                               \
   /* The definition of the recipe destroy function */                         \
static int CPL_CONCAT2X(RECIPE_NAME,destroy)(cpl_plugin * plugin)              \
{                                                                              \
    /* Propagate error, if any */                                              \
    return cpl_recipedefine_destroy(plugin)                                    \
        ? (int)cpl_error_set_where(cpl_func) : 0;                              \
}                                                                              \
                                                                               \
  /* This dummy declaration requires the macro to be invoked as if it was      \
     a kind of function declaration, with a terminating ; */                   \
extern int cpl_plugin_end


/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_recipedefine
   @brief  Define a standard CPL recipe
   @param  RECIPE_NAME         The name as an identifier
   @param  RECIPE_VERSION      The binary version number
   @param  RECIPE_FILL_PARAMS  A function call to fill the recipe parameterlist.
                               Must evaluate to zero if and only if successful
   @param  RECIPE_AUTHOR       The author as a string literal
   @param  RECIPE_AUTHOR_EMAIL The author email as a string literal
   @param  RECIPE_YEAR         The copyright year as a string literal
   @param  RECIPE_SYNOPSIS     The synopsis as a string literal
   @param  RECIPE_DESCRIPTION  The man-page as a string literal
   @deprecated Use cpl_recipe_define()

*/
/*----------------------------------------------------------------------------*/

#define CPL_RECIPE_DEFINE(RECIPE_NAME, RECIPE_VERSION, RECIPE_FILL_PARAMS,     \
                          RECIPE_AUTHOR, RECIPE_AUTHOR_EMAIL, RECIPE_YEAR,     \
                          RECIPE_SYNOPSIS, RECIPE_DESCRIPTION)                 \
                                                                               \
   /* The prototypes of the recipe create, exec and destroy functions */       \
static int CPL_CONCAT2X(RECIPE_NAME,create) (cpl_plugin * plugin);             \
static int CPL_CONCAT2X(RECIPE_NAME,exec)   (cpl_plugin * plugin);             \
static int CPL_CONCAT2X(RECIPE_NAME,destroy)(cpl_plugin * plugin);             \
                                                                               \
   /* The prototype of the function called by the recipe exec function */      \
static int RECIPE_NAME(cpl_frameset *, const cpl_parameterlist *);             \
                                                                               \
int cpl_plugin_get_info(cpl_pluginlist * list)                                 \
{                                                                              \
    /* Propagate error, if any. Return 1 on failure */                         \
    return cpl_recipedefine_init(list, CPL_VERSION_CODE,                       \
                                RECIPE_VERSION,                                \
                                #RECIPE_NAME,                                  \
                                RECIPE_SYNOPSIS,                               \
                                RECIPE_DESCRIPTION,                            \
                                RECIPE_AUTHOR,                                 \
                                RECIPE_AUTHOR_EMAIL,                           \
                                cpl_get_license(PACKAGE_NAME, RECIPE_YEAR),    \
                                CPL_CONCAT2X(RECIPE_NAME,create),              \
                                CPL_CONCAT2X(RECIPE_NAME,exec),                \
                                CPL_CONCAT2X(RECIPE_NAME,destroy))             \
        ? ((void)cpl_error_set_where(cpl_func), 1) : 0;                        \
}                                                                              \
                                                                               \
   /* The definition of the recipe create function */                          \
static int CPL_CONCAT2X(RECIPE_NAME,create)(cpl_plugin * plugin)               \
{                                                                              \
    cpl_recipe   * recipe   = (cpl_recipe *)plugin; /* Needed for the fill */  \
    cpl_errorstate prestate = cpl_errorstate_get(); /* To check the fill */    \
                                                                               \
    /* Propagate error, if any */                                              \
    /* - Need two function calls to ensure plugin validity before the fill */  \
    return cpl_recipedefine_create(plugin)                                     \
        || cpl_recipedefine_create_is_ok(prestate, (RECIPE_FILL_PARAMS))       \
        ? (int)cpl_error_set_where(cpl_func) : 0;                              \
}                                                                              \
                                                                               \
   /* The definition of the recipe exec function */                            \
static int CPL_CONCAT2X(RECIPE_NAME,exec)(cpl_plugin * plugin)                 \
{                                                                              \
    /* Propagate error, if any */                                              \
    return cpl_recipedefine_exec(plugin, RECIPE_NAME)                          \
        ? (int)cpl_error_set_where(cpl_func) : 0;                              \
}                                                                              \
                                                                               \
   /* The definition of the recipe destroy function */                         \
static int CPL_CONCAT2X(RECIPE_NAME,destroy)(cpl_plugin * plugin)              \
{                                                                              \
    /* Propagate error, if any */                                              \
    return cpl_recipedefine_destroy(plugin)                                    \
        ? (int)cpl_error_set_where(cpl_func) : 0;                              \
}                                                                              \
                                                                               \
  /* This dummy declaration requires the macro to be invoked as if it was      \
     a kind of function declaration, with a terminating ; */                   \
extern int cpl_plugin_end

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/


cpl_error_code cpl_recipedefine_init(cpl_pluginlist *, unsigned long,
                                     unsigned long, const char *, const char *,
                                     const char *, const char *, const char *,
                                     const char *, cpl_plugin_func,
                                     cpl_plugin_func, cpl_plugin_func)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(9, 10, 11, 12)))
#endif
    ;

cpl_error_code cpl_recipedefine_create(cpl_plugin *);

cpl_error_code cpl_recipedefine_create_is_ok(cpl_errorstate, cpl_error_code);

cpl_error_code cpl_recipedefine_exec(cpl_plugin *,
                                     int (*)(cpl_frameset *,
                                             const cpl_parameterlist *));

cpl_error_code cpl_recipedefine_destroy(cpl_plugin *);

CPL_END_DECLS

#endif
/* end of cpl_recipedefine.h */
