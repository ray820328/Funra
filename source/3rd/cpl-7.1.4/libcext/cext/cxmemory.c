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

/*
 * Acknowledgements: The cxmemory module implementation was inherited
 * from GLib (see http://www.gtk.org) and is basically a stripped down
 * version of the GLib gmem module.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "cxthread.h"
#include "cxmessages.h"
#include "cxmemory.h"


// TODO: This might be removed in the final version. It is here to switch
//       between proper locking and sloppy locking until it is decided on
//       how the final implementation is going to look like.
#define CX_MEMORY_ENABLE_STRICT_LOCKING


/**
 * @defgroup cxmemory Memory Management Utilities
 *
 * The module provides wrapper routines for the standard C memory management
 * functions. The wrappers for the system memory allocators guarantee to
 * always return valid pointer to the allocated memory block of memory. If
 * the requested memory cannot be allocated the functions stop the program
 * calling @b abort(), following the philosophy that it is better to
 * terminate the application immediately when running out of resources. The
 * memory deallocator is protected against passing @c NULL.
 *
 * @par Synopsis:
 * @code
 *   #include <cxmemory.h>
 * @endcode
 */

/**@{*/

/*
 * Flag indicating if a cx_memory_vtable may be set or not.
 * A cx_memory_vtable may be set only once during startup!
 */

static cxbool cx_memory_vtable_lock = FALSE;

CX_LOCK_DEFINE_INITIALIZED_STATIC(cx_memory_vtable_lock);


/*
 * Control variable to guarantee that the memory system initialization
 * function is executed only once! The argument must be the name of the
 * initialization function.
 */

CX_ONCE_DEFINE_INITIALIZED_STATIC(cx_memory_once);

/*
 * Internal initialization functions for the memory system. This
 * function sets the cx_memory_vtable flag indicating whether the
 * memory services have been initialized.
 */

inline static void
cx_memory_vtable_set_lock(void)
{
    cx_memory_vtable_lock = TRUE;
    return;
}


inline static cxbool
cx_memory_vtable_locked(void)
{
    return cx_memory_vtable_lock == TRUE;
}


static void
cx_memory_init(void)
{
    CX_LOCK(cx_memory_vtable_lock);

    if (!cx_memory_vtable_locked()) {
        cx_memory_vtable_set_lock();
    }

    CX_UNLOCK(cx_memory_vtable_lock);

    return;
}


/*
 * Standard memory services have to be encapsulated in functions,
 * to allow for assignement and or the comparison of memory services.
 * Otherwise the HP-UX compiler complains if the prototypes do not
 * match. Maybe a prototype verifications should be added?
 */

/*
 * Define our own version of realloc() in case NULL cannot be passed
 * to realloc() as first argument.
 */

#ifdef HAVE_WORKING_REALLOC

static cxptr
cx_memory_realloc_std(cxptr memory, cxsize nbytes)
{

    return realloc(memory, nbytes);

}

#else /* !HAVE_WORKING_REALLOC */

static cxptr
cx_memory_realloc_std(cxptr memory, cxsize nbytes)
{
    if (!memory)
        return malloc(nbytes);
    else
        return realloc(memory, nbytes);
}

#endif /* !HAVE_WORKING_REALLOC */

static cxptr
cx_memory_malloc_std(cxsize nbytes)
{

    return malloc(nbytes);

}

static cxptr
cx_memory_calloc_std(cxsize natoms, cxsize nbytes)
{

    return calloc(natoms, nbytes);

}

static void
cx_memory_free_std(cxptr memory)
{

    free(memory);
    return;

}


/*
 * Internal allocator vtable initialized to system defaults.
 */

static cx_memory_vtable cx_memory_services = {
    cx_memory_malloc_std,
    cx_memory_calloc_std,
    cx_memory_realloc_std,
    cx_memory_free_std,
};


/*
 * Fallback calloc implementation. It utilizes the malloc() implementation
 * provided by the memory vtable.
 */

static cxptr
cx_memory_calloc_fallback(cxsize natoms, cxsize nbytes)
{
    cxsize sz = natoms * nbytes;
    cxptr mblk = cx_memory_services.malloc(sz);

    if (mblk)
        memset(mblk, 0, sz);

    return mblk;
}


