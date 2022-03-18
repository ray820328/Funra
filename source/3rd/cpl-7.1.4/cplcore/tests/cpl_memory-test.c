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

/*-----------------------------------------------------------------------------
                                Includes
 -----------------------------------------------------------------------------*/

#include "cpl_memory.h"

#include "cpl_test.h"
#include "cpl_io_fits.h"
#include "cpl_tools.h"

#include <string.h>

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#ifndef CPL_XMEMORY_MODE
#  error "Must know CPL_XMEMORY_MODE"
#endif

#ifndef CPL_XMEMORY_MAXPTRS
#  error "Must know CPL_XMEMORY_MAXPTRS"
#endif

/* Fill the xmemory table to this level */
/* A positive number not exceeding 1.0 */
#ifndef CPL_XMEMORY_FILL
#  define CPL_XMEMORY_FILL 0.999
#endif

/* Half the number of pointers to test with */
#if CPL_XMEMORY_MODE == 2
#define CPL_MEMORY_MAXPTRS_HALF \
 ((int)(0.5*CPL_XMEMORY_MAXPTRS*CPL_XMEMORY_FILL))
#else
#define CPL_MEMORY_MAXPTRS_HALF \
 ((int)(0.5*200003*CPL_XMEMORY_FILL))
#endif

#if CPL_XMEMORY_MODE == 0
#  define CPL_MEMORY_IS_EMPTY -1
#  define CPL_MEMORY_IS_NON_EMPTY -1
#else
#  define CPL_MEMORY_IS_EMPTY 1
#  define CPL_MEMORY_IS_NON_EMPTY 0
#endif

static int cpl_memory_test_is_empty(void);
static int cpl_memory_test_is_non_empty(void);

static void cpl_memory_test_alloc(void);
static void cpl_memory_sprintf_test(void);
static void cpl_memory_strdup_test(void);

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_utils_test Testing of the CPL memory functions
 */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
   @brief   Unit tests of CPL memory module
**/
/*----------------------------------------------------------------------------*/

int main(void)
{
    cpl_boolean do_bench;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Need to disable the (unused) FITS caching so no memory is allocated */
    (void)cpl_io_fits_end();

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    /* Insert tests below */

#if defined CPL_XMEMORY_MODE
    cpl_msg_info(cpl_func, CPL_XSTRINGIFY(CPL_XMEMORY_MODE) ": "
                 CPL_STRINGIFY(CPL_XMEMORY_MODE) "\n");
#else
    cpl_msg_info(cpl_func, "CPL_XMEMORY_MODE is not defined\n");
#endif

    cpl_memory_test_alloc();
    cpl_memory_sprintf_test();
    cpl_memory_strdup_test();

    if (do_bench && CPL_MEMORY_MAXPTRS_HALF > 0) {
        /* Always compile the bench-marking code */
        int ii = 2;
        int i;

        void * buf1[CPL_MEMORY_MAXPTRS_HALF];
        void * buf2[CPL_MEMORY_MAXPTRS_HALF];

        cpl_msg_info("", "Testing memory allocation with %d X 2*%d", ii,
                     CPL_MEMORY_MAXPTRS_HALF);

        do {

            for (i = 0; i < CPL_MEMORY_MAXPTRS_HALF; i++) {
                buf1[i] = cpl_malloc(16);
                buf2[i] = cpl_calloc(4, 16);
            }

            cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());

            for (i = 0; i < CPL_MEMORY_MAXPTRS_HALF; i++) {
                buf1[i] = cpl_realloc(buf1[i], 32);
                buf2[i] = cpl_realloc(buf2[i], 32);
            }

            cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());

            for (i = 0; i < CPL_MEMORY_MAXPTRS_HALF; i++) {
                cpl_free(buf1[i]);
                cpl_free(buf2[i]);
            }

            cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_empty());

        } while (--ii);

    }

    if (cpl_msg_get_level() <= CPL_MSG_INFO) cpl_memory_dump();

    return cpl_test_end(0);
}

static int cpl_memory_test_is_empty(void) {

#if defined CPL_XMEMORY_MODE && CPL_XMEMORY_MODE != 0
    int is_empty =  1;
#else
    int is_empty =  -1;
#endif

    /* Copied from cpl_init() */

    const char * memory_mode_string = getenv("CPL_MEMORY_MODE");

    if (memory_mode_string != NULL) {
        if (strcmp("0", memory_mode_string) == 0) {
            is_empty = -1;
        } else if (strcmp("1", memory_mode_string) == 0) {
            is_empty = 1;
        } else if (strcmp("2", memory_mode_string) == 0) {
            is_empty = 1;
        }
    }

    return is_empty;

}

static int cpl_memory_test_is_non_empty(void) {

#if defined CPL_XMEMORY_MODE && CPL_XMEMORY_MODE != 0
    int is_non_empty =  0;
#else
    int is_non_empty =  -1;
#endif

    /* Copied from cpl_init() */

    const char * memory_mode_string = getenv("CPL_MEMORY_MODE");

    if (memory_mode_string != NULL) {
        if (strcmp("0", memory_mode_string) == 0) {
            is_non_empty = -1;
        } else if (strcmp("1", memory_mode_string) == 0) {
            is_non_empty = 0;
        } else if (strcmp("2", memory_mode_string) == 0) {
            is_non_empty = 0;
        }
    }

    return is_non_empty;


}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the memory de/allocation functions
  @return  void
  @see cpl_malloc(), cpl_calloc, cpl_realloc(), cpl_free()

 */
