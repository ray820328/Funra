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

#include <cxmemory.h>
#include <cxmessages.h>
#include <cxstrutils.h>
#include <cxutils.h>
#include <cxmacros.h>

#include "cpl_memory_impl.h"
#include "cpl_error_impl.h"
#include "cpl_xmemory.h"

/**
 * @defgroup cpl_memory Memory Management Utilities
 *
 * This module provides the CPL memory management utilities.
 */

/**@{*/

static int cpl_memory_mode = -1;

/**
 * @internal
 * @brief
 *   Initialise the memory management subsystem.
 *
 * @param mode  Mode (0: system, 1: Xmemory-count, 2: Xmemory-full)
 *
 * @return Nothing.
 *
 * The function initialises the CPL memory management subsystem, i.e. it
 * redirects any request for allocating or deallocating memory to either
 * the standard functions (free(), malloc(), etc.) or to the
 * @c cpl_xmemory module.
 *
 * This function is called from @c cpl_init(), and although repeated calls
 * have no effect, it should not be called by any other function.
 *
 * @see cpl_init()
 */

void
cpl_memory_init(int mode)
{
    if (cpl_memory_mode < 0) {
        if (mode == 0) {
            static const cx_memory_vtable cpl_memory_services_system = {
                (cxptr(*)(cxsize))malloc,
                (cxptr (*)(cxsize, cxsize))calloc,
                (cxptr (*) (cxptr, cxsize))realloc,
                (void (*) (cxptr))free
            };
            cx_memory_vtable_set(&cpl_memory_services_system);
            cpl_memory_mode = 0;
#ifndef _OPENMP
            /* Mode 2 is not supported with OpenMP - will default to 1 */
        } else if (mode == 2) {
            static const cx_memory_vtable cpl_memory_services_xmemory = {
                (cxptr(*)(cxsize))cpl_xmemory_malloc,
                (cxptr (*)(cxsize, cxsize))cpl_xmemory_calloc,
                (cxptr (*) (cxptr, cxsize))cpl_xmemory_realloc,
                (void (*) (cxptr))cpl_xmemory_free
            };
            cx_memory_vtable_set(&cpl_memory_services_xmemory);
            cpl_memory_mode = 2;
#endif
        } else {
            static const cx_memory_vtable cpl_memory_services_count = {
                (cxptr(*)(cxsize))cpl_xmemory_malloc_count,
                (cxptr (*)(cxsize, cxsize))cpl_xmemory_calloc_count,
                (cxptr (*) (cxptr, cxsize))cpl_xmemory_realloc_count,
                (void (*) (cxptr))cpl_xmemory_free_count
            };
            cx_memory_vtable_set(&cpl_memory_services_count);
            cpl_memory_mode = 1;
        }
    }

    return;
}


/**
 * @brief
 *   Allocate @em nbytes bytes.
 *
 * @param nbytes  Number of bytes.
 *
 * @return Pointer to the allocated memory block.
 *
 * The function allocates @em nbytes bytes of memory. The allocated memory
 * is not cleared.
 *
 * @note
 *   If the memory subsytem has not been initialised before calling
 *   this function, the program execution is stopped printing a message to
 *   the error channel showing the current code position.
 *
 */

void *
cpl_malloc(size_t nbytes)
{

    if (cpl_memory_mode < 0) {
        cx_log(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR, "%s: CPL " PACKAGE_VERSION
               " memory management subsystem is not initialized!", CX_CODE_POS);
    }

    return cx_malloc((cxsize)nbytes);

}

        
/**
 * @brief
 *   Allocate memory for @em natoms elements of size @em size.
 *
 * @param natoms  Number of atomic elements.
 * @param nbytes  Element size in bytes.
 *
 * @return Pointer to the allocated memory block.
 *
 * The function allocates memory suitable for storage of @em natoms
 * elements of size @em nbytes bytes. The allocated memory is cleared,
 * i.e. the value 0 is written to each single byte.
 *
 * @note
 *   If the memory subsytem has not been initialised before calling
 *   this function, the program execution is stopped printing a message to
 *   the error channel showing the current code position.
 */

void *
cpl_calloc(size_t natoms, size_t nbytes)
{

    if (cpl_memory_mode < 0) {
        cx_log(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR, "%s: CPL " PACKAGE_VERSION
               " memory management subsystem is not initialized!", CX_CODE_POS);
    }

    return cx_calloc((cxsize)natoms, (cxsize)nbytes);

}


/**
 * @brief
 *   Change the size of a memory block.
 *
 * @param memblk  Pointer to the memory to re-allocate.
 * @param nbytes  New memory block size in bytes.
 *
 * @return Pointer to the allocated memory block.
 *
 * The function changes the size of an already allocated memory block
 * @em memblk to the new size @em nbytes bytes. The contents is unchanged
 * to the minimum of old and new size; newly allocated memory is not
 * initialized. If @em memblk is @c NULL the call to @b cpl_realloc()
 * is equivalent to @b cpl_malloc(), and if @em nbytes is 0 the call
 * is equivalent to @b cpl_free(). Unless @em memblk is @c NULL, it must
 * have been returned by a previous call to @b cpl_malloc(),
 * @b cpl_calloc(), or @b cpl_realloc().
 *
 * @note
 *   - The returned memory block returned on successfull allocation may
 *     not be the same as the one pointed to by @em memblk. Existing
 *     references pointing to locations within the original memory block
 *     might be invalidated!
 *   - If the memory subsytem has not been initialised before calling
 *     this function, the program execution is stopped printing a message to
 *     the error channel showing the current code position.
 *
 * @see cpl_malloc(), cpl_calloc()
 */

