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

#include "cpl_matrix_impl.h"

#include "cpl_memory.h"
#include "cpl_math_const.h"
#include "cpl_error_impl.h"
#include "cpl_array_impl.h"

#include <string.h>
#include <math.h>
#include <assert.h>

/**
 * @defgroup cpl_matrix Matrices
 *
 * This module provides functions to create, destroy and use a @em cpl_matrix.
 * The elements of a @em cpl_matrix with M rows and N columns are counted 
 * from 0,0 to M-1,N-1. The matrix element 0,0 is the one at the upper left
 * corner of a matrix. The CPL matrix functions work properly only in the 
 * case the matrices elements do not contain garbage (such as @c NaN or 
 * infinity).
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_matrix.h>
 * @endcode
 */

/**@{*/

#ifndef inline
#define inline /* inline */
#endif

#define dtiny(x, t) ((x) < 0.0 ? (x) >= -(t) : (x) <= (t))


/*
 * The cpl_matrix type:
 */

struct _cpl_matrix_ {
    cpl_size nc;
    cpl_size nr;
    double  *m;
}; 

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

inline
static void swap_rows(cpl_matrix *, cpl_size, cpl_size) CPL_ATTR_NONNULL;
static void read_row(const cpl_matrix *, double *, cpl_size) CPL_ATTR_NONNULL;
static void write_row(cpl_matrix *, const double *, cpl_size) CPL_ATTR_NONNULL;
static void write_read_row(cpl_matrix *, double *, cpl_size) CPL_ATTR_NONNULL;
static
void read_column(const cpl_matrix *, double *, cpl_size) CPL_ATTR_NONNULL;
static
void write_column(cpl_matrix *, const double *, cpl_size) CPL_ATTR_NONNULL;
static
void write_read_column(cpl_matrix *, double *, cpl_size) CPL_ATTR_NONNULL;
static cpl_error_code cpl_matrix_set_size_(cpl_matrix *, cpl_size, cpl_size);
static cpl_error_code _cpl_matrix_decomp_sv_jacobi(cpl_matrix *U,
                                                   cpl_vector *S,
                                                   cpl_matrix *V);
/*-----------------------------------------------------------------------------
                              Function codes
 -----------------------------------------------------------------------------*/

/*
 * Private methods:
 */

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief   Set the size of a matrix while destroying its elements
   @param   self  The matrix to modify
   @param   nr    New number of matrix rows.
   @param   nc    New number of matrix columns.
   @note Any pointer returned by cpl_matrix_get_data_const() may be invalidated

*/
/*----------------------------------------------------------------------------*/
static
cpl_error_code cpl_matrix_set_size_(cpl_matrix * self, cpl_size nr, cpl_size nc)
{

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    if (self->nr != nr || self->nc != nc) {

        cpl_ensure_code(nr > 0, CPL_ERROR_ILLEGAL_INPUT);
        cpl_ensure_code(nc > 0, CPL_ERROR_ILLEGAL_INPUT);

        /* Need to resize the matrix */

        if (self->nr * self->nc != nr * nc) { 
            cpl_free(self->m);
            self->m = (double*)cpl_malloc((size_t)nr * (size_t)nc
                                          * sizeof(double));
        }
        self->nr = nr;
        self->nc = nc;
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief   Swap two (different) rows 
   @param   self  The matrix
   @param   row1  One row
   @param   row2  Another row
   @see cpl_matrix_swap_rows()
   @note No error checking done here

   Optimized due to repeated usage in cpl_matrix_decomp_lu():
       1) Removed redundant error checking
          - this allows the compiler to make 'swap' a register variable.
       2) Use a single index for loop control and adressing the two rows
          - this redues the instruction count.

   This can have an effect when the swap is not memory bound.

*/
/*----------------------------------------------------------------------------*/
inline
static void swap_rows(cpl_matrix * self, cpl_size row1, cpl_size row2)
{                                                                
#define NBLOCK 128
    double   swap[NBLOCK];
    cpl_size ncol = self->nc;
    double * pos1 = self->m + ncol * row1;
    double * pos2 = self->m + ncol * row2;

    while (ncol >= NBLOCK) {
        ncol -= NBLOCK;
        (void)memcpy(swap,        pos1 + ncol, NBLOCK * sizeof(double));
        (void)memcpy(pos1 + ncol, pos2 + ncol, NBLOCK * sizeof(double));
        (void)memcpy(pos2 + ncol, swap,        NBLOCK * sizeof(double));
    }

    if (ncol > 0) {
        (void)memcpy(swap, pos1, ncol * sizeof(double));
        (void)memcpy(pos1, pos2, ncol * sizeof(double));
        (void)memcpy(pos2, swap, ncol * sizeof(double));
    }
}


/*
 * @internal
 * @brief
 *   Just read a matrix row into the passed buffer.
 *
 * @param matrix  Matrix where to read the row from.
 * @param pos     Row number.
 * @param row     Allocated buffer.
 *
 * @return Nothing.
 *
 * This private function reads the content of a matrix row into an allocated
 * buffer. No checks are performed.
 */
static void read_row(const cpl_matrix *matrix, double *row, cpl_size pos)
{

    cpl_size i;

    for (i = 0, pos *= matrix->nc; i < matrix->nc; i++, pos++)
        row[i] = matrix->m[pos];

}

/*
 * @internal
 * @brief
 *   Just write into a matrix row the values in buffer.
 *
 * @param matrix  Matrix where to write the buffer to.
 * @param pos     Row number.
 * @param row     Allocated buffer.
 * 
 * @return Nothing.
 * 
 * This private function writes the content of a buffer into a matrix row.
 * No checks are performed.
 */
static void write_row(cpl_matrix *matrix, const double *row, cpl_size pos)
{
    
    cpl_size i;

    for (i = 0, pos *= matrix->nc; i < matrix->nc; i++, pos++)
        matrix->m[pos] = *row++;

}

/*
 * @internal
 * @brief
 *   Just swap values in buffer with values in a matrix row.
 *
 * @param matrix  Matrix to access.
 * @param pos     Row to access.
 * @param row     Allocated buffer.
 *
 * @return Nothing.
 *
 * This private function exchanges the values in buffer with the values in
 * a chosen matrix row. No checks are performed.
 */
static void write_read_row(cpl_matrix *matrix, double *row, cpl_size pos)
{

    cpl_size    i;
    double swap;

    for (i = 0, pos *= matrix->nc; i < matrix->nc; i++, pos++) {
        swap = matrix->m[pos];
        matrix->m[pos] = row[i];
        row[i] = swap;
    }

}

/*
 * @internal
 * @brief
 *   Just read a matrix column into the passed buffer.
 *
 * @param matrix  Matrix where to read the column from.
 * @param pos     Column number.
 * @param column  Allocated buffer.
 *
 * @return Nothing.
 *
 * This private function reads the content of a matrix column into an 
 * allocated buffer. No checks are performed.
 */
static void read_column(const cpl_matrix *matrix, double *column, cpl_size pos)
{

    cpl_size i;

    for (i = 0; i < matrix->nr; i++, pos += matrix->nc)
        column[i] = matrix->m[pos];

}

/*
 * @internal
 * @brief
 *   Just write into a matrix column the values in buffer.
 *
 * @param matrix  Matrix where to write the buffer to.
 * @param pos     Column number.
 * @param column  Allocated buffer.
 * 
 * @return Nothing.
 * 
 * This private function writes the content of a buffer into a matrix column.
 * No checks are performed.
 */
static void write_column(cpl_matrix *matrix, const double *column, cpl_size pos)
{

    cpl_size i;
    cpl_size size = matrix->nr * matrix->nc;


    for (i = pos; i < size; i += matrix->nc)
        matrix->m[i] = *column++;

}

/*
 * @internal
 * @brief
 *   Just swap values in buffer with values in a matrix column.
 *
 * @param matrix  Matrix to access.
 * @param pos     Column to access.
 * @param column  Allocated buffer.
 *
 * @return Nothing.
 *
 * This private function exchanges the values in buffer with the values in
 * a chosen matrix column. No checks are performed.
 */
static void write_read_column(cpl_matrix *matrix, double *column, cpl_size pos)
{
 
    cpl_size    i;
    double swap;

    for (i = 0; i < matrix->nr; i++, pos += matrix->nc) {
        swap = matrix->m[pos];
        matrix->m[pos] = column[i];
        column[i] = swap;
    }

}

/*
 * Public methods:
 */

/**
 * @brief
 *   Print a matrix.
 *
 * @param matrix     The matrix to print
 * @param stream     The output stream
 *
 * @return Nothing.
 *
 * This function is intended just for debugging. It just prints the
 * elements of a matrix, ordered in rows and columns, to the specified
 * stream or FILE pointer. If the specified stream is NULL, it is set
 * to @em stdout. The function used for printing is the standard C 
 * @c fprintf().
 */

void cpl_matrix_dump(const cpl_matrix *matrix, FILE *stream)
{
    cpl_size i, j, k;


    if (stream == NULL)
        stream = stdout;

    fprintf(stream, " ");

    if (matrix == NULL) {
        fprintf(stream, "NULL matrix\n\n");
        return;
    }

    fprintf(stream, "   ");
    for (j = 0; j < matrix->nc; j++)
        fprintf(stream, " %6" CPL_SIZE_FORMAT, j);

    fprintf(stream, "\n");

    for (k = 0, i = 0; i < matrix->nr; i++) {
        fprintf(stream, "  %" CPL_SIZE_FORMAT " ", i);
        for (j = 0; j < matrix->nc; j++, k++)
            fprintf(stream, " %6g", matrix->m[k]);

        fprintf(stream, "\n");
    }
    fprintf(stream, "\n");
}

/**
 * @brief
 *   Create a zero matrix of given size.
 *
 * @param rows     Number of matrix rows.
 * @param columns  Number of matrix columns.
 *
 * @return Pointer to new matrix, or @c NULL in case of error.
 * 
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>rows</i> or <i>columns</i> are not positive numbers.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function allocates and initialises to zero a matrix of given size.
 * To destroy this matrix the function @c cpl_matrix_delete() should be used.
 */

