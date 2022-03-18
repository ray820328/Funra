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

#ifndef CPL_PLUGININFO_H
#define CPL_PLUGININFO_H

#include <cpl_macros.h>
#include <cpl_pluginlist.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_plugin
 *
 * @brief
 *   Append the plugin information to the given list.
 *
 * @param cpl_plugin_list  A plugin list.
 *
 * @return The function must return 0 on success and 1 in case of an error.
 *
 * This function must be implemented by plugin developers. There must
 * be one such implementation per plugin library, regardless of how many
 * plugins the library actually offers, provided that there is at least
 * one plugin implemented in this library.
 *
 * This prototype is only provided in order to allow the compiler to do
 * some basic checks when compiling a plugin library. To have the
 * prototype available when you are compiling your plugin library,
 * you must add the line
 * @code
 *   #include <cpl_plugininfo.h>
 * @endcode
 *
 * to your plugin source file.
 *
 * The purpose of this function is to create a plugin object for each
 * plugin implementation provided by the plugin library, fill the
 * basic plugin interface (the @c cpl_plugin part of the created plugin
 * object) and append the created object to the plugin list @em list.
 *
 * The list will be provided by the application which is going to use the
 * plugin and it may be expected that @em list points to a valid plugin list
 * when this function is called.
 */

int cpl_plugin_get_info(cpl_pluginlist *cpl_plugin_list);

CPL_END_DECLS

#endif /* CPL_PLUGININFO_H */
