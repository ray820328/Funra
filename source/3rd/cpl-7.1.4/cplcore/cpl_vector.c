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

#include "cpl_vector_impl.h"

#include "cpl_error_impl.h"
#include "cpl_memory.h"
#include "cpl_matrix.h"
#include "cpl_vector_fit_impl.h"
#include "cpl_tools.h"
#include "cpl_math_const.h"
#include "cpl_io_fits.h"
#include "cpl_image_fft_impl.h"
#include "cpl_propertylist_impl.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <float.h>

/* Needed by memcpy(), memset() */
#include <string.h>

#include <fitsio.h>


/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_vector Vector
 *
 * This module provides functions to handle @em cpl_vector.
 *
 * A @em cpl_vector is an object containing a list of values (as doubles)
 * and the (always positive) number of values. The functionalities provided
 * here are simple ones like sorting, statistics, or simple operations.
 * The @em cpl_bivector object is composed of two of these vectors.
 * No special provisions are made to handle special values like NaN or Inf,
 * for data with such elements, the @em cpl_array object may be preferable.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_vector.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                   Type definition
 -----------------------------------------------------------------------------*/

struct _cpl_vector_ {
    cpl_size    size;
    double  *   data;
};


/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static cpl_vector * cpl_vector_gen_lowpass_kernel(cpl_lowpass,
                                                  cpl_size) CPL_ATTR_ALLOC;
static double cpl_tools_sinc(double) CPL_ATTR_CONST;
static cpl_error_code cpl_vector_fill_tanh_kernel(cpl_vector *, double);
static void cpl_vector_fill_alpha_kernel(cpl_vector *, double,
                                         double) CPL_ATTR_NONNULL;
static void cpl_vector_reverse_tanh_kernel(double *, cpl_size) CPL_ATTR_NONNULL;
static int cpl_fit_gaussian_1d_compare(const void *left, const void *right)
    CPL_ATTR_PURE CPL_ATTR_NONNULL;
static int gauss(const double x[], const double a[],
                 double *result) CPL_ATTR_NONNULL;
static int gauss_derivative(const double x[], const double a[],
                            double result[]) CPL_ATTR_NONNULL;

inline
static double cpl_erf_antideriv(double, double) CPL_ATTR_CONST;

static cpl_error_code cpl_vector_fill_lss_profile_symmetric(cpl_vector *,
                                                            double,
                                                            double);

/*-----------------------------------------------------------------------------
                               Private types
 -----------------------------------------------------------------------------*/
typedef struct {
    double x;
    double y;
} cpl_vector_fit_gaussian_input;


/*-----------------------------------------------------------------------------
                              Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a new cpl_vector
  @param    n    Number of element of the cpl_vector
  @return   1 newly allocated cpl_vector or NULL in case of an error

  The returned object must be deallocated using cpl_vector_delete().
  There is no default values assigned to the created object, they are
  undefined until they are set.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if n is negative or zero
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_new(cpl_size n)
{
    cpl_vector * self = NULL;

    if (n <= 0) {
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                     "n=%" CPL_SIZE_FORMAT " < 1", n);
    } else {
        /* Allocate memory */
        self = (cpl_vector*)cpl_malloc(sizeof(cpl_vector));
        self->size = n;
        self->data = (double*)cpl_malloc((size_t)n * sizeof(double));
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a cpl_vector from existing data
  @param    n       Number of elements in the vector
  @param    data    Pointer to array of n doubles
  @return   1 newly allocated cpl_vector or NULL on error
  @note The returned object must be deallocated using e.g. cpl_vector_unwrap()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if n is negative or zero
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_wrap(cpl_size n, double * data)
{
    cpl_vector * self = NULL;

    if (n <= 0) {
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                     "n=%" CPL_SIZE_FORMAT " < 1", n);
    } else if (data == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "data");
    } else {
        self = cpl_malloc(sizeof(cpl_vector));
        self->size = n;
        self->data = data;
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Replace the buffer in a CPL vector with a new one
  @param    self    CPL vector to process
  @param    n       Number of elements in the vector
  @param    data    Pointer to array of n doubles
  @return   A pointer to the old data array or NULL on error
  @note On error self is unmodified
  @see cpl_vector_set_size()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if n is negative or zero
 */
/*----------------------------------------------------------------------------*/
double * cpl_vector_rewrap(cpl_vector * self, cpl_size n, double * data)
{
    double * old = NULL;

    if (self == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "self");
    } else if (n <= 0) {
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                     "n=%" CPL_SIZE_FORMAT " < 1", n);
    } else if (data == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "data");
    } else {
        old = self->data; /* Return old buffer */
        self->size = n;
        self->data = data;
    }

    return old;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Delete a cpl_vector
  @param    v    cpl_vector to delete
  @return   void

  If the vector @em v is @c NULL, nothing is done and no error is set.

 */
/*----------------------------------------------------------------------------*/
void cpl_vector_delete(cpl_vector * v)
{
    if (v == NULL) return;
    if (v->data != NULL) cpl_free(v->data);
    cpl_free(v);
    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Delete a cpl_vector except the data array
  @param    v    cpl_vector to delete
  @return   A pointer to the data array or NULL if the input is NULL.

  @note
     The data array must subsequently be deallocated. Failure to do so will
     result in a memory leak.

 */
/*----------------------------------------------------------------------------*/
void * cpl_vector_unwrap(cpl_vector * v)
{
    void * data;

    if (v == NULL) return NULL;

    data = (void *) v->data;
    cpl_free(v);

    return data;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Read a list of values from an ASCII file and create a cpl_vector
  @param    filename  Name of the input ASCII file
  @return   1 newly allocated cpl_vector or NULL in case of an error
  @see cpl_vector_dump

  Parse an input ASCII file values and create a cpl_vector from it
  Lines beginning with a hash are ignored, blank lines also.
  In valid lines the value is preceeded by an integer, which is ignored.

  The returned object must be deallocated using cpl_vector_delete()

  In addition to normal files, FIFO (see man mknod) are also supported.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if the file cannot be read
  - CPL_ERROR_BAD_FILE_FORMAT if the file contains no valid lines
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_read(const char * filename)
{
    FILE       * in;
    cpl_vector * v = NULL;

    cpl_ensure(filename != NULL, CPL_ERROR_NULL_INPUT, NULL);

    /* Open the file */
    errno = 0;
    in = fopen(filename, "r");

    if (in != NULL) {

        cpl_size     np = 0;
        cpl_size     size = 1000; /* Default size */
        char         line[1024];
        double       x;
        int          error;

        /* Create and fill the vector */
        v = cpl_vector_new(size);

        while (fgets(line, 1024, in) != NULL) {
            if (line[0] != '#' && sscanf(line, "%*d %lg", &x) == 1) {
                /* Found new element
                   - increase vector size if necessary,
                   - insert element at end and
                   - increment size counter */
                if (np == size) cpl_vector_set_size(v, size *= 2);
                cpl_vector_set(v, np++, x);
            }
        }

        /* Check that the loop ended due to eof and not an error */
        error = ferror(in);
        fclose(in);

        if (error) {
            cpl_vector_delete(v);
            v = NULL;
            (void)cpl_error_set_(CPL_ERROR_FILE_IO);
        } else if (np == 0) {
            cpl_vector_delete(v);
            v = NULL;
            (void)cpl_error_set_(CPL_ERROR_BAD_FILE_FORMAT);
        } else if (cpl_vector_set_size(v, np)) {
            cpl_vector_delete(v);
            v = NULL;
            (void)cpl_error_set_where_();
        }

    } else if (errno != 0) {
        (void)cpl_error_set_message_(CPL_ERROR_FILE_IO, "File=%s. Error: %s",
                                     filename, strerror(errno));
    } else {
        (void)cpl_error_set_message_(CPL_ERROR_FILE_IO, "File=%s", filename);
    }

    return v;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a cpl_vector as ASCII to a stream
  @param    v       Input cpl_vector to dump
  @param    stream  Output stream, accepts @c stdout or @c stderr
  @return   void

  Each element is preceded by its index number (starting with 1!) and
  written on a single line.

  Comment lines start with the hash character.

  stream may be NULL in which case @c stdout is used.

  @note
    In principle a cpl_vector can be saved using cpl_vector_dump() and
    re-read using cpl_vector_read(). This will however introduce significant
    precision loss due to the limited accuracy of the ASCII representation.

 */
/*----------------------------------------------------------------------------*/
void cpl_vector_dump(
        const cpl_vector * v,
        FILE             * stream)
{
    cpl_size i;


    if (stream == NULL) stream = stdout;

    fprintf(stream, "#----- vector -----\n");

    if (v == NULL) {
        fprintf(stream, "#NULL vector\n");
        return;
    }

    fprintf(stream, "#Index\t\tX\n");
    for (i = 0; i<v->size; i++)
        fprintf(stream, "%" CPL_SIZE_FORMAT "\t\t%g\n", i+1, v->data[i]);

    fprintf(stream, "#------------------\n");

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Load a list of values from a FITS file
  @param    filename  Name of the input file
  @param    xtnum     Extension number in the file (0 for primary HDU)
  @return   1 newly allocated cpl_vector or NULL in case of an error
  @see cpl_vector_save

  This function loads a vector from a FITS file (NAXIS=1), using cfitsio.
  The returned image has to be deallocated with cpl_vector_delete().

  'xtnum' specifies from which extension the vector should be loaded.
  This could be 0 for the main data section or any number between 1 and N,
  where N is the number of extensions present in the file.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the extension is not valid
  - CPL_ERROR_FILE_IO if the file cannot be read
  - CPL_ERROR_UNSUPPORTOED_MODE if the file is too large to be read
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_load(const char * filename, cpl_size xtnum)
{
    const int             ixtnum = (int)xtnum;
    fitsfile            * fptr;
    int                   error = 0;
    void                * data;
    CPL_FITSIO_TYPE       nx;
    const CPL_FITSIO_TYPE fpixel[1] = {1};
    int                   naxis;

    /* Test entries */
    cpl_ensure(filename         != NULL,  CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(xtnum            >= 0,     CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure((cpl_size)ixtnum == xtnum, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Open the file */
    if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_open_diskfile,
                                 "filename='%s', xtnum=%" CPL_SIZE_FORMAT,
                                 filename, xtnum);
        return NULL;
    }

    /* The open call may be reusing file handle opened for
       previous I/O, so the file pointer needs to be moved also for xtnum = 0 */
    if (fits_movabs_hdu(fptr, ixtnum + 1, NULL, &error)) {
        int error2 = 0;
        cpl_io_fits_close_file(fptr, &error2);
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_movabs_hdu,
                                 "filename='%s', xtnum=%" CPL_SIZE_FORMAT,
                                 filename, xtnum);
        return NULL;
    }

    /* Get the HDU dimension */
    if (fits_get_img_dim(fptr, &naxis, &error)) {
        int error2 = 0;
        cpl_io_fits_close_file(fptr, &error2);
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_get_img_dim,
                                 "filename='%s', xtnum=%" CPL_SIZE_FORMAT,
                                 filename, xtnum);
        return NULL;
    }
    if (naxis != 1) {
        cpl_io_fits_close_file(fptr, &error);
        (void)cpl_error_set_message_(CPL_ERROR_BAD_FILE_FORMAT,
                                     "filename='%s', xtnum=%" CPL_SIZE_FORMAT
                                     ", naxis=%d", filename, xtnum, naxis);
        return NULL;
    }
    if (CPL_FITSIO_GET_SIZE(fptr, 1, &nx, &error)) {
        int error2 = 0;
        cpl_io_fits_close_file(fptr, &error2);
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, CPL_FITSIO_GET_SIZE,
                                 "filename='%s', xtnum=%" CPL_SIZE_FORMAT,
                                 filename, xtnum);
        return NULL;
    }
    if (nx < 1) {
        cpl_io_fits_close_file(fptr, &error);
        (void)cpl_error_set_message_(CPL_ERROR_BAD_FILE_FORMAT, "file"
                                    "name='%s', xtnum=%" CPL_SIZE_FORMAT ", "
                                    "nx=%lld", filename, xtnum, (long long)nx);
        return NULL;
    }

    /* FIXME: Change 1st (unlikely) if-clause to ifdef */
    if (sizeof(nx) > sizeof(cpl_size) && nx != (cpl_size)nx) {
        cpl_io_fits_close_file(fptr, &error);
        (void)cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE, "file"
                                     "name='%s', xtnum=%" CPL_SIZE_FORMAT ", "
                                     "nx=%lld", filename, xtnum, (long long)nx);
        return NULL;
    }

    /* Create the vector */
    data = cpl_malloc((size_t)nx * sizeof(double));

    /* Read */
    if (CPL_FITSIO_READ_PIX(fptr, TDOUBLE, fpixel, (LONGLONG)nx, NULL, data,
                            NULL, &error)) {
        int error2 = 0;
        cpl_io_fits_close_file(fptr, &error2);
        cpl_free(data);
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, CPL_FITSIO_READ_PIX_E,
                                 "filename='%s', xtnum=%" CPL_SIZE_FORMAT
                                 ", nx=%lld", filename, xtnum, (long long)nx);
        return NULL;
    }

    if (cpl_io_fits_close_file(fptr, &error)) {
        cpl_free(data);
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_close_file,
                                 "filename='%s', xtnum=%" CPL_SIZE_FORMAT
                                 ", nx=%lld", filename, xtnum, (long long)nx);
        return NULL;
    }

    return cpl_vector_wrap(nx, (double*)data);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Save a vector to a FITS file
  @param    self          Vector to write to disk or NULL
  @param    filename      Name of the file to write
  @param    type          The type used to represent the data in the file
  @param    pl            Property list for the output header or NULL
  @param    mode          The desired output options (combined with bitwise or)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  This function saves a vector to a FITS file (NAXIS=1), using cfitsio.
  If a property list is provided, it is written to the named file before the
  pixels are written.
  If the image is not provided, the created file will only contain the
  primary header. This can be useful to create multiple extension files.

  The type used in the file can be one of:
     CPL_TYPE_UCHAR  (8 bit unsigned),
     CPL_TYPE_SHORT  (16 bit signed),
     CPL_TYPE_USHORT (16 bit unsigned),
     CPL_TYPE_INT    (32 bit signed),
     CPL_TYPE_FLOAT  (32 bit floating point), or
     CPL_TYPE_DOUBLE (64 bit floating point).
  Use CPL_TYPE_DOUBLE when no loss of information is required.

  Supported output modes are CPL_IO_CREATE (create a new file) and
  CPL_IO_EXTEND  (append to an existing file)

  If you are in append mode, make sure that the file has writing
  permissions. You may have problems if you create a file in your
  application and append something to it with the umask set to 222. In
  this case, the file created by your application would not be writable,
  and the append would fail.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the type or the mode is not supported
  - CPL_ERROR_FILE_NOT_CREATED if the output file cannot be created
  - CPL_ERROR_FILE_IO if the data cannot be written to the file
  - CPL_ERROR_UNSUPPORTOED_MODE if the file is too large to be saved
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_save(const cpl_vector       * self,
                               const char             * filename,
                               cpl_type                 type,
                               const cpl_propertylist * pl,
                               unsigned                 mode)
{

    if (self == NULL) {
        return cpl_propertylist_save(pl, filename, mode)
            ? cpl_error_set_where_() : CPL_ERROR_NONE;
    } else {
        fitsfile   * fptr;
        int     error = 0;
        const char * badkeys = mode & CPL_IO_EXTEND
            ? CPL_FITS_BADKEYS_EXT  "|" CPL_FITS_COMPRKEYS
            : CPL_FITS_BADKEYS_PRIM "|" CPL_FITS_COMPRKEYS;

        /* Create the vector in the primary unit */
        const CPL_FITSIO_TYPE naxes[1] = {self->size};
        const int  bpp = cpl_tools_get_bpp(type);


        cpl_ensure_code(filename != NULL, CPL_ERROR_NULL_INPUT);

        cpl_ensure_code(bpp, CPL_ERROR_ILLEGAL_INPUT);

        cpl_ensure_code(mode == CPL_IO_CREATE || mode == CPL_IO_EXTEND,
                        CPL_ERROR_ILLEGAL_INPUT);

        cpl_ensure_code(sizeof(CPL_FITSIO_TYPE) >= sizeof(cpl_size) ||
                        (cpl_size)naxes[0] == self->size,
                        CPL_ERROR_UNSUPPORTED_MODE);

        if (mode & CPL_IO_EXTEND) {
            /* Open the file */
            if (cpl_io_fits_open_diskfile(&fptr, filename, READWRITE, &error)) {
                return cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                          fits_open_diskfile, "filename='%s', "
                                          "type=%d, mode=%u", filename, type,
                                          mode);
            }
        } else {
            /* Create the file */
            char * sval = cpl_sprintf("!%s", filename);
            cpl_io_fits_create_file(&fptr, sval, &error);
            cpl_free(sval);
            if (error) {
                return cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                          fits_create_file, "filename='%s', "
                                          "type=%d, mode=%u", filename, type,
                                          mode);
            }
        }

        if (CPL_FITSIO_CREATE_IMG(fptr, bpp, 1, naxes, &error)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                      CPL_FITSIO_CREATE_IMG_E, "filename='%s', "
                                      "type=%d, mode=%u", filename, type, mode);
        }

        /* Add DATE */
        if ((mode & CPL_IO_CREATE) && fits_write_date(fptr, &error)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                      fits_write_date, "filename='%s', "
                                      "type=%d, mode=%u", filename, type, mode);
        }

        /* Add the property list */
        if (cpl_fits_add_properties(fptr, pl, badkeys)) {
            return cpl_io_fits_close_file(fptr, &error)
                ? cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                     fits_close_file, "filename='%s', "
                                     "type=%d, mode=%u", filename, type, mode)

                : cpl_error_set_where_();
        }

        if (fits_write_img(fptr, TDOUBLE, 1, (LONGLONG)self->size,
                           (void*)self->data, &error)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                      fits_write_img, "filename='%s', "
                                      "type=%d, mode=%u", filename, type, mode);
        }


        /* Close (and write to disk) */
        return cpl_io_fits_close_file(fptr, &error)
            ? cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                 fits_close_file, "filename='%s', "
                                 "type=%d, mode=%u", filename, type, mode)
            : CPL_ERROR_NONE;
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    This function duplicates an existing vector and allocates memory
  @param    v   the input cpl_vector
  @return   a newly allocated cpl_vector or NULL in case of an error

  The returned object must be deallocated using cpl_vector_delete()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