cpl_matrix *cpl_matrix_new(cpl_size rows, cpl_size columns)
{


    cpl_matrix  *matrix;


    if (rows < 1) {
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    if (columns < 1) {
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    matrix = (cpl_matrix *)cpl_malloc(sizeof(cpl_matrix));

    matrix->m = (double *)cpl_calloc((size_t)rows * (size_t)columns,
                                     sizeof(double));
    matrix->nr = rows;
    matrix->nc = columns;

    return matrix;

}


/**
 * @brief
 *   Create a new matrix from existing data.
 *
 * @param data     Existing data buffer.
 * @param rows     Number of matrix rows.
 * @param columns  Number of matrix columns.
 *
 * @return Pointer to new matrix, or @c NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>data</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>rows</i> or <i>columns</i> are not positive numbers.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new matrix that will encapsulate the given 
 * data. At any error condition, a @c NULL pointer would be returned.
 * Note that the size of the input data array is not checked in any way, 
 * and it is expected to match the specified matrix sizes. The input 
 * array is supposed to contain in sequence all the new @em cpl_matrix 
 * rows. For instance, in the case of a 3x4 matrix, the input array 
 * should contain 12 elements 
 * @code
 *            0 1 2 3 4 5 6 7 8 9 10 11
 * @endcode
 * that would correspond to the matrix elements
 * @code
 *            0  1  2  3
 *            4  5  6  7
 *            8  9 10 11 
 * @endcode 
 * The data buffer is not copied, so it should not be deallocated while 
 * the matrix is still in use: the function @c cpl_matrix_delete() would 
 * take care of deallocating it. To avoid problems with the memory managment, 
 * the specified data buffer should be allocated using the functions of 
 * the @c cpl_memory module, and also statically allocated data should be 
 * strictly avoided. If this were not the case, this matrix should not be 
 * destroyed using @c cpl_matrix_delete(), but @c cpl_matrix_unwrap() 
 * should be used instead; moreover, functions implying memory handling 
 * (as @c cpl_matrix_set_size(), or @c cpl_matrix_delete_row() ) should not 
 * be used.
 */

cpl_matrix *cpl_matrix_wrap(cpl_size rows, cpl_size columns, double *data)
{


    cpl_matrix  *matrix;


    if (rows < 1 || columns < 1) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    matrix = (cpl_matrix *)cpl_malloc(sizeof(cpl_matrix));

    matrix->m = data;
    matrix->nr = rows;
    matrix->nc = columns;

    return matrix;

}


/**
 * @brief
 *   Delete a matrix.
 *
 * @param matrix  Pointer to a matrix to be deleted.
 *
 * @return Nothing.
 *
 * This function frees all the memory associated to a matrix. 
 * If @em matrix is @c NULL, nothing is done.
 */

void cpl_matrix_delete(cpl_matrix *matrix)
{

    if (matrix) {
        cpl_free(matrix->m);
        cpl_free(matrix);
    }

}


/**
 * @brief
 *   Delete a matrix, but not its data buffer.
 *
 * @param matrix  Pointer to a matrix to be deleted.
 *
 * @return Pointer to the internal data buffer.
 *
 * This function deallocates all the memory associated to a matrix, 
 * with the exception of its data buffer. This type of destructor
 * should be used on matrices created with the @c cpl_matrix_wrap() 
 * constructor, if the data buffer specified then was not allocated 
 * using the functions of the @c cpl_memory module. In such a case, the 
 * data buffer should be deallocated separately. See the documentation
 * of the function @c cpl_matrix_wrap(). If @em matrix is @c NULL, 
 * nothing is done, and a @c NULL pointer is returned.
 */

void *cpl_matrix_unwrap(cpl_matrix *matrix)
{

    void *data = NULL;

    if (matrix) {
        data = matrix->m;
        cpl_free(matrix);
    }

    return data;

}


/**
 * @brief
 *   Get the number of rows of a matrix.
 *
 * @param matrix  Pointer to the matrix to examine.
 *
 * @return Number of matrix rows, or zero in case of failure.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Determine the number of rows in a matrix.
 */

inline
cpl_size cpl_matrix_get_nrow(const cpl_matrix *matrix)
{

    if (matrix == NULL) {
        /* inline precludes using cpl_error_set_() due to its use of __func__ */
        (void) cpl_error_set("cpl_matrix_get_nrow", CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return matrix->nr;

}


/**
 * @brief
 *   Get the number of columns of a matrix.
 *
 * @param matrix  Pointer to the matrix to examine.
 *
 * @return Number of matrix columns, or zero in case of failure.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Determine the number of columns in a matrix.
 */

inline
cpl_size cpl_matrix_get_ncol(const cpl_matrix *matrix)
{

    if (matrix == NULL) {
        /* inline precludes using cpl_error_set_() due to its use of __func__ */
        (void) cpl_error_set("cpl_matrix_get_ncol", CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return matrix->nc;

}


/**
 * @brief
 *   Get the pointer to a matrix data buffer, or @c NULL in case of error.
 *
 * @param matrix  Input matrix.
 *
 * @return Pointer to the matrix data buffer.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_matrix object includes an array of values of type @em double.
 * This function returns a pointer to this internal array, whose first 
 * element corresponds to the @em cpl_matrix element 0,0. The internal
 * array contains in sequence all the @em cpl_matrix rows. For instance,
 * in the case of a 3x4 matrix, the array elements
 * @code
 *            0 1 2 3 4 5 6 7 8 9 10 11
 * @endcode
 * would correspond to the following matrix elements:
 * @code
 *            0  1  2  3
 *            4  5  6  7
 *            8  9 10 11
 * @endcode
 *
 * @note
 *   Use at your own risk: direct manipulation of matrix data rules out 
 *   any check performed by the matrix object interface, and may introduce  
 *   inconsistencies between the information maintained internally, and 
 *   the actual matrix data and structure.
 */

inline
double *cpl_matrix_get_data(cpl_matrix *matrix)
{

    if (matrix == NULL) {
        /* inline precludes using cpl_error_set_() due to its use of __func__ */
        (void) cpl_error_set("cpl_matrix_get_data", CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return matrix->m;

}


/**
 * @brief
 *   Get the pointer to a matrix data buffer, or @c NULL in case of error.
 *
 * @param matrix  Input matrix.
 *
 * @return Pointer to the matrix data buffer.
 *
 * @see cpl_matrix_get_data
 */

inline
const double *cpl_matrix_get_data_const(const cpl_matrix *matrix)
{
    if (matrix == NULL) {
        /* inline precludes using cpl_error_set_() due to its use of __func__ */
        (void) cpl_error_set("cpl_matrix_get_data_const", CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return matrix->m;
}


/**
 * @brief
 *   Get the value of a matrix element.
 *
 * @param matrix  Pointer to a matrix.
 * @param row     Matrix element row.
 * @param column  Matrix element column.
 *
 * @return Value of the specified matrix element, or 0.0 in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The accessed element is beyond the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the value of a matrix element. The matrix rows and columns are 
 * counted from 0,0.
 */

double cpl_matrix_get(const cpl_matrix *matrix, cpl_size row, cpl_size column)
{




    if (matrix == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    } 

    if (row < 0 || row >= matrix->nr || column < 0 || column >= matrix->nc) {
        (void)cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return 0.0;
    }

    return matrix->m[column + row * matrix->nc];

}


/**
 * @brief
 *   Write a value to a matrix element.
 *
 * @param matrix  Input matrix.
 * @param row     Matrix element row.
 * @param column  Matrix element column.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The accessed element is beyond the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a matrix element. The matrix rows and columns are 
 * counted from 0,0.
 */

inline
cpl_error_code cpl_matrix_set(cpl_matrix *matrix, 
                              cpl_size row, cpl_size column, double value)
{




    if (matrix == NULL)
        return cpl_error_set("cpl_matrix_set", CPL_ERROR_NULL_INPUT);

    if (row < 0 || row >= matrix->nr || column < 0 || column >= matrix->nc)
        return cpl_error_set("cpl_matrix_set", CPL_ERROR_ACCESS_OUT_OF_RANGE);

    matrix->m[column + row * matrix->nc] = value;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Make a copy of a matrix.
 *
 * @param matrix  Matrix to be duplicated.
 *
 * @return Pointer to the new matrix, or @c NULL in case of error.
 * 
 * @error
 *   <table class="ec" align="center">
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * A copy of the input matrix is created. To destroy the duplicated matrix 
 * the function @c cpl_matrix_delete() should be used.
 */

cpl_matrix *cpl_matrix_duplicate(const cpl_matrix *matrix)
{


    cpl_matrix  *new_matrix = NULL;

    if (matrix) {
        const cpl_size size = matrix->nr * matrix->nc;

        new_matrix = (cpl_matrix *)cpl_malloc(sizeof(cpl_matrix));
        new_matrix->nr = matrix->nr;
        new_matrix->nc = matrix->nc;
        new_matrix->m = (double *)cpl_malloc((size_t)size * sizeof(double));

        (void)cpl_matrix_copy(new_matrix, matrix, 0, 0);
    }
    else
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return new_matrix;

}


/**
 * @brief
 *   Extract a submatrix from a matrix.
 *
 * @param matrix        Pointer to the input matrix.
 * @param start_row     Matrix row where to begin extraction.
 * @param start_column  Matrix column where to begin extraction.
 * @param step_row      Step between extracted rows.
 * @param step_column   Step between extracted columns.
 * @param nrows         Number of rows to extract.
 * @param ncolumns      Number of columns to extract.
 *
 * @return Pointer to the new matrix, or @c NULL in case of error.
 * 
 * @error
 *   <table class="ec" align="center">
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The start position is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         While <i>nrows</i> and <i>ncolumns</i> are greater than 1, 
 *         the specified steps are not positive.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The new matrix will include the @em nrows x @em ncolumns values read 
 * from the input matrix elements starting from position (@em start_row, 
 * @em start_column), in a grid of steps @em step_row and @em step_column. 
 * If the extraction parameters exceed the input matrix boundaries, just 
 * the overlap is returned, and this matrix would have sizes smaller than 
 * @em nrows x @em ncolumns. To destroy the new matrix the function 
 * @c cpl_matrix_delete() should be used.
 */

cpl_matrix *cpl_matrix_extract(const cpl_matrix *matrix, 
                               cpl_size start_row, cpl_size start_column,
                               cpl_size step_row, cpl_size step_column,
                               cpl_size nrows, cpl_size ncolumns)
{


    cpl_matrix  *new_matrix = NULL;
    cpl_size          i, r, c;
    cpl_size          size;
    cpl_size          row, col;


    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (start_row < 0 || start_row >= matrix->nr || 
        start_column < 0 || start_column >= matrix->nc ||
        nrows < 1 || ncolumns < 1) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return NULL;
    }

    if (nrows == 1)          /* Step irrelevant... */
        step_row = 1;

    if (ncolumns == 1)       /* Step irrelevant... */
        step_column = 1;

    if (step_row < 1 || step_column < 1) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    if ((start_row + (nrows - 1) * step_row) >= matrix->nr)
        nrows = (matrix->nr - start_row - 1) / step_row + 1;

    if ((start_column + (ncolumns - 1) * step_column) >= matrix->nc)
        ncolumns = (matrix->nc - start_column - 1) / step_column + 1;

    size = nrows * ncolumns;

    new_matrix     = (cpl_matrix *)cpl_malloc(sizeof(cpl_matrix));
    new_matrix->m  = (double *)cpl_malloc((size_t)size * sizeof(double));
    new_matrix->nr = nrows;
    new_matrix->nc = ncolumns;

    for (i = 0, r = start_row, row = nrows; row--; r += step_row)
        for (c = r * matrix->nc + start_column, col = ncolumns; col--; 
                                                      c += step_column, i++)
            new_matrix->m[i] = matrix->m[c];

    return new_matrix;

}


/**
 * @brief
 *   Extract a matrix row.
 *
 * @param matrix Pointer to the matrix containing the row.
 * @param row    Sequence number of row to copy.
 *
 * @return Pointer to new matrix, or @c NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The <i>row</i> is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * If a MxN matrix is given in input, the extracted row is a new 1xN matrix. 
 * The row number is counted from 0. To destroy the new matrix the function 
 * @c cpl_matrix_delete() should be used.
 */

cpl_matrix *cpl_matrix_extract_row(const cpl_matrix *matrix, cpl_size row)
{

    cpl_matrix *vector;

    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    vector = cpl_matrix_extract(matrix, row, 0, 1, 1, 1, matrix->nc);

    if (vector == NULL)
        cpl_error_set_where("cpl_matrix_extract_row");

    return vector;

}


/**
 * @brief
 *   Copy a matrix column.
 *
 * @param matrix Pointer to matrix containing the column.
 * @param column Sequence number of column to copy.
 *
 * @return Pointer to new matrix, or @c NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The <i>column</i> is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * If a MxN matrix is given in input, the extracted row is a new Mx1 matrix.
 * The column number is counted from 0. To destroy the new matrix the 
 * function @c cpl_matrix_delete() should be used.
 */

cpl_matrix *cpl_matrix_extract_column(const cpl_matrix *matrix, cpl_size column)
{

    cpl_matrix *vector;

    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    vector = cpl_matrix_extract(matrix, 0, column, 1, 1, matrix->nr, 1);

    if (vector == NULL) 
        cpl_error_set_where("cpl_matrix_extract_column");

    return vector;

}


/**
 * @brief
 *   Extract a matrix diagonal.
 *
 * @param matrix   Pointer to the matrix containing the diagonal.
 * @param diagonal Sequence number of the diagonal to copy.
 *
 * @return Pointer to the new matrix, or @c NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The <i>diagonal</i> is outside the <i>matrix</i> boundaries
 *         (see description below).
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * If a MxN matrix is given in input, the extracted diagonal is a Mx1 
 * matrix if N >= M, or a 1xN matrix if N < M. The diagonal number is 
 * counted from 0, corresponding to the matrix diagonal starting at 
 * element (0,0). A square matrix has just one diagonal; if M != N, 
 * the number of diagonals in the matrix is |M - N| + 1. To specify 
 * a @em diagonal sequence number outside this range would set an
 * error condition, and a @c NULL pointer would be returned. To destroy 
 * the new matrix the function @c cpl_matrix_delete() should be used.
 */

cpl_matrix *cpl_matrix_extract_diagonal(const cpl_matrix *matrix, cpl_size diagonal)
{


    cpl_matrix  *output;
    cpl_size          diagonal_count;
    cpl_size          diagonal_length;
    cpl_size          i, r, c;
    double      *m;
    double      *om;


    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    diagonal_count = labs(matrix->nr - matrix->nc) + 1;

    if (diagonal < 0 || diagonal >= diagonal_count) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return NULL;
    }

    diagonal_length = matrix->nr < matrix->nc ? matrix->nr : matrix->nc;

    output = (cpl_matrix *)cpl_malloc(sizeof(cpl_matrix));
    om = output->m = (double *)cpl_malloc((size_t)diagonal_length
                                          * sizeof(*om));

    if (matrix->nc < matrix->nr) {
        r = diagonal;
        c = 0;
        output->nr = 1;
        output->nc = diagonal_length;
    }
    else {
        r = 0;
        c = diagonal;
        output->nr = diagonal_length;
        output->nc = 1;
    }

    m = matrix->m + c + r * matrix->nc;
    for (i = 0; i < diagonal_length; i++, m += matrix->nc + 1)
        *om++ = *m;

    return output;

}


/**
 * @brief
 *   Write the same value to all matrix elements.
 *
 * @param matrix   Pointer to the matrix to access
 * @param value    Value to write
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to all matrix elements.
 */

cpl_error_code cpl_matrix_fill(cpl_matrix *matrix, double value)
{
    if (matrix == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (value != 0.0) {
        cpl_size     size = matrix->nr * matrix->nc;
        double      *m = matrix->m;

        while (size--)
            *m++ = value;

    } else {
        (void)memset(matrix->m, 0,
                     (size_t)(matrix->nr * matrix->nc) * sizeof(*matrix->m));
    }

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write the same value to a matrix row.
 *
 * @param matrix Matrix to access
 * @param value  Value to write
 * @param row    Sequence number of row to overwrite.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified <i>row</i> is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a matrix row. Rows are counted starting from 0. 
 */

cpl_error_code cpl_matrix_fill_row(cpl_matrix *matrix, double value,
                                   cpl_size row)
{


    double      *m;
    cpl_size     count;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (row < 0 || row >= matrix->nr)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    m = matrix->m + row * matrix->nc;
    count = matrix->nc;

    while (count--)
        *m++ = value;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value to a matrix column.
 *
 * @param matrix Pointer to the matrix to access
 * @param value  Value to write
 * @param column Sequence number of column to overwrite
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified <i>column</i> is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a matrix column. Columns are counted starting 
 * from 0.
 */

cpl_error_code cpl_matrix_fill_column(cpl_matrix *matrix, 
                                      double value, cpl_size column)
{


    cpl_size     count;
    double      *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (column < 0 || column >= matrix->nc)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    count = matrix->nr;
    m = matrix->m + column;

    while (count--) {
        *m = value;
        m += matrix->nc;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a given value to all elements of a given matrix diagonal.
 *
 * @param matrix   Matrix to modify
 * @param value    Value to write to diagonal
 * @param diagonal Number of diagonal to overwrite, 0 for main, positive for
 *                 above main, negative for below main
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified <i>diagonal</i> is outside the <i>matrix</i> 
 *         boundaries (see description below).
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * 
 */

cpl_error_code cpl_matrix_fill_diagonal(cpl_matrix *matrix, 
                                        double value, cpl_size diagonal)
{
    cpl_size ij, nm;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (diagonal > matrix->nc)
        return cpl_error_set_message_(CPL_ERROR_ACCESS_OUT_OF_RANGE, "d=%"
                                      CPL_SIZE_FORMAT " > c=%" CPL_SIZE_FORMAT,
                                      diagonal, matrix->nc);

    if (-diagonal > matrix->nr)
        return cpl_error_set_message_(CPL_ERROR_ACCESS_OUT_OF_RANGE, "-d=%"
                                      CPL_SIZE_FORMAT " > c=%" CPL_SIZE_FORMAT,
                                      -diagonal, matrix->nc);

    nm = diagonal >= 0 ? (matrix->nr - diagonal) * matrix->nc
        : matrix->nr * matrix->nc;

    for (ij = diagonal >= 0 ? diagonal : -diagonal * matrix->nc;
         ij < nm; ij += matrix->nc + 1) {
        matrix->m[ij] = value;
    }

    return CPL_ERROR_NONE;

}


/**
 * @internal
 * @brief
 *   Fill a square matrix with the identity matrix
 *
 * @param self   Matrix to modify
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The number of rows and columns of the matrix are different.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * 
 */

cpl_error_code cpl_matrix_fill_identity(cpl_matrix *self)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (self->nr != self->nc) {
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Number of rows "
                                      "%d != %d number of columns",
                                      (int)self->nr, (int)self->nc);
    } else {
        double * p = self->m;

        *p = 1.0;
        for (cpl_size i = 0; i < self->nc - 1; i++) {
            p++;
            (void)memset(p, 0, sizeof(*p) * self->nc);
            p += self->nc;
            *p = 1.0;
        }
        assert(p + 1 == self->m + self->nc * self->nc);
    }

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write the values of a matrix into another matrix.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param submatrix Pointer to matrix to get the values from.
 * @param row       Position of row 0 of @em submatrix in @em matrix.
 * @param col       Position of column 0 of @em submatrix in @em matrix.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> or <i>submatrix</i> are <tt>NULL</tt> 
 *         pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         No overlap exists between the two matrices.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The values of @em submatrix are written to @em matrix starting at the 
 * indicated row and column. There are no restrictions on the sizes of 
 * @em submatrix: just the parts of @em submatrix overlapping @em matrix 
 * are copied. There are no restrictions on @em row and @em col either, 
 * that can also be negative. If the two matrices do not overlap, nothing 
 * is done, but an error condition is set.
 */

cpl_error_code cpl_matrix_copy(cpl_matrix *matrix, const cpl_matrix *submatrix,
                               cpl_size row, cpl_size col)
{


    cpl_size     endrow;
    cpl_size     endcol;
    cpl_size     subrow;
    cpl_size     subcol;
    cpl_size     r, c, sr, sc;
    double      *m;
    double      *sm;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    if (submatrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (row == 0 && col == 0 &&
        submatrix->nr == matrix->nr &&
        submatrix->nc == matrix->nc) {
        /* FIXME: Use in more cases */
        return
            cpl_tools_copy_window(matrix->m, submatrix->m, sizeof(*matrix->m),
                                  matrix->nc, matrix->nr, 1, 1,
                                  matrix->nc, matrix->nr)
            ? cpl_error_set_where_() : CPL_ERROR_NONE;
    }

    endrow = row + submatrix->nr;
    endcol = col + submatrix->nc;


   /*
    * Check whether matrices overlap.
    */

    if (row >= matrix->nr || endrow < 1 || col >= matrix->nc || endcol < 1)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);


   /*
    * Define the overlap region on both matrices reference system:
    */

    if (row < 0) {
        subrow = -row;
        row = 0;
    }
    else
        subrow = 0;

    if (col < 0) {
        subcol = -col;
        col = 0;
    }
    else
        subcol = 0;

    if (endrow > matrix->nr)
        endrow = matrix->nr;

    if (endcol > matrix->nc)
        endcol = matrix->nc;

    for (r = row, sr = subrow; r < endrow; r++, sr++) {
        m = matrix->m + r * matrix->nc + col;
        sm = submatrix->m + sr * submatrix->nc + subcol;
        for (c = col, sc = subcol; c < endcol; c++, sc++)
            *m++ = *sm++;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value into a submatrix of a matrix.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param value     Value to write.
 * @param row       Start row of matrix submatrix.
 * @param col       Start column of matrix submatrix.
 * @param nrow      Number of rows of matrix submatrix.
 * @param ncol      Number of columns of matrix submatrix.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified start position is outside the <i>matrix</i> 
 *         boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>nrow</i> or <i>ncol</i> are not positive.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The specified value is written to @em matrix starting at the indicated 
 * row and column; @em nrow and @em ncol can exceed the input matrix 
 * boundaries, just the range overlapping the @em matrix is used in that
 * case.
 */

cpl_error_code cpl_matrix_fill_window(cpl_matrix *matrix, double value,
                                      cpl_size row, cpl_size col,
                                      cpl_size nrow, cpl_size ncol)
{


    cpl_size     endrow;
    cpl_size     endcol;
    cpl_size     r, c;
    double      *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (row < 0 || row >= matrix->nr || col < 0 || col >= matrix->nc)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (nrow < 1 || ncol < 1)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    endrow = row + nrow;

    if (endrow > matrix->nr)
        endrow = matrix->nr;

    endcol = col + ncol;

    if (endcol > matrix->nc)
        endcol = matrix->nc;

    for (r = row; r < endrow; r++) {
        m = matrix->m + r * matrix->nc + col;
        for (c = col; c < endcol; c++)
            *m++ = value;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Shift matrix elements.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param rshift    Shift in the vertical direction.
 * @param cshift    Shift in the horizontal direction.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr> 
 *   </table>
 * @enderror
 *
 * The performed shift operation is cyclical (toroidal), i.e., matrix 
 * elements shifted out of one side of the matrix get shifted in from 
 * its opposite side. There are no restrictions on the values of the 
 * shift. Positive shifts are always in the direction of increasing 
 * row/column indexes.
 *
 */

cpl_error_code cpl_matrix_shift(cpl_matrix *matrix, cpl_size rshift, cpl_size cshift)
{
    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    /* Should not be able to fail now */

    rshift = rshift % matrix->nr;
    cshift = cshift % matrix->nc;

    /* The naive approach is to duplicate the entire buffer, and
        then write each element into its proper place. This would
        require n * m extra storage and 2 * n * m read+write operations.
        Instead:
        1) Copy up to half the elements into a temporary buffer.
        2) Move - via memmove() - the remaining buffer into place
        3) For a non-zero column shift, shift the smaller part of each
           row by one row
        4) Write the copied buffer into place
        In the worst case, 3/4 of the elements are still read/written twice
        with 1/4 read/written once, while in the best (non-trivial) case
        all but one row or column is read/written once.

        Both rshift and cshift can be positive, zero or negative, for a
        total of 8 non-trivial cases.
    */

    /* Maximize size of memmove() */
    if (2 * rshift > matrix->nr)
        rshift -= matrix->nr;
    else if (2 * rshift < -matrix->nr)
        rshift += matrix->nr;

    if (2 * cshift > matrix->nc)
        cshift -= matrix->nc;
    else if (2 * cshift < -matrix->nc)
        cshift += matrix->nc;

    if ((rshift > 0 && cshift == 0) || (rshift == 0 && cshift > 0)) {
        const cpl_size szcp = cshift + rshift * matrix->nc;
        void * copy = cpl_malloc((size_t)szcp * sizeof(double));
        double * drow = matrix->m;

        memcpy(copy, matrix->m + (size_t)(matrix->nr * matrix->nc - szcp),
               (size_t)szcp * sizeof(double));
        memmove(matrix->m + (size_t)szcp, matrix->m,
                (size_t)(matrix->nr * matrix->nc - szcp)
                * sizeof(double));
        if (cshift > 0) {
            cpl_size i;
            for (i = 0; i < matrix->nr - 1; i++, drow += matrix->nc) {
                memcpy(drow, drow + (size_t)matrix->nc, (size_t)szcp * sizeof(double));
            }
        }
        memcpy(drow, copy, (size_t)szcp * sizeof(double));
        cpl_free(copy);

    } else if ((rshift < 0 && cshift == 0) || (rshift == 0 && cshift < 0)) {
        const cpl_size szcp = -cshift - rshift * matrix->nc;
        void * copy = cpl_malloc((size_t)szcp * sizeof(double));
        double * drow = matrix->m + (size_t)(matrix->nr * matrix->nc - szcp);

        memcpy(copy, matrix->m, (size_t)szcp * sizeof(double));
        memmove(matrix->m, matrix->m + (size_t)szcp,
                (size_t)(matrix->nr * matrix->nc - szcp)
                * sizeof(double));
        if (cshift < 0) {
            cpl_size i;
            for (i = matrix->nr - 1; i > 0; i--, drow -= matrix->nc) {
                memcpy(drow, drow - (size_t)matrix->nc, (size_t)szcp * sizeof(double));
            }
        }
        memcpy(drow, copy, (size_t)szcp * sizeof(double));
        cpl_free(copy);
    } else if (rshift > 0 && cshift > 0) {
        const cpl_size szcp = rshift * matrix->nc + cshift;
        double * copy = cpl_malloc((size_t)szcp * sizeof(double));
        double * drow = matrix->m;
        const double * crow = copy;
        cpl_size i;

        memcpy(copy, matrix->m + (size_t)(matrix->nr * matrix->nc - szcp),
               (size_t)szcp * sizeof(double));
        memmove(matrix->m + (size_t)szcp, matrix->m,
                (size_t)(matrix->nr * matrix->nc - szcp)
                * sizeof(double));
        for (i = 0; i < rshift; i++, drow += matrix->nc, crow += matrix->nc) {
            memcpy(drow, crow + (size_t)matrix->nc,
                   (size_t)cshift * sizeof(double));
            memcpy(drow + (size_t)cshift, crow + (size_t)cshift,
                   (size_t)(matrix->nc - cshift) * sizeof(double));
        }
        for (; i < matrix->nr - 1; i++, drow += matrix->nc) {
            memcpy(drow, drow + (size_t)matrix->nc,
                   (size_t)cshift * sizeof(double));
        }
        memcpy(drow, copy, (size_t)cshift * sizeof(double));
        cpl_free(copy);
    } else if (rshift < 0 && cshift < 0) {
        const cpl_size szcp = -rshift * matrix->nc - cshift;
        double * copy = cpl_malloc((size_t)szcp * sizeof(double));
        double * drow = matrix->m + (size_t)(matrix->nr * matrix->nc + cshift);
        const double * crow = copy + szcp - matrix->nc + cshift;
        cpl_size i;

        memcpy(copy, matrix->m, (size_t)szcp * sizeof(double));
        memmove(matrix->m, matrix->m + (size_t)szcp,
                (size_t)(matrix->nr * matrix->nc - szcp)
                * sizeof(double));

        for (i = matrix->nr; i > matrix->nr + rshift; i--, drow -= matrix->nc,
                 crow -= matrix->nc) {
            memcpy(drow - (size_t)(matrix->nc + cshift), crow - (size_t)cshift,
                   (size_t)(matrix->nc + cshift) * sizeof(double));
            memcpy(drow, crow, (size_t)-cshift * sizeof(double));
        }
        for (; i > 1; i--, drow -= matrix->nc) {
            memcpy(drow, drow - (size_t)matrix->nc,
                   (size_t)-cshift * sizeof(double));
        }
        memcpy(drow, copy + (size_t)(szcp + cshift),
               (size_t)-cshift * sizeof(double));
        cpl_free(copy);
    } else if (rshift > 0 && cshift < 0) {
        const cpl_size szcp = rshift * matrix->nc;
        double * copy = cpl_malloc((size_t)szcp * sizeof(double));
        double * drow = matrix->m + (size_t)(matrix->nr * matrix->nc + cshift);
        const double * crow = copy + szcp - matrix->nc;
        cpl_size i;

        memcpy(copy, matrix->m + (size_t)(matrix->nr * matrix->nc - szcp),
               (size_t)szcp * sizeof(double));
        memmove(matrix->m + (size_t)szcp + (size_t)cshift, matrix->m,
                (size_t)(matrix->nr * matrix->nc - szcp)
                * sizeof(double));

        for (i = matrix->nr; i > rshift; i--, drow -= matrix->nc) {
            memcpy(drow, drow - (size_t)matrix->nc,
                   (size_t)-cshift * sizeof(double));
        }

        for (; i > 0; i--, drow -= matrix->nc, crow -= matrix->nc) {
            memcpy(drow, crow, (size_t)-cshift * sizeof(double));
            memcpy(drow - (size_t)(matrix->nc + cshift),
                   crow + (size_t)(-cshift),
                   (size_t)(matrix->nc + cshift) * sizeof(double));
        }
        cpl_free(copy);
    } else if (rshift < 0 && cshift > 0) {
        const cpl_size szcp = -rshift * matrix->nc;
        double * copy = cpl_malloc((size_t)szcp * sizeof(double));
        double * drow = matrix->m;
        const double * crow = copy;
        cpl_size i;

        memcpy(copy, matrix->m, (size_t)szcp * sizeof(double));
        memmove(matrix->m + (size_t)cshift, matrix->m + (size_t)szcp,
                (size_t)(matrix->nr * matrix->nc - szcp) * sizeof(double));

        for (i = 0; i < matrix->nr + rshift; i++, drow += matrix->nc) {
            memcpy(drow, drow + (size_t)matrix->nc,
                   (size_t)cshift * sizeof(double));
        }

        for (; i < matrix->nr; i++, drow += matrix->nc, crow += matrix->nc) {
            memcpy(drow, crow + (size_t)(matrix->nc - cshift),
                   (size_t)cshift * sizeof(double));
            memcpy(drow + (size_t)cshift, crow,
                   (size_t)(matrix->nc - cshift) * sizeof(double));
        }
        cpl_free(copy);
    }

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Rounding to zero very small numbers in matrix.
 * 
 * @param matrix    Pointer to matrix to be chopped.
 * @param tolerance Max tolerated rounding to zero.
 * 
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr> 
 *   </table>
 * @enderror
 * 
 * After specific manipulations of a matrix some of its elements 
 * may theoretically be expected to be zero (for instance, as a result 
 * of multiplying a matrix by its inverse). However, because of
 * numerical noise, such elements may turn out not to be exactly 
 * zero. With this function any very small number in the matrix is
 * turned to exactly zero. If the @em tolerance is zero or negative,
 * a default threshold of @c DBL_EPSILON is used.
 */

cpl_error_code cpl_matrix_threshold_small(cpl_matrix *matrix, double tolerance)
{


    cpl_size     size;
    double      *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    size = matrix->nr * matrix->nc;
    m = matrix->m;

    if (tolerance <= 0.0)
        tolerance = DBL_EPSILON;

    while (size--) {
        if (dtiny(*m, tolerance))
            *m = 0.0;
        m++;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Check for zero matrix.
 *
 * @param matrix    Pointer to matrix to be checked.
 * @param tolerance Max tolerated rounding to zero.
 *
 * @return 1 in case of zero matrix, 0 otherwise. If a @c NULL pointer 
 *   is passed, -1 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * After specific manipulations of a matrix some of its elements
 * may theoretically be expected to be zero. However, because of
 * numerical noise, such elements may turn out not to be exactly
 * zero. In this specific case, if any of the matrix element is
 * not exactly zero, the matrix would not be classified as a null
 * matrix. A threshold may be specified to consider zero any number
 * that is close enough to zero. If the specified @em tolerance is
 * negative, a default of @c DBL_EPSILON is used. A zero tolerance
 * may also be specified. 
 */


int cpl_matrix_is_zero(const cpl_matrix *matrix, double tolerance)
{


    cpl_size     size;
    double      *m;


    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }


    size = matrix->nr * matrix->nc;
    m = matrix->m;

    if (tolerance < 0.0)
        tolerance = DBL_EPSILON;

    while (size--) {
        if (!dtiny(*m, tolerance))
            return 0;
        m++;
    }

    return 1;

}


/**
 * @brief
 *   Check if a matrix is diagonal.
 *
 * @param matrix    Pointer to matrix to be checked.
 * @param tolerance Max tolerated rounding to zero.
 *
 * @return 1 in case of diagonal matrix, 0 otherwise. If a @c NULL pointer
 *   is passed, or the input matrix is not square, -1 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * A threshold may be specified to consider zero any number that 
 * is close enough to zero. If the specified @em tolerance is
 * negative, a default of @c DBL_EPSILON is used. A zero tolerance
 * may also be specified. No error is set if the input matrix is 
 * not square.
 */

int cpl_matrix_is_diagonal(const cpl_matrix *matrix, double tolerance)
{


    cpl_size     size;
    cpl_size     dist;


    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }


    if (matrix->nr != matrix->nc)
        return 0;

    size = matrix->nr * matrix->nr;
    dist = matrix->nr + 1;

    if (tolerance < 0.0)
        tolerance = DBL_EPSILON;

    while (size--)
        if (size % dist)
            if (!dtiny(matrix->m[size], tolerance))
                return 0;

    return 1;

}


/**
 * @brief
 *   Check for identity matrix.
 *
 * @param matrix    Pointer to matrix to be checked.
 * @param tolerance Max tolerated rounding to zero, or to one.
 *
 * @return 1 in case of identity matrix, 0 otherwise. If a @c NULL pointer
 *   is passed, or the input matrix is not square, -1 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * A threshold may be specified to consider zero any number that is 
 * close enough to zero, and 1 any number that is close enough to 1. 
 * If the specified @em tolerance is negative, a default of @c DBL_EPSILON 
 * is used. A zero tolerance may also be specified. No error is set if the 
 * input matrix is not square.
 */

int cpl_matrix_is_identity(const cpl_matrix *matrix, double tolerance)
{


    double       tiny;
    cpl_size     i;
    cpl_size     skip;


    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }


    if (matrix->nr != matrix->nc)
        return 0;

    if (!cpl_matrix_is_diagonal(matrix, tolerance))
        return 0;

    if (tolerance < 0.0)
        tolerance = DBL_EPSILON;

    skip = matrix->nc + 1;

    for (i = 0; i < matrix->nr; i += skip) {
        tiny = matrix->m[i] - 1.0;
        if (!dtiny(tiny, tolerance))
            return 0;
    }

    return 1;

}


/**
 * @brief
 *   Swap two matrix rows.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param row1      One matrix row.
 * @param row2      Another matrix row.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         Any of the specified rows is outside the <i>matrix</i> boundaries.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * The values of two given matrix rows are exchanged. Rows are counted 
 * starting from 0. If the same row number is given twice, nothing is 
 * done and no error is set.
 */

cpl_error_code cpl_matrix_swap_rows(cpl_matrix *matrix, cpl_size row1, cpl_size row2)
{

    cpl_ensure_code(matrix != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(row1 >= 0,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(row1 < matrix->nr, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(row2 >= 0,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(row2 < matrix->nr, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (row1 != row2) swap_rows(matrix, row1, row2);

    return CPL_ERROR_NONE;
    
}


/**
 * @brief
 *   Swap two matrix columns.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param column1   One matrix column.
 * @param column2   Another matrix column.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         Any of the specified columns is outside the <i>matrix</i> 
 *         boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The values of two given matrix columns are exchanged. Columns are 
 * counted starting from 0. If the same column number is given twice, 
 * nothing is done and no error is set.
 */

cpl_error_code cpl_matrix_swap_columns(cpl_matrix *matrix, 
                                       cpl_size column1, cpl_size column2)
{


    double       swap;
    cpl_size     nrow;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (column1 < 0 || column1 >= matrix->nc || 
        column2 < 0 || column2 >= matrix->nc)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);


    if (column1 == column2)
        return CPL_ERROR_NONE;
   
    nrow = matrix->nr;

    while (nrow--) {
        swap = matrix->m[column1];
        matrix->m[column1] = matrix->m[column2];
        matrix->m[column2] = swap;
        column1 += matrix->nc;
        column2 += matrix->nc;
    }

    return CPL_ERROR_NONE;
 
}


/**
 * @brief
 *   Swap a matrix column with a matrix row.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param row       Matrix row.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified <i>row</i> is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is not square.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The values of the indicated row are exchanged with the column having 
 * the same sequence number. Rows and columns are counted starting from 0. 
 */

cpl_error_code cpl_matrix_swap_rowcolumn(cpl_matrix *matrix, cpl_size row)
{


    double       swap;
    cpl_size     i;
    cpl_size     posr, posc;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (matrix->nr != matrix->nc)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (row < 0 || row >= matrix->nr)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);


    posr = row;
    posc = row * matrix->nc;

    for (i = 0; i < matrix->nr; i++) {
        swap = matrix->m[posr];
        matrix->m[posr] = matrix->m[posc];
        matrix->m[posc] = swap;
        posr += matrix->nc;
        posc++;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Reverse order of rows in matrix.
 *
 * @param matrix    Pointer to matrix to be reversed.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The order of the rows in the matrix is reversed in place.
 */

cpl_error_code cpl_matrix_flip_rows(cpl_matrix *matrix)
{


    cpl_size     i, j;

 
    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    for (i = 0, j = matrix->nr - 1; i < j; i++, j--)
        swap_rows(matrix, i, j);

    return CPL_ERROR_NONE;
    
}


/**
 * @brief
 *   Reverse order of columns in matrix.
 *
 * @param matrix    Pointer to matrix to be reversed.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The order of the columns in the matrix is reversed in place.
 */

cpl_error_code cpl_matrix_flip_columns(cpl_matrix *matrix)
{


    cpl_size     i, j;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    for (i = 0, j = matrix->nc - 1; i < j; i++, j--) 
        cpl_matrix_swap_columns(matrix, i, j);

    return CPL_ERROR_NONE;
 
}


/**
 * @brief
 *   Create transposed matrix.
 *
 * @param matrix    Pointer to matrix to be transposed.
 *
 * @return Pointer to transposed matrix. If a @c NULL pointer is passed,
 *   a @c NULL pointer is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The transposed of the input matrix is created. To destroy the new matrix 
 * the function @c cpl_matrix_delete() should be used.
 */

cpl_matrix *cpl_matrix_transpose_create(const cpl_matrix *matrix)
{


    cpl_matrix  *transposed = NULL;
    cpl_size     i, j;
    double      *m;
    double      *tm;


    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }


    tm = (double *)cpl_malloc((size_t)matrix->nc * (size_t)matrix->nr
                              * sizeof(*tm));
    transposed = cpl_matrix_wrap(matrix->nc, matrix->nr, tm);
    m = matrix->m;

    for (i = 0; i < matrix->nr; i++)
        for (j = 0, tm = transposed->m + i; 
             j < matrix->nc; j++, tm += matrix->nr)
            *tm = *m++;

    return transposed;

}


/**
 * @brief
 *   Sort matrix by rows.
 *
 * @param matrix    Pointer to matrix to be sorted.
 * @param mode      Sorting mode: 0, by absolute value, otherwise by value.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The matrix elements of the leftmost column are used as reference for
 * the row sorting, if there are identical the values of the second 
 * column are considered, etc. Rows with the greater values go on top. 
 * If @em mode is equal to zero, the rows are sorted according to their 
 * absolute values (zeroes at bottom).
 */

cpl_error_code cpl_matrix_sort_rows(cpl_matrix *matrix, int mode)
{


    double      *row;
    double       value1, value2;

    int   *done;
    cpl_size *sort_pattern;
    cpl_size *p;
    cpl_size i, j;
    cpl_size keep;
    cpl_size reach;
    int      start;
    cpl_size count;
    cpl_size pos;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (matrix->nr == 1)
      return CPL_ERROR_NONE;

    /*
     * Allocate the arrays for computing the sort pattern (i.e., the
     * list of arrival positions of array values after sorting).
     */

    sort_pattern = cpl_malloc((size_t)matrix->nr * sizeof(*sort_pattern));

    p = cpl_malloc((size_t)matrix->nr * sizeof(*p));

    for (i = 0; i < matrix->nr; i++)
        sort_pattern[i] = p[i] = i;


    /*
     * Find sorting pattern examining matrix columns.
     */

    j = 0;
    i = 1;
    reach = 1;

    while (i < matrix->nr) {

        value1 = matrix->m[p[i] * matrix->nc + j];
        value2 = matrix->m[p[i-1] * matrix->nc + j];

        if (mode == 0) {
            value1 = fabs(value1);
            value2 = fabs(value2);
        }

        if (value1 < value2) {
            i++;
            if (i > reach)
                reach = i;
        }
        else if (value1 > value2) {
            keep = p[i-1];
            p[i-1] = p[i];
            p[i] = keep;
            if (i > 1)
                i--;
            else
                i = reach + 1;
        }
        else {
            if (matrix->nc == 1) {
                i++;
                continue;
            }
            j++;
            if (j == matrix->nc) {
                j = 0;
                i++;
                if (i > reach)
                    reach = i;
            }
            continue;
        }

        if (j) {
            j = 0;
            continue;
        }

    }


    /*
     * To obtain the sort pattern this is the trick:
     */

    reach = 1;
    i = 1;
    while (i < matrix->nr) {
        if (p[sort_pattern[i]] > p[sort_pattern[i-1]]) {
            i++;
            if (i > reach)
                reach = i;
        }
        else {
            keep = sort_pattern[i-1];
            sort_pattern[i-1] = sort_pattern[i];
            sort_pattern[i] = keep;
            if (i > 1)
                i--;
            else
                i = reach + 1;
        }
    }

    cpl_free(p);


    /*
     * Finally, sort the rows. Starting from row zero, see where it 
     * should be written, save the overwritten row and write it to
     * the next position, etc. When a chain returns to the starting 
     * point, the next undone matrix row is the new starting point, 
     * and the process continues till all rows are done. This is
     * just a way to avoid doubling memory usage, paying with some 
     * more CPU.
     */

    done = (int *)cpl_calloc((size_t)matrix->nr, sizeof(*done));
    row = (double *)cpl_malloc((size_t)matrix->nc * sizeof(*row));

    pos = 0;
    start = 1;
    count = matrix->nr;
    
    while (count--) {

        done[pos] = 1;

        if (pos == sort_pattern[pos]) {

            if (count)
                for (pos = 0; pos < matrix->nr; pos++)
                     if (!done[pos]) {
                         start = 1;
                         break;
                     }

            continue;

        }

        if (start) {
            start = 0;
            read_row(matrix, row, pos);
        }

        pos = sort_pattern[pos];

        if (done[pos]) {

            write_row(matrix, row, pos);

            if (count) {
                for (pos = 0; pos < matrix->nr; pos++) {
                     if (!done[pos]) {
                         start = 1;
                         break;
                     }
                }
            }

        }
        else
            write_read_row(matrix, row, pos);

    }

    cpl_free(sort_pattern);
    cpl_free(done);
    cpl_free(row);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Sort matrix by columns.
 *
 * @param matrix    Pointer to matrix to be sorted.
 * @param mode      Sorting mode: 0, by absolute value, otherwise by value.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The matrix elements of the top row are used as reference for
 * the column sorting, if there are identical the values of the second 
 * row are considered, etc. Columns with the largest values go on the 
 * right. If @em mode is equal to zero, the columns are sorted according 
 * to their absolute values (zeroes at left).
 */

cpl_error_code cpl_matrix_sort_columns(cpl_matrix *matrix, int mode)
{


    double      *column;
    double       value1, value2;

    int   *done;
    int   *sort_pattern;
    int   *p;
    int    i, j;
    int    keep;
    int    reach;
    int    start;
    cpl_size count;
    cpl_size pos;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (matrix->nc == 1)
      return CPL_ERROR_NONE;

    /*
     * Allocate the arrays for computing the sort pattern (i.e., the
     * list of arrival positions of array values after sorting).
     */

    sort_pattern = cpl_malloc((size_t)matrix->nc * sizeof(int));

    p = cpl_malloc((size_t)matrix->nc * sizeof(int));

    for (i = 0; i < matrix->nc; i++)
        sort_pattern[i] = p[i] = i;


    /*
     * Find sorting pattern examining matrix rows.
     */

    j = 0;
    i = 1;
    reach = 1;

    while (i < matrix->nc) {

        value1 = matrix->m[j * matrix->nc + p[i]];
        value2 = matrix->m[j * matrix->nc + p[i-1]];

        if (mode == 0) {
            value1 = fabs(value1);
            value2 = fabs(value2);
        }

        if (value1 > value2) {
            i++;
            if (i > reach)
                reach = i;
        }
        else if (value1 < value2) {
            keep = p[i-1];
            p[i-1] = p[i];
            p[i] = keep;
            if (i > 1)
                i--;
            else
                i = reach + 1;
        }
        else {
            if (matrix->nr == 1) {
                i++;
                continue;
            }
            j++;
            if (j == matrix->nr) {
                j = 0;
                i++;
                if (i > reach)
                    reach = i;
            }
            continue;
        }

        if (j) {
            j = 0;
            continue;
        }

    }


    /*
     * To obtain the sort pattern this is the trick:
     */

    reach = 1;
    i = 1;
    while (i < matrix->nc) {
        if (p[sort_pattern[i]] > p[sort_pattern[i-1]]) {
            i++;
            if (i > reach)
                reach = i;
        }
        else {
            keep = sort_pattern[i-1];
            sort_pattern[i-1] = sort_pattern[i];
            sort_pattern[i] = keep;
            if (i > 1)
                i--;
            else
                i = reach + 1;
        }
    }

    cpl_free(p);


    /*
     * Finally, sort the columns. Starting from columns zero, see where 
     * it should be written, save the overwritten column and write it 
     * to the next position, etc. When a chain returns to the starting 
     * point, the next undone matrix column is the new starting point, 
     * and the process continues till all columns are done. This is
     * just a way to avoid doubling memory usage, paying with some 
     * more CPU.
     */

    done = (int *)cpl_calloc((size_t)matrix->nc, sizeof(*done));
    column = (double *)cpl_malloc((size_t)matrix->nr * sizeof(*column));

    pos = 0;
    start = 1;
    count = matrix->nc;
    
    while (count--) {

        done[pos] = 1;

        if (pos == sort_pattern[pos]) {

            if (count)
                for (pos = 0; pos < matrix->nc; pos++)
                     if (!done[pos]) {
                         start = 1;
                         break;
                     }

            continue;

        }

        if (start) {
            start = 0;
            read_column(matrix, column, pos);
        }

        pos = sort_pattern[pos];

        if (done[pos]) {

            write_column(matrix, column, pos);

            if (count) {
                for (pos = 0; pos < matrix->nc; pos++) {
                     if (!done[pos]) {
                         start = 1;
                         break;
                     }
                }
            }

        }
        else
            write_read_column(matrix, column, pos);

    }

    cpl_free(sort_pattern);
    cpl_free(done);
    cpl_free(column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Delete rows from a matrix.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param start     First row to delete.
 * @param count     Number of rows to delete.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified <i>start</i> is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is not positive.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         Attempt to delete all the rows of <i>matrix</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * 
 * A portion of the matrix data is physically removed. The pointer 
 * to matrix data may change, therefore pointers previously retrieved 
 * by calling @c cpl_matrix_get_data() should be discarded. The specified 
 * segment can extend beyond the end of the matrix, but the attempt to 
 * remove all matrix rows is flagged as an error because zero length 
 * matrices are illegal. Rows are counted starting from 0.
 */

cpl_error_code cpl_matrix_erase_rows(cpl_matrix *matrix, cpl_size start,
                                     cpl_size count)
{


    double      *m1;
    double      *m2;
    cpl_size     size;
    cpl_size     i;

    
    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 1)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (start < 0 || start >= matrix->nr)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (start + count > matrix->nr)
        count = matrix->nr - start;

    if (count == matrix->nr)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    size = matrix->nr * matrix->nc;
    m1 = matrix->m + start * matrix->nc;
    m2 = m1 + count * matrix->nc;
    
    for (i = (start + count) * matrix->nc; i < size; i++)
        *m1++ = *m2++;

    size = (matrix->nr - count) * matrix->nc;

    matrix->m = cpl_realloc(matrix->m, (size_t)size * sizeof(double));

    matrix->nr -= count;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Delete columns from a matrix.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param start     First column to delete.
 * @param count     Number of columns to delete.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified <i>start</i> is outside the <i>matrix</i> boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is not positive.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         Attempt to delete all the columns of <i>matrix</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * 
 * A portion of the matrix data is physically removed. The pointer
 * to matrix data may change, therefore pointers previously retrieved
 * by calling @c cpl_matrix_get_data() should be discarded. The specified 
 * segment can extend beyond the end of the matrix, but the attempt to 
 * remove all matrix columns is flagged as an error because zero length 
 * matrices are illegal. Columns are counted starting from 0.
 */

cpl_error_code cpl_matrix_erase_columns(cpl_matrix *matrix, 
                                        cpl_size start, cpl_size count)
{


    double      *m1;
    double      *m2;
    cpl_size     size;
    cpl_size     new_nc;
    cpl_size     i, j;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 1)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (start < 0 || start >= matrix->nc)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (start + count > matrix->nc)
        count = matrix->nc - start;

    if (count == matrix->nc)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    new_nc = matrix->nc - count;

    for (j = 0; j < matrix->nr; j++) {

        m1 = matrix->m + j * new_nc;
        m2 = matrix->m + j * matrix->nc;

        for (i = 0; i < start; i++)
            *m1++ = *m2++;

        m2 += count;

        for (i += count; i < matrix->nc; i++)
            *m1++ = *m2++;

    }

    size = (matrix->nc - count) * matrix->nr;

    matrix->m = cpl_realloc(matrix->m, (size_t)size * sizeof(double));

    matrix->nc -= count;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Reframe a matrix.
 *
 * @param matrix    Pointer to matrix to be modified.
 * @param top       Extra rows on top.
 * @param bottom    Extra rows on bottom.
 * @param left      Extra columns on left.
 * @param right     Extra columns on right.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         Attempt to shrink <i>matrix</i> to zero size (or less).
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input matrix is reframed according to specifications. Extra rows 
 * and column on the sides might also be negative, as long as they are 
 * compatible with the matrix sizes: the input matrix would be reduced 
 * in size accordingly, but an attempt to remove all matrix columns 
 * and/or rows is flagged as an error because zero length matrices are 
 * illegal. The old matrix elements contained in the new shape are left 
 * unchanged, and new matrix elements added by the reshaping are initialised 
 * to zero. No reshaping (i.e., all the extra rows set to zero) would
 * not be flagged as an error.
 *
 * @note
 *   The pointer to the matrix data buffer may change, therefore 
 *   pointers previously retrieved by calling @c cpl_matrix_get_data() 
 *   should be discarded.
 */

cpl_error_code cpl_matrix_resize(cpl_matrix *matrix, 
                                 cpl_size top, cpl_size bottom,
                                 cpl_size left, cpl_size right)
{


    cpl_matrix  *resized;
    cpl_size     nr, nc;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    if (top == 0 && bottom == 0 && left == 0 && right == 0)
        return CPL_ERROR_NONE;

    nr = matrix->nr + top + bottom;
    nc = matrix->nc + left + right;

    if (nr < 1 || nc < 1)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    resized = cpl_matrix_new(nr, nc);

    cpl_matrix_copy(resized, matrix, top, left);

    cpl_free(matrix->m);
    matrix->m = cpl_matrix_unwrap(resized);
    matrix->nr = nr;
    matrix->nc = nc;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Resize a matrix.
 *
 * @param matrix    Pointer to matrix to be resized.
 * @param rows      New number of rows.
 * @param columns   New number of columns.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         Attempt to shrink <i>matrix</i> to zero size (or less).
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input matrix is resized according to specifications. The old
 * matrix elements contained in the resized matrix are left unchanged,
 * and new matrix elements added by an increase of the matrix number of
 * rows and/or columns are initialised to zero.
 *
 * @note
 *   The pointer to the matrix data buffer may change, therefore 
 *   pointers previously retrieved by calling @c cpl_matrix_get_data() 
 *   should be discarded.
 */

cpl_error_code cpl_matrix_set_size(cpl_matrix *matrix, cpl_size rows,
                                   cpl_size columns)
{

    return cpl_matrix_resize(matrix, 0, rows - matrix->nr, 
                             0, columns - matrix->nc)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
   
}


/**
 * @brief
 *   Append a matrix to another.
 *
 * @param matrix1   Pointer to first matrix.
 * @param matrix2   Pointer to second matrix.
 * @param mode      Matrices connected horizontally (0) or vertically (1).
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>mode</i> is neither 0 nor 1.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         Matrices cannot be joined as indicated by <i>mode</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * If @em mode is set to 0, the matrices must have the same number
 * of rows, and are connected horizontally with the first matrix 
 * on the left. If @em mode is set to 1, the matrices must have the 
 * same number of columns, and are connected vertically with the 
 * first matrix on top. The first matrix is expanded to include the
 * values from the second matrix, while the second matrix is left 
 * untouched.
 *
 * @note
 *   The pointer to the first matrix data buffer may change, therefore 
 *   pointers previously retrieved by calling @c cpl_matrix_get_data() 
 *   should be discarded.
 */

cpl_error_code cpl_matrix_append(cpl_matrix *matrix1, 
                                 const cpl_matrix *matrix2, int mode)
{


    cpl_size old_size;


    if (matrix1 == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (matrix2 == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (mode == 0) {
        if (matrix1->nr != matrix2->nr)
            return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

        old_size = matrix1->nc;

        cpl_matrix_resize(matrix1, 0, 0, 0, matrix2->nc);
        cpl_matrix_copy(matrix1, matrix2, 0, old_size);
    } else if (mode == 1) {
        if (matrix1->nc != matrix2->nc)
            return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

        old_size = matrix1->nr;

        cpl_matrix_resize(matrix1, 0, matrix2->nr, 0, 0);
        cpl_matrix_copy(matrix1, matrix2, old_size, 0);
    } else {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Add two matrices.
 *
 * @param matrix1   Pointer to first matrix.
 * @param matrix2   Pointer to second matrix.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The specified matrices do not have the same size.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Add two matrices element by element. The two matrices must have
 * identical sizes. The result is written to the first matrix.
 */

cpl_error_code cpl_matrix_add(cpl_matrix *matrix1, const cpl_matrix *matrix2)
{


    cpl_size     size;
    double      *m1;
    double      *m2;


    if (matrix1 == NULL || matrix2 == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (matrix1->nr != matrix2->nr || matrix1->nc != matrix2->nc)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    
    size = matrix1->nr * matrix1->nc;
    m1 = matrix1->m;
    m2 = matrix2->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m1++ += *m2++;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Subtract a matrix from another.
 *
 * @param matrix1   Pointer to first matrix.
 * @param matrix2   Pointer to second matrix.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr"> 
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr> 
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The specified matrices do not have the same size.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Subtract the second matrix from the first one element by element. 
 * The two matrices must have identical sizes. The result is written 
 * to the first matrix.
 */

cpl_error_code cpl_matrix_subtract(cpl_matrix *matrix1, 
                                   const cpl_matrix *matrix2)
{


    cpl_size     size;
    double      *m1;
    double      *m2;


    if (matrix1 == NULL || matrix2 == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (matrix1->nr != matrix2->nr || matrix1->nc != matrix2->nc)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    
    size = matrix1->nr * matrix1->nc;
    m1 = matrix1->m;
    m2 = matrix2->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m1++ -= *m2++;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Multiply two matrices element by element.
 *
 * @param matrix1   Pointer to first matrix.
 * @param matrix2   Pointer to second matrix.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr"> 
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr> 
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The specified matrices do not have the same size.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Multiply the two matrices element by element. The two matrices must 
 * have identical sizes. The result is written to the first matrix.
 *
 * @note
 *   To obtain the rows-by-columns product between two matrices, 
 *   @c cpl_matrix_product_create() should be used instead.
 */

cpl_error_code cpl_matrix_multiply(cpl_matrix *matrix1, 
                                   const cpl_matrix *matrix2)
{


    cpl_size     size;
    double      *m1;
    double      *m2;


    if (matrix1 == NULL || matrix2 == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (matrix1->nr != matrix2->nr || matrix1->nc != matrix2->nc)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    size = matrix1->nr * matrix1->nc;
    m1 = matrix1->m;
    m2 = matrix2->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m1++ *= *m2++;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Divide a matrix by another element by element.
 *
 * @param matrix1   Pointer to first matrix.
 * @param matrix2   Pointer to second matrix.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr"> 
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr> 
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The specified matrices do not have the same size.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Divide each element of the first matrix by the corresponding
 * element of the second one. The two matrices must have the same 
 * number of rows and columns. The result is written to the first 
 * matrix. No check is made against a division by zero.
 */

cpl_error_code cpl_matrix_divide(cpl_matrix *matrix1, 
                                 const cpl_matrix *matrix2)
{


    cpl_size     size;
    double      *m1;
    double      *m2;


    if (matrix1 == NULL || matrix2 == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (matrix1->nr != matrix2->nr || matrix1->nc != matrix2->nc)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    size = matrix1->nr * matrix1->nc;
    m1 = matrix1->m;
    m2 = matrix2->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m1++ /= *m2++;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Add a scalar to a matrix.
 *
 * @param matrix   Pointer to matrix.
 * @param value    Value to add.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr"> 
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr> 
 *   </table>
 * @enderror
 *
 * Add the same value to each matrix element.
 */

cpl_error_code cpl_matrix_add_scalar(cpl_matrix *matrix, double value)
{


    cpl_size     size;
    double      *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m++ += value;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Subtract a scalar to a matrix.
 *   
 * @param matrix   Pointer to matrix.
 * @param value    Value to subtract.
 * 
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * 
 * Subtract the same value to each matrix element.
 */

cpl_error_code cpl_matrix_subtract_scalar(cpl_matrix *matrix, double value)
{


    cpl_size     size;
    double      *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m++ -= value;
    
    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Multiply a matrix by a scalar.
 *
 * @param matrix   Pointer to matrix.
 * @param value    Multiplication factor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Multiply each matrix element by the same factor.
 */

cpl_error_code cpl_matrix_multiply_scalar(cpl_matrix *matrix, double value)
{


    cpl_size     size;
    double      *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m++ *= value;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Divide a matrix by a scalar.
 *
 * @param matrix   Pointer to matrix.
 * @param value    Divisor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DIVISION_BY_ZERO</td>
 *       <td class="ecr">
 *         The input <i>value</i> is 0.0.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Divide each matrix element by the same value.
 */
 
cpl_error_code cpl_matrix_divide_scalar(cpl_matrix *matrix, double value)
{
 

    cpl_size size;
    double  *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (value == 0.0)
        return cpl_error_set_(CPL_ERROR_DIVISION_BY_ZERO);

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--)
        *m++ /= value;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the logarithm of matrix elements.
 *
 * @param matrix   Pointer to matrix.
 * @param base     Logarithm base.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>base</i>, or any <i>matrix</i> element, is 
 *         not positive.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each matrix element is replaced by its logarithm in the specified base.
 * The base and all matrix elements must be positive. If this is not the
 * case, the matrix would not be modified.
 */

cpl_error_code cpl_matrix_logarithm(cpl_matrix *matrix, double base)
{


    cpl_size     size;
    double      *m;
    double       logbase;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (base <= 0.0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);


    size = matrix->nr * matrix->nc;
    m = matrix->m;

    while (size--) {
        if (*m <= 0.0)
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        m++;
    }

    logbase = log(base);

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    cpl_tools_add_flops( 1 + 2 * (cpl_flops)size );
    while (size--) {
        *m = log(*m) / logbase;
        m++;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the exponential of matrix elements.
 *
 * @param matrix   Target matrix.
 * @param base     Exponential base.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>base</i> is not positive.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each matrix element is replaced by its exponential in the specified base.
 * The base must be positive.
 */

cpl_error_code cpl_matrix_exponential(cpl_matrix *matrix, double base)
{


    cpl_size     size;
    double      *m;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (base <= 0.0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    cpl_tools_add_flops( (cpl_flops)size );
    while (size--) {
        *m = pow(base, *m);
        m++;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute a power of matrix elements.
 *
 * @param matrix   Pointer to matrix.
 * @param exponent Constant exponent.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Any <i>matrix</i> element is not compatible with the
 *         specified <i>exponent</i> (see description below).
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each matrix element is replaced by its power to the specified 
 * exponent. If the specified exponent is not negative, all matrix 
 * elements must be not negative; if the specified exponent is 
 * negative, all matrix elements must be positive; otherwise, an
 * error condition is set and the matrix will be left unchanged. 
 * If the exponent is exactly 0.5 the (faster) @c sqrt() will be
 * applied instead of @c pow(). If the exponent is zero, then any 
 * (non negative) matrix element would be assigned the value 1.0.
 */

cpl_error_code cpl_matrix_power(cpl_matrix *matrix, double exponent)
{


    cpl_size     size;
    double      *m;
    int          negative = exponent < 0.0;


    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    if (negative) {
        while (size--) {
            if (*m <= 0.0)
                return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            m++;
        }
    }
    else {
        while (size--) {
            if (*m < 0.0)
                return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            m++;
        }
    }

    if (negative)
        exponent = -exponent;

    size = matrix->nr * matrix->nc;
    m = matrix->m;

    cpl_tools_add_flops( (cpl_flops)size );
    if (negative) {
        while (size--) {
            *m = 1 / pow(*m, exponent);
            m++;
        }
    }
    else {
        if (exponent == 0.5) {
            while (size--) {
                *m = sqrt(*m);
                m++;
            }
        }
        else {
            while (size--) {
                *m = pow(*m, exponent);
                m++;
            }
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Rows-by-columns product of two matrices.
 *
 * @param matrix1   Pointer to left side matrix.
 * @param matrix2   Pointer to right side matrix.
 *
 * @return Pointer to product matrix, or @c NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The number of columns of the first matrix is not equal to 
 *         the number of rows of the second matrix.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Rows-by-columns product of two matrices. The number of columns of the 
 * first matrix must be equal to the number of rows of the second matrix.
 * To destroy the new matrix the function @c cpl_matrix_delete() should 
 * be used.
 */

cpl_matrix *cpl_matrix_product_create(const cpl_matrix *matrix1, 
                                      const cpl_matrix *matrix2)
{

    cpl_matrix  *self;

    cpl_ensure(matrix1 != NULL,            CPL_ERROR_NULL_INPUT,         NULL);
    cpl_ensure(matrix2 != NULL,            CPL_ERROR_NULL_INPUT,         NULL);
    cpl_ensure(matrix1->nc == matrix2->nr, CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    /* Create data-buffer without overhead of initializing */
    self = cpl_matrix_wrap(matrix1->nr, matrix2->nc,
                           cpl_malloc((size_t)matrix1->nr * (size_t)matrix2->nc
                                      * sizeof(*self)));

    (void) cpl_matrix_product(self, matrix1, matrix2);

    return self;

}


/*
 * @brief Replace a matrix by its LU-decomposition
 * @param self  n X n non-singular matrix to decompose
 * @param perm  n-integer array to be filled with the row permutations
 * @param psig  On success set to 1/-1 for an even/odd number of permutations
 * @return CPL_ERROR_NONE on success, or the relevant CPL error code
 * @note If the matrix is singular the elements of self become undefined
 * @see Golub & Van Loan, Matrix Computations, Algorithms 3.2.1 (Outer Product
 *  Gaussian Elimination) and 3.4.1 (Gauss Elimination with Partial Pivoting).
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input pointer is <tt>NULL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>self</i> is not an n by n matrix.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         <i>self</i> and <i>perm</i> have incompatible sizes.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         <i>perm</i> is not of type CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_SINGULAR_MATRIX</td>
 *       <td class="ecr">
 *         <i>self</i> is singular.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 */
cpl_error_code cpl_matrix_decomp_lu(cpl_matrix * self, cpl_array * perm,
                                    int * psig)
{

    const cpl_size n     = cpl_array_get_size(perm);
    int          * iperm = cpl_array_get_data_int(perm);
    int            nn;
    cpl_size       i, j;
    const double * aread;
    double       * awrite;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(perm != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(psig != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(self->nc == self->nr, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(self->nc == n,        CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(cpl_array_get_type(perm) == CPL_TYPE_INT,
                    CPL_ERROR_TYPE_MISMATCH);

    nn = (int)n;

    cpl_ensure_code((cpl_size)nn == n, CPL_ERROR_ILLEGAL_INPUT);

    aread = awrite = self->m;

    /* No row permutations yet */
    *psig = 1;
    cpl_array_init_perm(perm);

    for (j = 0; j < n - 1; j++) {
        /* Find maximum in the j-th column */

        double pivot = fabs(aread[j + j * n]);
        cpl_size ipivot = j;

        for (i = j + 1; i < n; i++) {
           const double aij = fabs(aread[j + i * n]);

           if (aij > pivot) {
               pivot = aij;
               ipivot = i;
           }
        }

        if (pivot <= 0.0) {
            return cpl_error_set_message_(CPL_ERROR_SINGULAR_MATRIX, "Pivot %"
                                          CPL_SIZE_FORMAT " of %"
                                          CPL_SIZE_FORMAT " is non-positive: "
                                          "%g", j, n, pivot);
        }

        if (ipivot > j) {
            /* The maximum is below the diagonal - swap rows */
            const int iswap = iperm[j];

            iperm[j] = iperm[ipivot];
            iperm[ipivot] = iswap;

            *psig = - (*psig);

            swap_rows(self, j, ipivot);
        }

        pivot = aread[j + j * n];

        for (i = j + 1; i < n; i++) {
            const double aij = aread[j + i * n] / pivot;
            cpl_size k;

            awrite[j + i * n] = aij;

            for (k = j + 1; k < n; k++) {
                const double ajk = aread[k + j * n];
                const double aik = aread[k + i * n];
                awrite[k + i * n] = aik - aij * ajk;
            }
        }
    }
    cpl_tools_add_flops( (cpl_flops)(2 * n * n * n) / 3);

    /* Check if A(n,n) is non-zero */
    return fabs(aread[n * (n-1) + (n-1)]) > 0.0 ? CPL_ERROR_NONE
        : cpl_error_set_message_(CPL_ERROR_SINGULAR_MATRIX, "The last of %"
                                 CPL_SIZE_FORMAT " pivot(s) is zero", n);

}

/*
 * @brief Solve a LU-system
 * @param self  n X n LU-matrix from cpl_matrix_decomp_lu()
 * @param rhs   m right-hand-sides to be replaced by their solution
 * @param perm  n-integer array filled with the row permutations, or NULL
 * @return CPL_ERROR_NONE on success, or the relevant CPL error code
 * @see cpl_matrix_decomp_lu()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input pointer is <tt>NULL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>self</i> is not an n by n matrix.
 *       </td>
 *     </tr>
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The array or matrices not have the same number of rows.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         <i>perm</i> is non-NULL and not of type CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_DIVISION_BY_ZERO</td>
 *       <td class="ecr">
 *         The main diagonal of U contains a zero. This error can only occur
 *         if the LU-matrix does not come from a successful call to
 *         cpl_matrix_decomp_lu().
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 */
cpl_error_code cpl_matrix_solve_lu(const cpl_matrix * self,
                                   cpl_matrix * rhs, const cpl_array * perm)
{

    const int * iperm = NULL;
    cpl_size n, i, j, k;
    double * x = NULL; /* Avoid false uninit warning */
    const double * aread;
    const double * bread;
    double       * bwrite;
    cpl_ifalloc    mybuf;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(rhs  != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(self->nc == self->nr, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(rhs->nr == self->nr,  CPL_ERROR_INCOMPATIBLE_INPUT);

    n = self->nc;

    aread = self->m;
    bread = bwrite = rhs->m;

    if (perm != NULL) {

        cpl_ensure_code(cpl_array_get_size(perm) == n,
                        CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(cpl_array_get_type(perm) == CPL_TYPE_INT,
                        CPL_ERROR_TYPE_MISMATCH);

        iperm = cpl_array_get_data_int_const(perm);

        cpl_ifalloc_set(&mybuf, (size_t)n * sizeof(*x));
        x = (double*)cpl_ifalloc_get(&mybuf);
    }

    for (k=0; k < rhs->nc; k++) {
        if (perm != NULL) {
            /* Un-permute the rows in column k */
            for (i = 0; i < n; i++) {
                x[i] = bread[rhs->nc * i + k];
            }
            for (i = 0; i < n; i++) {
                bwrite[rhs->nc * i + k] = x[iperm[i]];
            }
        }

        /* Forward substitution in column k */
        for (i = 1; i < n; i++) {
            double tmp = bread[rhs->nc * i + k];
            for (j = 0; j < i; j++) {
                const double aij = aread[n * i + j];

                tmp -= aij * bread[rhs->nc * j + k];
            }
            bwrite[rhs->nc * i + k] = tmp;
        }

        /* Back substitution in column k */
        for (i = n - 1; i >= 0; i--) {
            double tmp = bread[rhs->nc * i + k];
            for (j = i + 1; j < n; j++) {
                const double aij = aread[n * i + j];

                tmp -= aij * bread[rhs->nc * j + k];
            }

            /* Check for a bug in the calling function */
            if (aread[n * i + i] == 0.0) break;

            bwrite[rhs->nc * i + k] = tmp/aread[n * i + i];

        }

        if (i >= 0) break;
    }

    if (perm != NULL) cpl_ifalloc_free(&mybuf);

    /* Flop count may be a bit too high if the below check fails */
    cpl_tools_add_flops( (cpl_flops)(k * 2 * n * n) );

    return k == rhs->nc ? CPL_ERROR_NONE
        : cpl_error_set_(CPL_ERROR_DIVISION_BY_ZERO);

}


/*----------------------------------------------------------------------------*/
/**
   @brief Replace a matrix by its Cholesky-decomposition, L * transpose(L) = A
   @param self  N by N symmetric positive-definite matrix to decompose
   @return CPL_ERROR_NONE on success, or the relevant CPL error code
   @note Only the upper triangle of self is read, L is written in the lower
         triangle
   @note If the matrix is singular the elements of self become undefined
   @see Golub & Van Loan, Matrix Computations, Algorithm 4.2.1
        (Cholesky: Gaxpy Version).

   @error
     <table class="ec" align="center">
       <tr>
         <td class="ecl">CPL_ERROR_NULL_INPUT</td>
         <td class="ecr">
           An input pointer is <tt>NULL</tt>.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
         <td class="ecr">
           <i>self</i> is not an n by n matrix.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_SINGULAR_MATRIX</td>
         <td class="ecr">
           <i>self</i> is not symmetric, positive definite.
         </td>
       </tr>
     </table>
   @enderror
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_matrix_decomp_chol(cpl_matrix * self)
{

    const cpl_size n = cpl_matrix_get_ncol(self);
    double * awrite = cpl_matrix_get_data(self);
    const double * aread = awrite;
    double sub = 0.0; /* Fix (false) uninit warning */
    cpl_size i, j = 0; /* Fix (false) uninit warning */

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cpl_matrix_get_nrow(self) == n, CPL_ERROR_ILLEGAL_INPUT);

    for (i = 0; i < n; i++) {
        for (j = i; j < n; j++) {
            cpl_size k;

            sub = aread[n * i + j];
            for (k = i-1; k >= 0; k--) {
                sub -= aread[n * i + k] * aread[n * j + k];
            }

            if (j > i) {
                awrite[n * j + i] = sub/aread[n * i + i];
            } else if (sub > 0.0) {
                awrite[n * i + i] = sqrt(sub);
            } else {
                break;
            }
        }
        if (j < n) break;
    }

    /* Dominant term: N^3 / 3  */
    /* FIXME: Probably inexact on error */
    cpl_tools_add_flops((cpl_flops)( i * ( 1 + j * ( 3 + 2 * n))) / 6);

    return i == n ? CPL_ERROR_NONE
        : cpl_error_set_message_(CPL_ERROR_SINGULAR_MATRIX, "Pivot "
                                 "%" CPL_SIZE_FORMAT " of %" CPL_SIZE_FORMAT
                                 " is non-positive: %g", i+1, n, sub);
}



/*----------------------------------------------------------------------------*/
/**
   @brief Solve a L*transpose(L)-system
   @param self  N by N L*transpose(L)-matrix from cpl_matrix_decomp_chol()
   @param rhs   M right-hand-sides to be replaced by their solution
   @return CPL_ERROR_NONE on success, or the relevant CPL error code
   @see cpl_matrix_decomp_chol()
   @note Only the lower triangle of self is accessed

   @error
     <table class="ec" align="center">
       <tr>
         <td class="ecl">CPL_ERROR_NULL_INPUT</td>
         <td class="ecr">
           An input pointer is <tt>NULL</tt>.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
         <td class="ecr">
           <i>self</i> is not an n by n matrix.
         </td>
       </tr>
       <tr> 
         <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
         <td class="ecr">
           The specified matrices do not have the same number of rows.
         </td>
       </tr>
       <tr> 
         <td class="ecl">CPL_ERROR_DIVISION_BY_ZERO</td>
         <td class="ecr">
           The main diagonal of L contains a zero. This error can only occur
           if the L*transpose(L)-matrix does not come from a successful call to
           cpl_matrix_decomp_chol().
         </td>
       </tr>
     </table>
   @enderror
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_matrix_solve_chol(const cpl_matrix * self,
                                     cpl_matrix * rhs)
{

    const cpl_size n    = cpl_matrix_get_ncol(self);
    const cpl_size nrhs = cpl_matrix_get_ncol(rhs);
    cpl_size i, j, k;
    const double * aread;
    const double * bread;
    double * bwrite;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(rhs  != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_matrix_get_nrow(self) == n, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_matrix_get_nrow(rhs)  == n, CPL_ERROR_INCOMPATIBLE_INPUT);

    aread = cpl_matrix_get_data_const(self);
    bread = bwrite = cpl_matrix_get_data(rhs);

    for (k=0; k < nrhs; k++) {
        /* Forward substitution in column k */
        for (i = 0; i < n; i++) {
            double sub = bread[nrhs * i + k];
            for (j = i-1; j >= 0; j--) {
                sub -= aread[n * i + j] * bread[nrhs * j + k];
            }
            cpl_ensure_code(aread[n * i + i] != 0.0,
                            CPL_ERROR_DIVISION_BY_ZERO);
            bwrite[nrhs * i + k] = sub/aread[n * i + i];
        }

        /* Back substitution in column k */

        for (i = n-1; i >= 0; i--) {
            double sub = bread[nrhs * i + k];
            for (j = i+1; j < n; j++) {
                sub -= aread[n * j + i] * bread[nrhs * j + k];
            }
            bwrite[nrhs * i + k] = sub/aread[n * i + i];
        }
    }

    cpl_tools_add_flops( 2 * (cpl_flops)(n * n * nrhs));

    return CPL_ERROR_NONE;

}





/**
 * @brief
 *   Compute the determinant of a matrix.
 *
 * @param matrix   Pointer to n by n matrix.
 *
 * @return Matrix determinant. In case of error, 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is not square.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is near-singular with a determinant so
 *         close to zero that it cannot be represented by a double.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input matrix must be a square matrix. In case of a 1x1 matrix, 
 * the matrix single element value is returned.
 */

double cpl_matrix_get_determinant(const cpl_matrix *matrix)
{

    cpl_matrix   * self;
    cpl_array    * perm;
    int          * iperm;
    double         determinant;
    int            sig;
    cpl_errorstate prevstate = cpl_errorstate_get();
    cpl_error_code error;


    cpl_ensure(matrix != NULL,           CPL_ERROR_NULL_INPUT,    0.0);
    cpl_ensure(matrix->nr == matrix->nc, CPL_ERROR_ILLEGAL_INPUT, 0.0);

    iperm = (int*) cpl_malloc((size_t)matrix->nr * sizeof(*iperm));

    perm = cpl_array_wrap_int(iperm, matrix->nr);

    self = cpl_matrix_duplicate(matrix);

    error = cpl_matrix_decomp_lu(self, perm, &sig);

    cpl_array_delete(perm);

    if (!error) {
        cpl_size i;
        /* Use long double because the intermediate products may vary greatly */
        cpl_long_double ldet = (cpl_long_double)sig;
        for (i = 0; i < matrix->nr; i++) {
            const double aii = cpl_matrix_get(self, i, i);

            ldet *= (cpl_long_double)aii;

            if (ldet == 0.0) break;

        }

        determinant = (double)ldet;

        if (i < matrix->nr) {
            (void)cpl_error_set_message_(CPL_ERROR_UNSPECIFIED,
                                         "Underflow at pivot %" CPL_SIZE_FORMAT
                                         " of %" CPL_SIZE_FORMAT,
                                         1+i, matrix->nr);
        } else if (determinant == 0.0) {
            (void)cpl_error_set_message_(CPL_ERROR_UNSPECIFIED,
                                         "Underflow at %Lg", ldet);
        }

    } else {

        determinant = 0.0;

        if (error == CPL_ERROR_SINGULAR_MATRIX) {
            cpl_errorstate_set(prevstate);
        } else {
            /* Should not be able to enter here */
            (void)cpl_error_set_where_();
        }
    }

    cpl_matrix_delete(self);

    return determinant;

}


/**
 * @brief
 *   Solution of a linear system.
 *
 * @param coeff   A non-singular N by N matrix.
 * @param rhs     A matrix containing one or more right-hand-sides.
 * @note rhs must have N rows and may contain more than one column,
 *       which each represent an independent right-hand-side.
 *
 * @return A newly allocated solution matrix with the size as rhs,
 *         or @c NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>coeff</i> is not a square matrix.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         <i>coeff</i> and <i>rhs</i> do not have the same number of rows.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_SINGULAR_MATRIX</td>
 *       <td class="ecr">
 *         <i>coeff</i> is singular (to working precision).
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Compute the solution of a system of N equations with N unknowns:
 *
 *   coeff * X = rhs
 *
 * @em coeff must be an NxN matrix, and @em rhs a NxM matrix.
 * M greater than 1 means that multiple independent right-hand-sides
 * are solved for.
 * To destroy the solution matrix the function @c cpl_matrix_delete()
 * should be used.
 * 
 */

cpl_matrix *cpl_matrix_solve(const cpl_matrix *coeff, const cpl_matrix *rhs)
{

    cpl_matrix * lu;
    cpl_matrix * x;
    cpl_array  * perm;
    int        * iperm;
    cpl_size     n;
    cpl_error_code code;


    cpl_ensure(coeff != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(rhs   != NULL, CPL_ERROR_NULL_INPUT, NULL);

    n = coeff->nc;

    cpl_ensure(coeff->nr == n, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(rhs->nr   == n, CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    lu = cpl_matrix_duplicate(coeff);
    x  = cpl_matrix_duplicate(rhs);

    iperm = (int*) cpl_malloc((size_t)n * sizeof(*iperm));
    perm = cpl_array_wrap_int(iperm, n);

    code = cpl_matrix_solve_(lu, x, perm);

    cpl_matrix_delete(lu);
    cpl_array_delete(perm);

    if (code) {
        (void)cpl_error_set_where_();
        cpl_matrix_delete(x);
        x = NULL;
    }

    return x;
}


/**
 * @brief
 *   Find a matrix inverse.
 *
 * @param matrix   Pointer to matrix to invert.
 *
 * @return Inverse matrix. In case of error a @c NULL is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>self</i> is not an n by n matrix.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_SINGULAR_MATRIX</td>
 *       <td class="ecr">
 *         <i>matrix</i> cannot be inverted.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input must be a square matrix. To destroy the new matrix the 
 * function @c cpl_matrix_delete() should be used.
 * 
 * @note
 *   When calling @c cpl_matrix_invert_create() with a nearly singular
 *   matrix, it is possible to get a result containin NaN values without 
 *   any error code being set. 
 */

cpl_matrix *cpl_matrix_invert_create(const cpl_matrix *matrix)
{

    cpl_matrix * lu;
    cpl_matrix * inverse;
    cpl_array  * perm;
    int        * iperm;
    cpl_size     n, i;
    int          sig, nn;
    cpl_error_code error;


    cpl_ensure(matrix != NULL, CPL_ERROR_NULL_INPUT, NULL);

    n = matrix->nc;

    cpl_ensure(matrix->nr == n, CPL_ERROR_ILLEGAL_INPUT, NULL);

    nn = (int)n;

    cpl_ensure((cpl_size)nn == n, CPL_ERROR_ILLEGAL_INPUT, NULL);

    lu = cpl_matrix_duplicate(matrix);

    iperm = (int*) cpl_malloc((size_t)n * sizeof(*iperm));

    perm = cpl_array_wrap_int(iperm, n);

    if (cpl_matrix_decomp_lu(lu, perm, &sig)) {
        cpl_matrix_delete(lu);
        cpl_array_delete(perm);
        (void)cpl_error_set_where_();
        return NULL;
    }

    /* Should not be able to fail at this point */
    inverse = cpl_matrix_new(n, n);
    /* Create an identity matrix with the rows permuted */
    for (i = 0; i < n; i++) {
        (void)cpl_matrix_set(inverse, i, iperm[i], 1.0);
    }

    cpl_array_delete(perm);

    error = cpl_matrix_solve_lu(lu, inverse, NULL);

    cpl_matrix_delete(lu);

    if (error) {
        cpl_matrix_delete(inverse);
        inverse = NULL;
        (void)cpl_error_set_where_();
    }

    return inverse;
    
}


/**
 * @brief
 *   Solution of overdetermined linear equations in a least squares sense.
 *
 * @param coeff   The N by M matrix of coefficients, where N >= M.
 * @param rhs     An N by K matrix containing K right-hand-sides.
 * @note rhs may contain more than one column,
 *       which each represent an independent right-hand-side.
 *
 * @return A newly allocated M by K solution matrix,
 *         or @c NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         <i>coeff</i> and <i>rhs</i> do not have the same number of rows.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_SINGULAR_MATRIX</td>
 *       <td class="ecr">
 *         The matrix is (near) singular and a solution cannot be computed.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The following linear system of N equations and M unknowns
 * is given:
 *
 * @code
 *   coeff * X = rhs
 * @endcode
 *
 * where @em coeff is the NxM matrix of the coefficients, @em X is the
 * MxK matrix of the unknowns, and @em rhs the NxK matrix containing 
 * the K right hand side(s).
 *
 * The solution to the normal equations is known to be a least-squares
 * solution, i.e. the 2-norm of coeff * X - rhs is minimized by the
 * solution to
 *    transpose(coeff) * coeff * X = transpose(coeff) * rhs.
 *
 * In the case that coeff is square (N is equal to M) it gives a faster
 * and more accurate result to use cpl_matrix_solve().
 *
 * The solution matrix should be deallocated with the function
 * @c cpl_matrix_delete(). 
 */

cpl_matrix *cpl_matrix_solve_normal(const cpl_matrix *coeff, 
                                    const cpl_matrix *rhs)
{

    cpl_matrix * solution;
    cpl_matrix * At;
    cpl_matrix * AtA;

    cpl_ensure(coeff != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(rhs   != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(rhs->nr == coeff->nr, CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    At  = cpl_matrix_transpose_create(coeff);
    solution = cpl_matrix_product_create(At, rhs);

    AtA = cpl_matrix_product_normal_create(At);

    cpl_matrix_delete(At);

    if (cpl_matrix_solve_spd(AtA, solution)) {
        cpl_matrix_delete(solution);
        solution = NULL;
        (void)cpl_error_set_where_();
    }

    cpl_matrix_delete(AtA);

    return solution;

}


/**
 * @brief
 *   Find the mean of all matrix elements.
 *
 * @param matrix   Pointer to matrix.
 *
 * @return Mean. In case of error 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The mean value of all matrix elements is calculated.
 *
 * @note
 *   This function works properly only in the case all the elements of
 *   the input matrix do not contain garbage (such as @c NaN or infinity).
 */

double cpl_matrix_get_mean(const cpl_matrix *matrix)
{

    if (matrix == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    } else {
        return cpl_tools_get_mean_double(matrix->m, matrix->nr * matrix->nc);
    }
}


/**
 * @brief
 *   Find the standard deviation of matrix elements.
 *
 * @param matrix   Pointer to matrix.
 *
 * @return Standard deviation. In case of error, or if a matrix is 
 *   1x1, 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The standard deviation of all matrix elements is calculated.
 *
 * @note
 *   This function works properly only in the case all the elements of
 *   the input matrix do not contain garbage (such as @c NaN or infinity).
 */

double cpl_matrix_get_stdev(const cpl_matrix *matrix)
{


    if (matrix == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    } else if (matrix->nr == 1 && matrix->nc == 1) {
        return 0.0;
    } else {

        const cpl_size size = matrix->nr * matrix->nc;
        const double varsum = cpl_tools_get_variancesum_double(matrix->m,
                                                               size, NULL);

        return sqrt( varsum / (double)(size - 1));
    }
}


/**
 * @brief
 *   Find the median of matrix elements.
 *
 * @param matrix   Pointer to matrix.
 *
 * @return Median. In case of error 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The median value of all matrix elements is calculated.
 */

double cpl_matrix_get_median(const cpl_matrix *matrix)
{
    double   median = 0.0;


    if (matrix == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else {
        const cpl_size size = matrix->nr * matrix->nc;
        double  *copybuf = (double*)cpl_malloc((size_t)size * sizeof(*copybuf));

        (void)memcpy(copybuf, matrix->m, (size_t)size * sizeof(double));
        median = cpl_tools_get_median_double(copybuf, size);
        cpl_free(copybuf);
    }

    return median;
}


/**
 * @brief
 *   Find the minimum value of matrix elements.
 *
 * @param matrix   Pointer to matrix.
 *
 * @return Minimum value. In case of error, 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The minimum value of all matrix elements is found.
 */

double cpl_matrix_get_min(const cpl_matrix *matrix)
{


    cpl_size i, j;

    if (cpl_matrix_get_minpos(matrix, &i, &j)) {
        (void)cpl_error_set_where_();
        return 0.0;
    } else {
        return cpl_matrix_get(matrix, i, j);
    }
}


/**
 * @brief
 *   Find the maximum value of matrix elements.
 *
 * @param matrix   Pointer to matrix.
 *
 * @return Maximum value. In case of error, 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The maximum value of all matrix elements is found.
 */
 
double cpl_matrix_get_max(const cpl_matrix *matrix)
{

    cpl_size i, j;

    if (cpl_matrix_get_maxpos(matrix, &i, &j)) {
        (void)cpl_error_set_where_();
        return 0.0;
    } else {
        return cpl_matrix_get(matrix, i, j);
    }
}


/**
 * @brief
 *   Find position of minimum value of matrix elements.
 *
 * @param matrix   Input matrix.
 * @param row      Returned row position of minimum.
 * @param column   Returned column position of minimum.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The position of the minimum value of all matrix elements is found.
 * If more than one matrix element have a value corresponding to
 * the minimum, the lowest element row number is returned in @em row.
 * If more than one minimum matrix elements have the same row number,
 * the lowest element column number is returned in @em column.
 */

cpl_error_code cpl_matrix_get_minpos(const cpl_matrix *matrix, 
                                     cpl_size *row, cpl_size *column)
{

    if (matrix == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (row == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (column == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else {

        const cpl_size size = matrix->nr * matrix->nc;
        cpl_size       i, pos = 0;


        for (i = 1; i < size; i++) {
            if (matrix->m[i] < matrix->m[pos]) {
                pos = i;
            }
        }

        *row    = pos / matrix->nc;
        *column = pos % matrix->nc;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Find position of the maximum value of matrix elements.
 *
 * @param matrix   Input matrix.
 * @param row      Returned row position of maximum.
 * @param column   Returned column position of maximum.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The input <i>matrix</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The position of the maximum value of all matrix elements is found.
 * If more than one matrix element have a value corresponding to
 * the maximum, the lowest element row number is returned in @em row.
 * If more than one maximum matrix elements have the same row number,
 * the lowest element column number is returned in @em column.
 */

cpl_error_code cpl_matrix_get_maxpos(const cpl_matrix *matrix, 
                                     cpl_size *row, cpl_size *column)
{

    if (matrix == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (row == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (column == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else {

        const cpl_size size = matrix->nr * matrix->nc;
        cpl_size       i, pos = 0;


        for (i = 1; i < size; i++) {
            if (matrix->m[i] > matrix->m[pos]) {
                pos = i;
            }
        }

        *row    = pos / matrix->nc;
        *column = pos % matrix->nc;
    }

    return CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief
     Solution of Symmetric, Positive Definite system of linear equations.
 *
   @param self   The N by N Symmetric, Positive Definite matrix.
   @param rhs    An N by K matrix containing K right-hand-sides.
   @note rhs may contain more than one column,
         which each represent an independent right-hand-side. The solution
         is written in rhs. Only the upper triangular part of self is read,
         while the lower triangular part is modified.
   @return CPL_ERROR_NONE on success, or the relevant CPL error code
 *
   @error
     <table class="ec" align="center">
       <tr>
         <td class="ecl">CPL_ERROR_NULL_INPUT</td>
         <td class="ecr">
           <i>self</i> or <i>rhs</i> is a <tt>NULL</tt> pointer.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
         <td class="ecr">
           <i>self</i> and <i>rhs</i> do not have the same number of rows.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
         <td class="ecr">
           <i>self</i> is not an N by N matrix.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_SINGULAR_MATRIX</td>
         <td class="ecr">
           <i>self</i> is (near) singular and a solution cannot be computed.
         </td>
       </tr>
     </table>
   @enderror
 */
/*----------------------------------------------------------------------------*/

cpl_error_code cpl_matrix_solve_spd(cpl_matrix *self,
                                    cpl_matrix *rhs)
{

    cpl_ensure_code(!cpl_matrix_decomp_chol(self), cpl_error_get_code());

    cpl_ensure_code(!cpl_matrix_solve_chol(self, rhs), cpl_error_get_code());

    return CPL_ERROR_NONE;

}


/**
 * @internal
 * @brief Compute A = B * transpose(B)
 *
 * @param self     Pre-allocated M x M matrix to hold the result
 * @param other    M x N Matrix to multiply with its transpose
 * @return   CPL_ERROR_NONE or the relevant CPL error code on error
 * @note Only the upper triangle of A is computed, while the elements
 *       below the main diagonal have undefined values.
 * @see cpl_matrix_product_create()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>self</i> is not an M by M matrix.
 *       </td>
 *     </tr>
 *     <tr> 
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The two matrices have a different number of rows.
 *       </td>
 *     </tr>
 *     <tr> 
 *
 *   </table>
 * @enderror
 *
 */

cpl_error_code cpl_matrix_product_normal(cpl_matrix * self,
                                         const cpl_matrix * other)
{

    double         sum;
    const double * ai = cpl_matrix_get_data_const(other);
    const double * aj;
    double       * bwrite = cpl_matrix_get_data(self);
    const size_t   m = (size_t)cpl_matrix_get_nrow(self);
    const size_t   n = (size_t)cpl_matrix_get_ncol(other);


    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cpl_matrix_get_ncol(self)  == (cpl_size)m,
                    CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_matrix_get_nrow(other) == (cpl_size)m,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

#ifdef CPL_MATRIX_PRODUCT_NORMAL_RESET
    /* Initialize all values to zero.
       This is done to avoid access of uninitilized memory in case
       someone passes the matrix to for example cpl_matrix_dump(). */
    (void)memset(bwrite, 0, m * n * sizeof(*bwrite));
#endif

    /* The result at (i,j) is the dot-product of i'th and j'th row */
    for (size_t i = 0; i < m; i++, bwrite += m, ai += n) {
        aj = ai; /* aj points to first entry in j'th row */
        for (size_t j = i; j < m; j++, aj += n) {
            sum = 0.0;
            for (size_t k = 0; k < n; k++) {
                sum += ai[k] * aj[k];
            }
            bwrite[j] = sum;
        }
    }

    cpl_tools_add_flops((cpl_flops)(n * m * (m + 1)));

    return CPL_ERROR_NONE;
}


/**
 * @internal
 * @brief Create and compute A = B * transpose(B)
 *
 * @param self     M x N Matrix
 * @return Pointer to created M x M product matrix, or @c NULL on error.
 * @note Only the upper triangle of A is computed, while the elements
 *       below the main diagonal have undefined values.
 * @see cpl_matrix_product_create()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input matrix is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * To destroy the new matrix the function @c cpl_matrix_delete() should
 * be used.
 */

cpl_matrix * cpl_matrix_product_normal_create(const cpl_matrix * self)
{

    const size_t m       = cpl_matrix_get_nrow(self);
    cpl_matrix * product = cpl_matrix_wrap((cpl_size)m, (cpl_size)m,
                                           cpl_malloc(m * m * sizeof(double)));

    if (cpl_matrix_product_normal(product, self)) {
        cpl_matrix_delete(product);
        product = NULL;
        (void)cpl_error_set_where_();
    }

    return product;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Fill a matrix with the product of A * B
  @param    self  The matrix to fill, is or else will be set to size M x N
  @param    ma    The matrix A, of size M x K
  @param    mb    The matrix B, of size K * N
  @return   CPL_ERROR_NONE or the relevant CPL error code on error
  @note If upon entry, self has a size m *n, which differs from its size on
        return, then the result of preceeding calls to cpl_matrix_get_data()
        are invalid.
  @see  cpl_matrix_product_create()

*/
/*----------------------------------------------------------------------------*/

cpl_error_code cpl_matrix_product(cpl_matrix * self,
                                  const cpl_matrix * ma,
                                  const cpl_matrix * mb)
{
    const size_t   bs = 48;
    double       * ds;
    const double * d1 = cpl_matrix_get_data_const(ma);
    const double * d2 = cpl_matrix_get_data_const(mb);

    const size_t   nr = cpl_matrix_get_nrow(ma);
    const size_t   nc = cpl_matrix_get_ncol(mb);
    const size_t   nk = cpl_matrix_get_nrow(mb);

    cpl_ensure_code(ma   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(mb   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code((size_t)ma->nc == nk, CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_matrix_set_size_(self, nr, nc)) return cpl_error_set_where_();

    ds = cpl_matrix_get_data(self);

    (void)memset(ds, 0, nr * nc * sizeof(*ds));

    /* simple blocking to reduce cache misses */
    for (size_t i = 0; i < nr; i += bs) {
        for (size_t j = 0; j < nc; j += bs) {
            for (size_t k = 0; k < nk; k += bs) {
                for (size_t ib = i; ib < CX_MIN(i + bs, nr); ib++) {
                    for (size_t jb = j; jb < CX_MIN(j + bs, nc); jb++) {
                        double sum = 0.;
                        for (size_t kb = k; kb < CX_MIN(k + bs, nk); kb++) {
                            sum += d1[ib * nk + kb] * d2[kb * nc + jb];
                        }
                        ds[ib * nc + jb] += sum;
                    }
                }
            }
        }
    }


    cpl_tools_add_flops( 2 * (cpl_flops)(ma->nr * mb->nr * mb->nc) );

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Fill a matrix with the product of A * B'
  @param    self  The matrix to fill, is or else will be set to size M x N
  @param    ma    The matrix A, of size M x K
  @param    mb    The matrix B, of size N x K
  @return   CPL_ERROR_NONE or the relevant CPL error code on error
  @note     The use of the transpose of B causes a more efficient memory access
  @note     Changing the order of A and B is allowed, it transposes the result
  @see      cpl_matrix_product_create()

*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_matrix_product_transpose(cpl_matrix * self,
                                            const cpl_matrix * ma,
                                            const cpl_matrix * mb)
{

    double         sum;

    double       * ds;
    const double * d1 = cpl_matrix_get_data_const(ma);
    const double * d2 = cpl_matrix_get_data_const(mb);
    const double * di;

    const cpl_size      nr = cpl_matrix_get_nrow(ma);
    const cpl_size      nc = cpl_matrix_get_nrow(mb);
    const cpl_size      nk = cpl_matrix_get_ncol(mb);
    cpl_size       i, j, k;


    cpl_ensure_code(ma   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(mb   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(ma->nc == nk, CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_matrix_set_size_(self, nr, nc)) return cpl_error_set_where_();

    ds = cpl_matrix_get_data(self);

    for (i = 0; i < nr; i++, d1 += nk) {
        /* Since ma and mb are addressed in the same manner,
           they can use the same index, k */

        di = d2; /* di points to first entry in i'th row */
        for (j = 0; j < nc; j++, di += nk) {
            sum = 0.0;
            for (k = 0; k < nk; k++) {
                sum += d1[k] * di[k];
            }
            ds[nc * i + j] = sum;
        }
    }

    cpl_tools_add_flops( 2 * (cpl_flops)(ma->nr * mb->nr * mb->nc) );

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Fill a matrix with the product of B * A * B'
  @param   self  The matrix to fill, is or else will be set to size N x N
  @param   ma    The matrix A, of size M x M
  @param   mb    The matrix B, of size N x M
  @return  CPL_ERROR_NONE or the relevant CPL error code on error
  @note    Requires 2 * N * M * (N + M) FLOPs and M doubles of temporary storage
  @see     cpl_matrix_product_create()

  The intermediate result, C = A * B', is computed one column at a time,
  each column of C is then used once to compute one row of the final result.

  The performed sequence of floating point operations is exactly the same as
  in the combination of the two calls
    cpl_matrix_product_transpose(C, A, B);
    cpl_matrix_product_create(B, C);
  and the result is therefore also exactly identical.

  The difference lies in the reduced use of temporary storage and in the
  unit stride access, both of which lead to better performance (for large
  matrices).
  
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_matrix_product_bilinear(cpl_matrix * self,
                                           const cpl_matrix * ma,
                                           const cpl_matrix * mb)
{

    double       * ds;
    double       * cj; /* Holds one column of A * B' */
    const double * d1 = cpl_matrix_get_data_const(ma);
    const double * d2 = cpl_matrix_get_data_const(mb);

    const cpl_size      nr = cpl_matrix_get_nrow(mb);
    const cpl_size      nc = cpl_matrix_get_ncol(mb);
    cpl_size       i, j, k;


    cpl_ensure_code(ma   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(mb   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(ma->nr == nc, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(ma->nc == nc, CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_matrix_set_size_(self, nr, nr)) return cpl_error_set_where_();

    ds = cpl_matrix_get_data(self);
    cj = cpl_malloc((size_t)nc * sizeof(*cj));

    for (j = 0; j < nr; j++) {

        /* First compute the jth column of C = A * B', whose kth element
           is the dot-product of kth row of A and the jth row of B,
           both of which are read with unit stride. */
        for (k = 0; k < nc; k++) {
            double sum = 0.0;
            for (i = 0; i < nc; i++) {
                sum += d1[nc * k + i] * d2[nc * j + i];
            }
            cj[k] = sum;
        }

        /* Then compute (i, j)th element of the result, which
           is the dot-product of the ith row of B and the jth column of C,
           both of which are read with unit stride. */
        for (i = 0; i < nr; i++) {
            double sum = 0.0;

            for (k = 0; k < nc; k++) {
                sum += d2[nc * i + k] * cj[k];
            }
            ds[nr * i + j] = sum;
        }
    }

    cpl_free(cj);

    cpl_tools_add_flops( 2 * (cpl_flops)(nr * nc * (nc + nr)) );

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Solve a L*transpose(L)-system with a transposed Right Hand Side
   @param self  N by N L*transpose(L)-matrix from cpl_matrix_decomp_chol()
   @param rhs   M right-hand-sides to be replaced by their solution
   @return CPL_ERROR_NONE on success, or the relevant CPL error code
   @see cpl_matrix_solve_chol()
   @note Only the lower triangle of self is accessed
   @note The use of the transpose of rhs causes a more efficient memory access

   @error
     <table class="ec" align="center">
       <tr>
         <td class="ecl">CPL_ERROR_NULL_INPUT</td>
         <td class="ecr">
           An input pointer is <tt>NULL</tt>.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
         <td class="ecr">
           <i>self</i> is not an n by n matrix.
         </td>
       </tr>
       <tr> 
         <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
         <td class="ecr">
           Selfs number of rows differs from rhs' number of columns.
         </td>
       </tr>
       <tr> 
         <td class="ecl">CPL_ERROR_DIVISION_BY_ZERO</td>
         <td class="ecr">
           The main diagonal of L contains a zero. This error can only occur
           if the L*transpose(L)-matrix does not come from a successful call to
           cpl_matrix_decomp_chol().
         </td>
       </tr>
     </table>
   @enderror

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_matrix_solve_chol_transpose(const cpl_matrix * self,
                                               cpl_matrix * rhs)
{

    const cpl_size n = cpl_matrix_get_ncol(self);
    const cpl_size nrhs = cpl_matrix_get_nrow(rhs);
    cpl_size i, j, k;
    const double * aread;
    const double * ai;
    double * bk;
    double sub;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(rhs  != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(self->nr == n, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(rhs->nc  == n, CPL_ERROR_INCOMPATIBLE_INPUT);

    aread = self->m;

    /* bk points to first entry in k'th right hand side */
    bk = rhs->m;

    for (k=0; k < nrhs; k++, bk += n) {

        /* Forward substitution in column k */

        /* Since self and rhs are addressed in the same manner,
           they can use the same index, j */
        ai = aread; /* ai points to first entry in i'th row */
        for (i = 0; i < n; i++, ai += n) {
            sub = 0.0;
            for (j = 0; j < i; j++) {
                sub += ai[j] * bk[j];
            }
            cpl_ensure_code(k > 0 || ai[j] != 0.0, CPL_ERROR_DIVISION_BY_ZERO);
            bk[j] = (bk[j] - sub) / ai[j];
        }

        /* Back substitution in column k */

        for (i = n-1; i >= 0; i--) {
            sub = bk[i];
            for (j = i+1; j < n; j++) {
                sub -= aread[n * j + i] * bk[j];
            }
            bk[i] = sub/aread[n * i + i];
        }
    }

    cpl_tools_add_flops( 2 * (cpl_flops)(n * n * nrhs));

    return CPL_ERROR_NONE;

}

/*
 * The following implementation of one-sided Jacobi orthogonalization
 * to compute the SVD of a MxN matrix, with M >= N is based on the
 * implementation found in the GNU Scientific Library (GSL).
 */

/**
 * @internal
 * @brief
 *   Compute the singular value decomposition of a real matrix using Jacobi
 *   plane rotations.
 *
 * @param U  The M x N matrix to be factorized.
 * @param S  A vector of N elements for storing the singular values.
 * @param V  An N x N square matrix for storing the right singular vectors.
 *
 * @return
 *   @c CPL_ERROR_NONE if the factorization was successful and an error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         One or more arguments are a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The matrix U has less rows than columns (M < N), or the matrix V
 *         is not a square matrix.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The number of rows of the matrix V does not match the number
 *         of columns of U (N), or the size of the vector S does not match
 *         the number of rows of U (M).
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function computes singular value decomposition (SVD) of a real M x N
 * matrix @em U, where M >= N. It uses a one-sided Jacobi orthogonalization
 * as described in James Demmel, Kresimir Veselic, "Jacobi's Method is more
 * accurate than QR", Lapack Working Note 15 (LAWN15), October 1989,
 * Algorithm 4.1 which is available at @em netlib.org.
 *
 * The input matrix @em U is overwritten during the process and contains
 * in the end the left singular vectors of @em U. The matrix @em V will be
 * updated with the right singular vectors of @em U, and the vector @em S
 * will contain the singular values. For efficiency reasons the singular
 * values are not written to the diagonal elements of a N x N diagonal matrix,
 * but an N-elements vector.
 */

static cpl_error_code
_cpl_matrix_decomp_sv_jacobi(cpl_matrix *U, cpl_vector *S, cpl_matrix *V)
{

    cpl_ensure_code((U != NULL) && (V != NULL) && (S != NULL),
                    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(U->nr >= U->nc, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(V->nr == V->nc, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(V->nr == U->nc, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(cpl_vector_get_size(S) == U->nc,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    const cpl_size m = U->nr;
    const cpl_size n = U->nc;

    /*
     * Initialize the rotation counter and the sweep counter.
     * Use a minimum number of 12 sweeps.
     */

    cpl_size count = 1;
    cpl_size sweep = 0;
    cpl_size sweepmax = CPL_MAX(5 * n, 12);

    double tolerance = 10. * m * DBL_EPSILON;


    /*
     * Fill V with the identity matrix.
     */

    cpl_matrix_fill_identity(V);

    /*
     * Store the column error estimates in S, for use during the
     * orthogonalization
     */

    cpl_size i;
    cpl_size j;

    cpl_vector *cv1 = cpl_vector_new(m);
    double *const _cv1 = cpl_vector_get_data(cv1);

    for (j = 0; j < n; ++j) {
        for (i = 0; i < m; ++i) {
            _cv1[i] = cpl_matrix_get(U, i, j);
        }

        double s = sqrt(cpl_vector_product(cv1, cv1));
        cpl_vector_set(S, j, DBL_EPSILON * s);
    }


    /*
     *  Orthogonalization of U by plane rotations
     */

    cpl_vector *cv2 = cpl_vector_new(m);
    double *const _cv2 = cpl_vector_get_data(cv2);

    while ((count > 0) && (sweep <= sweepmax)) {

        /*
         * Initialize the rotation counter.
         */

        count = n * (n - 1) / 2;

        for (j = 0; j < n - 1; ++j) {

            cpl_size k;

            for (k = j + 1; k < n; ++k) {

                for (i = 0; i < m; ++i) {
                    _cv1[i] = cpl_matrix_get(U, i, j);
                }

                for (i = 0; i < m; ++i) {
                    _cv2[i] = cpl_matrix_get(U, i, k);
                }

                /*
                 * Compute the (j, k) submatrix elements of Ut*U.
                 * The non-diagonal element c is scaled by 2.
                 */

                double a = sqrt(cpl_vector_product(cv1, cv1));
                double b = sqrt(cpl_vector_product(cv2, cv2));
                double c = 2. * cpl_vector_product(cv1, cv2);

                /*
                 * Test for columns j, k orthogonal, or dominant errors.
                 */

                double abserr_a = cpl_vector_get(S, j);
                double abserr_b = cpl_vector_get(S, k);

                cpl_boolean orthogonal = (fabs(c) <= tolerance * a * b);
                cpl_boolean sorted     = (a >= b);
                cpl_boolean noisy_a    = (a < abserr_a);
                cpl_boolean noisy_b    = (b < abserr_b);

                if (sorted && (orthogonal || noisy_a || noisy_b)) {
                    --count;
                    continue;
                }

                /*
                 * Compute the Jacobi rotation which diagonalizes the (j, k)
                 * submatrix.
                 */

                double q = a * a - b * b;
                double v = sqrt(c * c + q * q);

                double cs;  /* cos(theta) */
                double sn;  /* sin(theta) */

                if (v == 0. || !sorted) {
                    cs = 0.;
                    sn = 1.;
                }
                else {
                    cs = sqrt((v + q) / (2. * v));
                    sn = c / (2. * v * cs);
                }

                /*
                 * Update the columns j and k of U and V
                 */

                for (i = 0; i < m; ++i) {

                    const double u_ij = cpl_matrix_get(U, i, j);
                    const double u_ik = cpl_matrix_get(U, i, k);

                    cpl_matrix_set(U, i, j,  u_ij * cs + u_ik * sn);
                    cpl_matrix_set(U, i, k, -u_ij * sn + u_ik * cs);

                }

                for (i = 0; i < n; ++i) {

                    const double v_ij = cpl_matrix_get(V, i, j);
                    const double v_ik = cpl_matrix_get(V, i, k);

                    cpl_matrix_set(V, i, j,  v_ij * cs + v_ik * sn);
                    cpl_matrix_set(V, i, k, -v_ij * sn + v_ik * cs);
                }

                cpl_vector_set(S, j, fabs(cs) * abserr_a +
                               fabs(sn) * abserr_b);
                cpl_vector_set(S, k, fabs(sn) * abserr_a +
                               fabs(cs) * abserr_b);

            }

        }

        ++sweep;

    }

    cpl_vector_delete(cv2);


    /*
     * Compute singular values.
     */

    double prev_norm = -1.;

    for (j = 0; j < n; ++j) {

        for (i = 0; i < m; ++i) {
            _cv1[i] = cpl_matrix_get(U, i, j);
        }

        double norm = sqrt(cpl_vector_product(cv1, cv1));

        /*
         * Determine if the singular value is zero, according to the
         * criteria used in the main loop above (i.e. comparison with
         * norm of previous column).
         */

        if ((norm == 0.) || (prev_norm == 0.) ||
                ((j > 0) && (norm <= tolerance * prev_norm))) {

            cpl_vector_set(S, j, 0.);          /* singular */
            cpl_matrix_fill_column(U, 0., j);  /* annihilate column */

            prev_norm = 0.;

        }
        else {

            cpl_vector_set(S, j, norm);     /* non-singular */

            /*
             * Normalize the column vector of U
             */

            for (i = 0; i < m; ++i) {
                cpl_matrix_set(U, i, j, _cv1[i] / norm);
            }

            prev_norm = norm;

        }

    }

    cpl_vector_delete(cv1);

    if (count > 0) {
        return CPL_ERROR_CONTINUE;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Solve a linear system in a least square sense using an SVD factorization.
 *
 * @param coeff  An N by M matrix of linear system coefficients, where N >= M
 * @param rhs    An N by 1 matrix with the right hand side of the system
 *
 * @return
 *  A newly allocated M by 1 matrix containing the solution vector or
 *  @c NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         <i>coeff</i> and <i>rhs</i> do not have the same number of rows.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The matrix <i>rhs</i> has more than one column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function solves a linear system of the form Ax = b for the solution
 * vector x, where @c A is represented by the argument @em coeff and @c b
 * by the argument @em rhs. The linear system is solved using the singular
 * value decomposition (SVD) of the coefficient matrix, based on a one-sided
 * Jacobi orthogonalization.
 *
 * The returned solution matrix should be deallocated using
 * @b cpl_matrix_delete().
 */

cpl_matrix *
cpl_matrix_solve_svd(const cpl_matrix *coeff, const cpl_matrix *rhs)
{

    cpl_ensure((coeff != NULL) && (rhs != NULL), CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(coeff->nr == rhs->nr, CPL_ERROR_INCOMPATIBLE_INPUT, NULL);
    cpl_ensure(rhs->nc == 1, CPL_ERROR_ILLEGAL_INPUT, NULL);


    /*
     * Compute the SVD factorization of the coefficient matrix
     */

    cpl_matrix *U = cpl_matrix_duplicate(coeff);
    cpl_matrix *V = cpl_matrix_new(U->nc, U->nc);
    cpl_vector *S = cpl_vector_new(U->nc);

    cpl_error_code status = _cpl_matrix_decomp_sv_jacobi(U, S, V);

    if (status != CPL_ERROR_NONE) {
        cpl_vector_delete(S);
        cpl_matrix_delete(V);
        cpl_matrix_delete(U);

        cpl_error_set_where(cpl_func);

        return NULL;
    }


    /*
     * Solve the linear system.
     */

    cpl_matrix *Ut = cpl_matrix_transpose_create(U);
    cpl_matrix *w  = cpl_matrix_product_create(Ut, rhs);

    cpl_matrix_delete(Ut);

    const double *_s = cpl_vector_get_data_const(S);
    double *_w = cpl_matrix_get_data(w);

    cpl_size ic;
    for (ic = 0; ic < U->nc; ++ic) {

        double s = _s[ic];

        if (s != 0.) {
            s = 1. / s;
        }
        _w[ic] *= s;

    }

    cpl_matrix *solution = cpl_matrix_product_create(V, w);

    cpl_matrix_delete(w);
    cpl_vector_delete(S);
    cpl_matrix_delete(V);
    cpl_matrix_delete(U);

    return solution;

}

/**
 * @internal
 * @brief
 *   Solution of a linear system.
 *
 * @param a  A non-singular N by N matrix, becomes its LU-decomposition
 * @param x  One or more right-hand-sides, replaced by solution
 * @param p  N-integer array to be filled with the row permutations 
 *
 * @see cpl_matrix_solve()
 * 
 */
cpl_error_code cpl_matrix_solve_(cpl_matrix * a,
                                 cpl_matrix * x,
                                 cpl_array  * p)
{
    const cpl_size n = cpl_array_get_size(p);
    int            sig;

    assert(a->nc == n);
    assert(a->nr == n);
    assert(x->nr == n);

    if (cpl_matrix_decomp_lu(a, p, &sig)) {
        return cpl_error_set_where_();
    }

    /* Should not be able to fail at this point */
    if (cpl_matrix_solve_lu(a, x, p)) {
        return cpl_error_set_where_();
    }

    return CPL_ERROR_NONE;
}

/**@}*/
