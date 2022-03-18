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

#include <stdio.h>

#include <cxmemory.h>
#include <cxmessages.h>
#include <cxstrutils.h>

#include "cpl_macros.h"
#include "cpl_error_impl.h"
#include "cpl_memory.h"
#include "cpl_msg.h"
#include "cpl_plugin.h"


/**
 * @defgroup cpl_plugin  Plugin Interface
 *
 * @brief
 *   The basic plugin interface definition
 *
 * This module defines the basic plugin interface. The plugin interface
 * provides the possibility to dynamically load and execute software modules.
 * The basic plugin interface defined here serves as `superclass' for
 * context specific plugins. All context specific plugins inherit this
 * basic plugin interface. A plugin context is represented by a type code,
 * i.e. the different plugin contexts are represented as different plugin
 * types.
 *
 * Most of the time an application using the plugin interface is dealing
 * only with this basic, plugin type independent part of the interface.
 * It provides the application with the necessary information about
 * a particular plugin implementation and the services to initialise,
 * execute and cleanup a dynamically loaded module.
 *
 * In this way plugin type specific details may remain hidden from the
 * application until the plugin type and its implementation details
 * are known through querying the basic plugin interface part.
 *
 * To obtain a filled plugin interface structure the application will call
 * the function @c cpl_plugin_get_info(), which has the following prototype:
 * @code
 *   #include <cpl_pluginlist.h>
 *
 *   cpl_plugin_get_info(cpl_pluginlist *list);
 * @endcode
 *
 * For each plugin library (a shared object library containing one or
 * more plugins) this function must be implemented by the plugin developer.
 * Its purpose is to fill a plugin interface structure for each plugin the
 * plugin library contains and add it to a list provided by the application.
 * This list of plugin interfaces provides the application with all the
 * details on how to communicate with a particular plugin.
 *
 * As an example on how to create a context specific plugin, i.e. how to
 * create a new plugin type, you may have a look at @ref cpl_recipe.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_plugin.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *    Create a new, empty plugin interface
 *
 * @return
 *   The pointer to a newly allocated plugin or @c NULL if it
 *   could not be created.
 *
 * The function allocates memory for a @c cpl_plugin and initialises it. The
 * function returns a handle for the newly created plugin interface object.
 * The created plugin interface must be destroyed using the plugin interface
 * destructor @b cpl_plugin_delete().
 */

cpl_plugin *
cpl_plugin_new(void)
{

    cpl_plugin *self = (cpl_plugin *) cx_malloc(sizeof *self);


    self->api          = 0;
    self->version      = 0L;
    self->type         = 0L;

    self->name         = NULL;
    self->synopsis     = NULL;
    self->description  = NULL;
    self->author       = NULL;
    self->email        = NULL;
    self->copyright    = NULL;

    self->initialize   = NULL;
    self->execute      = NULL;
    self->deinitialize = NULL;

    return self;

}


/**
 * @brief
 *    Copy a plugin.
 *
 * @param self   A plugin.
 * @param other  The plugin structure to copy.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>other</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function copies all members of the plugin @em other to the plugin
 * @em self. The plugin @em other and its copy @em self do not share any
 * resources after the copy operation. If either @em self, or @em other
 * are invalid pointers, the function returns immediately.
 *
 * Note that the plugins @em self and @em other do not need to be of the
 * same kind (see below).
 *
 * @attention
 *   If a @b derived plugin (a @c cpl_recipe for instance) is passed to
 *   this function, after casting it to its base, a @c cpl_plugin this
 *   function most likely will not work as expected, since @b data slicing
 *   will happen. The function is only aware of the @c cpl_plugin part
 *   of the derived plugin and only these data members are copied. Any
 *   additional data members defined by the derived plugin are therefore
 *   lost, unless they are copied explicitly.
 *
 * The should be used function as follows. If necessary, create the target
 * plugin using the appropriate constructor, or allocate a memory block of the
 * appropriate size to hold the @b derived plugin type. Use this function
 * to copy the @c cpl_plugin part of your derived plugin. Copy additional
 * data members of the target plugin explicitly.
 */

