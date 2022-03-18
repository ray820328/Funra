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

#ifndef CPL_PLUGIN_H
#define CPL_PLUGIN_H

#include <stdio.h>

#include <cpl_macros.h>
#include <cpl_error.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_plugin
 *
 * @brief
 *   Plugin API version
 *
 * @hideinitializer
 */

#define CPL_PLUGIN_API  (1)


/**
 * @ingroup cpl_plugin
 *
 * @brief
 *   Definition of plugin types
 *
 * Predefined plugin types supported by the Common Pipeline Library itself.
 */

enum _cpl_plugin_type_ {

    /**
     * Plugin is of unknown or undefined type
     */
    CPL_PLUGIN_TYPE_NONE   = 0,

    /**
     * Plugin is a complete data reduction task, i.e. a sequence of individual
     * data reduction steps, turning a raw frame into a `final' product.
     */
    CPL_PLUGIN_TYPE_RECIPE = 1 << 0,

    /**
     * Plugin is a recipe, i.e. a complete data reduction task. In addition,
     * this recipe version provides extra data about the required input
     * data. This plugin is a subclass of @c CPL_PLUGIN_TYPE_RECIPE.
     */
    CPL_PLUGIN_TYPE_RECIPE_V2 = (1 << 1) | CPL_PLUGIN_TYPE_RECIPE

};


/**
 * @ingroup cpl_plugin
 *
 * @brief
 *   Data type used to store the plugin type code.
 */

typedef enum _cpl_plugin_type_ cpl_plugin_type;


/**
 * @ingroup cpl_plugin
 *
 * @brief
 *   The plugin data type.
 *
 * This defines the (public) plugin data type.
 */

typedef struct _cpl_plugin_ cpl_plugin;

typedef int (*cpl_plugin_func)(cpl_plugin *);


/**
 * @ingroup cpl_plugin
 *
 * @brief
 *   The type representation of the generic plugin interface.
 */

struct _cpl_plugin_ {

    /**
     * @brief
     *   The API version the Plugin complies to.
     *
     * The API version number identifies the internal layout of the plugin
     * interface structure. It may be used by an application calling a plugin
     * to setup the correct interface to communicate with the plugin or, in
     * the simplest case, to ignore any plugin which does not match the plugin
     * API an application has been buid for.
     */

    unsigned int api; /* Must always be the first entry in the interface! */

    /**
     * @brief
     *   The Plugin version.
     *
     * The Plugin version number defines the version number for the plugin.
     * The Plugin version number is an encoded version of the
     * usual @b MAJOR.MINOR.MICRO form for version numbers.
     */

    unsigned long version;

    /**
     * @brief
     *   The Plugin type.
     *
     * The Plugin type identifies the type of plugin. The data type is not a
     * cpl_plugin_type in order to keep this interface as generic as possible.
     */

    unsigned long type;

    /**
     * @brief
     *   Plugin's unique name.
     *
     * Variable contains the unique name of the Plugin. To ensure uniqueness
     * across all possible Plugins one should follow the hierarchical naming
     * convention mentioned in the CPL documentation.
     */

    const char *name;

    /**
     * @brief
     *   Plugin's short help string.
     *
     * Variable contains the plugin's null-terminated short help string.
     * The short help string should summarize the plugin's purpose in not more
     * than a few lines. It may contain new line characters. If the plugin
     * does not provide a short help the pointer should be set
     * to a @c NULL pointer.
     */

    const char *synopsis;

    /**
     * @brief
     *   Plugin's detailed description.
     *
     * Variable contains the plugin's null-terminated detailed
     * description string. The description is the detailed help for the
     * plugin. For formatting the output the C special characters @c '\\n',
     * @c '\\t' maybe embedded in the returned string. If the plugin does not
     * provide a detailed description the pointer should be set to a @c NULL
     * pointer.
     */

    const char *description;

    /**
     * @brief
     *   Name of the plugin's author.
     *
     * Variable contains the null-terminated identifier string of
     * the plugins author. If the plugin does not specify an author this
     * pointer should be set to a @c NULL pointer.
     */

    const char *author;

    /**
     * @brief
     *   Author's email address.
     *
     * Variable contains the null-terminated string of the author's
     * email address. If the plugin does not specify an email address this
     * pointer should be set to a @c NULL pointer.
     */

