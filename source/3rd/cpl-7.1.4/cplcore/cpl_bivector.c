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

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "cpl_memory.h"
#include "cpl_bivector.h"
#include "cpl_tools.h"
#include "cpl_error_impl.h"
#include "cpl_type.h"

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

#define HALF_CENTROID_DOMAIN        5

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_bivector   Bi-vector object
 *
 * This module provides functions to handle @em cpl_bivector.
 *
 * A @em cpl_bivector is composed of two vectors of the same size. It can
 * be used to store 1d functions, with the x and y positions of the
 * samples, offsets in x and y or simply positions in an image.
 *
 * This module provides among other things functions for interpolation and
 * for sorting one vector according to another.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_bivector.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                   Type definition
 -----------------------------------------------------------------------------*/

struct _cpl_bivector_ {
    cpl_vector * x;
    cpl_vector * y;
};

/*-----------------------------------------------------------------------------
                              Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a new cpl_bivector
  @param    n  Positive number of points
  @return   1 newly allocated cpl_bivector or NULL on error

  The returned object must be deallocated using cpl_bivector_delete() or
  cpl_bivector_unwrap_vectors(), provided the two cpl_vectors are deallocated
  separately.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if n is < 1.
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_bivector_new(cpl_size n)
{
    cpl_bivector * f;

    /* Test input */
    cpl_ensure(n > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Allocate memory */
    f = cpl_malloc(sizeof(cpl_bivector));
    f->x = cpl_vector_new(n);
    f->y = cpl_vector_new(n);

    return f;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a new cpl_bivector from two cpl_vectors
  @param    x   the x cpl_vector
  @param    y   the y cpl_vector
  @return   1 cpl_bivector or NULL on error
  @note The input cpl_vectors must have identical sizes. Afterwards one of
  those two vectors may be resized, which will corrupt the bivector. Such a
  corrupted bivector should not be used any more, but rather deallocated,
  using cpl_bivector_unwrap_vectors() or cpl_bivector_delete().

  The returned object must be deallocated using cpl_bivector_delete() or
  with cpl_bivector_unwrap_vectors(), provided the two cpl_vectors are
  deallocated separately.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input vectors have different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_bivector_wrap_vectors(
        cpl_vector    *   x,
        cpl_vector    *   y)
{
    cpl_bivector    *   f;

    /* Test input */
    cpl_ensure(x, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(y, CPL_ERROR_NULL_INPUT, NULL);

    cpl_ensure(cpl_vector_get_size(x)==cpl_vector_get_size(y),
            CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Allocate memory */
    f = cpl_malloc(sizeof(cpl_bivector));
    f->x = x;
    f->y = y;

    return f;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Duplicate a cpl_bivector
  @param    in cpl_bivector to duplicate
  @return   1 newly allocated cpl_bivector or NULL on error

  The returned object must be deallocated using cpl_bivector_delete()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input bivector contains vectors of
    different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_bivector_duplicate(const cpl_bivector * in)
{
    cpl_bivector    *   out;

    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_vector_get_size(in->x)==cpl_vector_get_size(in->y),
               CPL_ERROR_ILLEGAL_INPUT, NULL);

    out = cpl_malloc(sizeof(cpl_bivector));
    out->x = cpl_vector_duplicate(in->x);
    out->y = cpl_vector_duplicate(in->y);

    return out;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Copy contents of a bivector into another bivector
  @param    self     destination cpl_vector
  @param    other    source cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_vector_set_size() if source and destination have different sizes.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_bivector_copy(cpl_bivector       * self,
                                 const cpl_bivector * other)
{
    return cpl_vector_copy(cpl_bivector_get_x(self),
                           cpl_bivector_get_x_const(other)) ||
        cpl_vector_copy(cpl_bivector_get_y(self),
                        cpl_bivector_get_y_const(other))
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**

  @brief    Delete a cpl_bivector
  @param    f   cpl_bivector to delete
  @return   void

  This function deletes a bivector. If the input bivector @em f
  is @c NULL, nothing is done, and no error is set.

 */
/*----------------------------------------------------------------------------*/
void cpl_bivector_delete(cpl_bivector * f)
{
    if (f == NULL) return;
    cpl_vector_delete(f->x);
    cpl_vector_delete(f->y);
    cpl_free(f);
    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief   Free memory associated to a cpl_bivector, excluding the two vectors.
  @param   f  cpl_bivector to delete
  @return  void
  @see cpl_bivector_wrap_vectors

 */
/*----------------------------------------------------------------------------*/
void cpl_bivector_unwrap_vectors(cpl_bivector * f)
{
    cpl_free(f);
    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a cpl_bivector as ASCII to a stream
  @param    f       Input cpl_bivector to dump or NULL
  @param    stream  Output stream, accepts @c stdout or @c stderr or NULL
  @return   void

  Comment lines start with the hash character.

  stream may be NULL in which case @c stdout is used.

  @note
    In principle a cpl_bivector can be saved using cpl_bivector_dump() and
    re-read using cpl_bivector_read(). This will however introduce significant
    precision loss due to the limited accuracy of the ASCII representation.
 */
/*----------------------------------------------------------------------------*/
void cpl_bivector_dump(const cpl_bivector * f, FILE * stream)
{
    const double * data_x;
    const double * data_y;
    cpl_size       i;

    if (stream == NULL) stream = stdout;
    if (f == NULL) {
        fprintf(stream, "#NULL bivector\n");
        return;
    }

    if (cpl_vector_get_size(f->x) != cpl_vector_get_size(f->y)) return;

    data_x = cpl_vector_get_data_const(f->x);
    data_y = cpl_vector_get_data_const(f->y);

    fprintf(stream, "#--- Bi-vector ---\n");
    fprintf(stream, "#       X      Y  \n");

    for (i = 0; i < cpl_bivector_get_size(f); i++)
        fprintf(stream, "%g\t%g\n", data_x[i], data_y[i]);

    fprintf(stream, "#-----------------------\n");

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Read a list of values from an ASCII file and create a cpl_bivector
  @param    filename  Name of the input ASCII file
  @return   1 newly allocated cpl_bivector or NULL on error
  @see cpl_vector_load

  The input ASCII file must contain two values per line.

  The returned object must be deallocated using cpl_bivector_delete()
  Two columns of numbers are expected in the input file.

  In addition to normal files, FIFO (see man mknod) are also supported.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if the file cannot be read
  - CPL_ERROR_BAD_FILE_FORMAT if the file contains no valid lines
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_bivector_read(const char * filename)
{
    FILE        *   in;
    cpl_vector  *   v1;
    cpl_vector  *   v2;
    cpl_size        np = 0;
    cpl_size        size = 1000; /* Default size */
    char            line[1024];
    double          x, y;

    cpl_ensure(filename, CPL_ERROR_NULL_INPUT, NULL);

    /* Open the file */
    in = fopen(filename, "r");
    cpl_ensure(in,       CPL_ERROR_FILE_IO, NULL);

    /* Create and fill the vectors */
    v1 = cpl_vector_new(size);
    v2 = cpl_vector_new(size);

    while (fgets(line, 1024, in) != NULL) {
        if (line[0] != '#' && sscanf(line, "%lg %lg", &x, &y) == 2) {
            /* Found new element-pair
               - increase vector sizes if necessary,
               - insert element at end and
               - increment size counter */
            if (np == size) {
                size *= 2;
                cpl_vector_set_size(v1, size);
                cpl_vector_set_size(v2, size);
            }
            cpl_vector_set(v1, np, x);
            cpl_vector_set(v2, np, y);
            np++;
        }
    }

    /* Check that the loop ended due to eof and not an error */
    if (ferror(in)) {
        fclose(in);
        cpl_vector_delete(v1);
        cpl_vector_delete(v2);
        (void)cpl_error_set_(CPL_ERROR_FILE_IO);
        return NULL;
    }

    fclose(in);

    /* Check that the file was not empty and set the size to its true value */
    if (np == 0 || cpl_vector_set_size(v1, np) || cpl_vector_set_size(v2, np)) {
        cpl_vector_delete(v1);
        cpl_vector_delete(v2);
        (void)cpl_error_set_(CPL_ERROR_BAD_FILE_FORMAT);
        return NULL;
    }

    /* Cannot fail at this point */
    return cpl_bivector_wrap_vectors(v1, v2);

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the size of the cpl_bivector
  @param    in      the input bivector
  @return   The size or a negative number on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input bivector contains vectors of
    different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_bivector_get_size(const cpl_bivector * in)
{

    cpl_size     size;


    cpl_ensure(in, CPL_ERROR_NULL_INPUT, -1);

    assert( in->x );
    assert( in->y );

    /* Get sizes of x and y vectors */
    size = cpl_vector_get_size(in->x);
    cpl_ensure(size == cpl_vector_get_size(in->y), CPL_ERROR_ILLEGAL_INPUT,
               -2);

    return size;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the x vector of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the x vector or NULL on error

  The returned pointer refers to an already created cpl_vector.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_bivector_get_x(cpl_bivector * in)
{

    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->x );

    return in->x;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the x vector of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the x vector or NULL on error
  @see cpl_bivector_get_x
 */
/*----------------------------------------------------------------------------*/
const cpl_vector * cpl_bivector_get_x_const (const cpl_bivector * in)
{
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->x );

    return in->x;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the y vector of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the y vector or NULL on error

  The returned pointer refers to an already created cpl_vector.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_bivector_get_y(cpl_bivector * in)
{

    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->y );

    return in->y;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the y vector of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the y vector or NULL on error
 */
/*----------------------------------------------------------------------------*/
const cpl_vector * cpl_bivector_get_y_const(const cpl_bivector * in)
{
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->y );

    return in->y;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the x data part of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the double x array or NULL on error
  @see cpl_vector_get_data
  The returned pointer refers to already allocated data.

  @note
    Use at your own risk: direct manipulation of vector data rules out
    any check performed by the vector object interface, and may introduce
    inconsistencies between the information maintained internally, and
    the actual vector data and structure.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
double * cpl_bivector_get_x_data(cpl_bivector * in)
{

    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->x );

    return cpl_vector_get_data(in->x);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the x data part of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the double x array or NULL on error
  @see cpl_bivector_get_x_data

 */
/*----------------------------------------------------------------------------*/
const double * cpl_bivector_get_x_data_const(const cpl_bivector * in)
{
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->x );

    return cpl_vector_get_data_const(in->x);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the y data part of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the double y array or NULL on error
  @see cpl_vector_get_x_data

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
double * cpl_bivector_get_y_data(cpl_bivector * in)
{

    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->y );

    return cpl_vector_get_data(in->y);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the y data part of the cpl_bivector
  @param    in  a cpl_bivector
  @return   Pointer to the double y array or NULL on error
  @see cpl_bivector_get_y_data
 */
/*----------------------------------------------------------------------------*/
const double * cpl_bivector_get_y_data_const(const cpl_bivector * in)
{
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    assert( in->y );

    return cpl_vector_get_data_const(in->y);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Linear interpolation of a 1d-function
  @param    fout   Preallocated with X-vector set, to hold interpolation in Y
  @param    fref   Reference 1d-function
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_

  fref must have both its abscissa and ordinate defined.
  fout must have its abscissa defined and its ordinate allocated.

  The linear interpolation will be done from the values in fref to the abscissa
  points in fout.

  For each abscissa point in fout, fref must either have two neigboring abscissa
  points such that xref_i < xout_j < xref{i+1}, or a single identical abscissa
  point, such that xref_i == xout_j.

  This is ensured by monotonely growing abscissa points in both fout and fref
  (and by min(xref) <= min(xout) and max(xout) < max(xref)).

  However, for efficiency reasons (since fref can be very long) the monotonicity
  is only verified to the extent necessary to actually perform the interpolation.

  This input requirement implies that extrapolation is not allowed. 

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_DATA_NOT_FOUND if fout has an endpoint which is out of range
  - CPL_ERROR_ILLEGAL_INPUT if the monotonicity requirement on the 2 input
    abscissa vectors is not met.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_bivector_interpolate_linear(cpl_bivector * fout,
                                               const cpl_bivector * fref)
{
    const cpl_size m    = cpl_bivector_get_size(fref);
    const cpl_size n    = cpl_bivector_get_size(fout);
    const double * xref = cpl_bivector_get_x_data_const(fref);
    const double * yref = cpl_bivector_get_y_data_const(fref);
    double       * xout = cpl_bivector_get_x_data(fout);
    double       * yout = cpl_bivector_get_y_data(fout);

    double grad = DBL_MAX; /* Avoid (false) uninit warning */
    double y_0  = DBL_MAX; /* Avoid (false) uninit warning */
    cpl_size ibelow, iabove;
    cpl_size i;


    cpl_ensure_code( xref != NULL, cpl_error_get_code());
    cpl_ensure_code( yref != NULL, cpl_error_get_code());
    cpl_ensure_code( xout != NULL, cpl_error_get_code());
    cpl_ensure_code( yout != NULL, cpl_error_get_code());

    /* Upper extrapolation not allowed */
    cpl_ensure_code(xout[n-1] <= xref[m-1], CPL_ERROR_DATA_NOT_FOUND);

    /* Start interpolation from below */
    ibelow = cpl_vector_find(cpl_bivector_get_x_const(fref), xout[0]);

    if (xout[0] < xref[ibelow]) {
        /* Lower extrapolation also not allowed */
        cpl_ensure_code(ibelow > 0, CPL_ERROR_DATA_NOT_FOUND);
        ibelow--;
    }

    iabove = ibelow; /* Ensure grad initialization, for 1st interpolation */

    /* assert( xref[iabove] <= xout[0] ); */

    for (i = 0; i < n; i++) {
        /* When possible reuse reference function abscissa points */
        if (xref[iabove] < xout[i]) {
            /* No, need new points */
            while (xref[++iabove] < xout[i]);
            ibelow = iabove - 1;

            /* Verify that the pair of reference abscissa points are valid */
            if (xref[iabove] <= xref[ibelow]) break;

            grad = (yref[iabove] - yref[ibelow])
                 / (xref[iabove] - xref[ibelow]);

            y_0  = yref[ibelow] - grad * xref[ibelow];
        }

        if (xref[ibelow] < xout[i]) {

            yout[i] = y_0 + grad * xout[i];

        } else {

            /* assert( xout[i] == xref[ibelow] ); */

            yout[i] = yref[ibelow];

        }
    }

    return i == n ? CPL_ERROR_NONE : cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Sort a cpl_bivector
  @param  self   cpl_bivector to hold sorted result
  @param  other  Input cpl_bivector to sort, may equal self
  @param  dir    CPL_SORT_ASCENDING or CPL_SORT_DESCENDING
  @param  mode   CPL_SORT_BY_X or CPL_SORT_BY_Y
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  The values in the input are sorted according to direction and mode, and the
  result is placed self which must be of the same size as other.

  As for qsort():
  If two members compare as equal, their order in the sorted array is undefined.

  In place sorting is supported.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if self and other have different sizes
  - CPL_ERROR_ILLEGAL_INPUT if dir is neither CPL_SORT_DESCENDING nor
                            CPL_SORT_ASCENDING.
  - CPL_ERROR_UNSUPPORTED_MODE if self and other are the same or point to the
                               same underlying arrays, or if mode is neither 
                               CPL_SORT_BY_X nor CPL_SORT_BY_Y
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_bivector_sort(cpl_bivector * self,
                                 const cpl_bivector * other,
                                 cpl_sort_direction dir,
                                 cpl_sort_mode mode)
{

    const cpl_size n = cpl_bivector_get_size(self);
    cpl_ifalloc    permbuf; /* Avoid malloc() on small data sets */
    cpl_size     * perm;
    const double * data;
    int            reverse;
    cpl_size       i;
    cpl_error_code error = CPL_ERROR_NONE;


    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other != NULL, CPL_ERROR_NULL_INPUT);
    /* At least one of the two bivectors must have two distinct buffers
       (otherwise the call is utterly pointless) */
    cpl_ensure_code(cpl_bivector_get_x_data_const(self) !=
                    cpl_bivector_get_y_data_const(self) ||
                    cpl_bivector_get_x_data_const(other) !=
                    cpl_bivector_get_y_data_const(other),
                    CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code(cpl_bivector_get_size(other) == n,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    if (dir == CPL_SORT_ASCENDING) {
        reverse = 0;
    } else if (dir == CPL_SORT_DESCENDING) {
        reverse = 1;
    } else {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    /* The sorting mode. Could later be extended with f.ex.
       CPL_SORT_BY_XY and CPL_SORT_BY_YX */
    /* To allow for all possible sorting combinations, extensions to dir
       would be needed as well. This will probably not be requested */
    if (mode == CPL_SORT_BY_X) {
        data = cpl_bivector_get_x_data_const(other);
    } else if (mode == CPL_SORT_BY_Y) {
        data = cpl_bivector_get_y_data_const(other);
    } else {
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    cpl_ifalloc_set(&permbuf, (size_t)n * sizeof(*perm));
    perm = (cpl_size*)cpl_ifalloc_get(&permbuf);

    for (i = 0; i < n; i++) {
        perm[i] = i;
    }

    if (cpl_tools_sort_stable_pattern_double(data, n, reverse, 1, perm)) {
        error = cpl_error_set_where_();
    } else {
        double * dselfx = cpl_bivector_get_x_data(self);
        double * dselfy = cpl_bivector_get_y_data(self);
        const double * dotherx = cpl_bivector_get_x_data_const(other);
        const double * dothery = cpl_bivector_get_y_data_const(other);

        if (dselfx != dotherx && dselfy != dothery &&
            dselfx != dothery && dselfy != dotherx) {

            /* Out-of place copy of the permuted values to self */

            for (i = 0; i < n; i++) {
                dselfx[i] = dotherx[perm[i]];
                dselfy[i] = dothery[perm[i]];
            }

        } else if (cpl_tools_permute_double(perm, n, dselfx, dotherx,
                                            dselfy, dothery)) {
            error = cpl_error_set_where_();
        }
    }

    cpl_ifalloc_free(&permbuf);
    return error;

}
/**@}*/
