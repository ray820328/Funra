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

#ifndef CPL_TOOLS_H
#define CPL_TOOLS_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpl_cfitsio.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <float.h>

#include <cxtypes.h>

#include "cpl_error.h"
#include "cpl_errorstate.h"
#include "cpl_vector.h"
#include "cpl_msg.h"
#include "cpl_test.h"
#include "cpl_type.h"
#include "cpl_type_impl.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

/**
   @ingroup cpl_tools
   @internal
   @brief Generate and return a pseudo-random double in the range 0 to 1.
   @return A pseudo-random number in the range 0 to 1
   @see rand()
*/
#define cpl_drand() ((double)rand()/(double)RAND_MAX)

#define CPL_FITS_BADKEYS        "SIMPLE|BSCALE|BZERO|EXTEND" \
        "|XTENSION|BITPIX|NAXIS[0-9]*|PCOUNT|GCOUNT|DATE" "" \
        "|TFIELDS|TFORM[0-9]*|TTYPE[0-9]*|TUNIT[0-9]*|TZERO[0-9]*" \
        "|TSCAL[0-9]*|TDIM[0-9]*|TNULL[0-9]*"

#define CPL_FITS_BADKEYS_PRIM   "^(" CPL_FITS_BADKEYS \
        "|BLOCKED" ")$"

#define CPL_FITS_BADKEYS_EXT    "^(" CPL_FITS_BADKEYS \
        "|ORIGIN[0-9]*)$"

#define CPL_FITS_COMPRKEYS  "^(" "ZIMAGE|ZCMPTYPE|ZNAXIS" \
        "|ZTENSION|ZPCOUNT|ZGCOUNT|ZNAME[0-9]|ZVAL[0-9]"  \
        "|ZTILE[0-9]|ZBITPIX|ZNAXIS[0-9]|ZSCALE|ZZERO"    \
        "|ZBLANK|ZHECKSUM|ZDATASUM" ")$"

/**
   @internal
   @brief CFITSIO wrapper without warning for const violation
   @see fits_get_colnum
   @note Missing one const modifier in CFITSIO 3.42
*/
#define cpl_fits_get_colnum(A, B, C, D, E)      \
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);      \
    fits_get_colnum(A, B, (char*)C, D, E);      \
    CPL_DIAG_PRAGMA_POP

/**
   @internal
   @brief CFITSIO wrapper without warning for const violation
   @see fits_write_colnull
   @note Missing two const modifiers in CFITSIO 3.42
*/
#define cpl_fits_write_colnull(A, B, C, D, E, F, G, H, I)               \
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);                              \
    fits_write_colnull(A, B, C, D, E, F, (void*)G, (void*)H, I);        \
    CPL_DIAG_PRAGMA_POP

/**
   @internal
   @brief CFITSIO wrapper without warning for const violation
   @see fits_create_tbl
   @note Missing three const modifiers in CFITSIO 3.42
*/
#define cpl_fits_create_tbl(A, B, C, D, E, F, G, H, I)                  \
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);                              \
    fits_create_tbl(A, B, C, D, (char**)E, (char**)F, (char**)G, H, I); \
    CPL_DIAG_PRAGMA_POP

/* CFITSIO support for I/O of largest possible buffers. */
/* FIXME: Add CPL_FITSIO_FORMAT w. "ld" for long int, but
          need a format string for LONGLONG */
#if defined fits_get_img_sizell && defined fits_read_pixll && defined fits_create_imgll
#define CPL_FITSIO_GET_SIZE fits_get_img_sizell
#define CPL_FITSIO_READ_PIX cpl_fits_read_pixll
#define CPL_FITSIO_READ_PIX_E   fits_read_pixll
#define CPL_FITSIO_RESIZE_IMG fits_resize_imgll
#define CPL_FITSIO_CREATE_IMG cpl_fits_create_imgll
#define CPL_FITSIO_CREATE_IMG_E   fits_create_imgll
#define CPL_FITSIO_WRITE_PIX cpl_fits_write_pixll
#define CPL_FITSIO_WRITE_PIX_E   fits_write_pixll
#define CPL_FITSIO_TYPE LONGLONG
#else
#define CPL_FITSIO_GET_SIZE fits_get_img_size
#define CPL_FITSIO_READ_PIX cpl_fits_read_pix
#define CPL_FITSIO_READ_PIX_E   fits_read_pix
#define CPL_FITSIO_RESIZE_IMG fits_resize_img
#define CPL_FITSIO_CREATE_IMG cpl_fits_create_img
#define CPL_FITSIO_CREATE_IMG_E   fits_create_img
#define CPL_FITSIO_WRITE_PIX cpl_fits_write_pix
#define CPL_FITSIO_WRITE_PIX_E   fits_write_pix
#define CPL_FITSIO_TYPE long int
#endif