    const char *email;

    /**
     * @brief
     *   Plugin's copyright.
     *
     * Variable contains the copyright and license string applying
     * to the plugin. The returned string must be null-terminated. If no
     * copyright applies this pointer should be set to a @c NULL pointer.
     */

    const char *copyright;

    /**
     * @brief
     *   Initializes a plugin instance.
     *
     * @param plugin  The plugin to instantiate.
     *
     * @return The function must return 0 on success, and a non-zero value
     *   if the plugin instantiation failed.
     *
     * The function to initialize a plugin instance. This maybe NULL
     * if the initialization of the plugin is not needed. Otherwise it
     * has to be called before plugin type specific members are accessed.
     */

    cpl_plugin_func initialize;

    /**
     * @brief
     *   Executes a plugin instance.
     *
     * @param plugin  The plugin to execute.
     *
     * @return The function must return 0 on success, and a non-zero value
     *   if the plugin execution failed.
     *
     * The function executes the plugin instance @em plugin.
     */

    cpl_plugin_func execute;

    /**
     * @brief
     *   Deinitialization a plugin instance.
     *
     * @return The function must return 0 on success, and a non-zero value
     *   if the plugin deinitalization failed.
     *
     * The function to deinitialize the plugin instance @em plugin. If this is
     * @c NULL no deinitialization of the plugin instance is needed.
     */

    cpl_plugin_func deinitialize;

};


/*
 * Create, copy and destroy operations.
 */

cpl_plugin *cpl_plugin_new(void) CPL_ATTR_ALLOC;
cpl_error_code cpl_plugin_copy(cpl_plugin *self, const cpl_plugin *other);
void cpl_plugin_delete(cpl_plugin *self);

/*
 *  Accessor Functions
 */

cpl_error_code cpl_plugin_set_api(cpl_plugin *self , unsigned int api);
unsigned int cpl_plugin_get_api(const cpl_plugin *self);

int cpl_plugin_set_version(cpl_plugin *self, unsigned long version);
unsigned long cpl_plugin_get_version(const cpl_plugin *self);
char *cpl_plugin_get_version_string(const cpl_plugin *self) CPL_ATTR_ALLOC;

cpl_error_code cpl_plugin_set_type(cpl_plugin *self, unsigned long type);
unsigned long cpl_plugin_get_type(const cpl_plugin *self);
char *cpl_plugin_get_type_string(const cpl_plugin *self) CPL_ATTR_ALLOC;

cpl_error_code cpl_plugin_set_name(cpl_plugin *self, const char *name);
const char *cpl_plugin_get_name(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_synopsis(cpl_plugin *self, const char *synopsis);
const char *cpl_plugin_get_synopsis(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_description(cpl_plugin *self,
                                          const char *description);
const char *cpl_plugin_get_description(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_author(cpl_plugin *self, const char *author);
const char *cpl_plugin_get_author(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_email(cpl_plugin *self, const char *email);
const char *cpl_plugin_get_email(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_copyright(cpl_plugin *self,
                                        const char *copyright);
const char *cpl_plugin_get_copyright(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_init(cpl_plugin *self, cpl_plugin_func);
cpl_plugin_func cpl_plugin_get_init(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_exec(cpl_plugin *self, cpl_plugin_func);
cpl_plugin_func cpl_plugin_get_exec(const cpl_plugin *self);

cpl_error_code cpl_plugin_set_deinit(cpl_plugin *self, cpl_plugin_func);
cpl_plugin_func cpl_plugin_get_deinit(const cpl_plugin *self);


/*
 *  Convenience Functions
 */

cpl_error_code cpl_plugin_init(cpl_plugin *self, unsigned int api,
                               unsigned long version, unsigned long type,
                               const char *name, const char *synopsis,
                               const char *description, const char *author,
                               const char *email, const char *copyright,
                               cpl_plugin_func initialize,
                               cpl_plugin_func execute,
                               cpl_plugin_func deinitialize);

/*
 * Debugging
 */

void cpl_plugin_dump(const cpl_plugin *self, FILE *stream);

CPL_END_DECLS

#endif /* CPL_PLUGIN_H */
