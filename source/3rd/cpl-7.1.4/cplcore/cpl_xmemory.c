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
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <assert.h>

#include "cpl_xmemory.h"

/* The below doxygen has been inactivated by removing the '**' comment. */

/*----------------------------------------------------------------------------*/
/*
 * @defgroup    cpl_xmemory     POSIX-compatible extended memory handling
 *
 * cpl_xmemory is a small and efficient module offering memory extension
 * capabitilies to ANSI C programs running on POSIX-compliant systems. It
 * offers several useful features such as memory leak detection, protection for
 * free on NULL or unallocated pointers.
 * This module has been tested on a number of
 * current Unix * flavours and is reported to work fine.
 * The current limitation is the limited number of pointers it can handle at
 * the same time - and its absence of thread-safety.
 * 
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#ifndef inline
#define inline /* inline */
#endif

#define CPL_XSTRINGIFY(TOSTRING) #TOSTRING
#define CPL_STRINGIFY(TOSTRING) CPL_XSTRINGIFY(TOSTRING)

/*
  This symbol defines the level of usage of the memory module.

  0   Use the memory system calls.
  1   Use the memory system calls, but abort() if they are not succesfull
  2   Fully use the memory functions
*/

/* Initial number of entries in memory table */
/* If this number is big, the size of the memory table can become
   problematic.
 */
/* Use a prime number to reduce risk of hashing clashes */
#ifndef CPL_XMEMORY_MAXPTRS
#error "CPL_XMEMORY_MAXPTRS is not defined"
#endif

#if CPL_XMEMORY_MAXPTRS <= 0
#error "CPL_XMEMORY_MAXPTRS must be positive"
#endif

#ifndef CPL_XMEMORY_RESIZE_THRESHOLD
#define CPL_XMEMORY_RESIZE_THRESHOLD 0.9
#endif

#ifndef CPL_XMEMORY_RESIZE_FACTOR
#define CPL_XMEMORY_RESIZE_FACTOR 2
#endif

/* A very simple hash */
#define PTR_HASH(ptr) (((size_t)ptr) % cpl_xmemory_table_size)

#define CPL_XMEMORY_TYPE_FREE    0
#define CPL_XMEMORY_TYPE_MALLOC  1
#define CPL_XMEMORY_TYPE_CALLOC  2
#define CPL_XMEMORY_TYPE_REALLOC 3

/*-----------------------------------------------------------------------------
                        Private variables
 -----------------------------------------------------------------------------*/

static const char * cpl_xmemory_type[]
    = {"free", "malloc", "calloc", "realloc"};

/* Number of active cells */
static size_t       cpl_xmemory_ncells = 0;

/* Peak number of pointers ever seen for diagnostics */
static size_t       cpl_xmemory_max_cells = 0;

/* Total number of allocations for diagnostics - via free()/realloc()*/
static size_t       cpl_xmemory_sum_cells = 0;

/* Total allocated RAM in bytes */
static size_t       cpl_xmemory_alloc_ram = 0;

/* Peak allocation ever seen for diagnostics */
static size_t       cpl_xmemory_alloc_max = 0;

/* The current size of the xmemory table */
static size_t       cpl_xmemory_table_size = 0;

/* The default size of the xmemory table, when it is non-empty */
static size_t       cpl_xmemory_table_size_max = 0;

static int cpl_xmemory_fatal = 0;

/* Various infos about the pointers */
/* List of pointers */
static const void   ** cpl_xmemory_p_val;

/* Type of allocation */
/* - CPL_XMEMORY_TYPE_FREE means no allocation */
static unsigned char * cpl_xmemory_p_type;

/* Size of allocated memory [bytes] */
static size_t        * cpl_xmemory_p_size;

/*-----------------------------------------------------------------------------
                    Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_xmemory_init(void);
static void cpl_xmemory_init_alloc(void);
static void cpl_xmemory_end(void);
static void cpl_xmemory_resize(void);
static void cpl_xmemory_addcell(size_t, const void *, size_t, unsigned char);
static void cpl_xmemory_remcell(size_t);
static size_t cpl_xmemory_findcell(const void *) CPL_ATTR_PURE;
static size_t cpl_xmemory_findfree(const void *);

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    malloc() with failure check
  @param    size        Size (in bytes) to allocate.
  @return   1 newly allocated pointer (possibly NULL on zero bytes)
  @note Will abort() on failure

 */