#ifdef CPL_ADD_FLOPS
#define cpl_tools_add_flops cpl_tools_add_flops_
#else
#define cpl_tools_add_flops(A) /* Do nothing */
#endif

/* FIXME: Move to cpl_macros.h */
#if __GNUC__ >= 7
#  define CPL_ATTR_FALLTRHU __attribute__ ((fallthrough))
#else
#  define CPL_ATTR_FALLTRHU /* fall through */
#endif


/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Create a tracing message
   @param LEVEL The CPL messaging level to use, CPL_MSG_ERROR, CPL_MSG_WARNING,
                CPL_MSG_INFO, CPL_MSG_DEBUG. Nothing happens for other values.
   @return void
   @note This macro will also fflush() both stdout and stderr which can be
   useful in tracing a process that terminates unexpectedly.
*/
/*----------------------------------------------------------------------------*/
#define cpl_tools_trace(LEVEL)                                          \
    do {                                                                \
        const cpl_msg_severity cpl_tools_trace_level = LEVEL;           \
        if (cpl_tools_trace_level == CPL_MSG_ERROR ||                   \
            cpl_tools_trace_level == CPL_MSG_WARNING  ||                \
            cpl_tools_trace_level == CPL_MSG_INFO  ||                   \
            cpl_tools_trace_level == CPL_MSG_DEBUG) {                   \
            (cpl_tools_trace_level == CPL_MSG_ERROR ? cpl_msg_error :   \
             (cpl_tools_trace_level == CPL_MSG_WARNING ? cpl_msg_warning : \
              (cpl_tools_trace_level == CPL_MSG_INFO ? cpl_msg_info :   \
               cpl_msg_debug))) (cpl_func, "TRACE: " __FILE__           \
                                 " at line " CPL_STRINGIFY(__LINE__));  \
            (void)fflush(stdout);                                       \
            (void)fflush(stderr);                                       \
        }                                                               \
    } while (0)


#if defined HAVE_LONG_DOUBLE && HAVE_LONG_DOUBLE == 1
typedef long double cpl_long_double;
#define CPL_LDBL_EPSILON LDBL_EPSILON
#define CPL_LDBL_EPSILON_STR "LDBL_EPSILON"
#else
typedef double cpl_long_double;
#define CPL_LDBL_EPSILON DBL_EPSILON
#define CPL_LDBL_EPSILON_STR "DBL_EPSILON"
#endif

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_tools
   @internal
   @brief Maximum size [byte] of ifalloc variable without heap allocation
   @see cpl_ifalloc_set
 */
/*----------------------------------------------------------------------------*/
#ifndef CPL_IFALLOC_SZ
#define CPL_IFALLOC_SZ 1024
#endif

/*----------------------------------------------------------------------------*/
/**
   @ingroup cpl_tools
   @internal
   @brief The struct used for the ifalloc variable
   @see cpl_ifalloc_set
 */
/*----------------------------------------------------------------------------*/
typedef struct {
    void * data;
    void * heap;
    char stack[CPL_IFALLOC_SZ];
} cpl_ifalloc;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/
void cpl_ifalloc_set(cpl_ifalloc *, size_t) CPL_ATTR_NONNULL;
void * cpl_ifalloc_get(cpl_ifalloc *) CPL_ATTR_NONNULL;
void cpl_ifalloc_free(cpl_ifalloc *) CPL_ATTR_NONNULL;