void *
cpl_realloc(void *memblk, size_t nbytes)
{

    if (cpl_memory_mode < 0) {
        cx_log(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR, "%s: CPL " PACKAGE_VERSION
               " memory management subsystem is not initialized!", CX_CODE_POS);
    }

    return cx_realloc(memblk, (cxsize)nbytes);

}


/**
 * @brief
 *   Memory block deallocation.
 *
 * @return Nothing.
 *
 * Deallocates a memory block previously allocated by any of the CPL
 * allocator functions.
 *
 * @see cpl_malloc(), cpl_calloc()
 */

void
cpl_free(void *memblk)
{

    if (cpl_memory_mode < 0) {
        cx_log(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR, "%s: CPL " PACKAGE_VERSION
               " memory management subsystem is not initialized!", CX_CODE_POS);
    }

    cx_free(memblk);
    return;

}

/**
 * @brief
 *   Duplicate a string.
 *
 * @param string  String to be duplicated.
 *
 * @return Newly allocated copy of the original string.
 *
 * Duplicates the input string @em string. The newly allocated copy returned
 * to the caller can be deallocated using @b cpl_free().
 *
 * @see cpl_free()
 */

char *cpl_strdup(const char *string)
{

    if (cpl_memory_mode < 0) {
        cx_log(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR, "%s: CPL " PACKAGE_VERSION
               " memory management subsystem is not initialized!", CX_CODE_POS);
    }

    return cx_strdup(string);

}


/*----------------------------------------------------------------------------*/
/**
 * @brief  Create a string and fill it in an vsprintf()-like manner
 * @param  format  The format string
 * @param  arglist The argument list for the format
 * @return The created string or NULL on error
 * @note   The created string must be deallocated with cpl_free()
 * @see vsprintf()
 *
 * The allocated memory is exactly what is needed to hold the string.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The format string is <tt>NULL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The format string has an invalid format.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 */
/*----------------------------------------------------------------------------*/
char * cpl_vsprintf(const char * format, va_list arglist)
{

    char * self = NULL;
    int    nlen;


    if (cpl_memory_mode < 0) {
        cx_log(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR, "%s: CPL " PACKAGE_VERSION
               " memory management subsystem is not initialized!", CX_CODE_POS);
    }

    cpl_ensure(format != NULL, CPL_ERROR_NULL_INPUT, NULL);

    nlen = (int)cx_vasprintf((cxchar **)&self, (const cxchar *)format, arglist);

    /* The format string may be invalid */
    if (nlen < 0) {
        cx_free(self);
        self = NULL;
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "cx_vasprintf() "
                                     "returned %d < 0 on format=%s", nlen,
                                     format);
    } else if (self == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "cx_vasprintf() "
                                     "returned %d but set self=NULL on "
                                     "format=%s", nlen, format);
    }

    return self;

}

/*----------------------------------------------------------------------------*/
/**
 * @brief  Create a string and fill it in an sprintf()-like manner
 * @param  format    The format string
 * @param  ...       Variable argument list for format
 * @return The created string or NULL on error
 * @note   The created string must be deallocated with cpl_free()
 * @see cpl_vsprintf()
 *
 * The allocated memory is exactly what is needed to hold the string.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The format string is <tt>NULL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The format string has an invalid format.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 *
 * Example of usage (note how strlen() is not needed to get the string length):
 *  @code
 *
 *      int error;
 *
 *      int length;
 *
 *      char * cp_cmd = cpl_sprintf("cp %s %s/%s%n", long_file, new_dir,
 *                                                   new_file, &length);
 *      assert( cp_cmd != NULL);
 *
 *      assert( length == (int)strlen(cp_cmd));
 *
 *      error = system(cp_cmd);
 *
 *      assert(!error);
 *
 *      cpl_free(cp_cmd);
 *                      
 *  @endcode
 *
 */
/*----------------------------------------------------------------------------*/
char * cpl_sprintf(const char * format, ...)
{

    char    * self;
    va_list   arglist;


    cpl_ensure(format != NULL, CPL_ERROR_NULL_INPUT, NULL);

    va_start(arglist, format);
    self = cpl_vsprintf(format, arglist);
    va_end(arglist);

    cpl_ensure(self != NULL, cpl_error_get_code(), NULL);

    return self;

}


/**
 * @brief   Tell if there is some memory allocated
 * @return  -1 if the model is off, 1 if it is empty, 0 otherwise
 */
int cpl_memory_is_empty(void)
{
    return cpl_xmemory_is_empty(cpl_memory_mode);
}

/**
 * @brief   Display the memory status
 */
void cpl_memory_dump(void)
{ 
    cpl_xmemory_status(cpl_memory_mode);

    return;
}

/**@}*/
