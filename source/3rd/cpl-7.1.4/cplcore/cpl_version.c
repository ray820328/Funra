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

#include "cpl_version.h"


/**
 * @defgroup cpl_version Library Version Information
 *
 * This module provides functions to access the library's version
 * information.
 *
 * The library version applies to all component libraries of the Common
 * pipeline library, and changes if any of the component libraries changes.
 *
 * The library version is a version code made up of three components, the
 * major, minor and micro version number, and is usually represented by a
 * string of the form "major.minor.micro".
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_version.h>
 * @endcode
 */

/**@{*/


/**
 * @brief
 *   Get the library's major version number.
 *
 * @return
 *   The function returns the library's major version number.
 *
 * The function returns the major version number of the library.
 */

unsigned int
cpl_version_get_major(void)
{
    return CPL_MAJOR_VERSION;
}


/**
 * @brief
 *   Get the library's minor version number.
 *
 * @return
 *   The function returns the library's minor version number.
 *
 * The function returns the minor version number of the library.
 */

unsigned int
cpl_version_get_minor(void)
{
    return CPL_MINOR_VERSION;
}


/**
 * @brief
 *   Get the library's micro version number.
 *
 * @return
 *   The function returns the library's micro version number.
 *
 * The function returns the micro version number of the library.
 */

unsigned int
cpl_version_get_micro(void)
{
    return CPL_MICRO_VERSION;
}


/**
 * @brief
 *   Get the library's interface age.
 *
 * @return
 *   The function returns the library's interface age.
 *
 * The function returns the interface age of the library.
 */

unsigned int
cpl_version_get_interface_age(void)
{
    return CPL_INTERFACE_AGE;
}


/**
 * @brief
 *   Get the library's binary age.
 *
 * @return
 *   The function returns the library's binary age.
 *
 * The function returns the binary age of the library.
 */

unsigned int
cpl_version_get_binary_age(void)
{
    return CPL_BINARY_AGE;
}


/**
 * @brief
 *   Get the library's version string.
 *
 * @return
 *   The function returns the library's version string.
 *
 * The function returns the package version of the library as a string. The
 * returned version string is composed of the major, minor and, possibly, the
 * micro version of the library separated by dots. The micro version may
 * not be there if it is @c 0.
 */

const char *
cpl_version_get_version(void)
{
    return PACKAGE_VERSION;
}


/**
 * @brief
 *   Get the library's binary version number.
 *
 * @return
 *   The function returns the library's version number.
 *
 * The function returns the version number of the library encoded as a single
 * integer number.
 */

unsigned int
cpl_version_get_binary_version(void)
{
    return CPL_BINARY_VERSION;
}

/**@}*/