/* Operations on arrays */
double cpl_tools_get_kth_double(double *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
float cpl_tools_get_kth_float(float *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
int cpl_tools_get_kth_int(int *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
long cpl_tools_get_kth_long(long *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
long long cpl_tools_get_kth_long_long(long long *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
cpl_size cpl_tools_get_kth_cplsize(cpl_size *, cpl_size, cpl_size) CPL_ATTR_NONNULL;

double cpl_tools_quickselection_double(double *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
float cpl_tools_quickselection_float(float *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
int cpl_tools_quickselection_int(int *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
long cpl_tools_quickselection_long(long *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
long long cpl_tools_quickselection_long_long(long long *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
cpl_size cpl_tools_quickselection_cplsize(cpl_size *, cpl_size, cpl_size) CPL_ATTR_NONNULL;

double cpl_tools_get_mean_double(const double *, cpl_size);
double cpl_tools_get_mean_float(const float *, cpl_size);
double cpl_tools_get_mean_int(const int *, cpl_size);
double cpl_tools_get_mean_long(const long *, cpl_size);
double cpl_tools_get_mean_long_long(const long long *, cpl_size);
double cpl_tools_get_mean_cplsize(const cpl_size *, cpl_size);

double cpl_tools_get_variancesum_double(const double *, cpl_size, double *);
double cpl_tools_get_variancesum_float(const float *, cpl_size, double *);
double cpl_tools_get_variancesum_int(const int *, cpl_size, double *);
double cpl_tools_get_variancesum_long(const long *, cpl_size, double *);
double cpl_tools_get_variancesum_long_long(const long long *, cpl_size, double *);
double cpl_tools_get_variancesum_cplsize(const cpl_size *, cpl_size, double *);

double cpl_tools_get_variance_double(const double *, cpl_size,
                                     double *) CPL_ATTR_NONNULL;
double cpl_tools_get_variance_float(const float *, cpl_size,
                                    double *) CPL_ATTR_NONNULL;
double cpl_tools_get_variance_int(const int *, cpl_size,
                                  double *) CPL_ATTR_NONNULL;
double cpl_tools_get_variance_long(const long *, cpl_size,
                                   double *) CPL_ATTR_NONNULL;
double cpl_tools_get_variance_long_long(const long long *, cpl_size,
                                        double *) CPL_ATTR_NONNULL;
double cpl_tools_get_variance_cplsize(const cpl_size *, cpl_size,
                                  double *) CPL_ATTR_NONNULL;
double cpl_tools_get_median_double(double *, cpl_size) CPL_ATTR_NONNULL;
float cpl_tools_get_median_float(float *, cpl_size) CPL_ATTR_NONNULL;
int cpl_tools_get_median_int(int *, cpl_size) CPL_ATTR_NONNULL;
long cpl_tools_get_median_long(long *, cpl_size) CPL_ATTR_NONNULL;
long long cpl_tools_get_median_long_long(long long *, cpl_size) CPL_ATTR_NONNULL;
cpl_size cpl_tools_get_median_cplsize(cpl_size *, cpl_size) CPL_ATTR_NONNULL;

cpl_error_code cpl_tools_copy_window(void *, const void *, size_t,
                                     cpl_size, cpl_size, cpl_size, cpl_size,
                                     cpl_size, cpl_size) CPL_ATTR_NONNULL;

cpl_error_code cpl_tools_shift_window(void *, size_t, cpl_size, cpl_size, int,
                                      cpl_size, cpl_size) CPL_ATTR_NONNULL;

/* Sorting */
cpl_error_code cpl_tools_sort_stable_pattern_cplsize(cpl_size const *, 
                                                 cpl_size, int, int,
                                                 cpl_size *);
cpl_error_code cpl_tools_sort_stable_pattern_int(int const *, 
                                                 cpl_size, int, int,
                                                 cpl_size *);
cpl_error_code cpl_tools_sort_stable_pattern_long(long const *,
                                                  cpl_size, int, int,
                                                  cpl_size *);
cpl_error_code cpl_tools_sort_stable_pattern_long_long(long long const *,
                                                       cpl_size, int, int,
                                                       cpl_size *);
cpl_error_code cpl_tools_sort_stable_pattern_float(float const *, 
                                                   cpl_size, int, int,
                                                   cpl_size *);
cpl_error_code cpl_tools_sort_stable_pattern_double(double const *, 
                                                    cpl_size, int, int,
                                                    cpl_size *);
cpl_error_code cpl_tools_sort_stable_pattern_string(char * const *,
                                                    cpl_size, int, int,
                                                    cpl_size *);

cpl_error_code cpl_tools_permute_cplsize(cpl_size *, cpl_size, cpl_size *, 
                                      cpl_size const *,
                                      cpl_size *, cpl_size const *);
cpl_error_code cpl_tools_permute_int(cpl_size *, cpl_size, int *, int const *,
                                     int *, int const *);
cpl_error_code cpl_tools_permute_long(cpl_size *, cpl_size, long *, long const *,
                                      long *, long const *);
cpl_error_code cpl_tools_permute_long_long(cpl_size *, cpl_size,
                                           long long *, long long const *,
                                           long long *, long long const *);
cpl_error_code cpl_tools_permute_float(cpl_size *, cpl_size, float *, float const *,
                                       float *, float const *);
cpl_error_code cpl_tools_permute_double(cpl_size *, cpl_size, double *,
                                        double const *,
                                        double *, double const *);
cpl_error_code cpl_tools_permute_string(cpl_size *, cpl_size, char **,
                                        char * const *,
                                        char **, char * const *);

cpl_error_code cpl_tools_sort_double(double *, cpl_size) CPL_ATTR_NONNULL;
cpl_error_code cpl_tools_sort_float(float *, cpl_size) CPL_ATTR_NONNULL;
cpl_error_code cpl_tools_sort_int(int *, cpl_size) CPL_ATTR_NONNULL;
cpl_error_code cpl_tools_sort_long(long *, cpl_size) CPL_ATTR_NONNULL;
cpl_error_code cpl_tools_sort_long_long(long long *, cpl_size) CPL_ATTR_NONNULL;
cpl_error_code cpl_tools_sort_cplsize(cpl_size *, cpl_size) CPL_ATTR_NONNULL;

void cpl_tools_sort_ascn_double(double *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_ascn_float(float *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_ascn_int(int *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_ascn_long(long *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_ascn_long_long(long long *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_ascn_cplsize(cpl_size *, int) CPL_ATTR_NONNULL;

void cpl_tools_sort_desc_double(double *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_desc_float(float *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_desc_int(int *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_desc_long(long *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_desc_long_long(long long *, int) CPL_ATTR_NONNULL;
void cpl_tools_sort_desc_cplsize(cpl_size *, int) CPL_ATTR_NONNULL;

void cpl_tools_flip_double(double *, size_t) CPL_ATTR_NONNULL CPL_INTERNAL;
void cpl_tools_flip_float(float *, size_t) CPL_ATTR_NONNULL CPL_INTERNAL;
void cpl_tools_flip_int(int *, size_t) CPL_ATTR_NONNULL CPL_INTERNAL;

cpl_vector * cpl_vector_transform_mean(const cpl_vector *, double *)
    CPL_ATTR_NONNULL CPL_ATTR_ALLOC;

void cpl_tools_add_flops_(cpl_flops);

cpl_flops cpl_tools_get_flops(void)
#ifdef CPL_ADD_FLOPS
     CPL_ATTR_PURE
#else
     CPL_ATTR_CONST
#endif
     ;

/* Others */
cpl_size cpl_tools_is_power_of_2(cpl_size) CPL_ATTR_CONST;
double cpl_tools_ipow(double, int) CPL_ATTR_CONST;
double * cpl_tools_fill_kernel_profile(cpl_kernel, int *);

int cpl_tools_get_bpp(cpl_type);

cpl_error_code cpl_vector_count_distinct(cpl_vector *, cpl_size *);
cpl_error_code cpl_vector_ensure_distinct(const cpl_vector *, cpl_size);

CPL_END_DECLS

#endif 

