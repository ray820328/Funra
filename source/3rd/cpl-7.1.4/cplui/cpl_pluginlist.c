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

#include <string.h>

#include <cxlist.h>
#include <cxmemory.h>
#include <cxmessages.h>

#include "cpl_plugin.h"
#include "cpl_pluginlist.h"
#include "cpl_error_impl.h"


/**
 * @defgroup cpl_pluginlist  Plugin List
 *
 * This module implements a list container for plugin objects and provides
 * the facilities to query, to traverse and to update the container. The
 * purpose of this container is to be able to store references to available
 * plugins and to handle sets of plugins as a whole.
 *
 * Since the plugin list just stores pointers to @c cpl_plugin, a plugin
 * list may contain plugins of different kind at the same time, because
 * all context specific plugins inherit the @c cpl_plugin type (see
 * @ref cpl_plugin).
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_pluginlist.h>
 * @endcode
 */

/**@{*/

struct _cpl_pluginlist_ {
    cx_list *data;
    cx_list_iterator search;
};


/**
 * @brief
 *   Creates an empty plugin list.
 *
 * @return
 *   The newly created plugin list, or @c NULL if it could not  be created.
 *
 * The function allocates memory for a plugin list object and initialises it
 * to be empty.
 */

cpl_pluginlist *
cpl_pluginlist_new(void)
{

    cpl_pluginlist *self;


    self = cx_malloc(sizeof *self);

    if (self == NULL) {
        return NULL;
    }

    self->data = cx_list_new();

    if (self->data == NULL) {
        cx_free(self);
        return NULL;
    }

    self->search = NULL;

    return self;

}


/**
 * @brief
 *    Delete a plugin list.
 *
 * @param self  The plugin list to delete.
 *
 * @return Nothing.
 *
 * The function deletes the plugin list @em self and destroys all plugins the
 * list potentially contains. If @em self is @c NULL, nothing is done and no error is set.
 */

void
cpl_pluginlist_delete(cpl_pluginlist *self)
{

    if (self) {

        if (self->data) {
            cx_list_destroy(self->data, (cx_free_func)cpl_plugin_delete);
            self->data = NULL;
        }

        self->search = NULL;

        cx_free(self);
        self = NULL;
    }

    return;

}


/**
 * @brief
 *   Get the current size of a plugin list.
 *
 * @param self  A plugin list.
 *
 * @return
 *   The plugin list's current size, or 0 if the list is empty. The function
 *   returns 0 if an error occurs and sets an appropriate error code.
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
 * The function reports the current number of plugins stored in the plugin
 * list @em self. If @em self does not point to a valid plugin list the
 * function returns 0.
 */

int
cpl_pluginlist_get_size(cpl_pluginlist *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    cx_assert(self->data != NULL);

    return (int)cx_list_size(self->data);

}


/**
 * @brief
 *   Append a plugin to a plugin list.
 *
 * @param self    A plugin list.
 * @param plugin  The plugin to append.
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
 *         The parameter <i>self</i> or <i>plugin</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The plugin @em plugin is inserted into the plugin list @em self after the
 * last element.
 *
 * If @em self does not point to a valid plugin list, or if @em plugin is not
 * a valid pointer the function returns immediately.
 */