/*----------------------------------------------------------------------------*/
static void cpl_memory_test_alloc(void)
{
    const size_t size = 997;
    void * buf1;
    void * buf2;

    cpl_free(NULL);

    if (cpl_msg_get_level() <= CPL_MSG_INFO) cpl_memory_dump();

    cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_empty());

    if (cpl_msg_get_level() <= CPL_MSG_INFO) cpl_memory_dump();

    buf1 = cpl_malloc(size);
    buf2 = cpl_calloc(size, size);

    cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());

    cpl_free(buf1);
    cpl_free(buf2);

    cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_empty());

    buf1 = cpl_malloc(size);
    buf2 = cpl_calloc(size, size);

    cpl_test_nonnull(buf1);
    cpl_test_nonnull(buf2);
    cpl_test(buf1 != buf2);
    cpl_test(memset(buf1, 0, size) == buf1);
    cpl_test_zero(memcmp(buf1, buf2, size));

    buf2 = cpl_realloc(buf2, size);

    cpl_test_nonnull(buf2);
    cpl_test(buf1 != buf2);
        
    cpl_test_zero(memcmp(buf1, buf2, size));

    cpl_free(buf1);
    cpl_free(buf2);

    cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_empty());

    /* Test cpl_malloc() involving zero size */
    buf1 = cpl_malloc(0);
    if (buf1 == NULL) {
        cpl_msg_info(cpl_func, "cpl_malloc(0) returned NULL");
    } else {
        cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());
    }

    cpl_free(buf1);

    cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_empty());

    /* Test cpl_calloc() involving zero size */
    buf2 = cpl_calloc(0, 0);
    if (buf2 == NULL) {
        cpl_msg_info(cpl_func, "cpl_calloc(0, 0) returned NULL");
    } else {
        cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());
    }
    cpl_free(buf2);

    buf1 = cpl_calloc(0, size);
    if (buf1 == NULL) {
        cpl_msg_info(cpl_func, "cpl_calloc(0, size) returned NULL");
    }
    cpl_free(buf1);

    buf1 = cpl_calloc(size, 0);
    if (buf1 == NULL) {
        cpl_msg_info(cpl_func, "cpl_calloc(size, 0) returned NULL");
    } else {
        cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());
    }
    cpl_free(buf1);

    /* Test cpl_realloc() involving zero size */
    buf1 = cpl_realloc(NULL, 0);
    if (buf1 == NULL) {
        cpl_msg_info(cpl_func, "cpl_realloc(NULL, 0) returned NULL");
    } else {
        cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());
    }

    buf1 = cpl_realloc(buf1, 0);
    if (buf1 == NULL) {
        cpl_msg_info(cpl_func, "cpl_realloc(buf1, 0) returned NULL");
    } else {
        cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());
    }

    buf1 = cpl_realloc(buf1, size);
    cpl_test_nonnull(buf1);

    buf1 = cpl_realloc(buf1, 0);
    if (buf1 == NULL) {
        cpl_msg_info(cpl_func, "cpl_realloc(buf1, 0) returned NULL");
    } else {
        cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_non_empty());
    }

    cpl_free(buf1);

    cpl_test_eq(cpl_memory_is_empty(), cpl_memory_test_is_empty());

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test cpl_sprintf()
  @return  void
  @see cpl_sprintf()

 */
/*----------------------------------------------------------------------------*/
static void cpl_memory_sprintf_test(void)
{

    const char * string = __FILE__; /* Some non-empty string */
    const size_t stringlen = strlen(string);
    char * null; /* This string is supposed to always be NULL */
    const char * null2; /* This string is supposed to always be NULL */
    char * nonnull; /* String is supposed to never be NULL */
    int length;

    /* Create a copy of an empty string */
    nonnull = cpl_sprintf("%s%n", "", &length);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq_string(nonnull, "");
    cpl_test_zero(length);

    cpl_free(nonnull);

    /* Create a 2nd copy of an empty string */
    nonnull = cpl_sprintf("%n", &length);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq_string(nonnull, "");
    cpl_test_zero(length);

    cpl_free(nonnull);

    /* Create a copy of the string */
    nonnull = cpl_sprintf("%s%n", string, &length);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq_string(nonnull, string);

    cpl_test_eq(length, (int)stringlen);

    cpl_free(nonnull);

    /* Successfully create an illegal format string */
    nonnull = cpl_sprintf("%s %s", string, "%");

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq_string(nonnull, __FILE__ " %");

    /* Check if an error is produced using that illegal format */
    null = cpl_sprintf(nonnull, ".");

    if (null != NULL) {
        cpl_test_error(CPL_ERROR_NONE);
        cpl_msg_warning(cpl_func, "The supposedly illegal format '%s' "
                        "produced the string: '%s'", nonnull, null);
        cpl_free(null);
        null = NULL;
    } else {
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    }

    cpl_free(nonnull);

    null2 = cpl_sprintf(NULL);

    cpl_test_error(CPL_ERROR_NULL_INPUT);

    cpl_test_null(null2);

    /* A NULL-pointer as a format argument does not trigger an error */
    nonnull = cpl_sprintf("%s", null2);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_nonnull(nonnull);
    cpl_test_noneq_string(nonnull, "");

    cpl_free(nonnull);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test cpl_strdup()
  @return  void
  @see cpl_strdup()

 */
/*----------------------------------------------------------------------------*/
static void cpl_memory_strdup_test(void)
{

    const char * string = __FILE__; /* Some non-empty string */
    char * nonnull; /* String is supposed to never be NULL */

    /* Create a copy of the string */
    nonnull = cpl_strdup(string);

    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_nonnull(nonnull);
    cpl_test_eq_string(nonnull, string);

    cpl_free(nonnull);

}