/*----------------------------------------------------------------------------*/
inline
void * cpl_xmemory_malloc_count(size_t size)
{

    void * ptr = malloc(size);


    if (ptr != NULL) {

#       ifdef _OPENMP
#       pragma omp atomic
#       endif
        cpl_xmemory_ncells++;
        /* Remember peak allocation */
        /* FIXME: This non-thread-safe check is incorrect, but might still
           produce a useful estimate */
        if (cpl_xmemory_ncells > cpl_xmemory_max_cells)
            cpl_xmemory_max_cells = cpl_xmemory_ncells;

    } else if (size != 0) {

        fprintf(stderr, "cpl_xmemory fatal error: malloc(%zu) returned NULL:\n",
                size);
        perror(NULL);
        cpl_xmemory_fatal = 1;
        cpl_xmemory_status(cpl_xmemory_table_size ? 2 : 1);
        assert(ptr != NULL); /* More informative, if enabled */
        abort();

    }

    return ptr;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    calloc() with failure check
  @param    nmemb       Number of elements to allocate.
  @param    size        Size (in bytes) to allocate.
  @return   1 newly allocated pointer (possibly NULL on zero bytes)
  @note Will abort() on failure

 */
/*----------------------------------------------------------------------------*/
inline
void * cpl_xmemory_calloc_count(size_t nmemb, size_t size)
{

    void * ptr = calloc(nmemb, size);


    if (ptr != NULL) {

#       ifdef _OPENMP
#       pragma omp atomic
#       endif
        cpl_xmemory_ncells++;
        /* Remember peak allocation */
        /* FIXME: This non-thread-safe check is incorrect, but might still
           produce a useful estimate */
        if (cpl_xmemory_ncells > cpl_xmemory_max_cells)
            cpl_xmemory_max_cells = cpl_xmemory_ncells;

    } else if (size != 0 && nmemb != 0) {

        fprintf(stderr, "cpl_xmemory fatal error: calloc(%zu, %zu) returned "
                "NULL:\n", nmemb, size);
        perror(NULL);
        cpl_xmemory_fatal = 1;
        cpl_xmemory_status(cpl_xmemory_table_size ? 2 : 1);
        assert(ptr != NULL); /* More informative, if enabled */
        abort();

    }

    return ptr;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    realloc() with failure check
  @param    oldptr      Pointer to reallocate.
  @param    size        Size (in bytes) to reallocate.
  @return   1 newly allocated pointer (possibly NULL on zero bytes)
  @note Will abort() on failure

 */
/*----------------------------------------------------------------------------*/
inline
void * cpl_xmemory_realloc_count(void * oldptr, size_t size)
{

    void * ptr;

    if (oldptr == NULL) {
        /* This is just another way to do a malloc() */

        ptr = cpl_xmemory_malloc_count(size);

#ifndef _OPENMP
        /* Need to disable this non-threadsafe check with thread-support */
    } else if (cpl_xmemory_ncells == 0) {
        fprintf(stderr, "cpl_xmemory error: Ignoring realloc() to %zu bytes on "
                "unallocated pointer (%p)\n", size, oldptr);
        ptr = NULL;
#endif
    } else {

        /* assert( oldptr != NULL ); */

        ptr = realloc(oldptr, size);

        cpl_xmemory_sum_cells++; /* Approximate during multi-threading */

        if (ptr == NULL) {

            if (size != 0) {
                fprintf(stderr, "cpl_xmemory fatal error: realloc(%p, %zu) "
                        "returned NULL:\n", oldptr, size);
                perror(NULL);
                cpl_xmemory_fatal = 1;
                cpl_xmemory_status(cpl_xmemory_table_size ? 2 : 1);
                assert(ptr != NULL); /* More informative, if enabled */
                abort();
            }

            /* This is just another way to do a free() */
#           ifdef _OPENMP
#           pragma omp atomic
#           endif
            cpl_xmemory_ncells--;
        }
    }

    return ptr;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Free memory.
  @param    ptr Pointer to free.
  @return   void
  @see free()

 */
/*----------------------------------------------------------------------------*/
void cpl_xmemory_free_count(void * ptr)
{

    if (ptr != NULL) {
#ifndef _OPENMP
        /* Need to disable this non-threadsafe check with thread-support */
        if (cpl_xmemory_ncells == 0) {
            fprintf(stderr, "cpl_xmemory error: Ignoring free() on "
                    "unallocated pointer (%p)\n", ptr);
            return;
        }
#endif
        free(ptr);

#       ifdef _OPENMP
#       pragma omp atomic
#       endif
        cpl_xmemory_ncells--;

        cpl_xmemory_sum_cells++; /* Approximate during multi-threading */
    }

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Allocate memory.
  @param    size        Size (in bytes) to allocate.
  @return   1 newly allocated pointer.

 */
/*----------------------------------------------------------------------------*/
void * cpl_xmemory_malloc(size_t size)
{
    void * ptr;
    size_t pos;

    if (size == 0) {
        /* In this case it is allowed to return NULL
            - do that and save space in the table */
        return NULL;
    }

    ptr = cpl_xmemory_malloc_count(size);

    if (cpl_xmemory_ncells
        >= cpl_xmemory_table_size * CPL_XMEMORY_RESIZE_THRESHOLD) {

        /* Initialize table if needed */
        if (cpl_xmemory_table_size == 0) {
            cpl_xmemory_init();
        }

        /* Memory table is too full, resize */
        cpl_xmemory_resize();
    }

    /* Add cell into general table */
    pos = cpl_xmemory_findfree(ptr);
    cpl_xmemory_addcell(pos, ptr, size, CPL_XMEMORY_TYPE_MALLOC);

    return ptr;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Allocate memory.
  @param    nmemb       Number of elements to allocate.
  @param    size        Size (in bytes) of each element.
  @return   1 newly allocated pointer.

 */
/*----------------------------------------------------------------------------*/
void * cpl_xmemory_calloc(size_t nmemb, size_t size)
{
    void * ptr;
    size_t pos;


    if (size == 0 || nmemb == 0) {
        /* In this case it is allowed to return NULL
            - do that and save space in the table */
        return NULL;
    }

    ptr = cpl_xmemory_calloc_count(nmemb, size);

    if (cpl_xmemory_ncells
        >= cpl_xmemory_table_size * CPL_XMEMORY_RESIZE_THRESHOLD) {

        /* Initialize table if needed */
        if (cpl_xmemory_table_size == 0) {
            cpl_xmemory_init();
        }

        /* Memory table is too full, resize */
        cpl_xmemory_resize();
    }

    /* Add cell into general table */
    pos = cpl_xmemory_findfree(ptr);
    cpl_xmemory_addcell(pos, ptr, nmemb * size, CPL_XMEMORY_TYPE_CALLOC);

    return ptr;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Re-Allocate memory.
  @param    oldptr      Pointer to free.
  @param    size        Size (in bytes) to allocate.
  @return   1 newly allocated pointer.
 */
/*----------------------------------------------------------------------------*/
void * cpl_xmemory_realloc(void * oldptr, size_t size)
{
    void * ptr;
    size_t pos = cpl_xmemory_table_size; /* Avoid (false) uninit warning */

    if (size == 0) {
        /* In this case it is allowed to return NULL
            - do that and save space in the table */
        cpl_xmemory_free(oldptr);
        return NULL;
    }

    if (oldptr != NULL) {

        if (cpl_xmemory_table_size == 0) {
            fprintf(stderr, "cpl_xmemory error: Ignoring realloc() of %zu bytes"
                    " requested on unallocated pointer (%p)\n", size, oldptr);
            return NULL;
        }

        pos = cpl_xmemory_findcell(oldptr);

        if (pos == cpl_xmemory_table_size) {
            fprintf(stderr, "cpl_xmemory error: Ignoring realloc() of %zu bytes"
                    " requested on unallocated pointer (%p)\n", size, oldptr);
            return NULL;
        }

        if (size == cpl_xmemory_p_size[pos]) {
            /* No actual realloc() is needed, so just update the
               allocation type since the caller will expect the pointer
               to come from realloc(). */
            cpl_xmemory_p_type[pos] = CPL_XMEMORY_TYPE_REALLOC;
            return oldptr;
        }

        /* Remove cell from main table */
        cpl_xmemory_remcell(pos);

    }

    ptr = cpl_xmemory_realloc_count(oldptr, size);

    /* assert( ptr != NULL ); */

    if (ptr != oldptr) {

        if (cpl_xmemory_ncells >= cpl_xmemory_table_size
            * CPL_XMEMORY_RESIZE_THRESHOLD) {

            /* Initialize table if needed */
            if (cpl_xmemory_table_size == 0) {
                cpl_xmemory_init();
            }

            /* Memory table is too full, resize */
            cpl_xmemory_resize();
        }

        pos = cpl_xmemory_findfree(ptr);

    }

    /* Add cell into general table */
    cpl_xmemory_addcell(pos, ptr, size, CPL_XMEMORY_TYPE_REALLOC);

    return ptr;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Free memory.
  @param    ptr         Pointer to free.
  @return   void
  @note Nothing is done on NULL
  @see free()

  Free the memory associated to a given pointer. Prints a warning on stderr
  if the requested pointer cannot be found in the memory table. If the pointer
  @em ptr is @c NULL, nothing is done and no error is set.

 */
/*----------------------------------------------------------------------------*/
void cpl_xmemory_free(void * ptr)
{

    size_t pos;

    /* Do nothing for a NULL pointer */
    if (ptr == NULL) return;

    if (cpl_xmemory_ncells == 0) {
        fprintf(stderr, "cpl_xmemory error: Ignoring free() on unallocated "
                "pointer (%p)\n", ptr);
        return;
    }

    /* assert( cpl_xmemory_table_size > 0 ); */

    pos = cpl_xmemory_findcell(ptr);

    if (pos == cpl_xmemory_table_size) {
        fprintf(stderr, "cpl_xmemory error: Ignoring free() on unallocated "
                "pointer (%p)\n", ptr);
        return;

    }

    /* free + below decrement copied from cpl_xmemory_free_count() */
    free(ptr);

    /* Decrement number of allocated pointers */
#   ifdef _OPENMP
#   pragma omp atomic
#   endif
    cpl_xmemory_ncells--;

    if (cpl_xmemory_ncells > 0) {
        /* Remove cell from main table */
        cpl_xmemory_remcell(pos);
    } else {
        /* There are no more active pointers, deallocate internal memory */
        cpl_xmemory_end();
    }

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Display memory status information.
  @param mode  Mode (0: system, 1: Xmemory-failure check, 2: Xmemory-full)
  @return   void

  This function is meant for debugging purposes, but it is recommended to
  call it at the end of every executable making use of the extended memory
  features.
 */
/*----------------------------------------------------------------------------*/
void cpl_xmemory_status(int mode)
{

    if (mode > 0)
        {
            const size_t rowsize =
                sizeof(void*) +
                sizeof(size_t)+
                sizeof(unsigned char);

            fprintf(stderr, "#----- Memory Diagnostics -----\n");

            if (mode == 2) {
                fprintf(stderr,
                        "Total allocations [pointer]:  %zu\n"
                        "Peak pointer usage [pointer]: %zu\n"
                        "Peak memory allocation [B]:   %zu\n"
                        "Peak table size [pointer]:    %zu\n"
                        "Peak table size [B]:          %zu\n",
                        cpl_xmemory_sum_cells,
                        cpl_xmemory_max_cells,
                        cpl_xmemory_alloc_max,
                        cpl_xmemory_table_size_max,
                        cpl_xmemory_table_size_max * rowsize);
            } else {
                fprintf(stderr, "Total number of allocations"
#               ifdef _OPENMP
                        /* cpl_xmemory_sum_cells is not updated thread-safely */
                        " (approximate)"
#               endif
                        ": %zu\n", cpl_xmemory_sum_cells);
                fprintf(stderr, "Maximum number of pointers"
#               ifdef _OPENMP
                        /* cpl_xmemory_max_cells is not updated thread-safely */
                        " (approximate)"
#               endif
                        ": %zu\n", cpl_xmemory_max_cells);
            }

            fprintf(stderr, "#----- Memory Currently Allocated -----\n");

            fprintf(stderr, "Number of active pointers:    %zu\n",
                    cpl_xmemory_ncells);

            if (mode == 2 && cpl_xmemory_ncells > 0) {
                size_t ii;
                size_t ntype[4] = {0, 0, 0, 0};

                /* assert( cpl_xmemory_table_size != 0 ); */

                fprintf(stderr,
                        "Memory allocation [B]:        %zu\n"
                        "Current table size [pointer]: %zu\n"
                        "Current table size [B]:       %zu\n",
                        cpl_xmemory_alloc_ram,
                        cpl_xmemory_table_size,
                        cpl_xmemory_table_size * rowsize);

                /* Do not print entire table on fatal error
                   - it is likely huge */
                if (cpl_xmemory_fatal == 0)
                    fprintf(stderr, "#- Active pointer details\n");
                for (ii=0; ii < cpl_xmemory_table_size; ii++) {
                    if (cpl_xmemory_p_type[ii] != CPL_XMEMORY_TYPE_FREE) {
                        ntype[cpl_xmemory_p_type[ii]]++;
                        if (cpl_xmemory_fatal == 0)
                            fprintf(stderr, "(%p) - %s() of %zu bytes\n",
                                    cpl_xmemory_p_val[ii],
                                    cpl_xmemory_type[cpl_xmemory_p_type[ii]],
                                    cpl_xmemory_p_size[ii]);
                    }
                }
                fprintf(stderr, "#- Types of active pointers\n");
                fprintf(stderr, "%7s(): %zu\n", cpl_xmemory_type[1], ntype[1]);
                fprintf(stderr, "%7s(): %zu\n", cpl_xmemory_type[2], ntype[2]);
                fprintf(stderr, "%7s(): %zu\n", cpl_xmemory_type[3], ntype[3]);
            }
            if (cpl_xmemory_fatal != 0) (void)fflush(stderr);
        }

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Tell if there is still some memory allocated
  @param mode  Mode (0: system, 1: Xmemory-failure check, 2: Xmemory-full)
  @return   1 if the memory table is empty, 0 if no,
            -1 if the memory model is off
 */
/*----------------------------------------------------------------------------*/
int cpl_xmemory_is_empty(int mode)
{
    if (mode > 0) {
        /*
          assert(cpl_xmemory_ncells >  0 || cpl_xmemory_alloc_ram == 0);
          if (mode == 2)
          assert(cpl_xmemory_ncells == 0 || cpl_xmemory_alloc_ram  > 0);
        */
        return cpl_xmemory_ncells == 0 ? 1 : 0;
    } else {
        return -1;
    }
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Initialize extended memory features.
  @return   void

  This function is implicitly called by the first of xmemory_alloc() and
  xmemory_free().
  It allocates the default number of memory cells.

 */
/*----------------------------------------------------------------------------*/
static void cpl_xmemory_init(void)
{

    if (CPL_XMEMORY_RESIZE_THRESHOLD > 1.0) {
        fprintf(stderr, "cpl_xmemory fatal error: Memory table resize threshold "
                CPL_XSTRINGIFY(CPL_XMEMORY_RESIZE_THRESHOLD) " must be less "
                "than or equal to 1, not "
                CPL_STRINGIFY(CPL_XMEMORY_RESIZE_THRESHOLD) "\n");
        assert(CPL_XMEMORY_RESIZE_THRESHOLD <= 1.0); /* More informative */

        abort();
    }

    if (CPL_XMEMORY_RESIZE_THRESHOLD <= 0.0) {
        fprintf(stderr, "cpl_xmemory fatal error: Memory table resize threshold "
                CPL_XSTRINGIFY(CPL_XMEMORY_RESIZE_THRESHOLD) " must be positive"
                ", not " CPL_STRINGIFY(CPL_XMEMORY_RESIZE_THRESHOLD) "\n");
        assert(CPL_XMEMORY_RESIZE_THRESHOLD > 0.0); /* More informative */
        abort();
    }

    if (CPL_XMEMORY_RESIZE_FACTOR <= 1.0) {
        fprintf(stderr, "cpl_xmemory fatal error: Memory table resize factor "
                CPL_XSTRINGIFY(CPL_XMEMORY_RESIZE_FACTOR) " must be greater "
                "than 1, not " CPL_STRINGIFY(CPL_XMEMORY_RESIZE_FACTOR) "\n");
        assert(CPL_XMEMORY_RESIZE_FACTOR > 1.0); /* More informative */
        abort();
    }

    cpl_xmemory_table_size = CPL_XMEMORY_MAXPTRS;

    cpl_xmemory_init_alloc();

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Allocate memory for memory features.
  @return   void
  @note This allocation is done directly with calloc()

 */
/*----------------------------------------------------------------------------*/
static void cpl_xmemory_init_alloc(void)
{

    {
        cpl_xmemory_p_val  = calloc(cpl_xmemory_table_size, sizeof(void*));
        cpl_xmemory_p_type = calloc(cpl_xmemory_table_size,
                                    sizeof(unsigned char));
        cpl_xmemory_p_size = malloc(cpl_xmemory_table_size * sizeof(size_t));

        /* FIXME: pragma omp atomic */
        if (cpl_xmemory_table_size > cpl_xmemory_table_size_max) {
            cpl_xmemory_table_size_max = cpl_xmemory_table_size;
        }

    }

    if (cpl_xmemory_p_val == NULL || cpl_xmemory_p_size == NULL ||
        cpl_xmemory_p_type == NULL) {
        /* The table could not be allocated */
        fprintf(stderr, "cpl_xmemory fatal error: calloc() of memory table "
                "failed with size %zu:\n", cpl_xmemory_table_size);
        perror(NULL);
        cpl_xmemory_fatal = 1;
        cpl_xmemory_status(1); /* Mode-2 data is invalid! */
        assert(cpl_xmemory_p_val != NULL);  /* More informative, if enabled */
        assert(cpl_xmemory_p_size != NULL);
        assert(cpl_xmemory_p_type != NULL);
        abort();
    }

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Resize the pointer allocation table
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_xmemory_resize(void)
{

    const void   ** p_val    = cpl_xmemory_p_val;
    size_t        * p_size   = cpl_xmemory_p_size;
    unsigned char * p_type   = cpl_xmemory_p_type;
    const size_t    old_size = cpl_xmemory_table_size;
    size_t          ii;

    assert( cpl_xmemory_table_size > 0 );

    assert( cpl_xmemory_ncells > 0 );

    cpl_xmemory_table_size = cpl_xmemory_table_size * CPL_XMEMORY_RESIZE_FACTOR;

    /* Ensure that the table size is odd, to reduce hashing clashes */
    cpl_xmemory_table_size |= 1;

    if (cpl_xmemory_table_size <= old_size) {
        /* The table could not be resized */
        fprintf(stderr, "cpl_xmemory fatal error: Memory table could not be "
                "resized, %zu < %zu:\n", cpl_xmemory_table_size, old_size);
        perror(NULL);
        cpl_xmemory_fatal = 1;
        cpl_xmemory_status(1); /* Mode-2 data is invalid! */
        assert(cpl_xmemory_table_size > old_size);  /* More informative */
        abort();
    }

    cpl_xmemory_init_alloc();

    /* Locate pointer in main table */
    /* #   pragma omp parallel for private(ii,pos) schedule(static) */
    for (ii = 0; ii < old_size; ii++) {
        if (p_type[ii]) {
            /* Old table has a pointer here */
            /* Find position in enlarged table */
            const size_t pos = cpl_xmemory_findfree(p_val[ii]);

            /* Duplicated from cpl_xmemory_addcell() */
            cpl_xmemory_p_val[pos] = p_val[ii];
            cpl_xmemory_p_size[pos] = p_size[ii];
            cpl_xmemory_p_type[pos] = p_type[ii];
        }
    }

    free((void*)p_val);
    free(p_size);
    free(p_type);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Deinitialize extended memory features.
  @return   void

 */
/*----------------------------------------------------------------------------*/
inline
static void cpl_xmemory_end(void)
{

    assert( cpl_xmemory_ncells == 0 );

    cpl_xmemory_table_size = 0;

    cpl_xmemory_alloc_ram = 0;

    free(cpl_xmemory_p_val);
    free(cpl_xmemory_p_type);
    free(cpl_xmemory_p_size);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Add allocation cell.
  @param    pos             Position in the table
  @param    pointer         Pointer value.
  @param    size            Pointer size.
  @param    type            Allocation type
  @return   void

  Add a memory cell in the xtended memory table to register that a new
  allocation took place.
  This call is not protected against illegal parameter values, so make sure
  the passed values are correct!
 */
/*----------------------------------------------------------------------------*/
inline
static void cpl_xmemory_addcell(size_t        pos,
                                const void  * pointer,
                                size_t        size,
                                unsigned char type)
{

    /* Store information */
    cpl_xmemory_p_val[pos] = pointer;
    cpl_xmemory_p_size[pos] = size;

    /* Allocation type */
    cpl_xmemory_p_type[pos] = type;

    cpl_xmemory_alloc_ram += size;

    /* Remember peak allocation */
    /* FIXME: pragma omp atomic */
    if (cpl_xmemory_alloc_ram > cpl_xmemory_alloc_max)
        cpl_xmemory_alloc_max = cpl_xmemory_alloc_ram;

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Find the cell of a given pointer
  @param    ptr       Xmemory-allocated pointer, i.e. not NULL
  @return   Index of cell with pointer, or cpl_xmemory_table_size when not found

 */
/*----------------------------------------------------------------------------*/
inline
static size_t cpl_xmemory_findcell(const void * ptr)
{
    const size_t pos = PTR_HASH(ptr);
    size_t       ii = pos;

    /* Locate pointer in main table */
    do {
        if (cpl_xmemory_p_val[ii] == ptr) return ii; /* Found */
        if (++ii == cpl_xmemory_table_size) ii = 0; /* Wrap around */
    } while (ii != pos);

    /* Not found */
    return cpl_xmemory_table_size;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Find the first free cell suitable for the pointer
  @param    ptr      Allocated pointer to keep in Xmemory
  @return   Index of cell with pointer
  @note This function will exit when the table is full

 */
/*----------------------------------------------------------------------------*/
inline
static size_t cpl_xmemory_findfree(const void * ptr)
{

    /* Find an available slot */
    const size_t pos = PTR_HASH(ptr);

    /* In comparison with the method of cpl_xmemory_findcell()
       this is faster when the table starts to be full */
    const void * ppos = memchr(cpl_xmemory_p_type+pos, CPL_XMEMORY_TYPE_FREE,
                               cpl_xmemory_table_size - pos);

    if (ppos == NULL) {
        ppos = memchr(cpl_xmemory_p_type, CPL_XMEMORY_TYPE_FREE, pos);

        if (ppos == NULL) {
            /* No available slot */
            fprintf(stderr, "cpl_xmemory internal, fatal error: "
                    "Could not find place for new pointer\n");
            cpl_xmemory_fatal = 1;
            cpl_xmemory_status(2);
            assert(ppos != NULL); /* More informative, if enabled */
            abort();
        }
    }

    return ((size_t)ppos - (size_t)cpl_xmemory_p_type);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Remove the specified memory cell from the xtended memory table.
  @param    pos     Position of the pointer in the table.
  @return   void
  @note This call is not protected against illegal parameter values,
        so make sure the passed value is correct!

 */
/*----------------------------------------------------------------------------*/
inline
static void cpl_xmemory_remcell(size_t pos)
{
    /* Set pointer to NULL */
    cpl_xmemory_p_val[pos] = NULL;

    /* Set type to free */
    cpl_xmemory_p_type[pos] = CPL_XMEMORY_TYPE_FREE;

    cpl_xmemory_alloc_ram -= cpl_xmemory_p_size[pos];

    return;
}
