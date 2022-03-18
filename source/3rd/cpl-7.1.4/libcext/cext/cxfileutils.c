/*
 * This file is part of the ESO C Extension Library
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

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "cxmemory.h"
#include "cxfileutils.h"


#ifdef STAT_MACROS_BROKEN
#undef S_ISDIR
#endif
#if !defined S_ISDIR && defined S_IFMT && defined S_IFDIR
#define S_ISDIR(mode) ((mode) & S_IFMT == S_IFDIR)
#endif


/**
 * @defgroup cxfileutils File Utilities
 *
 * The module provides a collection of useful file and file path related
 * utility functions.
 *
 * @par Synopsis:
 * @code
 *   #include <cxfileutils.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *   Get the maximum length of a path relative to the given directory.
 *
 * @param path  Directory name.
 *
 * @return Maximum path length.
 *
 * The function returns the maximum length a relative path when @em path
 * is the current working directory. To get the maximum possible length
 * of an absolute path pass "/" for @em path, but see the note below.
 * The size is determined through the @b pathconf() function, or if this
 * is not available the maximum length is simply set to the ad hoc length
 * of 4095 characters.
 *
 * @note
 *   The maximum length of a pathname depends on the underlying filesystem.
 *   This means that the method to determine the maximum possible length
 *   of an absolute pathname might be incorrect if the root directory
 *   and the actual location of the file under consideration are located
 *   on different filesystems.
 */

cxlong
cx_path_max(const cxchar *path)
{

    cxlong sz = 4095;

    if (path == NULL) return sz;

#ifdef HAVE_PATHCONF
    sz = pathconf(path, _PC_PATH_MAX);
#endif

    return sz;

}


/**
 * @brief
 *   Allocate a buffer suitable for storing a relative pathname staring at
 *   a given directory.
 *
 * @param path  Directory name the relative path should start from.
 *
 * @return Allocated buffer of appropriate size, or @c NULL in case of an
 *   error.
 *
 * The function determines the maximum possible length of a relative
 * path name when @em path is the current working directory. A buffer
 * with the appropriate length for the relative pathname including the
 * trailing zero. The argument @em path must be the name of an existing,
 * or the function fails. The allocated buffer can be destroyed by calling
 * @b cx_free().
 *
 * IF the string "/" is used for @em path the returned buffer will have the
 * maximum length possible for an absolute path supported by the system
 * (but see the note for @b cx_path_max() for pitfalls).
 */

cxchar *
cx_path_alloc(const char *path)
{

#ifdef S_ISDIR
    struct stat sb;
#endif


    if (!path || path[0] == '\0')
        return NULL;

#ifdef S_ISDIR
    /*
     * Check that path is a directory
     */

    if (stat(path, &sb) != 0 || !S_ISDIR(sb.st_mode))
        return NULL;
#endif

    return cx_calloc(cx_path_max(path) + 1, sizeof(cxchar));

}
/**@}*/