/**
 * @brief
 *   Install a new set of memory managmement functions.
 *
 * @param table  Set of memory management functions.
 *
 * @return Nothing.
 *
 * The function installs the replacements for @b malloc(), @b calloc(),
 * @b realloc() and @b free() provided by @em table in the internal
 * vtable.
 *
 * @attention
 *   The function can be called only @b once before any of the memory
 *   handling functions are called, either explicitly or implicitly through
 *   another library functions! For thread-safety reasons the function may
 *   @b only be called from the @em main thread!
 */

void
cx_memory_vtable_set(const cx_memory_vtable *table)
{

    CX_LOCK(cx_memory_vtable_lock);

    if (!cx_memory_vtable_locked()) {

        cx_memory_vtable_set_lock();

        if (table->malloc && table->realloc && table->free) {
            cx_memory_services.malloc = table->malloc;
            cx_memory_services.realloc = table->realloc;
            cx_memory_services.free = table->free;

            cx_memory_services.calloc = (table->calloc ? table->calloc :
                                         cx_memory_calloc_fallback);
        }
        else {
            cx_warning(CX_CODE_POS ": Memory allocation vtable lacks one of "
                       "malloc(), realloc() or free()");
        }

    }
    else {
        cx_warning(CX_CODE_POS ": Memory allocation vtable can only be set "
                   "once");
    }

    CX_UNLOCK(cx_memory_vtable_lock);

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
 * is not cleared. If the allocation fails the function does not return,
 * but the program execution is stopped printing a message to the error
 * channel showing the current code position.
 *
 * @see cx_malloc_clear()
 */

cxptr
cx_malloc(cxsize nbytes)
{

    /*
     * Lock memory vtable on first entry.
     */

#ifdef CX_MEMORY_ENABLE_STRICT_LOCKING
    cx_thread_once(cx_memory_once, cx_memory_init, NULL);
#else
    if (!cx_memory_vtable_locked()) {
        CX_LOCK(cx_memory_vtable_lock);
        cx_memory_vtable_set_lock();
        CX_UNLOCK(cx_memory_vtable_lock);
    }
#endif


    if (nbytes) {
        cxptr mblk = cx_memory_services.malloc(nbytes);

        if (mblk)
            return mblk;

        cx_error(CX_CODE_POS ": failed to allocate %" CX_PRINTF_FORMAT_SIZE_TYPE
                 " bytes", nbytes);
    }

    return NULL;

}


/**
 * @brief
 *   Allocate @em nbytes bytes and clear them
 *
 * @param nbytes  Number of bytes.
 *
 * @return Pointer to the allocated memory block.
 *
 * The function works as @b cx_malloc(), but the allocated memory is
 * cleared, i.e. a 0 is written to each byte of the allocated block.
 *
 * @see cx_malloc()
 *
 */

cxptr
cx_malloc_clear(cxsize nbytes)
{

    /*
     * Lock memory vtable on first entry.
     */

#ifdef CX_MEMORY_ENABLE_STRICT_LOCKING
    cx_thread_once(cx_memory_once, cx_memory_init, NULL);
#else
    if (!cx_memory_vtable_locked()) {
        CX_LOCK(cx_memory_vtable_lock);
        cx_memory_vtable_set_lock();
        CX_UNLOCK(cx_memory_vtable_lock);
    }
#endif


    if (nbytes) {
        cxptr mblk = cx_memory_services.calloc(1, nbytes);

        if (mblk)
            return mblk;

        cx_error(CX_CODE_POS ": failed to allocate %" CX_PRINTF_FORMAT_SIZE_TYPE
                 " bytes", nbytes);
    }

    return NULL;

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
 * i.e. the value 0 is written to each single byte. If the allocation
 * fails the function does not return, but the program execution is
 * stopped printing a message to the error channel showing the current
 * code position.
 */

cxptr
cx_calloc(cxsize natoms, cxsize nbytes)
{

    /*
     * Lock memory vtable on first entry.
     */

#ifdef CX_MEMORY_ENABLE_STRICT_LOCKING
    cx_thread_once(cx_memory_once, cx_memory_init, NULL);
#else
    if (!cx_memory_vtable_locked()) {
        CX_LOCK(cx_memory_vtable_lock);
        cx_memory_vtable_set_lock();
        CX_UNLOCK(cx_memory_vtable_lock);
    }
#endif


    if (natoms && nbytes) {
        cxptr mblk = cx_memory_services.calloc(natoms, nbytes);

        if (mblk)
            return mblk;

        cx_error(CX_CODE_POS ": failed to allocate %" CX_PRINTF_FORMAT_SIZE_TYPE
                 " bytes", natoms * nbytes);
    }

    return NULL;

}


/**
 * @brief
 *   Change the size of a memory block.
 *
 * @param memory  Number of atomic elements.
 * @param nbytes  New memory block size in bytes.
 *
 * @return Pointer to the allocated memory block.
 *
 * The function changes the size of an already allocated memory block
 * @em memory to the new size @em nbytes bytes. The contents is unchanged
 * to the minimum of old and new size; newly allocated memory is not
 * initialized. If @em memory is @c NULL the call to @b cx_realloc()
 * is equivalent to @b cx_malloc(), and if @em nbytes is 0 the call
 * is equivalent to @b cx_free(). Unless @em memory is @c NULL, it must
 * have been returned by a previous call to @b cx_malloc(),
 * @b cx_malloc_clear(), @b cx_calloc(), or @b cx_realloc().
 *
 * @note
 *   The returned memory block returned on successfull allocation may
 *   not be the same as the one pointed to by @em memory. Existing
 *   references pointing to locations within the original memory block
 *   might be invalidated!
 *
 * @see cx_malloc(), cx_malloc_clear(), cx_calloc()
 */

cxptr
cx_realloc(cxptr memory, cxsize nbytes)
{

    cxptr mblk;

    /*
     * Lock memory vtable on first entry.
     */

#ifdef CX_MEMORY_ENABLE_STRICT_LOCKING
    cx_thread_once(cx_memory_once, cx_memory_init, NULL);
#else
    if (!cx_memory_vtable_locked()) {
        CX_LOCK(cx_memory_vtable_lock);
        cx_memory_vtable_set_lock();
        CX_UNLOCK(cx_memory_vtable_lock);
    }
#endif


    mblk = cx_memory_services.realloc(memory, nbytes);

    if (mblk == NULL && nbytes != 0) {
        cx_error(CX_CODE_POS ": failed to allocate %" CX_PRINTF_FORMAT_SIZE_TYPE
                 " bytes", nbytes);
    }

    return mblk;

}


/**
 * @brief
 *   Memory block deallocation.
 *
 * @return Nothing.
 *
 * Deallocates a memory block previously allocated by @b cx_malloc(),
 * @b cx_malloc_clear(), @b cx_calloc(), or @b cx_realloc().
 *
 * @see cx_malloc(), cx_malloc_clear(), cx_calloc()
 */

void
cx_free(cxptr memory)
{

    /*
     * Lock memory vtable on first entry.
     */

#ifdef CX_MEMORY_ENABLE_STRICT_LOCKING
    cx_thread_once(cx_memory_once, cx_memory_init, NULL);
#else
    if (!cx_memory_vtable_locked()) {
        CX_LOCK(cx_memory_vtable_lock);
        cx_memory_vtable_set_lock();
        CX_UNLOCK(cx_memory_vtable_lock);
    }
#endif


    if (memory)
        cx_memory_services.free(memory);

    return;
}


/**
 * @brief
 *   Check if the system's defaults are used for memory allocation.
 *
 * @return It returns @c TRUE if memory is allocated through the system's
 *   @b malloc() implementation, it not it returns @c FALSE.
 *
 * Checks whether the allocator used by @b cx_malloc() is the system's
 * malloc implementation. If the system's malloc implementation is used
 * memory allocated with the system's @b malloc() call can be used
 * interchangeable with memory allocated by @b cx_malloc().
 *
 * @see cx_memory_vtable_set()
 */

cxbool cx_memory_is_system_malloc(void)
{

    return cx_memory_services.malloc == cx_memory_malloc_std;

}
/**@}*/