cpl_error_code
cpl_pluginlist_append(cpl_pluginlist *self, const cpl_plugin *plugin)
{



    if (self == NULL || plugin == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_list_push_back(self->data, (cxptr)plugin);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Prepend a plugin to a plugin list.
 *
 * @param self    A plugin list.
 * @param plugin  The plugin to prepend.
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
 *         The parameter <i>self</i> or <i>plugin</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The plugin @em plugin is inserted into the plugin list @em self before the
 * first element.
 *
 * If @em self does not point to a valid plugin list, or if @em plugin is not
 * a valid pointer the function returns immediately.
 */

cpl_error_code
cpl_pluginlist_prepend(cpl_pluginlist *self, const cpl_plugin *plugin)
{



    if (self == NULL || plugin == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_list_push_front(self->data, (cxptr)plugin);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the first plugin of a plugin list.
 *
 * @param self  A plugin list.
 *
 * @return
 *   The first plugin stored in the plugin list @em self, or @c NULL
 *   if the list is empty. The function returns @c NULL if an error
 *   occurs and sets an appropriate error code.
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
 * The function returns a handle to the first plugin stored in the @em self.
 */


cpl_plugin *
cpl_pluginlist_get_first(cpl_pluginlist *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->data != NULL);

    self->search = cx_list_begin(self->data);

    if (self->search == cx_list_end(self->data)) {
        self->search = NULL;
        return NULL;
    }

    return (cpl_plugin *) cx_list_get(self->data, self->search);

}


/**
 * @brief
 *   Get the next plugin from a plugin list.
 *
 * @param self  A plugin list.
 *
 * @return
 *   The function returns the next plugin in the list, or @c NULL if
 *   the end of the list has been reached. The function returns @c NULL
 *   if an error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The function was called without initialising the plugin list
 *         <i>self</i> by calling @b cpl_pluginlist_first().
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the next plugin in @em self. To find the next plugin,
 * the plugin list caches the position of the most recently obtained plugin.
 * This requires a call to @b cpl_pluginlist_get_first() prior to calling this
 * function in order to properly initialise the internal cache.
 *
 * If the end of @em self has been reached the internal cache is reset to
 * the first plugin in the list.
 */

cpl_plugin *
cpl_pluginlist_get_next(cpl_pluginlist *self)
{


    cx_list_iterator next;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (self->search == NULL) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    cx_assert(self->data != NULL);

    next =  cx_list_next(self->data, self->search);

    if (next == cx_list_end(self->data)) {
        self->search = cx_list_begin(self->data);
        return NULL;
    }

    self->search = next;

    return (cpl_plugin *) cx_list_get(self->data, self->search);

}


/**
 * @brief
 *   Get the last plugin of a plugin list.
 *
 * @param self  A plugin list.
 *
 * @return The last plugin stored in the plugin list @em self, or @c NULL
 *   if the list is empty. The function returns @c NULL if an error
 *   occurs and sets an appropriate error code.
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
 * The function returns a pointer to the last plugin stored in the plugin
 * list @em self.
 */

cpl_plugin *
cpl_pluginlist_get_last(cpl_pluginlist *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->data != NULL);

    if (cx_list_empty(self->data)) {
        return NULL;
    }

    return (cpl_plugin *) cx_list_back(self->data);

}


/**
 * @brief
 *   Find a plugin with a given name in a plugin list.
 *
 * @param self  The plugin list to query.
 * @param name  The plugin's unique name to look for.
 *
 * @return
 *   The first plugin with the given name @em name, or @c NULL if it
 *   no plugin with this name could be found. The function returns
 *   @c NULL if an error occurs and sets an appropriate error code.
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
 * This function searches the plugin list @em self for a plugin with the
 * unique name @em name and returns a pointer to the first one found.
 * If no plugin with the given name is found the function returns @c NULL.
 */

cpl_plugin *
cpl_pluginlist_find(cpl_pluginlist *self, const char *name)
{


    cx_list_iterator first, last;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->data != NULL);

    first = cx_list_begin(self->data);
    last = cx_list_end(self->data);

    while (first != last) {
        cpl_plugin *p = cx_list_get(self->data, first);

        if (strcmp(cpl_plugin_get_name(p), name) == 0) {
            return p;
        }
        first = cx_list_next(self->data, first);
    }

    return NULL;

}


/**
 * @brief
 *   Dump the contents of a plugin list to the given stream.
 *
 * @param self    The plugin list.
 * @param stream  The output stream to use.
 *
 * @return Nothing.
 *
 * The function dumps the debugging information for each plugin found in
 * the plugin list @em self to the output stream @em stream. The debugging
 * information for each individual plugin is dumped using
 * @b cpl_plugin_dump(). If @em self is @c NULL the function does nothing.
 *
 * @see cpl_plugin_dump()
 */

void
cpl_pluginlist_dump(const cpl_pluginlist *self, FILE *stream)
{

    if (self != NULL) {

        cx_list_const_iterator pos = cx_list_begin(self->data);

        if (stream == NULL) {
            stream = stdout;
        }

        while (pos != cx_list_end(self->data)) {

            const cpl_plugin *p = cx_list_get(self->data, pos);

            cpl_plugin_dump(p, stream);
            pos = cx_list_next(self->data, pos);

        }

    }

    return;

}
/**@}*/