cpl_error_code
cpl_plugin_copy(cpl_plugin *self, const cpl_plugin *other)
{



    if (self == NULL || other == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->api = other->api;
    self->version = other->version;
    self->type = other->type;

    if (self->name != NULL) {
        cx_free((cxptr)self->name);
    }
    self->name = cx_strdup(other->name);

    if (self->synopsis != NULL) {
        cx_free((cxptr)self->synopsis);
    }
    self->synopsis = cx_strdup(other->synopsis);

    if (self->description != NULL) {
        cx_free((cxptr)self->description);
    }
    self->description = cx_strdup(other->description);

    if (self->author != NULL) {
        cx_free((cxptr)self->author);
    }
    self->author = cx_strdup(other->author);

    if (self->email != NULL) {
        cx_free((cxptr)self->email);
    }
    self->email = cx_strdup(other->email);

    if (self->copyright != NULL) {
        cx_free((cxptr)self->copyright);
    }
    self->copyright = cx_strdup(other->copyright);

    self->initialize = other->initialize;
    self->execute = other->execute;
    self->deinitialize = other->deinitialize;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *    Destroy a plugin.
 *
 * @param self  The plugin to destroy.
 *
 * @return Nothing.
 *
 * The function destroys a plugin. First, the memory used by the members
 * of the @c cpl_plugin type is released and then the plugin itself is
 * destroyed.
 *
 * @attention
 *   The function may also be used to destroy plugins which have been
 *   derived from the @c cpl_plugin type. But if the derived plugin type
 *   defines members which are dynamically allocated, they have to be
 *   destroyed explicitly before (in the plugin's cleanup handler for
 *   instance) the derived plugin is passed to this function, or the
 *   references to these memory blocks have to be kept and cleaned up
 *   later. Otherwise memory leaks may result. If the plugin @em self
 *   is @c NULL, nothing is done and no error is set.
 */

void
cpl_plugin_delete(cpl_plugin *self)
{

    if (self != NULL) {

        self->api          = 0;
        self->version      = 0L;
        self->type         = 0L;

        if (self->name != NULL) {
            cx_free((cxptr)self->name);
            self->name = NULL;
        }

        if (self->synopsis != NULL) {
            cx_free((cxptr)self->synopsis);
            self->synopsis = NULL;
        }

        if (self->description != NULL) {
            cx_free((cxptr)self->description);
            self->description = NULL;
        }

        if (self->author != NULL) {
            cx_free((cxptr)self->author);
            self->author = NULL;
        }

        if (self->email != NULL) {
            cx_free((cxptr)self->email);
            self->email = NULL;
        }

        if (self->copyright != NULL) {
            cx_free((cxptr)self->copyright);
            self->copyright = NULL;
        }

        self->initialize   = NULL;
        self->execute      = NULL;
        self->deinitialize = NULL;

        cx_free(self);

    }

    return;

}


/**
 * @brief
 *   Set the plugin interface version number.
 *
 * @param self  A plugin
 * @param api   The plugin interface version to set.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function sets the version number of the plugin interface of the
 * plugin @em self to the version @em api.
 *
 * @attention
 *   The plugin interface version describes the internal layout of the
 *   plugin interface. It should be used by an application to decide whether
 *   a particular plugin can be used or not, i.e. if the plugin interface
 *   supported by the application is compatible with the interface presented
 *   by the plugin itself.
 */

cpl_error_code
cpl_plugin_set_api(cpl_plugin *self, unsigned int api)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->api = api;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the version number of the plugin interface implementation.
 *
 * @param self  A plugin.
 *
 * @return
 *   The version number of the plugin interface implementation the
 *   plugin @em self complies with. If an error occurs the function
 *   returns 0 and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns the plugin interface version number, i.e. the
 * version number describing the layout of the plugin data type itself.
 * Valid version numbers are always greater or equal to 1.
 */

unsigned int
cpl_plugin_get_api(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return self->api;

}


/**
 * @brief
 *   Set the version number of a plugin.
 *
 * @param self     A plugin
 * @param version  The plugin's version number to set.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function sets the version number of the plugin interface of the
 * plugin @em self to the version @em api.
 *
 * This function sets the version number of the plugin @em self to
 * @em version.
 */

int
cpl_plugin_set_version(cpl_plugin *self, unsigned long version)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->version = version;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the version number of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The plugin's version number. If an error occurs the function
 *   returns 0 and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns the version number of the plugin @em self.
 */

unsigned long
cpl_plugin_get_version(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return self->version;

}


/**
 * @brief
 *   Get the version number of a plugin as a string.
 *
 * @param self  A plugin.
 *
 * @return
 *   The string representation of the plugin's version number. If an
 *   error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns the version number of the plugin @em self as
 * a string. The function assumes that the integer representation of
 * the plugin version can be decoded into a version string of the
 * usual form: "major.minor.micro"
 *
 * The resulting string is placed in a newly allocated buffer. This
 * buffer must be deallocated using @b cpl_free() by the caller if it
 * is no longer needed.
 */

char *
cpl_plugin_get_version_string(const cpl_plugin *self)
{


    unsigned long version;
    unsigned major;
    unsigned minor;
    unsigned micro;

    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    version = self->version;

    micro   = version % 100;
    version = version / 100;
    minor   = version % 100;
    version = version / 100;
    major   = version;

    return cpl_sprintf("%-u.%-u.%-u", major, minor, micro);

}


/**
 * @brief
 *   Set the type of a plugin.
 *
 * @param self  A plugin.
 * @param type  The plugin type to set.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function sets the type (cf. @ref cpl_plugin_type) of the plugin
 * @em self to @em type.
 */

cpl_error_code
cpl_plugin_set_type(cpl_plugin *self, unsigned long type)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->type = type;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the type of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns the plugin type code. If an error occurs the
 *   function returns CPL_PLUGIN_TYPE_NONE and sets an appropriate error
 *   code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns the type (cf. @ref cpl_plugin_type) of the
 * plugin @em self.
 */

unsigned long
cpl_plugin_get_type(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return (unsigned long) CPL_PLUGIN_TYPE_NONE;
    }

    return self->type;

}


/**
 * @brief
 *   Get the type of a plugin as string.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns the string representation of the plugin type.
 *   If an error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns the plugin type of @em self as a string.
 * The type string is placed into a newly allocated buffer. This
 * buffer must be deallocated using @b cpl_free() by the caller if it
 * is no longer needed.
 */

char *
cpl_plugin_get_type_string(const cpl_plugin *self)
{

    const char *stype;

    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    switch (self->type) {
        case (unsigned long)CPL_PLUGIN_TYPE_NONE:
            stype = "none";
            break;

        case (unsigned long)CPL_PLUGIN_TYPE_RECIPE:
            stype = "recipe";
            break;

        default:
            stype = "undefined";
            break;
    }

    return cx_strdup(stype);
}


/**
 * @brief
 *   Set the name of a plugin.
 *
 * @param self  A plugin.
 * @param name  The plugin's unique name.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>name</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function assigns the name @em name to the plugin @em self.
 *
 * @attention
 *   Since plugins are selected through their name this name should
 *   be choosen carefully in order to avoid name clashes with other
 *   plugins.
 */

cpl_error_code
cpl_plugin_set_name(cpl_plugin *self, const char *name)
{



    if (self == NULL || name == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_free((cxptr)self->name);

    self->name = cx_strdup(name);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the name of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns a pointer to the plugin's unique name string.
 *   If an error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns a reference to the unique name of the plugin
 * @em self. The plugin's name must not be modified using the returned
 * pointer. The appropriate method should be used instead.
 */

const char *
cpl_plugin_get_name(const cpl_plugin *self)
{


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->name;

}


/**
 * @brief
 *   Set the short description of a plugin.
 *
 * @param self      A plugin.
 * @param synopsis  The plugin's synopsis, or NULL.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function copies the short description text from the string
 * @em synopsis to the plugin @em self.
 */

cpl_error_code
cpl_plugin_set_synopsis(cpl_plugin *self, const char *synopsis)
{

    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_free((cxchar *)self->synopsis);

    self->synopsis = synopsis ? cx_strdup(synopsis) : NULL;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the short description of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns a pointer to the plugin's short description.
 *   If an error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns a reference to the short description (its purpose
 * for instance) of the plugin @em self. The plugin's short description
 * must not be modified using this pointer. Use the appropriate method
 * instead!
 */

const char *
cpl_plugin_get_synopsis(const cpl_plugin *self)
{

    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->synopsis;

}


/**
 * @brief
 *   Set the detailed description of a plugin.
 *
 * @param self         A plugin.
 * @param description  The plugin's detailed description, or @c NULL.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function copies the detailed description text from the string
 * @em description to the plugin @em self. The C formatting characters
 * @c '\\n' and @c '\\t' may be embedded in the string @em description.
 */

cpl_error_code
cpl_plugin_set_description(cpl_plugin *self, const char *description)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_free((cxchar *)self->description);

    self->description = description ? cx_strdup(description) : NULL;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the detailed description of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns a pointer to the plugin's detailed description.
 *   If an error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns a reference to the detailed description (a
 * description of the algorithm for instance) of the plugin @em self.
 * The plugin's description must not be modified using this pointer.
 * Use the appropriate method instead!
 */

const char *
cpl_plugin_get_description(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->description;

}


/**
 * @brief
 *   Set the name of the plugin author.
 *
 * @param self    A plugin
 * @param author  The name of the plugin author.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>author</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function copies the plugin author's name from the string @em author
 * to the plugin @em self.
 */

cpl_error_code
cpl_plugin_set_author(cpl_plugin *self, const char *author)
{

    if (self == NULL || author == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_free((cxchar *)self->author);

    self->author = cx_strdup(author);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the name of the plugin author.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns a pointer to the plugin author's name string.
 *   If an error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns a reference to the name of the author of the
 * plugin @em self. The plugin author's name must not be modified using
 * the returned pointer. The appropriate method should be used instead!
 */

const char *
cpl_plugin_get_author(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->author;

}


/**
 * @brief
 *   Set the contact information of a plugin.
 *
 * @param self   A plugin.
 * @param email  The plugin author's contact information.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * @returns The function returns 0 on success, or a non-zero value otherwise.
 *   If @em self is not a valid pointer the function returns 1.
 *
 * This function copies the plugin author contact information from the
 * string @em email to the plugin @em self.
 */

cpl_error_code
cpl_plugin_set_email(cpl_plugin *self, const char *email)
{

    if (self == NULL || email == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_free((cxchar *)self->email);

    self->email = cx_strdup(email);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the contact information of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns a pointer to the plugin author's contact
 *   information string. If an error occurs the function returns
 *   @c NULL and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns a reference to the e-mail address of the author of
 * the plugin @em self. The plugin author's e-mail address must not be
 * modified using the returned pointer. The appropriate method should be
 * used instead!
 */

const char *
cpl_plugin_get_email(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->email;

}


/**
 * @brief
 *   Set the license and copyright information of a plugin.
 *
 * @param self       A plugin.
 * @param copyright  The plugin's license information.
 * @note The license information must be compatible with that of CPL.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>copyright</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function copies the plugin's license information from the
 * string @em copyright to the plugin @em self.
 */

cpl_error_code
cpl_plugin_set_copyright(cpl_plugin *self, const char *copyright)
{

    if (self == NULL || copyright == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_free((cxchar *)self->copyright);

    self->copyright = cx_strdup(copyright);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the license and copyright information of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The function returns a pointer to the plugin's copyright
 *   information string. If an error occurs the function returns
 *   @c NULL and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function returns a reference to the license and copyright information
 * of the plugin @em self. The copyright information must not be
 * modified using the returned pointer. The appropriate method should be
 * used instead!
 */

const char *
cpl_plugin_get_copyright(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->copyright;

}


/**
 * @brief
 *   Set the initialisation handler of a plugin.
 *
 * @param self  A plugin
 * @param func  The plugin's initialisation function
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function installs the function @em func as the initialisation
 * function of the plugin @em self. The registered function must be called
 * by an application before the plugin is executed.
 */

cpl_error_code
cpl_plugin_set_init(cpl_plugin *self, cpl_plugin_func func)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->initialize = func;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the initialisation handler of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The plugin's initalization function. If an error occurs the
 *   function returns @c NULL and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the initalisation function of the plugin @em self.
 */

cpl_plugin_func
cpl_plugin_get_init(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->initialize;

}


/**
 * @brief
 *   Set the execution handler of a plugin.
 *
 * @param self  A plugin.
 * @param func  The plugin's execution function.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function installs the function @em func as the execution
 * function of the plugin @em self. The registered function must be called
 * by an application after the plugin's initialisation function was called.
 * Calling the registered function executes the plugin.
 */

cpl_error_code
cpl_plugin_set_exec(cpl_plugin *self, cpl_plugin_func func)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->execute = func;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the execution handler of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The plugin's execution function. If an error occurs the function
 *   returns @c NULL and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the execution function of the plugin @em self.
 */

cpl_plugin_func
cpl_plugin_get_exec(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->execute;

}


/**
 * @brief
 *   Set the cleanup handler of a plugin.
 *
 * @param self  A plugin
 * @param func  The plugin's cleanup handler.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function installs the function @em func as the cleanup handler
 * of the plugin @em self. The registered function is called after the
 * plugin has been executed to release any acquired resources.
 */

cpl_error_code
cpl_plugin_set_deinit(cpl_plugin *self, cpl_plugin_func func)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->deinitialize = func;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the cleanup handler of a plugin.
 *
 * @param self  A plugin.
 *
 * @return
 *   The plugin's cleanup handler. If an error occurs the function
 *   returns @c NULL and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the cleanup handler of the plugin @em self.
 */

cpl_plugin_func
cpl_plugin_get_deinit(const cpl_plugin *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->deinitialize;

}


/**
 * @brief
 *   Initialise a plugin
 *
 * @param self         The plugin to initialise.
 * @param api          The plugin interface version number.
 * @param version      The plugin's version number.
 * @param type         The plugin's type.
 * @param name         The plugin's unique name.
 * @param synopsis     The plugin's short description (purpose, synopsis ...).
 * @param description  The plugin's detailed description.
 * @param author       The plugin's author name.
 * @param email        The plugin author's e-mail address.
 * @param copyright    The plugin's copyright and licensing information.
 * @param create       The function used to create the plugin.
 * @param execute      The function used to execute the plugin.
 * @param destroy      The function used to destroy the plugin.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function fills the @c cpl_plugin part of a plugin structure. For
 * information on which information is required and which is optional
 * please refer to the documentation of the plugin structure _cpl_plugin_.
 *
 * If @em self is not a valid pointer, the function returns immediately.
 */

cpl_error_code
cpl_plugin_init(cpl_plugin *self, unsigned int api, unsigned long version,
                unsigned long type, const char *name, const char *synopsis,
                const char *description, const char *author, const char *email,
                const char *copyright, cpl_plugin_func create,
                cpl_plugin_func execute, cpl_plugin_func destroy)
{

    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (name == NULL || synopsis == NULL || description == NULL ||
        author == NULL || email == NULL || copyright == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cpl_plugin_set_api(self, api);
    cpl_plugin_set_version(self, version);
    cpl_plugin_set_type(self, type);
    cpl_plugin_set_name(self, name);
    cpl_plugin_set_synopsis(self, synopsis);
    cpl_plugin_set_description(self, description);
    cpl_plugin_set_author(self, author);
    cpl_plugin_set_email(self, email);
    cpl_plugin_set_copyright(self, copyright);
    cpl_plugin_set_init(self, create);
    cpl_plugin_set_exec(self, execute);
    cpl_plugin_set_deinit(self, destroy);

    return CPL_ERROR_NONE;

}

/**
 * @brief
 *   Dump the plugin debugging information to the given stream.
 *
 * @param self    The plugin.
 * @param stream  The output stream to use.
 *
 * @return Nothing.
 *
 * The function dumps the contents of of the plugin @em self to the
 * output stream @em stream. If @em stream is @c NULL the function writes
 * to the standard output. If @em self is @c NULL the function does nothing.
 */

void
cpl_plugin_dump(const cpl_plugin *self, FILE *stream)
{

    if (self != NULL) {

        const char *s;
        char *a;

        if (stream == NULL) {
            stream = stdout;
        }

        fprintf(stream, "Plugin at %p:\n", (const void*)self);

        s = cpl_plugin_get_name(self);
        fprintf(stream, "  Name (%p): %s\n", s, s);

        a = cpl_plugin_get_version_string(self);
        fprintf(stream, "  Version: %s (%lu)\n", a,
                cpl_plugin_get_version(self));
        cx_free(a);

        a = cpl_plugin_get_type_string(self);
        fprintf(stream, "  Type: %s (%lu)\n", a, cpl_plugin_get_type(self));
        cx_free(a);

        fprintf(stream, "  API Version: %u", cpl_plugin_get_api(self));

CPL_DIAG_PRAGMA_PUSH_IGN(-Wformat=);
        fprintf(stream, "  Initialization handler at %p\n",
                cpl_plugin_get_init(self));
        fprintf(stream, "  Execution handler at %p\n",
                cpl_plugin_get_exec(self));
        fprintf(stream, "  Cleanup handler at %p\n",
                cpl_plugin_get_deinit(self));
CPL_DIAG_PRAGMA_POP;

        s = cpl_plugin_get_synopsis(self);
        fprintf(stream, "  Synopsis (%p): %s\n", s, s);

        s = cpl_plugin_get_copyright(self);
        fprintf(stream, "  Copyright (%p): %s\n", s, s);

        s = cpl_plugin_get_author(self);
        fprintf(stream, "  Author (%p): %s\n", s, s);

        s = cpl_plugin_get_email(self);
        fprintf(stream, "  Email (%p): %s\n", s, s);

        s = cpl_plugin_get_description(self);
        fprintf(stream, "  Description (%p): %s\n", s, s);
    }

    return;

}
/**@}*/