*/
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_duplicate(const cpl_vector * v)
{
    cpl_vector      *   out;

    /* Test inputs */
    cpl_ensure(v != NULL,    CPL_ERROR_NULL_INPUT, NULL);

    /* Create a new cpl_vector  */
    out = cpl_vector_new(v->size);

    /* Copy the contents */
    cpl_vector_copy(out, v);

    return out;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    This function copies contents of a vector into another vector
  @param    destination     destination cpl_vector
  @param    source          source cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_vector_set_size() if source and destination have different sizes.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_copy(
        cpl_vector          *   destination,
        const cpl_vector    *   source)
{

    cpl_ensure_code(source      != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(!cpl_vector_set_size(destination, source->size),
                    cpl_error_get_code());

    memcpy(destination->data, source->data,
           (size_t)source->size * sizeof(double));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the size of the vector
  @param    in      the input vector
  @return   The size or -1 in case of an error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_vector_get_size(const cpl_vector * in)
{
    /* Test entries */
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    return cpl_vector_get_size_(in);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get the size of the vector
  @param    self      the input vector
  @return   The size or -1 in case of an error
  @see cpl_vector_get_size()
  @note No error checking in this internal function

 */
/*----------------------------------------------------------------------------*/
inline cpl_size cpl_vector_get_size_(const cpl_vector * self)
{
    return self->size;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Resize the vector
  @param    in       The vector to be resized
  @param    newsize  The new (positive) number of elements in the vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  @note
     On succesful return the value of the elements of the vector is
     unchanged to the minimum of the old and new sizes; any newly allocated
     elements are undefined.
     The pointer to the vector data buffer may change, therefore
     pointers previously retrieved by calling @c cpl_vector_get_data()
     should be discarded.
     If the vector was created with cpl_vector_wrap() the argument pointer
     to that call should be discarded as well.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if newsize is negative or zero
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_set_size(cpl_vector * in, cpl_size newsize)
{

    cpl_ensure_code(in != NULL,  CPL_ERROR_NULL_INPUT);

    if (in->size != newsize) {
        cpl_ensure_code(newsize > 0, CPL_ERROR_ILLEGAL_INPUT);

        in->data = (double*)cpl_realloc(in->data,
                                        (size_t)newsize * sizeof(double));

        in->size = newsize;
    }

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the data part of the vector
  @param    in      the input vector
  @return   Pointer to the data or NULL in case of an error

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
double * cpl_vector_get_data(cpl_vector * in)
{
    /* Test entries */
    cpl_ensure(in != NULL, CPL_ERROR_NULL_INPUT, NULL);

    return cpl_vector_get_data_(in);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get a pointer to the data part of the vector
  @param    self      the input vector
  @return   Pointer to the data or NULL in case of an error
  @see cpl_vector_get_data()
  @note No error checking in this internal function

 */
/*----------------------------------------------------------------------------*/
inline double * cpl_vector_get_data_(cpl_vector * self)
{
    assert( self->data != NULL);

    return self->data;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the data part of the vector
  @param    in      the input vector
  @return   Pointer to the data or NULL in case of an error
  @see cpl_vector_get_data

 */
/*----------------------------------------------------------------------------*/
const double * cpl_vector_get_data_const(const cpl_vector * in)
{
    /* Test entries */
    cpl_ensure(in != NULL, CPL_ERROR_NULL_INPUT, NULL);

    return cpl_vector_get_data_const_(in);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get a pointer to the data part of the vector
  @param    self      the input vector
  @return   Pointer to the data or NULL in case of an error
  @see cpl_vector_get_data_()
  @note No error checking in this internal function

 */
/*----------------------------------------------------------------------------*/
inline const double * cpl_vector_get_data_const_(const cpl_vector * self)
{
    assert( self->data != NULL);

    return self->data;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get an element of the vector
  @param    in      the input vector
  @param    idx     the index of the element (0 to nelem-1)
  @return   The element value

  In case of error, the #_cpl_error_code_ code is set, and the returned double
  is undefined.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if idx is negative
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if idx is out of the vector bounds
 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get(const cpl_vector * in, cpl_size idx)
{

    /* Test entries */
    cpl_ensure(in,             CPL_ERROR_NULL_INPUT, -1.0);
    cpl_ensure(idx >= 0,       CPL_ERROR_ILLEGAL_INPUT, -2.0);
    cpl_ensure(idx < in->size, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3.0);

    return cpl_vector_get_(in, idx);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get an element of the vector
  @param    in      the input vector
  @param    idx     the index of the element (0 to nelem-1)
  @return   The element value
  @see cpl_vector_get()
  @note No error checking in this internal function
 */
/*----------------------------------------------------------------------------*/
inline double cpl_vector_get_(const cpl_vector * in, cpl_size idx)
{
    assert( in->data );

    return in->data[idx];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Set an element of the vector
  @param    in      the input vector
  @param    idx   the index of the element (0 to nelem-1)
  @param    value   the value to set in the vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if idx is negative
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if idx is out of the vector bounds
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_set(
        cpl_vector * in,
        cpl_size     idx,
        double       value)
{

    /* Test entries */
    cpl_ensure_code(in != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(idx >= 0,       CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(idx < in->size, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    /* Assign the value */
    cpl_vector_set_(in, idx, value);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Set an element of the vector
  @param    in      the input vector
  @param    idx   the index of the element (0 to nelem-1)
  @param    value   the value to set in the vector
  @return   Nothing
  @see cpl_vector_get()
  @note No error checking in this internal function
 */
/*----------------------------------------------------------------------------*/
inline void cpl_vector_set_(cpl_vector * in,
                            cpl_size     idx,
                            double       value)
{
    assert( in->data );

    /* Assign the value */
    in->data[idx] = value;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Add a cpl_vector to another
  @param    v1  First cpl_vector (modified)
  @param    v2  Second cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  The second vector is added to the first one. The input first vector is
  modified.

  The input vectors must have the same size.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if v1 and v2 have different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_add(
        cpl_vector       * v1,
        const cpl_vector * v2)
{
    cpl_size            i;

    /* Test inputs */
    cpl_ensure_code(v1 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v2 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v1->size == v2->size, CPL_ERROR_INCOMPATIBLE_INPUT);

    /* Add v2 to v1 */
    for (i = 0; i < v1->size; i++) v1->data[i] += v2->data[i];

    cpl_tools_add_flops(v1->size);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Subtract a cpl_vector from another
  @param    v1  First cpl_vector (modified)
  @param    v2  Second cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see      cpl_vector_add()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_subtract(
        cpl_vector       * v1,
        const cpl_vector * v2)
{
    cpl_size     i;

    /* Test inputs */
    cpl_ensure_code(v1 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v2 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v1->size == v2->size, CPL_ERROR_INCOMPATIBLE_INPUT);

    /* subtract v2 from v1 */
    for (i = 0; i < v1->size; i++) v1->data[i] -= v2->data[i];

    cpl_tools_add_flops(v1->size);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Multiply two vectors component-wise
  @param    v1        First cpl_vector
  @param    v2        Second cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see      cpl_vector_add()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_multiply(
        cpl_vector        * v1,
        const cpl_vector  * v2)
{
    cpl_size            i;

    /* Test inputs   */
    cpl_ensure_code(v1 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v2 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v1->size == v2->size, CPL_ERROR_INCOMPATIBLE_INPUT);

    /* Multiply the cpl_vector fields */
    for (i = 0; i < v1->size; i++) v1->data[i] *= v2->data[i];

    cpl_tools_add_flops(v1->size);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Divide two vectors element-wise
  @param    v1  First cpl_vector
  @param    v2  Second cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see      cpl_vector_add()

  If an element in v2 is zero v1 is not modified and CPL_ERROR_DIVISION_BY_ZERO
  is returned.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if v1 and v2 have different sizes
  - CPL_ERROR_DIVISION_BY_ZERO if a division by 0 would occur
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_divide(
        cpl_vector       * v1,
        const cpl_vector * v2)
{
    cpl_size            i;

    /* Test inputs   */
    cpl_ensure_code(v1 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v2 != NULL,           CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(v1->size == v2->size, CPL_ERROR_INCOMPATIBLE_INPUT);

    for (i = 0; i < v2->size; i++)
        cpl_ensure_code(v2->data[i] != 0, CPL_ERROR_DIVISION_BY_ZERO);

    /* Divide the cpl_vector fields */
    for (i = 0; i < v1->size; i++) v1->data[i] /= v2->data[i];

    cpl_tools_add_flops(v1->size);

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Perform a cyclic shift to the right of the elements of a vector
  @param    self  Vector to hold the shifted result
  @param    other Vector to read from, or NULL for in-place shift of self
  @param    shift The number of positions to cyclic right-shift
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_fft_image()
  @note Passing two distinct vectors (partially) wrapped around the same
        buffer is not supported and will lead to undefined behaviour.

  A shift of +1 will move the last element to the first, a shift of -1 will
  move the first element to the last, a zero-shift will perform a copy (or
  do nothing in case of an in-place operation).

  A non-integer shift will perform the shift in the Fourier domain. Large
  discontinuities in the vector to shift will thus lead to FFT artifacts
  around each discontinuity.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the self pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if self and other have different sizes
  - CPL_ERROR_UNSUPPORTED_MODE if the shift is non-integer and FFTW is
    unavailable
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_cycle(cpl_vector * self,
                                const cpl_vector * other,
                                double shift)
{
    const cpl_size n      = cpl_vector_get_size(self);
    const cpl_size nshift = (cpl_size)shift % (cpl_size)n;
    const cpl_size ishift = nshift < 0 ? n + nshift : nshift;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    assert(ishift >= 0);

    if (other != NULL && cpl_vector_get_size(other) != n) {
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    } else if (shift != floor(shift)) {
        cpl_image* imgself = cpl_image_wrap_double(n, 1,
                                                   cpl_vector_get_data(self));
        cpl_image* imgother;
        cpl_error_code code;

        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
        imgother = other ? cpl_image_wrap_double(n, 1, cpl_vector_get_data
                                                 ((cpl_vector*)other))
            : imgself;
        CPL_DIAG_PRAGMA_POP;

        code = cpl_fft_image_cycle(imgself, imgother, shift, 0.0);

        assert(!code);

        if (imgother != imgself) (void)cpl_image_unwrap(imgother);
        (void)cpl_image_unwrap(imgself);
    } else if (other == NULL || self == other) {
        /* Perform integer in-place shift of self */
        const cpl_size irem = n - ishift;
        double* copy = NULL;

        assert(irem > 0);

        /* Either of the two branches will work in all cases, the first one
           is just there to reduce the lengths of the memcpy() calls */
        if (ishift > irem) {
            /* A large shift, copy the first (smaller) part of the buffer */
            copy = (double*)cpl_malloc((size_t)irem * sizeof(*copy));

            memcpy(copy, self->data, irem * sizeof(*copy));
            memmove(self->data, self->data + irem, ishift * sizeof(double));
            memcpy(self->data + ishift, copy, irem * sizeof(double));

        } else if (ishift > 0) {
            /* A small shift, copy the last (smaller) part of the buffer */
            copy = (double*)cpl_malloc((size_t)ishift * sizeof(*copy));

            memcpy(copy, self->data + irem, ishift * sizeof(*copy));
            memmove(self->data + ishift, self->data, irem * sizeof(double));
            memcpy(self->data, copy, ishift * sizeof(double));

        }
        cpl_free(copy);
    } else if (ishift > 0) {
        /* Perform integer shift of self to other */
        const cpl_size irem = n - ishift;

        assert(irem > 0);

        (void)memcpy(self->data + ishift, other->data, irem * sizeof(double));
        (void)memcpy(self->data, other->data + irem, ishift * sizeof(double));
    } else {
        /* This is just a copy */
        (void)memcpy(self->data, other->data, (size_t)n * sizeof(double));
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the vector dot product.
  @param    v1  One vector
  @param    v2  Another vector of the same size
  @return   The (non-negative) product or negative on error.

  The same vector may be passed twice, in which case the square of its 2-norm
  is computed.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if v1 and v2 have different sizes
 */
/*----------------------------------------------------------------------------*/
double cpl_vector_product(const cpl_vector * v1, const cpl_vector * v2)
{
    const double * x     = cpl_vector_get_data_const(v1);
    const double * y     = cpl_vector_get_data_const(v2);
    const cpl_size n     = cpl_vector_get_size(v1);
    double sum = 0;
    cpl_size i;


    cpl_ensure(x, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(y, CPL_ERROR_NULL_INPUT, -2);
    cpl_ensure(cpl_vector_get_size(v2) == n, CPL_ERROR_INCOMPATIBLE_INPUT,
               -3);

    for (i = 0; i < n; i++) sum += x[i] * y[i];

    cpl_tools_add_flops(2*n);

    return sum;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Sort a cpl_vector
  @param    self  cpl_vector to sort in place
  @param    dir   CPL_SORT_ASCENDING or CPL_SORT_DESCENDING
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  The input cpl_vector is modified to sort its values in either ascending
  (CPL_SORT_ASCENDING) or descending (CPL_SORT_DESCENDING) order.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if dir is neither CPL_SORT_DESCENDING nor
                            CPL_SORT_ASCENDING
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_sort(cpl_vector         * self,
                               cpl_sort_direction   dir)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    if (dir == CPL_SORT_ASCENDING) {

        /* Sort by increasing data */
        if (cpl_tools_sort_double(self->data, self->size)) {
            return cpl_error_set_where_();
        }

    } else if (dir == CPL_SORT_DESCENDING) {

        /* Sort by decreasing data */
        cpl_tools_sort_desc_double(self->data, self->size);

    } else {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Fill a cpl_vector
  @param    v      cpl_vector to be filled with the value val
  @param    val    Value used to fill the cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Input vector is modified

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_fill(cpl_vector * v, double val)
{

    cpl_ensure_code(v, CPL_ERROR_NULL_INPUT);

    if (val != 0.0) {
        for (size_t i = 0; i < (size_t)v->size; i++) v->data[i] = val;
    } else {
        (void)memset(v->data, 0, (size_t)v->size * sizeof(*v->data));
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the sqrt of a cpl_vector
  @param    v    cpl_vector
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  The sqrt of the data is computed. The input cpl_vector is modified

  If an element in v is negative v is not modified and CPL_ERROR_ILLEGAL_INPUT
  is returned.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if one of the vector values is negative
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_sqrt(cpl_vector * v)
{
    cpl_size          i;

    /* Test inputs */
    cpl_ensure_code(v!=NULL,             CPL_ERROR_NULL_INPUT);

    for (i = 0; i<v->size; i++)
        cpl_ensure_code(v->data[i] >= 0, CPL_ERROR_ILLEGAL_INPUT);

    /* Compute the sqrt */
    for (i = 0; i < v->size; i++) v->data[i] = sqrt(v->data[i]);

    cpl_tools_add_flops(v->size);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief In a sorted vector find the element closest to the given value
  @param sorted  CPL vector sorted using CPL_SORT_ASCENDING
  @param key     Value to find
  @return The index that minimizes fabs(sorted[index] - key) or
          negative on error
  @see cpl_vector_sort()

  Bisection is used to find the element.

  If two (neighboring) elements with different values both minimize
  fabs(sorted[index] - key) the index of the larger element is returned.

  If the vector contains identical elements that minimize
  fabs(sorted[index] - key) then it is undefined which element has its index
  returned.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if two elements are found to not be sorted
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_vector_find(const cpl_vector * sorted,
                         double             key)
{
    cpl_size low, high;

    /* Test inputs */
    cpl_ensure(sorted != NULL, CPL_ERROR_NULL_INPUT, -1);

    /* Initialize */
    low  = 0;
    high = sorted->size - 1;

    cpl_ensure(sorted->data[low] <= sorted->data[high], CPL_ERROR_ILLEGAL_INPUT,
               -2);

    if (key <= sorted->data[ low]) return  low;
    if (key >= sorted->data[high]) return high;

    /* key is in the range of the vector values */

    while (high - low > 1) {
        const cpl_size middle = low + (high - low) / 2; 

        /* Three or four comparisons are done per iteration,
           however with the non-localized access pattern data reads
           will likely dominate the cost */

        cpl_ensure(sorted->data[low] <= sorted->data[middle],
                   CPL_ERROR_ILLEGAL_INPUT, -3);
        cpl_ensure(sorted->data[middle] <= sorted->data[high],
                   CPL_ERROR_ILLEGAL_INPUT, -4);

        if (sorted->data[middle] > key) high = middle;
        else if (sorted->data[middle] < key) low  = middle;
        else low = high = middle;
    }

    /* assert( low == high || low == high - 1); */

    return key - sorted->data[low] < sorted->data[high] - key ? low : high;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Extract a sub_vector from a vector
  @param    v        Input cpl_vector
  @param    istart   Start index (from 0 to number of elements - 1)
  @param    istop    Stop  index (from 0 to number of elements - 1)
  @param    istep    Extract every step element
  @return   A newly allocated cpl_vector or NULL in case of an error

  The returned object must be deallocated using cpl_vector_delete()

  FIXME: Currently istop must be greater than istart.
  FIXME: Currently istep must equal 1.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if istart, istop, istep are not as requested
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_extract(
        const cpl_vector * v,
        cpl_size           istart,
        cpl_size           istop,
        cpl_size           istep)
{
    cpl_vector  *   out;

    /* Test inputs */
    cpl_ensure(v,                CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(istart >= 0,      CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);
    cpl_ensure(istart <= istop,  CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(istep == 1,       CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(istop  < v->size, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);

    /* Allocate and fill the output vector */
    out = cpl_vector_new(istop - istart + 1);

    assert( out );

    memcpy(out->data, &(v->data[istart]), (size_t)out->size * sizeof(double));

    return out;
}

/*----------------------------------------------------------------------------*/
/**
  @brief   Get the index of the minimum element of the cpl_vector
  @param   self    The vector to process
  @return  The index (0 for first) of the minimum value or negative on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_vector_get_minpos(const cpl_vector * self)
{
    size_t minpos = 0;

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, -1.0);

    for (size_t i = 1; i < (size_t)self->size; i++) {
        if (self->data[i] < self->data[minpos]) {
            minpos = i;
        }
    }

    return (cpl_size)minpos;
}

/*----------------------------------------------------------------------------*/
/**
  @brief   Get the index of the maximum element of the cpl_vector
  @param   self    The vector to process
  @return  The index (0 for first) of the maximum value or negative on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_vector_get_maxpos(const cpl_vector * self)
{
    size_t maxpos = 0;

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, -1.0);

    for (size_t i = 1; i < (size_t)self->size; i++) {
        if (self->data[i] > self->data[maxpos]) {
            maxpos = i;
        }
    }

    return (cpl_size)maxpos;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the minimum of the cpl_vector
  @param    v    const cpl_vector
  @return   The minimum value of the vector or undefined on error

  In case of error, the #_cpl_error_code_ code is set, and the returned double
  is undefined.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get_min(const cpl_vector * v)
{
    const cpl_size minpos = cpl_vector_get_minpos(v);

    if (minpos < 0) {
        (void)cpl_error_set_where_();
        return -1.0;
    }

    return v->data[minpos];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the maximum of the cpl_vector
  @param    v    const cpl_vector
  @return   the maximum value of the vector or undefined on error
  @see      cpl_vector_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get_max(const cpl_vector * v)
{
    const cpl_size maxpos = cpl_vector_get_maxpos(v);

    if (maxpos < 0) {
        (void)cpl_error_set_where_();
        return -1.0;
    }

    return v->data[maxpos];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the sum of the elements of the cpl_vector
  @param    v    const cpl_vector
  @return   the sum of the elements of the vector or undefined on error
  @see      cpl_vector_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get_sum(const cpl_vector * v)
{
    double          sum = 0.0;
    cpl_size        i;

    /* Test entries */
    cpl_ensure(v != NULL,   CPL_ERROR_NULL_INPUT, -1.0);

    for (i=0; i < v->size; i++) sum += v->data[i];

    return sum;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the mean value of vector elements.
  @param    v  Input const cpl_vector
  @return   Mean value of vector elements or undefined on error.
  @see      cpl_vector_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get_mean(const cpl_vector * v)
{
    cpl_ensure(v != NULL, CPL_ERROR_NULL_INPUT, -1.0);

    return cpl_tools_get_mean_double(v->data, v->size);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the median of the elements of a vector.
  @param    v  Input cpl_vector
  @return   Median value of the vector elements or undefined on error.
  @see      cpl_vector_get_median_const()
  @note For efficiency reasons, this function modifies the order of the elements
        of the input vector.

 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get_median(cpl_vector * v)
{
    cpl_ensure(v != NULL, CPL_ERROR_NULL_INPUT, 0.0);

    return cpl_tools_get_median_double(v->data, v->size);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the median of the elements of a vector.
  @param    v  Input const cpl_vector
  @return   Median value of the vector elements or undefined on error.
  @see      cpl_vector_get_min()

  For a finite population or sample, the median is the middle value of an odd
  number of values (arranged in ascending order) or any value between the two
  middle values of an even number of values. The criteria used for an even
  number of values in the input array is to choose the mean between the two
  middle values. Note that in this case, the median might not be a value
  of the input array. Also, note that in the case of integer data types, 
  the result will be converted to an integer. Consider to transform your int 
  array to float if that is not the desired behavior.  

 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get_median_const(const cpl_vector * v)
{
    double  *   darr;
    double      med;

    cpl_ensure(v != NULL, CPL_ERROR_NULL_INPUT, 0.0);

    /* Create a separate data buffer because  */
    /* cpl_tools_get_median_double() modifies its input */
    darr = (double*)cpl_malloc((size_t)v->size * sizeof(*darr));

    memcpy(darr, v->data, (size_t)v->size * sizeof(double));

    med = cpl_tools_get_median_double(darr, v->size);

    cpl_free(darr);

    return med;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the bias-corrected standard deviation of a vectors elements
  @param    v      Input const cpl_vector
  @return   standard deviation of the elements or a negative number on error.
  @see      cpl_vector_get_min()

  S(n-1) = sqrt((1/n-1) sum((xi-mean)^2) (i=1 -> n)

  The length of v must be at least 2.
 */
/*----------------------------------------------------------------------------*/
double cpl_vector_get_stdev(const cpl_vector * v)
{
    double varsum;


    cpl_ensure(v != NULL,   CPL_ERROR_NULL_INPUT,-1);
    cpl_ensure(v->size > 1, CPL_ERROR_ILLEGAL_INPUT,-2);

    varsum = cpl_tools_get_variancesum_double(v->data, v->size, NULL);

    /* Compute the bias-corrected standard deviation.
       - With the recurrence relation rounding can likely not cause
       the variance to become negative, but check just to be safe */
    return sqrt(varsum / (double) (v->size-1));
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Cross-correlation of two vectors
  @param    vxc   Odd-sized vector with the computed cross-correlations
  @param    v1    1st vector to correlate
  @param    v2    2nd vector to correlate
  @return   Index of maximum cross-correlation, or negative on error

  vxc must have an odd number of elements, 2*half_search+1, where half_search
  is the half-size of the search domain.

  The length of v2 may not exceed that of v1.
  If the difference in length between v1 and v2 is less than half_search
  then this difference must be even (if the difference is odd resampling
  of v2 may be useful).

  The cross-correlation is computed with shifts ranging from -half_search
  to half_search.

  On succesful return element i (starting with 0) of vxc contains the cross-
  correlation at offset i-half_search. On error vxc is unmodified.

  The cross-correlation is in fact the dot-product of two unit-vectors and
  ranges therefore from -1 to 1.

  The cross-correlation is, in absence of rounding errors, commutative only for
  equal-sized vectors, i.e. changing the order of v1 and v2 will move element j
  in vxc to 2*half_search - j and thus change the return value from i to
  2*half_search - i.

  If, in absence of rounding errors, more than one shift would give the maximum
  cross-correlation, rounding errors may cause any one of those shifts to be
  returned. If rounding errors have no effect the index corresponding to the
  shift with the smallest absolute value is returned (with preference given to
  the smaller of two indices that correspond to the same absolute shift).

  If v1 is longer than v2, the first element in v1 used for the resulting
  cross-correlation is
  max(0,shift + (cpl_vector_get_size(v1)-cpl_vector_get_size(v2))/2).

  Cross-correlation with half_search == 0 requires about 8n FLOPs, where
  n = cpl_vector_get_size(v2).
  Each increase of half_search by 1 requires about 4n FLOPs more, when all of
  v2's elements can be cross-correlated, otherwise the extra cost is about 4m,
  where m is the number of elements in v2 that can be cross-correlated,
  n - half_search <= m < n.

  In case of error, the #_cpl_error_code_ code is set, and the returned delta
  and cross-correlation is undefined.

  Example of 1D-wavelength calibration (error handling omitted for brevity):
  @code

  cpl_vector   * model = my_model(dispersion);
  cpl_vector   * vxc   = cpl_vector_new(1+2*maxshift);
  const cpl_size shift = cpl_vector_correlate(vxc, model, observed) - maxshift;
  cpl_error_code error = cpl_polynomial_shift_1d(dispersion, 0, (double)shift);

  cpl_msg_info(cpl_func, "Shifted dispersion relation by %" CPL_SIZE_FORMAT
               " pixels, shift);

  @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if v1 and v2 have different sizes or if vxc
    is not as requested
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_vector_correlate(cpl_vector * vxc, const cpl_vector * v1,
                              const cpl_vector * v2)
{

    /* FIXME:
       The API should be extended with a boolean
       @param is_cyclic Use a cyclic boundary => v1 and v2 must have equal size

       The API should be extended with an integer
       @param ishift2   Expected shift of v2 relative to v1. :-(((((((((((((

       This would allow the caller to specify an asymmetric search range,
       which would be very useful for example when x-correlating two nodded
       images of identical size each with their spatial direction collapsed
       into a 1D-spatial profile.

       Example:
       Two such spatial profiles of length 1000 may each contain a bright
       feature, one at pixel position 600 then other at 400, each with an
       uncertainty of hsearch=10. Define ishift2 = 600 - 400 = 200.
       In order to find the correlation maximum with the current API,
       the length of vxc must be
           1 + 2 * (hsearch + abs(ishift2)) = 421,
       but with ishift2 available inside cpl_vector_correlate(),
       the length of vxc can be kept at the optimal
           1 + 2 * hsearch = 21.

       The missing ishift2 causes not only a large computational overhead,
       but also a risk of finding a false maximum.

       Work-around, to avoid risk of a false maximum:
       Set the size of vxc to s = 1 + 2 * (hsearch + abs(ishift2)),
       and then extract the maximum cross-correlation from the elements at
           vxc[s/2 - ishift2 - hsearch] to vxc[s/2 - ishift2 + hsearch],
       i.e. either the first or the last 1 + 2 * hsearch elements of vxc.
    */

    double   xc    = 0.0;
    double   mean1 = 0.0;
    double   mean2 = 0.0;
    double   var1  = 0.0;
    double   var2  = 0.0;

    cpl_size half_search;
    cpl_size delta;

    cpl_size i;
    cpl_size i1;
    cpl_size size;

    cpl_ensure(vxc    != NULL,       CPL_ERROR_NULL_INPUT, -10);
    cpl_ensure(v1     != NULL,       CPL_ERROR_NULL_INPUT, -20);
    cpl_ensure(v2     != NULL,       CPL_ERROR_NULL_INPUT, -30);
    cpl_ensure(v1->size >= v2->size, CPL_ERROR_ILLEGAL_INPUT, -50);

    half_search = cpl_vector_get_size(vxc)-1;
    cpl_ensure(!(half_search & 1),   CPL_ERROR_ILLEGAL_INPUT, -60);
    half_search >>=1;

    size = v2->size; /* Number of elements used in cross-correlation */

    /* 1st v1 index used - non-zero iff v1 is longer than v2 */
    i1 = (v1->size - v2->size)/2;

    cpl_ensure(half_search <= i1 || 2*i1 == v1->size - v2->size,
                                     CPL_ERROR_ILLEGAL_INPUT, -70);

    /* The current implementation requires O(size * half_search) FLOPs.
       If size and half_search are large enough it should be preferable
       to compute the 1+2*half_search cross-correlations as a Toeplitz
       matrix vector product in terms of FFTs - since this requires
       O(size * log(size)) FLOPs */

    /* 1st v1 index not used :  i1 + size == (v1->size + size)/2 */

    /* Compute mean of vectors, normalization factors and cross-correlation
       - over those elements used in the no-shift cross-correlation */
#if 0
    /* This approach requires 10n FLOPS */
    for (i = 0; i<size; i++) {
        mean1 += v1->data[i+i1];
        mean2 += v2->data[i   ];
    }
    mean1 /= (double)size;
    mean2 /= (double)size;
    for (i = 0; i<size; i++) {
        var1 += (v1->data[i+i1] - mean1) * (v1->data[i+i1] - mean1);
        var2 += (v2->data[i   ] - mean2) * (v2->data[i   ] - mean2);
        xc   += (v1->data[i+i1] - mean1) * (v2->data[i   ] - mean2);
    }

    cpl_tools_add_flops(10 * size + 2);

#else
    /* This approach requires only 8n FLOPS.
       It is only faster if v1 and v2 are already in cache - or if
       they do not fit in the cache. The round-off can be 10 times bigger */
    for (i = 0; i<size; i++) {
        mean1 += v1->data[i+i1];
        mean2 += v2->data[i   ];
        var1  += v1->data[i+i1] * v1->data[i+i1];
        var2  += v2->data[i   ] * v2->data[i   ];
        xc    += v1->data[i+i1] * v2->data[i   ];
    }

    mean1 /= (double)size;
    mean2 /= (double)size;

    var1 -= mean1 * mean1 * (double)size;
    var2 -= mean2 * mean2 * (double)size;
    xc   -= mean1 * mean2 * (double)size;

    cpl_tools_add_flops(8 * size + 11);

#endif

    /* var can only be zero with a constant vector
       - in which case xc is zero */
    if ( var1 > 0.0 && var2 > 0.0) {
        xc /= sqrt(var1 * var2);

        /* xc can only be outside [-1;1] due to rounding errors */
        if (xc < -1.0) {
#ifndef NDEBUG
            const double m1 = fabs(mean1) > 1.0 ? fabs(mean1) : 1.0;
            const double m2 = fabs(mean2) > 1.0 ? fabs(mean2) : 1.0;
            assert( -1.0 - xc < (double)size * DBL_EPSILON * sqrt(m1 * m2));
#endif

            xc = -1.0;
        } else if (xc > 1.0) {
#ifndef NDEBUG
            const double m1 = fabs(mean1) > 1.0 ? fabs(mean1) : 1.0;
            const double m2 = fabs(mean2) > 1.0 ? fabs(mean2) : 1.0;
            assert( xc-1.0 < (double)size * DBL_EPSILON * sqrt(m1 * m2));
#endif

            xc =  1.0;
        }

    } else {
        /* Remove some rounding errors */
        if (var1 < 0) var1 = 0.0;
        if (var2 < 0) var2 = 0.0;
        xc = 0.0;
    }

    delta = 0;
    (void)cpl_vector_set(vxc, half_search, xc);

    if (half_search > 0) {

        /* less than maximal precision OK here */
        const double rsize = 1.0 / (double)size;
        const double dsize = 1.0 + rsize;
        double mean1_p = mean1;
        double mean1_n = mean1;
        double mean2_p = mean2;
        double mean2_n = mean2;

        /* The normalization factors: size * variance
           They are updated by use of the relation
           variance = sum of squares - square of sums / size
        */

        double var1_p = var1;
        double var1_n = var1;
        double var2_p = var2;
        double var2_n = var2;

        /* No use to search further than i1 + size - 1 */
        const cpl_size max_hs = i1 + size - 2;
        const cpl_size hs     = half_search > max_hs ? max_hs : half_search;
        const cpl_size hs1    = hs > i1 ? i1 : hs;

        /* Skip non-drop part if var2 == 0 */
        cpl_size step = var2 > 0 ? 1 : hs1 + 1;

        /* Number of elements in shortened cross-correlation */
        cpl_size istop = i1 + size - step;

        for (; step<=hs1; step++, istop--) {

            /* Can cross-correlate over all of v2
               - mean2 and var2 are unchanged */

            double xc_p = 0.0;
            double xc_n = 0.0;

            /* Subtract term from normalization factors */
            var1_p -= (v1->data[i1+step-1] * dsize - 2 * mean1_p)
                    *  v1->data[i1+step-1];

            var1_n -= (v1->data[istop    ] * dsize - 2 * mean1_n)
                    *  v1->data[istop    ];

            /* Update mean1 */
            mean1_p += (v1->data[i1+size+step-1] - v1->data[i1+step-1   ])
                     * rsize;
            mean1_n += (v1->data[i1-step       ] - v1->data[i1+size-step])
                     * rsize;

            /* Add term to normalization factors */
            var1_n += (v1->data[i1-step       ] * dsize - 2 * mean1_n)
                    *  v1->data[i1-step       ];

            var1_p += (v1->data[i1+size+step-1] * dsize - 2 * mean1_p)
                    *  v1->data[i1+size+step-1];

            /* Compute xc - exploiting that the dot-product is
                 distributive over subtraction */
            for (i = 0; i<size; i++) {
                xc_n += v2->data[i] * v1->data[i+i1-step];
                xc_p += v2->data[i] * v1->data[i+i1+step];
            }

            if (var1_n > 0.0) {
                /* Subtract the mean-term */
                xc_n -= mean2 * mean1_n * (double)size;
                /* - and divide by the norm of the mean-corrected vectors */
                xc_n /= sqrt(var1_n * var2);

                /* xc_n can only be outside [-1;1] due to rounding errors */
                if (xc_n < -1.0) {
#ifndef NDEBUG
                    const double m1 = fabs(mean1_n) > 1.0 ? fabs(mean1_n) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( -1.0 - xc_n < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_n = -1.0;
                } else if (xc_n > 1.0) { 
#ifndef NDEBUG
                    const double m1 = fabs(mean1_n) > 1.0 ? fabs(mean1_n) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( xc_n - 1.0 < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_n =  1.0;
                }

            } else
                xc_n = 0.0;

            if (xc_n > xc) {
                xc = xc_n;
                delta = -step;
            }
            (void)cpl_vector_set(vxc, half_search - step, xc_n);

            if (var1_p > 0.0) {
                /* Subtract the mean-term */
                xc_p -= mean2 * mean1_p * (double)size;
                /* - and divide by the norm of the mean-corrected vectors */
                xc_p /= sqrt(var1_p * var2);

                /* xc_p can only be outside [-1;1] due to rounding errors */
                if (xc_p < -1.0) {
#ifndef NDEBUG
                    const double m1 = fabs(mean1_p) > 1.0 ? fabs(mean1_p) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( -1.0 - xc_p < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_p = -1.0;
                } else if (xc_p > 1.0) {
#ifndef NDEBUG
                    const double m1 = fabs(mean1_p) > 1.0 ? fabs(mean1_p) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( xc_p - 1.0 < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_p =  1.0;
                }

            } else
                xc_p = 0.0;

            if (xc_p > xc) {
                xc = xc_p;
                delta = step;
            }

            (void)cpl_vector_set(vxc, half_search + step, xc_p);

        }
        assert( step == hs1 + 1 );

        if (var2 > 0) {
            cpl_tools_add_flops( hs1 * (4*size + 26) );
        }

        for (; step < 1 + hs; step++, istop--) {

            /* v1 is too short
                - Cross-correlate on reduced number of elements
                - elements out of range are defined to be zero */

            double xc_p = 0.0;
            double xc_n = 0.0;

            /* Subtract dropped term from normalization factors */
            var1_p -= (v1->data[step+i1-1] * dsize - 2 * mean1_p)
                    *  v1->data[step+i1-1];

            var1_n -= (v1->data[istop    ] * dsize - 2 * mean1_n)
                    *  v1->data[istop    ];

            var2_p -= (v2->data[istop    ] * dsize - 2 * mean2_p)
                    *  v2->data[istop    ];

            var2_n -= (v2->data[step-i1-1] * dsize - 2 * mean2_n)
                    *  v2->data[step-i1-1];

            /* Subtract dropped elements from mean */
            mean1_p -= v1->data[step+i1-1] * rsize;
            mean1_n -= v1->data[istop    ] * rsize;

            mean2_p -= v2->data[istop    ] * rsize;
            mean2_n -= v2->data[step-i1-1] * rsize;

            /* Compute xc */
            for (i = 0; i<istop; i++) {
                xc_n += v1->data[i] * v2->data[i-i1+step];
                xc_p += v2->data[i] * v1->data[i+i1+step];
            }

            if (var1_n * var2_n > 0) {
                /* Subtract the mean-term */
                xc_n -= mean1_n * mean2_n * (double)(2*size - istop);
                /* - and divide by the norm of the mean-corrected vectors */
                xc_n /= sqrt(var1_n * var2_n);

                /* xc_n can only be outside [-1;1] due to rounding errors */
                if (xc_n < -1.0) {
#ifndef NDEBUG
                    const double m1 = fabs(mean1_n) > 1.0 ? fabs(mean1_n) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( -1.0 - xc_n < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_n = -1.0;
                } else if (xc_n > 1.0) {
#ifndef NDEBUG
                    const double m1 = fabs(mean1_n) > 1.0 ? fabs(mean1_n) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( xc_n - 1.0 < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_n =  1.0;
                }

            } else
                xc_n = 0.0;

            if (xc_n > xc) {
                xc = xc_n;
                delta = -step;
            }

            (void)cpl_vector_set(vxc, half_search - step, xc_n);

            if (var1_p * var2_p > 0) {
                /* Subtract the mean-term */
                xc_p -= mean1_p * mean2_p * (double)(2*size - istop);
                /* - and divide by the norm of the mean-corrected vectors */
                xc_p /= sqrt(var1_p * var2_p);

                /* xc_p can only be outside [-1;1] due to rounding errors */
                if (xc_p < -1.0) {
#ifndef NDEBUG
                    const double m1 = fabs(mean1_p) > 1.0 ? fabs(mean1_p) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( -1.0 - xc_p < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_p = -1.0;
                } else if (xc_p > 1.0) {
#ifndef NDEBUG
                    const double m1 = fabs(mean1_p) > 1.0 ? fabs(mean1_p) : 1.0;
                    const double m2 = fabs(mean2)   > 1.0 ? fabs(mean2)   : 1.0;
                    assert( xc_p-1.0 < 4.0 * (double)size * DBL_EPSILON
                            * sqrt(m1 * m2));
#endif

                    xc_p =  1.0;
                }

            } else
                xc_p = 0.0;

            if (xc_p > xc) {
                xc = xc_p;
                delta = step;
            }
            (void)cpl_vector_set(vxc, half_search + step, xc_p);

        }
        if (hs > hs1) {
            cpl_tools_add_flops( (hs-hs1) * ( 4*istop + 28 ) );
        }
    }

    return half_search + delta;

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Apply a low-pass filter to a cpl_vector
  @param    v            cpl_vector
  @param    filter_type Type of filter to use
  @param    hw          Filter half-width
  @return   Pointer to newly allocated cpl_vector or NULL in case of an error

  This type of low-pass filtering consists in a convolution with a given
  kernel. The chosen filter type determines the kind of kernel to apply for
  convolution.
  Supported kernels are CPL_LOWPASS_LINEAR and CPL_LOWPASS_GAUSSIAN.

  In the case of CPL_LOWPASS_GAUSSIAN, the gaussian sigma used is
  1/sqrt(2). As this function is not meant to be general and cover all
  possible cases, this sigma is hardcoded and cannot be changed.

  The returned smooth cpl_vector must be deallocated using cpl_vector_delete().
  The returned signal has exactly as many samples as the input signal.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if filter_type is not supported or if hw is
    negative or bigger than half the vector v size
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_filter_lowpass_create(
        const cpl_vector * v,
        cpl_lowpass        filter_type,
        cpl_size           hw)
{
    cpl_vector * filtered;
    cpl_vector * kernel;
    double       replace;
    cpl_size     i, j;


    cpl_ensure(v != NULL,   CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(hw >= 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(2*hw <= v->size, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* generate low-pass filter kernel */
    kernel = cpl_vector_gen_lowpass_kernel(filter_type, hw);

    /* This will catch illegal values for filter_type */
    cpl_ensure(kernel != NULL, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* allocate output cpl_vector */
    filtered = cpl_vector_new(v->size);

    /* compute edge effects for the first hw elements */
    for (i = 0; i<hw; i++) {
        replace = 0.0;
        for (j=-hw; j<=hw; j++) {
            if (i+j<0) {
                replace += kernel->data[hw+j] * v->data[0];
            } else {
                replace += kernel->data[hw+j] * v->data[i+j];
            }
        }
        filtered->data[i] = replace;
    }

    /* compute edge effects for the last hw elements */
    for (i=v->size-hw; i<v->size; i++) {
        replace = 0.0;
        for (j=-hw; j<=hw; j++) {
            if (i+j>v->size-1) {
                replace += kernel->data[hw+j] * v->data[v->size-1];
            } else {
                replace += kernel->data[hw+j] * v->data[i+j];
            }
        }
        filtered->data[i] = replace;
    }

    /* compute all other elements */
    for (i=hw; i<v->size-hw; i++) {
        replace = 0.0;
        for (j=-hw; j<=hw; j++) {
            replace += kernel->data[hw+j] * v->data[i+j];
        }
        filtered->data[i] = replace;
    }
    cpl_vector_delete(kernel);
    cpl_tools_add_flops(4*hw*v->size);
    return filtered;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Apply a 1D median filter of given half-width to a cpl_vector
  @param    self    Input vector to be filtered
  @param    hw      Filter half-width
  @return   Pointer to newly allocated cpl_vector or NULL in case of an error
  @see cpl_image_filter_mask()

  This function applies a median smoothing to a cpl_vector and returns a
  newly allocated cpl_vector containing a median-smoothed version of the
  input. The returned cpl_vector must be deallocated using cpl_vector_delete().

  The returned cpl_vector has exactly as many samples as the input one. The
  outermost hw values are copies of the input, each of the others is set to
  the median of its surrounding 1 + 2 * hw values.

  For historical reasons twice the half-width is allowed to equal the
  vector length, although in this case the returned vector is simply a
  duplicate of the input one.

  If different processing of the outer values is needed or if a more general
  kernel is needed, then cpl_image_filter_mask() can be called instead with
  CPL_FILTER_MEDIAN and the 1D-image input wrapped around self.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if hw is negative or bigger than half the vector
    size
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_filter_median_create(const cpl_vector * self,
                                             cpl_size           hw)
{

    cpl_vector *   other;

    cpl_ensure(self != NULL,       CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(hw   >= 0,          CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(2*hw <= self->size, CPL_ERROR_ILLEGAL_INPUT, NULL);

    if (2 * hw == self->size) {
        other = cpl_vector_duplicate(self);
    } else {
        /* Cannot fail now */

        cpl_image *    imgself;
        cpl_image *    imgother;
        cpl_mask  *    kernel;
        double    *    data;
        cpl_error_code code;

        /* Create (non-modified) 1D-image from self */
        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
        data = (double*)self->data;
        CPL_DIAG_PRAGMA_POP;
        imgself = cpl_image_wrap_double(self->size, 1, data);

        /* Create the filtered vector and its 1D-image wrapper */
        other = cpl_vector_new(self->size);
        imgother = cpl_image_wrap_double(other->size, 1, other->data);

        kernel = cpl_mask_new(1 + 2 * hw, 1);
        code = cpl_mask_not(kernel);
        assert(code == CPL_ERROR_NONE);

        code = cpl_image_filter_mask(imgother, imgself, kernel,
                                     CPL_FILTER_MEDIAN, CPL_BORDER_COPY);
        assert(code == CPL_ERROR_NONE);

        (void)cpl_image_unwrap(imgself);
        (void)cpl_image_unwrap(imgother);
        cpl_mask_delete(kernel);
    }

    return other;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Elementwise addition of a scalar to a vector
  @param    v       cpl_vector to modify
  @param    addend  Number to add
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Add a number to each element of the cpl_vector.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_add_scalar(
        cpl_vector * v,
        double       addend)
{
    cpl_size            i;

    /* Test inputs     */
    cpl_ensure_code(v != NULL,   CPL_ERROR_NULL_INPUT);

    /* Perform the addition */
    for (i = 0; i<v->size; i++) v->data[i] += addend;

    cpl_tools_add_flops(v->size);

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Elementwise subtraction of a scalar from a vector
  @param    v           cpl_vector to modify
  @param    subtrahend  Number to subtract
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Subtract a number from each element of the cpl_vector.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_subtract_scalar(
        cpl_vector * v,
        double       subtrahend)
{
    cpl_size            i;

    /* Test inputs     */
    cpl_ensure_code(v != NULL,   CPL_ERROR_NULL_INPUT);

    /* Perform the subtraction */
    for (i = 0; i<v->size; i++) v->data[i] -= subtrahend;

    cpl_tools_add_flops(v->size);

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Elementwise multiplication of a vector with a scalar
  @param    v        cpl_vector to modify
  @param    factor   Number to multiply with
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Multiply each element of the cpl_vector with a number.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_multiply_scalar(
        cpl_vector * v,
        double       factor)
{
    cpl_size            i;

    /* Test inputs     */
    cpl_ensure_code(v != NULL,   CPL_ERROR_NULL_INPUT);

    /* Perform the subtraction */
    for (i = 0; i<v->size; i++) v->data[i] *= factor;

    cpl_tools_add_flops(v->size);

    return CPL_ERROR_NONE;
}



/*----------------------------------------------------------------------------*/
/**
  @brief    Elementwise division of a vector with a scalar
  @param    v        cpl_vector to modify
  @param    divisor  Non-zero number to divide with
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Divide each element of the cpl_vector with a number.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_DIVISION_BY_ZERO if divisor is 0.0
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_divide_scalar(
        cpl_vector * v,
        double       divisor)
{
    cpl_size            i;

    /* Test inputs     */
    cpl_ensure_code(v != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(divisor != 0, CPL_ERROR_DIVISION_BY_ZERO);

    /* Perform the subtraction */
    for (i = 0; i<v->size; i++) v->data[i] /= divisor;

    cpl_tools_add_flops(v->size);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the element-wise logarithm.
  @param    v    cpl_vector to modify.
  @param    base Logarithm base.
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  The base and all the vector elements must be positive and the base must be
  different from 1, or a cpl_error_code will be returned and the vector will be
  left unmodified.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if base is negative or zero or if one of the
    vector values is negative or zero
  - CPL_ERROR_DIVISION_BY_ZERO if a division by zero occurs
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_logarithm(
                cpl_vector * v,
                double       base)
{
    cpl_size            i;

    cpl_ensure_code(v != NULL,          CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(base > 0,           CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(base != 1,          CPL_ERROR_DIVISION_BY_ZERO);

    for (i = 0; i < v->size; i++)
        cpl_ensure_code(v->data[i] > 0, CPL_ERROR_ILLEGAL_INPUT);

    for (i = 0; i < v->size; i++) v->data[i] = log(v->data[i]) / log(base);

    cpl_tools_add_flops(2*v->size);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the exponential of all vector elements.
  @param    v    Target cpl_vector.
  @param    base Exponential base.
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  If the base is zero all vector elements must be positive and if the base is
  negative all vector elements must be integer, otherwise a cpl_error_code is
  returned and the vector is unmodified.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT base and v are not as requested
  - CPL_ERROR_DIVISION_BY_ZERO if one of the v values is negative or 0
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_exponential(
                cpl_vector * v,
                double       base)
{
    cpl_size     i;


    cpl_ensure_code(v != NULL,   CPL_ERROR_NULL_INPUT);

    if (base == 0.0) {
        for (i = 0; i < v->size; i++)
            cpl_ensure_code(v->data[i] > 0.0, CPL_ERROR_DIVISION_BY_ZERO);
    } else if (base < 0.0) {
        for (i = 0; i < v->size; i++)
            cpl_ensure_code(v->data[i] == ceil(v->data[i]),
                            CPL_ERROR_ILLEGAL_INPUT);
    }

    for (i = 0; i < v->size; i++) v->data[i] = pow(base, v->data[i]);

    cpl_tools_add_flops(v->size);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the power of all vector elements.
  @param    v        Target cpl_vector.
  @param    exponent Constant exponent.
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  If the exponent is negative all vector elements must be non-zero and if
  the exponent is non-integer all vector elements must be non-negative,
  otherwise a cpl_error_code is returned and the vector is unmodified.

  Following the behaviour of C99 pow() function, this function sets 0^0 = 1.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if v and exponent are not as requested
  - CPL_ERROR_DIVISION_BY_ZERO if one of the v values is 0
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_power(
        cpl_vector * v,
        double       exponent)
{
    cpl_size     i;

    cpl_ensure_code(v != NULL, CPL_ERROR_NULL_INPUT);

    if (exponent < 0) {
        for (i = 0; i < v->size; i++)
            cpl_ensure_code(v->data[i] != 0, CPL_ERROR_DIVISION_BY_ZERO);
    } else if (exponent != ceil(exponent)) {
        for (i = 0; i < v->size; i++)
            cpl_ensure_code(v->data[i] >= 0, CPL_ERROR_ILLEGAL_INPUT);
    }

    for (i = 0; i < v->size; i++) v->data[i] = pow(v->data[i], exponent);

    cpl_tools_add_flops(v->size);

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Fill a vector with a kernel profile
  @param    profile  cpl_vector to be filled
  @param    type     Type of kernel profile
  @param    radius   Radius of the profile in pixels
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_image_get_interpolated

  A number of predefined kernel profiles are available:
  - CPL_KERNEL_DEFAULT: default kernel, currently CPL_KERNEL_TANH
  - CPL_KERNEL_TANH: Hyperbolic tangent
  - CPL_KERNEL_SINC: Sinus cardinal
  - CPL_KERNEL_SINC2: Square sinus cardinal
  - CPL_KERNEL_LANCZOS: Lanczos2 kernel
  - CPL_KERNEL_HAMMING: Hamming kernel
  - CPL_KERNEL_HANN: Hann kernel
  - CPL_KERNEL_NEAREST: Nearest neighbor kernel (1 when dist < 0.5, else 0)

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if radius is non-positive, or in case of the
    CPL_KERNEL_TANH profile if the length of the profile exceeds 32768

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_fill_kernel_profile(cpl_vector * profile,
                                              cpl_kernel type,
                                              double radius)
{
    const cpl_size size = cpl_vector_get_size(profile);
    /* TABS per pixel == 1 / dx */
    /* If size is 1, the profile is flat (and hardly useful) */
    const double dx = size > 1 ? radius / (double)(size-1) : 1.0;
    cpl_size i;


    cpl_ensure_code(size   > 0,   cpl_error_get_code());
    cpl_ensure_code(radius > 0.0, CPL_ERROR_ILLEGAL_INPUT);

    /* Switch on the requested kernel type */
    switch (type) {
        case CPL_KERNEL_TANH: {
            if (cpl_vector_fill_tanh_kernel(profile, 0.5 / dx))
                return cpl_error_set_where_();
            break;
        }
        case CPL_KERNEL_SINC:
            for (i = 0; i < size; i++) {
                const double x = (double)i * dx;
                profile->data[i] = cpl_tools_sinc(x);
            }
            cpl_tools_add_flops( 4 * size );
            break;
        case CPL_KERNEL_SINC2:
            for (i = 0; i < size; i++) {
                const double x = (double)i * dx;
                profile->data[i] = cpl_tools_sinc(x);
                profile->data[i] *= profile->data[i];
            }
            cpl_tools_add_flops( 5 * size );
            break;
        case CPL_KERNEL_LANCZOS:
            for (i = 0; i < size; i++) {
                const double x = (double)i * dx;

                if (x >= 2) break;
                profile->data[i] = cpl_tools_sinc(x) * cpl_tools_sinc(x/2);
            }
            for (; i < size; i++) profile->data[i] = 0;
            cpl_tools_add_flops( 8 * i );
            break;
        case CPL_KERNEL_HAMMING:
            cpl_vector_fill_alpha_kernel(profile, dx, 0.54);
            break;
        case CPL_KERNEL_HANN:
            cpl_vector_fill_alpha_kernel(profile, dx, 0.50);
            break;
        case CPL_KERNEL_NEAREST:
            for (i = 0; i < size; i++) {
                const double x = (double)i * dx;
                profile->data[i] = x < 0.5 ? 1.0 : 0.0;
            }
            cpl_tools_add_flops( size );
            break;
        default:
            /* Only reached if cpl_kernel is extended in error */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;
}



/*----------------------------------------------------------------------------*/
/**
   @brief   Apply a 1d gaussian fit
   @param   x           Positions to fit
   @param   sigma_x     Uncertainty (one sigma, gaussian errors assumed)
                        assosiated with @em x. Taking into account the
            uncertainty of the independent variable is currently
            unsupported, and this parameter must therefore be set
            to NULL.
   @param   y           Values to fit
   @param   sigma_y     Uncertainty (one sigma, gaussian errors assumed)
                        associated with @em y.
                        If NULL, constant uncertainties are assumed.
   @param   fit_pars    Specifies which parameters participate in the fit
                        (any other parameters will be held constant). Possible
            values are CPL_FIT_CENTROID, CPL_FIT_STDEV,
            CPL_FIT_AREA, CPL_FIT_OFFSET and any bitwise
            combination of these. As a shorthand for including all
            four parameters in the fit, use CPL_FIT_ALL.
   @param   x0          (output) Center of best fit gaussian.
                        If CPL_FIT_CENTROID is not set, this is also an input
            parameter.
   @param   sigma       (output) Width of best fit gaussian. A positive number
                        on success. If CPL_FIT_STDEV is not set, this is
            also an input parameter.
   @param   area        (output) Area of gaussian. A positive number on succes.
                        If CPL_FIT_AREA is not set, this is also an input
            parameter.
   @param   offset      (output) Fitted background level. If CPL_FIT_OFFSET
                        is not set, this is also an input parameter.
   @param   mse         (output) If non-NULL, the mean squared error of the best
                        fit is returned.
   @param   red_chisq   (output) If non-NULL, the reduced chi square of the best
                        fit is returned. This requires the noise vector to be
            specified.
   @param   covariance  (output) If non-NULL, the formal covariance matrix of
                        the best fit is returned. This requires @em sigma_y
            to be specified. The order of fit parameters in the
            covariance matrix is defined as (@em x0, @em sigma,
            @em area, @em offset), for example the (3,3)
            element of the matrix (counting from zero) is the
            variance of the fitted @em offset. The matrix must
            be deallocated by calling @c cpl_matrix_delete() . On
            error, NULL is returned.

   @return  CPL_ERROR_NONE iff okay

   This function fits to the input vectors a 1d gaussian function of the form

   f(x) = @em area / sqrt(2 pi @em sigma^2) *
   exp( -(@em x - @em x0)^2/(2 @em sigma^2)) + @em offset

   (@em area > 0) by minimizing chi^2 using a Levenberg-Marquardt algorithm.

   The values to fit are read from the input vector @em x. Optionally, a vector
   @em sigma_x (of same size as @em x) may be specified.

   Optionally, the mean squared error, the reduced chi square and the covariance
   matrix of the best fit are computed . Set corresponding parameter to NULL to
   ignore.

   If the covariance matrix is requested and successfully computed, the diagonal
   elements (the variances) are guaranteed to be positive.

   Occasionally, the Levenberg-Marquardt algorithm fails to converge to a set of
   sensible parameters. In this case (and only in this case), a
   CPL_ERROR_CONTINUE is set. To allow the caller to recover from this
   particular error,
   the parameters @em x0, @em sigma, @em area and @em offset will on output
   contain estimates of the best fit parameters, specifically estimated as the
   median position, the median of the absolute residuals multiplied by 1.4828,
   the minimum flux value and the maximum flux difference multiplied by
   sqrt(2 pi sigma^2), respectively. In this case, @em mse, @em red_chisq and
   @em covariance are not computed. Note that the variance of @em x0 (the (0,0)
   element of the covariance matrix) in this case can be estimated by
   @em sigma^2 / @em area .

   A CPL_ERROR_SINGULAR_MATRIX occurs if the covariance matrix cannot be
   computed. In that case all other output parameters are valid.

   Current limitations
   - Taking into account the uncertainties of the independent variable
     is not supported.

   Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_NULL_INPUT if @em x, @em y, @em x0, @em sigma, @em area or @em
     offset is NULL.
   - CPL_ERROR_INVALID_TYPE if the specified @em fit_pars is not a bitwise
     combination of the allowed values (e.g. 0 or 1).
   - CPL_ERROR_UNSUPPORTED_MODE @em sigma_x is non-NULL.
   - CPL_ERROR_INCOMPATIBLE_INPUT if the sizes of any input vectors are
     different, or if the computation of reduced chi square or covariance
     is requested, but @em sigma_y is not provided.
   - CPL_ERROR_ILLEGAL_INPUT if any input noise values, @em sigma or @em area
     is non-positive, or if chi square computation is requested and
     there are less than 5 data points to fit.
   - CPL_ERROR_ILLEGAL_OUTPUT if memory allocation failed.
   - CPL_ERROR_CONTINUE if the fitting algorithm failed.
   - CPL_ERROR_SINGULAR_MATRIX if the covariance matrix could not be calculated.
*/
/*----------------------------------------------------------------------------*/

cpl_error_code
cpl_vector_fit_gaussian(const cpl_vector *x, const cpl_vector *sigma_x,
            const cpl_vector *y, const cpl_vector *sigma_y,
            cpl_fit_mode fit_pars,
            double *x0, double *sigma, double *area, double *offset,
            double *mse, double *red_chisq,
            cpl_matrix **covariance)
{
    cpl_matrix *x_matrix = NULL;    /* LM algorithm needs a matrix,
                       not a vector                 */

    cpl_size N;                          /* Number of data points        */
    double xlo, xhi;                /* Min/max x                    */

    /* Initial parameter values */
    double x0_guess    = 0;  /* Avoid warnings about uninitialized variables */
    double sigma_guess = 0;
    double area_guess;
    double offset_guess;

    /* Validate input */
    cpl_ensure_code( x       != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code( sigma_x == NULL, CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code( y       != NULL, CPL_ERROR_NULL_INPUT);
    /* sigma_y may be NULL or non-NULL */
    /* Bits in 'fit_pars' must be non-empty subset of bits in 'CPL_FIT_ALL' */
    cpl_ensure_code( fit_pars != 0 &&
             (CPL_FIT_ALL | fit_pars) == CPL_FIT_ALL,
             CPL_ERROR_INVALID_TYPE);

    N = cpl_vector_get_size(x);

    cpl_ensure_code( N == cpl_vector_get_size(y),
             CPL_ERROR_INCOMPATIBLE_INPUT);

    if (sigma_x != NULL)
    {
        cpl_ensure_code( N == cpl_vector_get_size(sigma_x),
                 CPL_ERROR_INCOMPATIBLE_INPUT);
    }
    if (sigma_y != NULL)
    {
        cpl_ensure_code( N == cpl_vector_get_size(sigma_y),
                 CPL_ERROR_INCOMPATIBLE_INPUT);
    }

    cpl_ensure_code( x0     != NULL &&
             sigma  != NULL &&
             area   != NULL &&
             offset != NULL, CPL_ERROR_NULL_INPUT);

    if (! (fit_pars & CPL_FIT_STDEV))
    {
        cpl_ensure_code( *sigma > 0, CPL_ERROR_ILLEGAL_INPUT);
    }
    if (! (fit_pars & CPL_FIT_AREA))
    {
        cpl_ensure_code( *area > 0, CPL_ERROR_ILLEGAL_INPUT);
    }

    /* mse, red_chisq may be NULL */

    /* Need more than number_of_parameters points to calculate chi^2.
     * There are less than 5 parameters. */
    cpl_ensure_code( red_chisq == NULL || N >= 5, CPL_ERROR_ILLEGAL_INPUT);

    if (covariance != NULL) *covariance = NULL;
    /* If covariance computation is requested, then
     * return either the covariance matrix or NULL
     * (don't return undefined pointer).
     */

    /* Cannot compute chi square & covariance without sigma_y */
    cpl_ensure_code( (red_chisq == NULL && covariance == NULL) ||
             sigma_y != NULL,
             CPL_ERROR_INCOMPATIBLE_INPUT);

    /* Create (non-modified) matrix from x-data */
CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    x_matrix = cpl_matrix_wrap(N, 1, (double*)cpl_vector_get_data_const(x));
CPL_DIAG_PRAGMA_POP;
    if (x_matrix == NULL)
    {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
    }

    /* Check that any provided sigmas are positive. */
    if (sigma_x != NULL && cpl_vector_get_min(sigma_x) <= 0)
    {
        cpl_matrix_unwrap(x_matrix);
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }
    if (sigma_y != NULL && cpl_vector_get_min(sigma_y) <= 0)
    {
        cpl_matrix_unwrap(x_matrix);
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    /* Compute guess parameters using robust estimation
     * (This step might be improved by taking into account the sigmas,
     *  but for simplicity do not)
     */
    {
    double sum  = 0.0;
    double quartile[3];
    double fraction[3] = {0.25 , 0.50 , 0.75};
    const double *y_data = cpl_vector_get_data_const(y);

    if (fit_pars & CPL_FIT_OFFSET)
        {
        /* Estimate offset as 25% percentile of y-values.
           (The minimum value may be too low for noisy input,
           the median is too high if there is not much
           background in the supplied data, so use
           something inbetween).
        */

        cpl_vector *y_dup = cpl_vector_duplicate(y);

        if (y_dup == NULL)
            {
            cpl_matrix_unwrap(x_matrix);
            return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
            }

        offset_guess = cpl_tools_get_kth_double(
            cpl_vector_get_data(y_dup), N, N/4);

        cpl_vector_delete(y_dup);
        }
    else
        {
        offset_guess = *offset;
        }

    /* Get quartiles of distribution
       (only bother if it's needed for estimation of x0 or sigma) */
    if ( (fit_pars & CPL_FIT_CENTROID) ||
         (fit_pars & CPL_FIT_STDEV   )
        )
        {
        /* The algorithm requires the input to be sorted
           as function of x, so do that (using qsort), and
           work on the sorted copy. Of course, the y-vector
           must be re-ordered along with x.
           sigma_x and sigma_y are not used, so don't copy those.
        */

        cpl_vector_fit_gaussian_input
            *sorted_input = cpl_malloc((size_t)N * sizeof(*sorted_input));
        double *x_data = cpl_matrix_get_data(x_matrix);
        cpl_boolean is_sorted = CPL_TRUE;
        cpl_size i;

        if (sorted_input == NULL)
            {
            cpl_matrix_unwrap(x_matrix);
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            }

        for (i = 0; i < N; i++)
            {
            sorted_input[i].x = x_data[i];
            sorted_input[i].y = y_data[i];

            is_sorted = is_sorted &&
                (i==0 || (x_data[i-1] < x_data[i]));
            }

        /* For efficiency, sort only if necessary. This is to
         * keep the asymptotic time complexity at O(n) if the
         * input was already sorted. (Otherwise the time
         * complexity is dominated by the following qsort call.)
         */
        if (!is_sorted)
            {
            qsort(sorted_input, (size_t)N, sizeof(*sorted_input),
                  &cpl_fit_gaussian_1d_compare);
            }

        for(i = 0; i < N; i++)
            {
            double flux = sorted_input[i].y;

            sum += (flux - offset_guess);
            }
        /* Note that 'sum' must be calculated the same way as
           'running_sum' below, Otherwise (due to round-off error)
           'running_sum' might end up being different from 'sum'(!).
           Specifically, it will not work to calculate 'sum' as

           (flux1 + ... + fluxN)  -  N*offset_guess
        */

        if (sum > 0.0)
            {
            double flux, x1, x2;
            double running_sum = 0.0;
            cpl_size j;

            i = 0;
            flux = sorted_input[i].y - offset_guess;

            for (j = 0; j < 3; j++)
                {
                double limit = fraction[j] * sum;
                double k;

                while (running_sum + flux < limit && i < N-1)
                    {
                    running_sum += flux;
                    i++;
                    flux = sorted_input[i].y - offset_guess;
                    }

                /* Fraction [0;1] of current flux needed
                   to reach the quartile */
                k = (limit - running_sum)/flux;

                if (k <= 0.5)
                    {
                    /* Interpolate linearly between
                       current and previous position
                    */
                    if (0 < i)
                        {
                        x1 = sorted_input[i-1].x;
                        x2 = sorted_input[i].x;

                        quartile[j] =
                            x1*(0.5-k) +
                            x2*(0.5+k);
                        /*
                          k=0   => quartile = midpoint,
                          k=0.5 => quartile = x2
                        */
                        }
                    else
                        {
                        quartile[j] = sorted_input[i].x;
                        }
                    }
                else
                    {
                    /* Interpolate linearly between
                       current and next position */
                    if (i < N-1)
                        {
                        x1 = sorted_input[i].x;
                        x2 = sorted_input[i+1].x;

                        quartile[j] =
                            x1*( 1.5-k) +
                            x2*(-0.5+k);
                        /*
                          k=0.5 => quartile = x1,
                          k=1.0 => quartile = midpoint
                        */
                        }
                    else
                        {
                        quartile[j] = sorted_input[i].x;
                        }
                    }
                }
            }
        else
            {
            /* If there's no flux (sum = 0) then
               set quartiles to something that's not
               completely insensible.
            */

            quartile[1] = cpl_matrix_get_mean(x_matrix);

            quartile[2] = quartile[1];
            quartile[0] = quartile[2] - 1.0;
            /* Then sigma_guess = 1.0 */
            }

        cpl_free(sorted_input);
        }

        /* x0_guess = median of distribution */
    x0_guess = (fit_pars & CPL_FIT_CENTROID) ? quartile[1] : *x0;

    /* sigma_guess = median of absolute residuals
     *
     *  (68% is within 1 sigma, and 50% is within 0.6744
     *  sigma,  => quartile3-quartile1 = 2*0.6744 sigma)
     */
    sigma_guess = (fit_pars & CPL_FIT_STDEV) ?
        (quartile[2] - quartile[0]) / (2*0.6744) : *sigma;

    area_guess  = (fit_pars & CPL_FIT_AREA) ?
        (cpl_vector_get_max(y) - offset_guess) * CPL_MATH_SQRT2PI * sigma_guess
        : *area;

    /* Make sure that the area/sigma are positive number */
    if (area_guess  <= 0) area_guess  = 1.0;
    if (sigma_guess <= 0) sigma_guess = 1.0;
    }

    /* Wrap parameters, fit, unwrap */
    {
    cpl_vector *a = cpl_vector_new(4);  /* There are four parameters */
    int ia[4];
    cpl_error_code ec;

    if (a == NULL)
        {
        cpl_matrix_unwrap(x_matrix);
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
        }

    cpl_vector_set(a, 0, x0_guess);
    cpl_vector_set(a, 1, sigma_guess);
    cpl_vector_set(a, 2, area_guess);
    cpl_vector_set(a, 3, offset_guess);

    ia[0] = fit_pars & CPL_FIT_CENTROID;
    ia[1] = fit_pars & CPL_FIT_STDEV;
    ia[2] = fit_pars & CPL_FIT_AREA;
    ia[3] = fit_pars & CPL_FIT_OFFSET;

    ec = cpl_fit_lvmq_(x_matrix, NULL,
              y, sigma_y,
              a, ia, NULL, NULL, gauss, gauss_derivative,
              CPL_FIT_LVMQ_TOLERANCE,
              CPL_FIT_LVMQ_COUNT,
              CPL_FIT_LVMQ_MAXITER,
              mse, red_chisq,
              covariance);

    cpl_matrix_unwrap(x_matrix);

    /* Check return status of fitting routine */
    if (ec == CPL_ERROR_NONE      ||
        ec == CPL_ERROR_SINGULAR_MATRIX)
        {
        /* The LM algorithm converged. The computation
         *  of the covariance matrix might have failed.
         */

        /* In principle, the LM algorithm might have converged
         * to a negative sigma (even if the guess value was
         * positive). Make sure that the returned sigma is positive
         * (by convention).
         */

        if (ia[0]) *x0     = cpl_vector_get(a, 0);
        if (ia[1]) *sigma  = fabs(cpl_vector_get(a, 1));
        if (ia[2]) *area   = cpl_vector_get(a, 2);
        if (ia[3]) *offset = cpl_vector_get(a, 3);
        }

    cpl_vector_delete(a);

    xlo = cpl_vector_get_min(x);
    xhi = cpl_vector_get_max(x);

    if (ec == CPL_ERROR_CONTINUE ||
        !(
        xlo <= *x0 && *x0 <= xhi &&
        0 < *sigma && *sigma < (xhi - xlo + 1) &&
        0 < *area
        /* This extra check on the background level makes sense
           iff the input flux is assumed to be positive
           && *offset > - *area  */
        )
        )
        {
        /* The LM algorithm did not converge, or it converged to
         * a non-sensical result. Return the guess parameter values
         * in order to enable the caller to recover. */

        *x0         = x0_guess;
        *sigma      = sigma_guess;
        *area       = area_guess;
        *offset     = offset_guess;

        /* In this case the covariance matrix will not make sense
           (because the LM algorithm failed), so delete it */
        if (covariance != NULL && *covariance != NULL)
            {
            cpl_matrix_delete(*covariance);
            *covariance = NULL;
            }

        /* Return CPL_ERROR_CONTINUE if the fitting routine failed */
        return cpl_error_set_(CPL_ERROR_CONTINUE);
        }
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Create Right Half of a symmetric smoothing kernel for LSS
  @param    slitw  The slit width [pixel]
  @param    fwhm   The spectral FWHM [pixel]
  @return   Right Half of (symmetric) smoothing vector
  @deprecated Unstable API, may change or disappear. Do not use in new code!

 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_new_lss_kernel(double  slitw,
                                       double  fwhm)
{
    const double   sigma  = fwhm * CPL_MATH_SIG_FWHM;
    const cpl_size size   = 1 + (cpl_size)(5.0 * sigma + 0.5*slitw);
    cpl_vector   * kernel = cpl_vector_new(size);


    if (cpl_vector_fill_lss_profile_symmetric(kernel, slitw, fwhm)) {
        cpl_vector_delete(kernel);
        kernel = NULL;
        (void)cpl_error_set_where_();
    }

    return kernel;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Convolve a 1d-signal with a symmetric 1D-signal
  @param    smoothed  Preallocated vector to be smoothed in place
  @param    conv_kernel     Vector with symmetric convolution function
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @deprecated Unstable API, may change or disappear. Do not use in new code!

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_convolve_symmetric(cpl_vector       * smoothed,
                                             const cpl_vector * conv_kernel)
{
    cpl_size        nsamples;
    cpl_size        ihwidth;
    cpl_vector  *   raw;
    double      *   psmoothe;
    double      *   praw;
    const double*   psymm;
    cpl_size        i, j;

    /* Test entries */
    cpl_ensure_code(smoothed, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(conv_kernel, CPL_ERROR_NULL_INPUT);

    /* Initialise */
    nsamples = cpl_vector_get_size(smoothed);
    ihwidth = cpl_vector_get_size(conv_kernel) - 1;
    cpl_ensure_code(ihwidth<nsamples, CPL_ERROR_ILLEGAL_INPUT);
    psymm = cpl_vector_get_data_const(conv_kernel);
    psmoothe = cpl_vector_get_data(smoothed);

    /* Create raw vector */
    raw = cpl_vector_duplicate(smoothed);
    praw = cpl_vector_get_data(raw);

    /* Convolve with the symmetric function */
    for (i = 0; i<ihwidth; i++) {
        psmoothe[i] = praw[i] * psymm[0];
        for (j=1; j <= ihwidth; j++) {
            const cpl_size k = i-j < 0 ? 0 : i-j;
            psmoothe[i] += (praw[k]+praw[i+j]) * psymm[j];
        }
    }

    for (i=ihwidth; i<nsamples-ihwidth; i++) {
        psmoothe[i] = praw[i] * psymm[0];
        for (j=1; j<=ihwidth; j++)
            psmoothe[i] += (praw[i-j]+praw[i+j]) * psymm[j];
    }
    for (i=nsamples-ihwidth; i<nsamples; i++) {
        psmoothe[i] = praw[i] * psymm[0];
        for (j=1; j<=ihwidth; j++) {
            const cpl_size k = i+j > nsamples-1 ? nsamples - 1 : i+j;
            psmoothe[i] += (praw[k]+praw[i-j]) * psymm[j];
        }
    }
    cpl_vector_delete(raw);

    return CPL_ERROR_NONE;
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    The antiderivative of erx(x/sigma/sqrt(2)) with respect to x
  @param    x      x
  @param    sigma  sigma
  @return   The antiderivative
  @note This function is even.

 */
/*----------------------------------------------------------------------------*/
inline
static double cpl_erf_antideriv(double x, double sigma)
{
    return x * erf( x / (sigma * CPL_MATH_SQRT2))
       + 2.0 * sigma/CPL_MATH_SQRT2PI * exp(-0.5 * x * x / (sigma * sigma));
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Fill right half of a symmetric long-slit spectroscopy line profile
  @param    self   The pre-allocated vector to be filled
  @param    slitw  The slit width [pixel]
  @param    fwhm   The spectral FWHM [pixel]
  @return   CPL_ERROR_NONE or the relevant error code on failure

  The smoothing function is the right half of the convolution of a Gaussian with
  sigma =  fwhm / (2 * sqrt(2*log(2))) and a top-hat with a width equal to the
  slit width, and area 1, and a tophat with unit width and area.
  (the convolution with a tophat with unit width and area equals integration
   from  i-1/2 to 1+1/2).
  Since this function is symmetric only the central, maximum value and the
  right half is computed,

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if a pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the slit width or fwhm is non-positive
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_vector_fill_lss_profile_symmetric(cpl_vector * self,
                                                            double  slitw,
                                                            double  fwhm)
{

    const double   sigma = fwhm * CPL_MATH_SIG_FWHM;
    const cpl_size n     = cpl_vector_get_size(self);
    cpl_size       i;


    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(slitw > 0.0,  CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(fwhm  > 0.0,  CPL_ERROR_ILLEGAL_INPUT);

    /* Cannot fail now */

    /* Special case for i = 0 */
    (void)cpl_vector_set(self, 0,
                         (cpl_erf_antideriv(0.5*slitw + 0.5, sigma) -
                          cpl_erf_antideriv(0.5*slitw - 0.5, sigma)) / slitw);

    for (i = 1; i < n; i++) {
        /* FIXME: Reuse two cpl_erf_antideriv() calls from previous value */
        const double x1p = (double)i + 0.5*slitw + 0.5;
        const double x1n = (double)i - 0.5*slitw + 0.5;
        const double x0p = (double)i + 0.5*slitw - 0.5;
        const double x0n = (double)i - 0.5*slitw - 0.5;
        const double val = 0.5/slitw *
            (cpl_erf_antideriv(x1p, sigma) - cpl_erf_antideriv(x1n, sigma) -
             cpl_erf_antideriv(x0p, sigma) + cpl_erf_antideriv(x0n, sigma));
        (void)cpl_vector_set(self, i, val);
    }

    cpl_tools_add_flops( 24 * n );

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
   @brief   Define order of input data
   @param   left      Left operand
   @param   right     Right operand

   This function uses void pointers because it is used as input for ANSI's
   qsort().
*/
/*----------------------------------------------------------------------------*/
static int cpl_fit_gaussian_1d_compare(const void *left,
                                       const void *right)
{
    return
    (((const cpl_vector_fit_gaussian_input *)left )->x <
     ((const cpl_vector_fit_gaussian_input *)right)->x) ? -1 :
    (((const cpl_vector_fit_gaussian_input *)left )->x ==
     ((const cpl_vector_fit_gaussian_input *)right)->x) ? 0  : 1;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Generate a kernel for smoothing filters (low-pass)
  @param    filt_type   Type of kernel to generate
  @param    hw          Kernel half-width.
  @return   Pointer to newly allocated cpl_vector or NULL in case of an error
  @see cpl_vector_filter_lowpass_create()

  The kernel is returned as a cpl_vector (values of the Y field)
  The returned cpl_vector must be deallocated using cpl_vector_delete(). The
  returned cpl_vector's size is 2hw+1.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if hw is negative
 */
/*----------------------------------------------------------------------------*/
static cpl_vector * cpl_vector_gen_lowpass_kernel(
        cpl_lowpass filt_type,
        cpl_size    hw)
{
    cpl_vector * kernel;
    double       norm;
    cpl_size     i;


    cpl_ensure(hw >= 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Create the kernel */
    kernel = cpl_vector_new(2*hw+1);
    switch(filt_type) {
        case CPL_LOWPASS_LINEAR:
            /* flat kernel */
            for (i=-hw; i<=hw; i++) kernel->data[hw+i] = 1.0/(double)(2*hw+1);
            break;
        case CPL_LOWPASS_GAUSSIAN:
            norm = 0.00;
            for (i=-hw; i < 1 + hw; i++) {
                /* gaussian kernel */
                kernel->data[hw+i] = exp(-(double)(i*i));
                norm += kernel->data[hw+i];
            }
            for (i = 0; i<2*hw+1; i++) kernel->data[i] /= norm;
            cpl_tools_add_flops(2*hw*4);
            break;
        default:
            /* Only reached if cpl_lowpass is extended in error */
            cpl_vector_delete(kernel);
            kernel = NULL;
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;
    }
    return kernel;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Cardinal sine, sin(pi*x)/(pi*x).
  @param    x   double value.
  @return   The cardinal sine of x.

 */
/*----------------------------------------------------------------------------*/
static double cpl_tools_sinc(double x)
{
    const double xpi = x * CPL_MATH_PI;

    /* sinc(0) == 1, sinx(x*pi) = 0, for integral x */
    return x != 0.0 ? (ceil(x) != x ? sin(xpi) / xpi : 0.0) : 1.0;
}

/*----------------------------------------------------------------------------*/
/**
 @brief Fill a vector with an alpha kernel useful for resampling
 @param  dx     x increment
 @param  alpha  Use 0.5 for Hann resampling, 0.54 for Hamming resampling
 @return void
 @note The function will SIGABRT on invalid input.

  An alpha kernel is the product of sinc(x) and (alpha + (1-alpha) * cos(x)),
  for x < 1 and zero for x >= 1.

 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_fill_alpha_kernel(cpl_vector * profile, double dx,
                                         double alpha)
{

    cpl_size i;


    assert( profile );

    for (i = 0; i < profile->size; i++) {
        const double x = (double)i * dx;
        if (x >= 1) break;
        profile->data[i] = (alpha + (1.0 - alpha) * cos (x * CPL_MATH_PI))
            * cpl_tools_sinc(x);
    }
    cpl_tools_add_flops( 5 * i );
    for (; i < profile->size; i++) profile->data[i] = 0;

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Fill a vector with a hyperbolic tangent kernel.
  @param    width Half of the number of profile values per pixel
  @return   void

  The following function builds up a good approximation of a box filter. It
  is built from a product of hyperbolic tangents. It has the following
  properties:

  \begin{itemize}
  \item It converges very quickly towards +/- 1.
  \item The converging transition is very sharp.
  \item It is infinitely differentiable everywhere (i.e. smooth).
  \item The transition sharpness is scalable.
  \end{itemize}

  The function will SIGABRT on invalid input.

 */
/*----------------------------------------------------------------------------*/

/* Kernel definitions */
#ifndef CPL_KERNEL_TANH_STEEPNESS
#define CPL_KERNEL_TANH_STEEPNESS 5
#endif
#define CPL_hk_gen(x,s) (((tanh(s*(x+0.5))+1)/2)*((tanh(s*(-x+0.5))+1)/2))
static cpl_error_code cpl_vector_fill_tanh_kernel(cpl_vector * profile,
                                                  double width)
{
    double       * x;
    const cpl_size np = 32768; /* Hardcoded: should never be changed */
    const double   inv_np = 2.0 / (double)np;
    const double   steep = CPL_KERNEL_TANH_STEEPNESS;
    double         ind;
    const cpl_size samples = cpl_vector_get_size(profile);
    cpl_size       i;


    cpl_ensure_code(profile != NULL, CPL_ERROR_NULL_INPUT);
    if (samples > np)
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                      "Vector-length %" CPL_SIZE_FORMAT
                                      " exceeds maximum of %" CPL_SIZE_FORMAT,
                                      samples, np);

    /*
     * Generate the kernel expression in Fourier space
     * with a correct frequency ordering to allow standard FT
     */

    x = cpl_malloc((2*np+1)*sizeof(double));
    for (i = 0; i < np/2; i++) {
        ind      = (double)i * width * inv_np;
        x[2*i]   = CPL_hk_gen(ind, steep);
        x[2*i+1] = 0;
    }
    for (i=np/2; i<np; i++) {
        ind      = (double)(i-np) * width * inv_np;
        x[2*i]   = CPL_hk_gen(ind, steep);
        x[2*i+1] = 0;
    }

    /* Reverse Fourier to come back to image space */
    cpl_vector_reverse_tanh_kernel(x, np);

    /* fill array */
    for (i = 0; i < samples; i++) {
        profile->data[i] = width * x[2*i] * inv_np;
    }
    cpl_free(x);

    cpl_tools_add_flops( (13 * np + samples * 4)/2 );

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Bring a hyperbolic tangent kernel from Fourier to normal space.
  @param    data    Kernel samples in Fourier space.
  @param    nn      Number of samples in the input kernel.
  @return   void

  Bring back a hyperbolic tangent kernel from Fourier to normal space. Do
  not try to understand the implementation and DO NOT MODIFY THIS FUNCTION.
 */
/*----------------------------------------------------------------------------*/
#define CPL_KERNEL_SW(a,b) tempr=(a);(a)=(b);(b)=tempr
static void cpl_vector_reverse_tanh_kernel(double * data, cpl_size nn)
{
    unsigned long   n, mmax, m, i, j, istep;
    double  wtemp, wr, wpr, wpi, wi, theta;
    double  tempr, tempi;

    n = (unsigned long)nn << 1;
    j = 1;
    for (i=1; i<n; i+=2) {
        if (j > i) {
            CPL_KERNEL_SW(data[j-1],data[i-1]);
            CPL_KERNEL_SW(data[j],data[i]);
        }
        m = n >> 1;
        while (m>=2 && j>m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    mmax = 2;
    while (n > mmax) {
        istep = mmax << 1;
        theta = CPL_MATH_2PI / mmax;
        wtemp = sin(0.5 * theta);
        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);
        wr  = 1.0;
        wi  = 0.0;
        for (m=1; m<mmax; m+=2) {
            for (i=m; i<=n; i+=istep) {
                j = i + mmax;
                tempr = wr * data[j-1] - wi * data[j];
                tempi = wr * data[j]   + wi * data[j-1];
                data[j-1] = data[i-1] - tempr;
                data[j]   = data[i]   - tempi;
                data[i-1] += tempr;
                data[i]   += tempi;
            }
            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
        }
        mmax = istep;
    }
    /* FIXME: cpl_tools_add_flops(); */
}


/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief   Evaluate a gaussian
   @param   x             The evaluation point
   @param   a             The parameters defining the gaussian
   @param   result        The function value

   @return  0 iff okay.

   This function computes

   @em a3 +  @em a2 / sqrt(2 pi @em a1^2) *
   exp( -(@em x0 - @em a0)^2/(2 @em a1^2)).

   where @em a0, ..., @em a3 are the first four elements of @em a, and @em
   x0 is the first element of @em x .

   The function fails iff @em a1 is zero and @em x0 is equal to @em a0.

*/
/*----------------------------------------------------------------------------*/

static int
gauss(const double x[], const double a[], double *result)
{
    const double my    = a[0];
    const double sigma = a[1];

    if (sigma != 0.0) {

        const double A = a[2];
        const double B = a[3];

        *result = B +
            A/(CPL_MATH_SQRT2PI * fabs(sigma)) *
            exp(- (x[0] - my)*(x[0] - my)
                / (2*sigma*sigma));

    } else {

        /* Dirac's delta function */
        *result = x[0] != my ? 0.0 : DBL_MAX;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief   Evaluate the derivatives of a gaussian
   @param   x             The evaluation point
   @param   a             The parameters defining the gaussian
   @param   result        The derivatives wrt to parameters

   @return  0 iff okay.

   This function computes the partial derivatives of
   @em f(@em x0,@em a) =
   @em a3 +  @em a2 / sqrt(2 pi @em a1^2) *
   exp( -(@em x0 - @em a0)^2/(2 @em a1^2))
   with respect to @em a0, ..., @em a3.
   On successful evaluation, the i'th element of the @em result vector
   contains df/da_i.

   The function never returns failure.

*/
/*----------------------------------------------------------------------------*/

static int
gauss_derivative(const double x[], const double a[], double result[])
{

    if (a[1] != 0.0) {

        const double my    = a[0];
        const double sigma = a[1];
        const double A     = a[2];
        /* a[3] not used */

        /* f(x) = B + A/sqrt(2 pi s^2) exp(-(x-my)^2/2s^2)
         *
         * df/d(my) = A/sqrt(2 pi s^2) exp(-(x-my)^2/2s^2) * (x-my)  / s^2
         *          = A * fac. * (x-my)  / s^2
         * df/ds    = A/sqrt(2 pi s^2) exp(-(x-my)^2/2s^2) * ((x-my)^2/s^3 - 1/s)
         *          = A * fac. * ((x-my)^2 / s^2 - 1) / s
         * df/dA    = 1/sqrt(2 pi s^2) exp(-(x-my)^2/2s^2)
         *          = fac.
         * df/dB    = 1
         */


        const double factor = exp( -(x[0] - my)*(x[0] - my)/(2.0*sigma*sigma) )
            / (CPL_MATH_SQRT2PI * fabs(sigma));

        result[0] = A * factor * (x[0]-my) / (sigma*sigma);
        result[1] = A * factor * ((x[0]-my)*(x[0]-my) / (sigma*sigma) - 1)
            / sigma;
        result[2] = factor;
        result[3] = 1.0;

    } else {
        /* Derivative of Dirac's delta function */
        result[0] = 0.0;
        result[1] = 0.0;
        result[2] = 0.0;
        result[3] = 0.0;
    }

    return 0;
}
