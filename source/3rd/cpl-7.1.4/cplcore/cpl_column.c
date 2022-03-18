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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cpl_column.h>
#include <cpl_array_impl.h>

#include "cpl_error_impl.h"
#include "cpl_errorstate.h"
#include "cpl_tools.h"
#include "cpl_memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <complex.h>
#include <math.h>
#include <errno.h>
#include <assert.h>


/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define INT_FORM "% 7d"
#define LONG_FORM "% 7ld"
#define LONG_LONG_FORM "% 7lld"
#define SIZE_TYPE_FORM "% 7" CPL_SIZE_FORMAT
#define FLOAT_FORM "% 1.5e"
#define DOUBLE_FORM "% 1.5e"
#define STRING_FORM "%s"

#define CPL_COLUMN_GET_VALUE(SELF, INDX, MEMBER, TYPEBOOL)              \
                                                                        \
    if (SELF == NULL) {                                                 \
        (void)cpl_error_set_where_();                                   \
        return NULL;                                                    \
    } else {                                                            \
                                                                        \
        const cpl_size length = cpl_column_get_size(SELF);              \
        const cpl_type type   = cpl_column_get_type(SELF);              \
                                                                        \
        if (INDX < 0 || INDX >= length) {                               \
            (void)cpl_error_set_message_(CPL_ERROR_ACCESS_OUT_OF_RANGE, \
                                         "index=%" CPL_SIZE_FORMAT      \
                                         ", length=%" CPL_SIZE_FORMAT   \
                                         ", type=%s",                   \
                                         INDX, length,                  \
                                         cpl_type_get_name(type));      \
            return NULL;                                                \
        }                                                               \
                                                                        \
        if (!(TYPEBOOL)) {                                              \
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,       \
                                         "index=%" CPL_SIZE_FORMAT      \
                                         ", length=%" CPL_SIZE_FORMAT   \
                                         ", type=%s", INDX, length,     \
                                         cpl_type_get_name(type));      \
            return NULL;                                                \
        }                                                               \
    }                                                                   \
                                                                        \
    return SELF->values->MEMBER[indx]

#define CPL_COLUMN_GET_POINTER(SELF, MEMBER, TYPE)                      \
                                                                        \
    if (SELF == NULL) {                                                 \
        (void)cpl_error_set_where_();                                   \
        return NULL;                                                    \
    } else {                                                            \
                                                                        \
        const cpl_type type = cpl_column_get_type(SELF);                \
                                                                        \
        if (type != (TYPE)) {                                           \
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,       \
                                         "type=%s != %s",               \
                                         cpl_type_get_name(type),       \
                                         cpl_type_get_name(TYPE));      \
            return NULL;                                                \
        }                                                               \
    }                                                                   \
                                                                        \
    return SELF->values->MEMBER


/*-----------------------------------------------------------------------------
                            New data types
 -----------------------------------------------------------------------------*/


/* 
 * @defgroup cpl_column Columns
 *
 * This module provides functions to create, destroy and use a @em cpl_column.
 * All the functions should be considered private to the cpl_table object,
 * and not used elsewhere.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_column.h>
 * @endcode
 */

/* @{*/

/*
 *  Container of column values (mirroring the legal types). Private.
 */

typedef union _cpl_column_values_ {
    int            *i;                          /* CPL_TYPE_INT            */
    long           *l;                          /* CPL_TYPE_LONG           */
    long long      *ll;                         /* CPL_TYPE_LONG_LONG      */
    cpl_size       *sz;                         /* CPL_TYPE_SIZE           */
    float          *f;                          /* CPL_TYPE_FLOAT          */
    double         *d;                          /* CPL_TYPE_DOUBLE         */
    float complex  *cf;                         /* CPL_TYPE_FLOAT_COMPLEX  */
    double complex *cd;                         /* CPL_TYPE_DOUBLE_COMPLEX */
    char          **s;                          /* CPL_TYPE_STRING         */
    cpl_array     **array;                      /* Array types             */
} cpl_column_values;


/*
 *  The real thing: the column type (private);
 */

struct _cpl_column_ {
    char              *name;
    char              *unit;
    char              *format;
    cpl_size           length;
    cpl_size           depth;
    cpl_type           type;
    cpl_type           savetype;
    cpl_column_values *values;
    cpl_column_flag   *null;       /* NULL flags buffer              */
    cpl_size           nullcount;  /* Number of NULLs in column      */
    cpl_array         *dimensions; /* Number of dimensions in column */
};


/*-----------------------------------------------------------------------------
                            Static Function Prototypes
 -----------------------------------------------------------------------------*/

static inline int cpl_get_error_check_(const char *, const cpl_column *,
                                       cpl_size, int *, cpl_type)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

inline static int cpl_set_error_check_(const char *, const cpl_column *,
                                       cpl_size, cpl_type)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

static inline int cpl_column_base_type_invalid_(const cpl_column *,
                                                cpl_size, cpl_size)
    CPL_ATTR_NONNULL;

static inline cpl_size cpl_column_get_size_(const cpl_column *)
    CPL_ATTR_NONNULL;

static inline cpl_type cpl_column_get_type_(const cpl_column *)
    CPL_ATTR_NONNULL;

static inline cpl_size cpl_column_get_invalid_numerical(const cpl_column *)
    CPL_ATTR_NONNULL;

static void cpl_column_pow_float_complex(cpl_column *, double)
    CPL_ATTR_NONNULL;
static void cpl_column_pow_double_complex(cpl_column *, double)
    CPL_ATTR_NONNULL;

static void cpl_column_cpow_float_complex(cpl_column *, double complex)
    CPL_ATTR_NONNULL;

static void cpl_column_cpow_double_complex(cpl_column *, double complex)
    CPL_ATTR_NONNULL;

/* These macros are needed for support of the different element types */

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)

#define ADDTYPE(a) CONCAT2X(a, ELEM_NAME)

#define ELEM_TYPE float
#define ELEM_NAME float
#define ELEM_POW  powf
#define ELEM_SQRT sqrtf
#ifdef HAVE_CBRTF
#define ELEM_CBRT cbrtf
#endif
#define ELEM_CAST (float)
#include "cpl_column_body.h"
#undef ELEM_TYPE
#undef ELEM_NAME
#undef ELEM_POW
#undef ELEM_SQRT
#undef ELEM_CBRT
#undef ELEM_CAST

#define ELEM_TYPE double
#define ELEM_NAME double
#define ELEM_POW  pow
#define ELEM_SQRT sqrt
#ifdef HAVE_CBRT
#define ELEM_CBRT cbrt
#endif
#define ELEM_CAST (double)
#include "cpl_column_body.h"
#undef ELEM_TYPE
#undef ELEM_NAME
#undef ELEM_POW
#undef ELEM_SQRT
#undef ELEM_CBRT
#undef ELEM_CAST

#define ELEM_TYPE int
#define ELEM_NAME int
#define ELEM_POW  pow
#define ELEM_SQRT sqrt
#ifdef HAVE_CBRT
#define ELEM_CBRT cbrt
#endif
#define ELEM_CAST (int)round
#include "cpl_column_body.h"
#undef ELEM_TYPE
#undef ELEM_NAME
#undef ELEM_POW
#undef ELEM_SQRT
#undef ELEM_CBRT
#undef ELEM_CAST

#define ELEM_TYPE long
#define ELEM_NAME long
#define ELEM_POW  pow
#define ELEM_SQRT sqrt
#ifdef HAVE_CBRT
#define ELEM_CBRT cbrt
#endif
#define ELEM_CAST (long)round
#include "cpl_column_body.h"
#undef ELEM_TYPE
#undef ELEM_NAME
#undef ELEM_POW
#undef ELEM_SQRT
#undef ELEM_CBRT
#undef ELEM_CAST

#define ELEM_TYPE long long
#define ELEM_NAME long_long
#define ELEM_POW  pow
#define ELEM_SQRT sqrt
#ifdef HAVE_CBRT
#define ELEM_CBRT cbrt
#endif
#define ELEM_CAST (long long)round
#include "cpl_column_body.h"
#undef ELEM_TYPE
#undef ELEM_NAME
#undef ELEM_POW
#undef ELEM_SQRT
#undef ELEM_CBRT
#undef ELEM_CAST

#define ELEM_TYPE cpl_size
#define ELEM_NAME cplsize
#define ELEM_POW  pow
#define ELEM_SQRT sqrt
#ifdef HAVE_CBRT
#define ELEM_CBRT cbrt
#endif
#define ELEM_CAST (cpl_size)round
#include "cpl_column_body.h"
#undef ELEM_TYPE
#undef ELEM_NAME
#undef ELEM_POW
#undef ELEM_SQRT
#undef ELEM_CBRT
#undef ELEM_CAST



/*-----------------------------------------------------------------------------
                            Function definitions
 -----------------------------------------------------------------------------*/


/*
 * @brief
 *   Get size in bytes of a given column type (private).
 *
 * @param type  Column legal data type.
 *
 * @return Size in bytes of a given column type
 *
 * This private function computes the number of bytes of the data
 * types listed in the @em cpl_type enum, applying @c sizeof() 
 * to the corresponding dereferenced member of the @em cpl_column_values 
 * union. Undefined or illegal types returns 0.
 */

inline static size_t cpl_column_type_size(cpl_type type)
{

    size_t sz = 0;

    switch (type & ~CPL_TYPE_POINTER) {
    case CPL_TYPE_INT:
        sz = sizeof(int);
        break;
    case CPL_TYPE_LONG:
        sz = sizeof(long);
        break;
    case CPL_TYPE_LONG_LONG:
        sz = sizeof(long long);
        break;
    case CPL_TYPE_SIZE:
        sz = sizeof(cpl_size);
        break;
    case CPL_TYPE_FLOAT:
        sz = sizeof(float);
        break;
    case CPL_TYPE_DOUBLE:
        sz = sizeof(double);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        sz = sizeof(float complex);
        break;
    case CPL_TYPE_DOUBLE_COMPLEX:
        sz = sizeof(double complex);
        break;
    case CPL_TYPE_STRING:
        sz = sizeof(char *);
        break;
    default:
        break;
    }

    if (sz != 0 && (type & CPL_TYPE_POINTER)) {
        /* The type is known and of a pointer kind */
        sz = sizeof(cpl_array *);
    }

    return sz;

}


/*
 * @brief
 *   Create a column values container (private).
 *
 * @return Pointer to column values container.
 *
 * This private function allocates and initializes a container of
 * column values.
 */

static cpl_column_values *cpl_column_values_new(void)
{

    cpl_column_values *value = cpl_calloc(1, sizeof(cpl_column_values));


    value->i = NULL;

    return value;

}

/*
 * @brief
 *   Returns the values container of a column (private).
 *
 * @return Pointer to column values container, or @em NULL.
 */

static cpl_column_values *cpl_column_get_values(cpl_column *column)
{

    if (column)
        return column->values;

    return NULL;

}


/*
 * @brief
 *   Destructor of the container of column values (private).
 *
 * @param values  Pointer to container of column values.
 * @param length  Column length.
 * @param type    Column type.
 *
 * @return Nothing.
 *
 * This is a private function used to free all the column data and their
 * container.
 */

static void cpl_column_values_delete(cpl_column_values *values, 
                                     cpl_size length, cpl_type type)
{

    if (values) {
        if (type & CPL_TYPE_POINTER) {
            switch (type & ~CPL_TYPE_POINTER) {
            case CPL_TYPE_INT:
            case CPL_TYPE_LONG:
            case CPL_TYPE_LONG_LONG:
            case CPL_TYPE_SIZE:
            case CPL_TYPE_FLOAT:
            case CPL_TYPE_DOUBLE:
            case CPL_TYPE_STRING:
            case CPL_TYPE_FLOAT_COMPLEX:
            case CPL_TYPE_DOUBLE_COMPLEX:

                for (cpl_size i = 0; i < length; i++)
                    cpl_array_delete(values->array[i]);

                cpl_free((void *)values->array);

                break;
            default:
                break;
            }
        } else {
            if (type == CPL_TYPE_STRING) {
                for (cpl_size i = 0; i < length; i++)
                    cpl_free(values->s[i]);
            }

            cpl_free((void *)values->s);

        }
        cpl_free(values);
    }
}


/*
 * @brief
 *   Unset a NULL flag (private).
 *
 * @param column  Column to be accessed.
 * @param row     Position where to unset the NULL.
 *
 * @return 0 on success.
 *
 * This function is private: the only way for a user to unset a
 * null flag, is to write to the corresponding column element a
 * valid value. This function cannot be used for string or array 
 * columns.
 * Being this function private, all safety checks are removed to
 * make this function faster, assuming that all checks has been
 * already performed by the caller.
 */

static int cpl_column_unset_null(cpl_column *column, cpl_size row)
{

    cpl_size length;


    if (column->nullcount == 0)       /* There are no nulls to unset */
        return 0;

    if (!column->null)                /* There are just nulls        */
        column->null = cpl_malloc(column->length * sizeof(cpl_column_flag));

    length = column->length;

    if (column->nullcount == length)
        while (length--)
            column->null[length] = 1;

    if (column->null[row] == 1) {
        column->null[row] = 0;
        column->nullcount--;
    }

    if (column->nullcount == 0) {
        if (column->null)
            cpl_free(column->null);
        column->null = NULL;
    }

    return 0;

}


/*
 * @brief
 *   Unset an interval of NULL flags (private).
 *
 * @param column  Column to be accessed.
 * @param start   Position where to start unsetting NULLs.
 * @param count   Number of column elements to unset.
 *
 * @return 0 on success.
 *
 * This function is private: the only way for a user to unset a
 * null flag, is to write to the corresponding column element a
 * valid value. This function cannot be used for string or array columns.
 * Being this function private, all safety checks are removed to
 * make this function faster, assuming that all checks has been
 * already performed by the caller.
 */

static int cpl_column_unset_null_segment(cpl_column *column, 
                                         cpl_size start, cpl_size count)
{

    cpl_size length = cpl_column_get_size(column);


    if (start == 0 && count == length) {
        if (column->null)
            cpl_free(column->null);
        column->null = NULL;
        column->nullcount = 0;
    }

    if (column->nullcount == 0)
        return 0;

    if (!column->null)
        column->null = cpl_malloc(length * sizeof(cpl_column_flag));

    if (column->nullcount == length) {
        while (length--)
            column->null[length] = 1;
        memset(column->null + start, 0, count * sizeof(cpl_column_flag));
        column->nullcount -= count;
    }
    else {
        while (count--) {
            if (column->null[start] == 1) {
                column->null[start] = 0;
                column->nullcount--;
            }
            start++;
        }
    }

    if (column->nullcount == 0) {
        if (column->null)
            cpl_free(column->null);
        column->null = NULL;
    }

    return 0;

}


void cpl_column_dump_structure(cpl_column *column)
{
    cpl_errorstate prevstate = cpl_errorstate_get();
    const char *name   = cpl_column_get_name(column);
    const char *unit   = cpl_column_get_unit(column);
    const char *format = cpl_column_get_format(column);
    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);


    if (column) {

        printf("Column name   : ");

        if (name)
            printf("\"%s\"\n", name);
        else
            printf("NONE\n");

        printf("Column unit   : ");

        if (unit)
            printf("\"%s\"\n", unit);
        else
            printf("NONE\n");

        printf("Column format : ");

        if (format)
            printf("\"%s\"\n", format);
        else
            printf("NONE\n");

        printf("Column type   : ");

        switch (type) {
        case CPL_TYPE_INT:
            printf("int\n");
            break;
        case CPL_TYPE_LONG:
            printf("long\n");
            break;
        case CPL_TYPE_LONG_LONG:
            printf("long long\n");
            break;
        case CPL_TYPE_SIZE:
            printf("size_type\n");
            break;
        case CPL_TYPE_FLOAT:
            printf("float\n");
            break;
        case CPL_TYPE_DOUBLE:
            printf("double\n");
            break;
        case CPL_TYPE_STRING:
            printf("string\n");
            break;
        default:
            printf("UNDEFINED\n");
            break;
        }

        printf("Column length : %" CPL_SIZE_FORMAT "\n", length);

        printf("Column nulls  : %" CPL_SIZE_FORMAT "\n", cpl_column_count_invalid(column));
        printf("                (the NULL column %sexists)\n", 
                      column->null ? "" : "does not ");

    }
    else {
        printf("Column is NULL\n");
        cpl_errorstate_set(prevstate);
    }
        
}


void cpl_column_dump(cpl_column *column, cpl_size start, cpl_size count)
{

    cpl_errorstate prevstate = cpl_errorstate_get();
    cpl_type type   = cpl_column_get_type(column);
    cpl_size length = cpl_column_get_size(column);
    cpl_size nulls  = cpl_column_count_invalid(column);
    cpl_size i      = start;


    if (column) {
        if (start < length) {

            if (count > length - start)
                count = length - start;

            switch (type) {
            case CPL_TYPE_INT:
                if (nulls == 0) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %d\n", i, column->values->i[i]);
                        i++;
                    }
                }
                else if (nulls == length) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %d NULL\n", i, column->values->i[i]);
                        i++;
                    }
                }
                else {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %d %d\n", i,
                        column->values->i[i], column->null[i]);
                        i++;
                    }
                }
                break;
            case CPL_TYPE_LONG:
                if (nulls == 0) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT " %ld\n", i, column->values->l[i]);
                        i++;
                    }
                }
                else if (nulls == length) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %ld NULL\n", i, column->values->l[i]);
                        i++;
                    }
                }
                else {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %ld %d\n", i,
                        column->values->l[i], column->null[i]);
                        i++;
                    }
                }
                break;
            case CPL_TYPE_LONG_LONG:
                if (nulls == 0) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT " %lld\n", i, column->values->ll[i]);
                        i++;
                    }
                }
                else if (nulls == length) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %lld NULL\n", i, column->values->ll[i]);
                        i++;
                    }
                }
                else {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %lld %d\n", i,
                        column->values->ll[i], column->null[i]);
                        i++;
                    }
                }
                break;
            case CPL_TYPE_SIZE:
                if (nulls == 0) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT " %" CPL_SIZE_FORMAT "\n",
                               i, column->values->sz[i]);
                        i++;
                    }
                }
                else if (nulls == length) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %" CPL_SIZE_FORMAT
                               " NULL\n", i, column->values->sz[i]);
                        i++;
                    }
                }
                else {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %" CPL_SIZE_FORMAT
                               " %d\n", i, column->values->sz[i],
                               column->null[i]);
                        i++;
                    }
                }
                break;
            case CPL_TYPE_FLOAT:
                if (nulls == 0) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %f\n",
                               i, column->values->f[i]);
                        i++;
                    }
                }
                else if (nulls == length) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %f NULL\n",
                               i, column->values->f[i]);
                        i++;
                    }
                }
                else {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %f %d\n", i,
                        column->values->f[i], column->null[i]);
                        i++;
                    } 
                } 
                break;
            case CPL_TYPE_DOUBLE:
                if (nulls == 0) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %f\n",
                               i, column->values->d[i]);
                        i++;
                    }
                }
                else if (nulls == length) {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %f NULL\n",
                               i, column->values->d[i]);
                        i++;
                    }
                }
                else {
                    while (count--) {
                        printf("%" CPL_SIZE_FORMAT  " %f %d\n", i,
                        column->values->d[i], column->null[i]);
                        i++;
                    } 
                } 
                break;
            case CPL_TYPE_STRING:
                while (count--) {
                    printf("%" CPL_SIZE_FORMAT  " %s\n", i,
                    column->values->s[i] ? column->values->s[i] : "NULL");
                    i++;
                } 
                break;
            default:
                break;
            }
        }
    }
    else {
        printf("Column is NULL\n");
        cpl_errorstate_set(prevstate);
    }
    
}


/*
 * @brief
 *   Create a new empty column (private).
 *
 * @param type    Column type.
 *
 * @return Pointer to the new column, or @c NULL.
 *
 * This private function allocates memory for a column, its type is 
 * assigned, and the container of column values is initialized together 
 * with all the other column properties.
 */

static cpl_column *cpl_column_new(cpl_type type)
{

    cpl_column *column = cpl_calloc(1, sizeof(cpl_column));


    column->values     = cpl_column_values_new();
    column->name       = NULL;
    column->unit       = NULL;
    column->format     = NULL;
    column->length     = 0;
    column->depth      = 0;
    column->type       = type;
    column->savetype   = type & (~CPL_TYPE_POINTER);
    column->null       = NULL;
    column->nullcount  = 0;
    column->dimensions = NULL;

    return column;

}


/* 
 * @brief
 *   Create a new @em integer column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for a column, its type is assigned,
 * and its number of elements is allocated. All column elements are 
 * flagged as invalid. If a negative length is specified, an error 
 * @c CPL_ERROR_ILLEGAL_INPUT is set. Zero length columns are allowed.
 */

cpl_column *cpl_column_new_int(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_INT);

    if (length)
        column->values->i = (int *)cpl_calloc(length, sizeof(int));
    else
        column->values->i = NULL;

    column->format    = cpl_strdup(INT_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/* 
 * @brief
 *   Create a new @em long integer column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for a column, its type is assigned,
 * and its number of elements is allocated. All column elements are
 * flagged as invalid. If a negative length is specified, an error
 * @c CPL_ERROR_ILLEGAL_INPUT is set. Zero length columns are allowed.
 */

cpl_column *cpl_column_new_long(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_LONG);

    if (length)
        column->values->l = (long *)cpl_calloc(length, sizeof(long));
    else
        column->values->l = NULL;

    column->format    = cpl_strdup(LONG_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/*
 * @brief
 *   Create a new @em long long integer column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for a column, its type is assigned,
 * and its number of elements is allocated. All column elements are
 * flagged as invalid. If a negative length is specified, an error
 * @c CPL_ERROR_ILLEGAL_INPUT is set. Zero length columns are allowed.
 */

cpl_column *cpl_column_new_long_long(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_LONG_LONG);

    if (length)
        column->values->ll = (long long *)cpl_calloc(length, sizeof(long long));
    else
        column->values->ll = NULL;

    column->format    = cpl_strdup(LONG_LONG_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/*
 * @brief
 *   Create a new @em cpl_size column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for a column, its type is assigned,
 * and its number of elements is allocated. All column elements are
 * flagged as invalid. If a negative length is specified, an error
 * @c CPL_ERROR_ILLEGAL_INPUT is set. Zero length columns are allowed.
 */

cpl_column *cpl_column_new_cplsize(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_SIZE);

    if (length)
        column->values->sz = (cpl_size *)cpl_calloc(length, sizeof(cpl_size));
    else
        column->values->sz = NULL;

    column->format    = cpl_strdup(SIZE_TYPE_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/*
 * @brief
 *   Create a new @em float column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of @c cpl_column_new_int().
 */

cpl_column *cpl_column_new_float(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_FLOAT);

    if (length)
        column->values->f = (float *)cpl_malloc(length * sizeof(float));
    else
        column->values->f = NULL;

    column->format    = cpl_strdup(FLOAT_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/* 
 * @brief
 *   Create a new @em float complex column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of @c cpl_column_new_int().
 */

cpl_column *cpl_column_new_float_complex(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_FLOAT_COMPLEX);

    if (length)
        column->values->cf = 
           (float complex *)cpl_malloc(length * sizeof(float complex));
    else
        column->values->cf = NULL;

    column->format    = cpl_strdup(FLOAT_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/* 
 * @brief
 *   Create a new @em double column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of @c cpl_column_new_int().
 */

cpl_column *cpl_column_new_double(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_DOUBLE);

    if (length)
        column->values->d = (double *)cpl_malloc(length * sizeof(double));
    else
        column->values->d = NULL;

    column->format    = cpl_strdup(DOUBLE_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/* 
 * @brief
 *   Create a new @em double complex column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of @c cpl_column_new_int().
 */

cpl_column *cpl_column_new_double_complex(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_DOUBLE_COMPLEX);

    if (length)
        column->values->cd = 
           (double complex *)cpl_malloc(length * sizeof(double complex));
    else
        column->values->cd = NULL;

    column->format    = cpl_strdup(DOUBLE_FORM);
    column->length    = length;
    column->nullcount = length;

    return column;

}


/* 
 * @brief
 *   Create a new string column.
 *
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for a column of pointers to @em char,
 * all initialized to @c NULL pointers. No memory is allocated for the 
 * single column elements. If a negative length is specified, an error 
 * @c CPL_ERROR_ILLEGAL_INPUT is set. Zero length columns are allowed. 
 * No memory is allocated for the single column elements.
 */

cpl_column *cpl_column_new_string(cpl_size length)
{

    cpl_column  *column;


    if (length < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_STRING);

    if (length)
        column->values->s = (char **)cpl_calloc(length, sizeof (char *));
    else
        column->values->s = NULL;

    column->format = cpl_strdup(STRING_FORM);
    column->length = length;

    return column;

}


/*
 * @brief
 *   Create a new array column.
 *
 * @param type      Column type
 * @param length    Number of arrays in column.
 * @param depth     Number of elements per array in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for a column, its type is assigned,
 * and its number of elements is allocated. All column elements are
 * flagged as invalid. If a negative length is specified, an error
 * @c CPL_ERROR_ILLEGAL_INPUT is set. Zero length columns are allowed.
 */

cpl_column *cpl_column_new_array(cpl_type type, cpl_size length, cpl_size depth)
{

    cpl_column  *column;


    if (length < 0 || depth < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(type | CPL_TYPE_POINTER);

    if (length)
        column->values->array = cpl_calloc(length, sizeof (cpl_array *));
    else
        column->values->array = NULL;

    switch(type & ~CPL_TYPE_POINTER) {
    case CPL_TYPE_INT: 
        column->format = cpl_strdup(INT_FORM);
        break;
    case CPL_TYPE_LONG:
        column->format = cpl_strdup(LONG_FORM);
        break;
    case CPL_TYPE_LONG_LONG:
        column->format = cpl_strdup(LONG_LONG_FORM);
        break;
    case CPL_TYPE_SIZE:
        column->format = cpl_strdup(SIZE_TYPE_FORM);
        break;
    case CPL_TYPE_FLOAT:
        column->format = cpl_strdup(FLOAT_FORM);
        break;
    case CPL_TYPE_DOUBLE: 
        column->format = cpl_strdup(DOUBLE_FORM);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        column->format = cpl_strdup(FLOAT_FORM);
        break;
    case CPL_TYPE_DOUBLE_COMPLEX: 
        column->format = cpl_strdup(DOUBLE_FORM);
        break;
    case CPL_TYPE_STRING:
        column->format = cpl_strdup(STRING_FORM);
        break;
    default:
        cpl_column_delete(column);
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    column->length = length;
    column->depth = depth;

    return column;

}

/*
 * @brief
 *   Create a new complex array column from a pair of non-complex ones
 *
 * @param real      Column of real values, double or float
 * @param imag      Column of imaginary values, of matching type and length
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for a column, and fills it with values
 * from the two input columns, which must be of matching type and length.
 * For each row any invalid values are or'ed together to form the output value.
 */
cpl_column *cpl_column_new_complex_from_arrays(const cpl_column * real,
                                               const cpl_column * imag)
{
    cpl_column * self = NULL;
    const size_t   length = (size_t)cpl_column_get_size(real);
    const cpl_type type   = cpl_column_get_type(imag);


    if (real == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }
    if (imag == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if ((size_t)cpl_column_get_size(imag) != length) {
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT, "type=%s. "
                                     "length: %zu != %zu",
                                     cpl_type_get_name(type),
                                     length, (size_t)cpl_column_get_size(imag));
        return NULL;
    }

    if (cpl_column_get_type(imag) != type) {
        (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH, "types: %s != %s."
                                     " length=%zu", cpl_type_get_name(type),
                                     cpl_type_get_name
                                     (cpl_column_get_type(imag)), length);
        return NULL;
    }

    if (type == CPL_TYPE_DOUBLE) {
        self = cpl_column_new_double_complex((cpl_size)length);

        if ((size_t)real->nullcount < length &&
            (size_t)imag->nullcount < length) {
            for (size_t i = 0; i < length; i++) {
                if ((real->nullcount == 0 || real->null[i] == 0) &&
                    (imag->nullcount == 0 || imag->null[i] == 0)) {
                    self->values->cd[i] = 
                        real->values->d[i] +
                        imag->values->d[i] * I;

                    cpl_column_unset_null(self, i);
                }
            }
        }
    } else if (type == CPL_TYPE_FLOAT) {
        self = cpl_column_new_float_complex((cpl_size)length);

        if ((size_t)real->nullcount < length &&
            (size_t)imag->nullcount < length) {
            for (size_t i = 0; i < length; i++) {
                if ((real->nullcount == 0 || real->null[i] == 0) &&
                    (imag->nullcount == 0 || imag->null[i] == 0)) {
                    self->values->cf[i] = 
                        real->values->f[i] +
                        imag->values->f[i] * I;

                    cpl_column_unset_null(self, i);
                }
            }
        }
    } else {
        (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "type=%s. "
                                     "length=%zu", cpl_type_get_name(type),
                                     length);
        return NULL;
    }

    return self;
}

/*
 * @brief
 *   Change saving type for a given column.
 *
 * @param column Column.
 * @param type   New saving type for this column.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any argument is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified column cannot be saved with the specified type</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function indicates that the specified column should be saved
 * by the function @c cpl_table_save() into the specified type.
 * If this function is not called for a given column, that column
 * would be saved into the same type of the column. It is not
 * possible to save an integer column into a floating point type,
 * or the other way around: if you need to do so, use the function
 * @c cpl_table_cast_column() first. For each column type, the
 * (currently) legal saving types are listed below, together with
 * the FITS types they would map into:
 *
 * @code
 *
 *  CPL_TYPE_INT:        CPL_TYPE_INT        TINT        (default)
 *                       CPL_TYPE_BOOL       TLOGICAL
 *                       CPL_TYPE_CHAR       TSBYTE
 *                       CPL_TYPE_UCHAR      TBYTE
 *                       CPL_TYPE_SHORT      TSHORT
 *  CPL_TYPE_LONG:       CPL_TYPE_LONG       TLONG       (default)
 *                       CPL_TYPE_INT        TINT
 *                       CPL_TYPE_BOOL       TLOGICAL
 *                       CPL_TYPE_CHAR       TSBYTE
 *                       CPL_TYPE_UCHAR      TBYTE
 *                       CPL_TYPE_SHORT      TSHORT
 *  CPL_TYPE_LONG_LONG:  CPL_TYPE_LONG_LONG  TLONGLONG   (default)
 *                       CPL_TYPE_LONG       TLONG
 *                       CPL_TYPE_INT        TINT
 *                       CPL_TYPE_BOOL       TLOGICAL
 *                       CPL_TYPE_CHAR       TSBYTE
 *                       CPL_TYPE_UCHAR      TBYTE
 *                       CPL_TYPE_SHORT      TSHORT
 *  CPL_TYPE_SIZE:       CPL_TYPE_SIZE       TLONGLONG   (default)
 *                       CPL_TYPE_LONG       TLONG
 *                       CPL_TYPE_INT        TINT
 *                       CPL_TYPE_BOOL       TLOGICAL
 *                       CPL_TYPE_CHAR       TSBYTE
 *                       CPL_TYPE_UCHAR      TBYTE
 *                       CPL_TYPE_SHORT      TSHORT
 *  CPL_TYPE_FLOAT:      CPL_TYPE_FLOAT      TFLOAT      (default)
 *  CPL_TYPE_DOUBLE:     CPL_TYPE_DOUBLE     TDOUBLE     (default)
 *  CPL_TYPE_STRING:     CPL_TYPE_STRING     TSTRING     (default)
 *
 * @endcode
 *
 * As it can be seen, only CPL_TYPE_BOOL is currently supported.
 */

cpl_error_code cpl_column_set_save_type(cpl_column *column, cpl_type type)
{
    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    type &= ~CPL_TYPE_POINTER;

    switch (column->type & ~CPL_TYPE_POINTER) {
    case CPL_TYPE_INT:
        switch (type) {
        case CPL_TYPE_INT:   /* List of legal types, ended by break */
        case CPL_TYPE_BOOL:
        case CPL_TYPE_CHAR:
        case CPL_TYPE_UCHAR:
        case CPL_TYPE_SHORT:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_LONG:
        switch (type) {
        case CPL_TYPE_LONG:   /* List of legal types, ended by break */
        case CPL_TYPE_INT:
        case CPL_TYPE_BOOL:
        case CPL_TYPE_CHAR:
        case CPL_TYPE_UCHAR:
        case CPL_TYPE_SHORT:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_LONG_LONG:
        switch (type) {
        case CPL_TYPE_LONG_LONG:   /* List of legal types, ended by break */
        case CPL_TYPE_LONG:
        case CPL_TYPE_INT:
        case CPL_TYPE_BOOL:
        case CPL_TYPE_CHAR:
        case CPL_TYPE_UCHAR:
        case CPL_TYPE_SHORT:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_SIZE:
        switch (type) {
        case CPL_TYPE_LONG_LONG:   /* List of legal types, ended by break */
        case CPL_TYPE_LONG:
        case CPL_TYPE_INT:
        case CPL_TYPE_BOOL:
        case CPL_TYPE_CHAR:
        case CPL_TYPE_UCHAR:
        case CPL_TYPE_SHORT:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_FLOAT:
        switch (type) {
        case CPL_TYPE_FLOAT:   /* List of legal types, ended by break */
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_DOUBLE:
        switch (type) {
        case CPL_TYPE_DOUBLE:   /* List of legal types, ended by break */
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        switch (type) {
        case CPL_TYPE_FLOAT_COMPLEX:  /* List of legal types, ended by break */
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_DOUBLE_COMPLEX:
        switch (type) {
        case CPL_TYPE_DOUBLE_COMPLEX: /* List of legal types, ended by break */
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    case CPL_TYPE_STRING:
        switch (type) {
        case CPL_TYPE_STRING:   /* List of legal types, ended by break */
            break;
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    default:
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    column->savetype = type;
    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Get the saving type of a column.
 *
 * @param column  Column to get the saving type from.
 *
 * @return Saving type of column, or @c CPL_TYPE_INVALID if a @c NULL column is
 *   passed to the function.
 *
 * If the column is @c NULL, @c CPL_ERROR_NULL_INPUT is set.
 */

cpl_type cpl_column_get_save_type(const cpl_column *column)
{
    if (column)
        return column->savetype;

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return CPL_TYPE_INVALID;
}


/*
 * @brief
 *   Create a new @em integer column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * This function creates a new @em integer column that will encapsulate 
 * the given data. Note that the size of the data array is not checked in
 * any way, and that the data values are all considered valid: invalid
 * values should be marked using the functions @c cpl_column_set_invalid()
 * and @c cpl_column_fill_invalid(). The data array is not copied,
 * so it should never be deallocated: to deallocate it, the function
 * @c cpl_column_delete() should be called instead. Alternatively, the
 * function @c cpl_column_unwrap() might be used, and the data array
 * deallocated afterwards. A zero or negative length is illegal, and
 * would cause an error @c CPL_ERROR_ILLEGAL_INPUT to be set. An input
 * @c NULL pointer would set an error @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle columns assume that a column data array
 *   is dynamically allocated: with a statically allocated array
 *   any function implying memory handling (@c cpl_column_set_size(),
 *   @c cpl_column_delete(), etc.) would crash the program. This means
 *   that a static array should never be passed to this function if
 *   memory handling is planned. In case of a static array, only the
 *   @c cpl_column_unwrap() destructor can be used.
 */

cpl_column *cpl_column_wrap_int(int *data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_INT);

    column->format    = cpl_strdup(INT_FORM);
    column->length    = length;
    column->values->i = data;

    return column;

}


/*
 * @brief
 *   Create a new @em long @em integer column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * This function creates a new @em long @em integer column that will
 * encapsulate the given data. Note that the size of the data array is
 * not checked in any way, and that the data values are all considered valid:
 * invalid values should be marked using the functions
 * @c cpl_column_set_invalid() and @c cpl_column_fill_invalid().
 * The data array is not copied, so it should never be deallocated: to
 * deallocate it, the function @c cpl_column_delete() should be called
 * instead. Alternatively, the function @c cpl_column_unwrap() might be used,
 * and the data array deallocated afterwards. A zero or negative length is
 * illegal, and would cause an error @c CPL_ERROR_ILLEGAL_INPUT to be set.
 * An input @c NULL pointer would set an error @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle columns assume that a column data array
 *   is dynamically allocated: with a statically allocated array
 *   any function implying memory handling (@c cpl_column_set_size(),
 *   @c cpl_column_delete(), etc.) would crash the program. This means
 *   that a static array should never be passed to this function if
 *   memory handling is planned. In case of a static array, only the
 *   @c cpl_column_unwrap() destructor can be used.
 */

cpl_column *cpl_column_wrap_long(long *data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_LONG);

    column->format    = cpl_strdup(LONG_FORM);
    column->length    = length;
    column->values->l = data;

    return column;

}


/*
 * @brief
 *   Create a new @em long @em long @em integer column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * This function creates a new @em long @em long @em integer column that
 * will encapsulate the given data. Note that the size of the data array is
 * not checked in any way, and that the data values are all considered valid:
 * invalid values should be marked using the functions
 * @c cpl_column_set_invalid() and @c cpl_column_fill_invalid(). The data
 * array is not copied, so it should never be deallocated: to deallocate it,
 * the function @c cpl_column_delete() should be called instead.
 * Alternatively, the function @c cpl_column_unwrap() might be used, and
 * the data array deallocated afterwards. A zero or negative length is
 * illegal, and would cause an error @c CPL_ERROR_ILLEGAL_INPUT to be set.
 * An input @c NULL pointer would set an error @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle columns assume that a column data array
 *   is dynamically allocated: with a statically allocated array
 *   any function implying memory handling (@c cpl_column_set_size(),
 *   @c cpl_column_delete(), etc.) would crash the program. This means
 *   that a static array should never be passed to this function if
 *   memory handling is planned. In case of a static array, only the
 *   @c cpl_column_unwrap() destructor can be used.
 */

cpl_column *cpl_column_wrap_long_long(long long *data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_LONG_LONG);

    column->format     = cpl_strdup(LONG_LONG_FORM);
    column->length     = length;
    column->values->ll = data;

    return column;

}


/*
 * @brief
 *   Create a new @em cpl_size column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * This function creates a new @em cpl_size column that will encapsulate
 * the given data. Note that the size of the data array is not checked
 * in any way, and that the data values are all considered valid:
 * invalid values should be marked using the functions
 * @c cpl_column_set_invalid() and @c cpl_column_fill_invalid(). The data
 * array is not copied, so it should never be deallocated: to deallocate it,
 * the function @c cpl_column_delete() should be called instead.
 * Alternatively, the function @c cpl_column_unwrap() might be used, and
 * the data array deallocated afterwards. A zero or negative length is
 * illegal, and would cause an error @c CPL_ERROR_ILLEGAL_INPUT to be set.
 * An input @c NULL pointer would set an error @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle columns assume that a column data array
 *   is dynamically allocated: with a statically allocated array
 *   any function implying memory handling (@c cpl_column_set_size(),
 *   @c cpl_column_delete(), etc.) would crash the program. This means
 *   that a static array should never be passed to this function if
 *   memory handling is planned. In case of a static array, only the
 *   @c cpl_column_unwrap() destructor can be used.
 */

cpl_column *cpl_column_wrap_cplsize(cpl_size *data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_SIZE);

    column->format     = cpl_strdup(SIZE_TYPE_FORM);
    column->length     = length;
    column->values->sz = data;

    return column;

}


/* 
 * @brief
 *   Create a new @em float column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_column_wrap_int().
 */

cpl_column *cpl_column_wrap_float(float *data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_FLOAT);

    column->format    = cpl_strdup(FLOAT_FORM);
    column->length    = length;
    column->values->f = data;

    return column;

}


/* 
 * @brief
 *   Create a new @em float complex column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_column_wrap_int().
 */

cpl_column *cpl_column_wrap_float_complex(float complex *data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_FLOAT_COMPLEX);

    column->format     = cpl_strdup(FLOAT_FORM);
    column->length     = length;
    column->values->cf = data;

    return column;

}


/* 
 * @brief
 *   Create a new @em double column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_column_wrap_int().
 */

cpl_column *cpl_column_wrap_double(double *data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_DOUBLE);

    column->format    = cpl_strdup(DOUBLE_FORM);
    column->length    = length;
    column->values->d = data;

    return column;

}


/* 
 * @brief
 *   Create a new @em double complex column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_column_wrap_int().
 */

cpl_column *cpl_column_wrap_double_complex(double complex *data, 
                                           cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_DOUBLE_COMPLEX);

    column->format     = cpl_strdup(DOUBLE_FORM);
    column->length     = length;
    column->values->cd = data;

    return column;

}


/* 
 * @brief
 *   Create a new character string column from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in column.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_column_wrap_int().
 */

cpl_column *cpl_column_wrap_string(char **data, cpl_size length)
{

    cpl_column  *column;


    if (data == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (length <= 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    column = cpl_column_new(CPL_TYPE_STRING);

    column->format    = cpl_strdup(STRING_FORM);
    column->length    = length;
    column->values->s = data;

    return column;

}


/* 
 * @brief
 *   Copy array of numerical data to a numerical column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the column is not a numerical 
 *   column, a @c CPL_ERROR_INVALID_TYPE is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified column. If the type of the
 * accessed column is not double, data values will be truncated according
 * to C casting rules. The size of the input array is not checked in any 
 * way, and the data values are all considered valid: invalid values 
 * should be marked using the functions @c cpl_column_set_invalid() and 
 * @c cpl_column_fill_invalid(). If @em N is the length of the column,
 * the first @em N values of the input data buffer would be copied to
 * the column buffer. If the column had length zero, no values would
 * be copied.
 */

cpl_error_code cpl_column_copy_data(cpl_column *column, const double *data)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (length == 0)
        return CPL_ERROR_NONE;


    switch (type) {
    case CPL_TYPE_INT:
    {
        int *idata = cpl_column_get_data_int(column);
        while (length--)
            *idata++ = *data++;
        break;
    }
    case CPL_TYPE_LONG:
    {
        long *ldata = cpl_column_get_data_long(column);
        while (length--)
            *ldata++ = *data++;
        break;
    }
    case CPL_TYPE_LONG_LONG:
    {
        long long *lldata = cpl_column_get_data_long_long(column);
        while (length--)
            *lldata++ = *data++;
        break;
    }
    case CPL_TYPE_SIZE:
    {
        cpl_size *szdata = cpl_column_get_data_cplsize(column);
        while (length--)
            *szdata++ = *data++;
        break;
    }
    case CPL_TYPE_FLOAT:
    {
        float *fdata = cpl_column_get_data_float(column);
        while (length--)
            *fdata++ = *data++;
        break;
    }
    case CPL_TYPE_DOUBLE:
    {
        memcpy(column->values->d, data, column->length * sizeof(double));
        break;
    }
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy array of complex data to a complex column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the column is not a complex 
 *   column, a @c CPL_ERROR_INVALID_TYPE is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified column. If the type of the
 * accessed column is not double, data values will be truncated according
 * to C casting rules. The size of the input array is not checked in any 
 * way, and the data values are all considered valid: invalid values 
 * should be marked using the functions @c cpl_column_set_invalid() and 
 * @c cpl_column_fill_invalid(). If @em N is the length of the column,
 * the first @em N values of the input data buffer would be copied to
 * the column buffer. If the column had length zero, no values would
 * be copied.
 */

cpl_error_code cpl_column_copy_data_complex(cpl_column *column,
                                            const double complex *data)
{

    cpl_size       length = cpl_column_get_size(column);
    cpl_type       type   = cpl_column_get_type(column);
    

    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (length == 0)
        return CPL_ERROR_NONE;


    switch (type) {
    case CPL_TYPE_FLOAT_COMPLEX:
    {
        float complex *cfdata = cpl_column_get_data_float_complex(column);
        while (length--)
            *cfdata++ = *data++;
        break;
    }
    case CPL_TYPE_DOUBLE_COMPLEX:
        memcpy(column->values->cd, data, 
               column->length * sizeof(double complex));
        break;
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy existing data to an @em integer column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return 0 on success. If the input column is not integer, a 
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified column. The size of the 
 * input array is not checked in any way, and the data values are all 
 * considered valid: invalid values should be marked using the functions 
 * @c cpl_column_set_invalid() and @c cpl_column_fill_invalid(). If @em N 
 * is the length of the column, the first @em N values of the input data 
 * buffer would be copied to the column buffer. If the column had length 
 * zero, no values would be copied.
 */

cpl_error_code cpl_column_copy_data_int(cpl_column *column, const int *data)
{

    cpl_type     type = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_INT)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->i, data, column->length * sizeof(int));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy existing data to a @em long @em integer column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return 0 on success. If the input column is not long integer, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified column. The size of the
 * input array is not checked in any way, and the data values are all
 * considered valid: invalid values should be marked using the functions
 * @c cpl_column_set_invalid() and @c cpl_column_fill_invalid(). If @em N
 * is the length of the column, the first @em N values of the input data
 * buffer would be copied to the column buffer. If the column had length
 * zero, no values would be copied.
 */

cpl_error_code cpl_column_copy_data_long(cpl_column *column, const long *data)
{

    cpl_type     type = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_LONG)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->l, data, column->length * sizeof(long));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy existing data to a @em long @em long @em integer column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return 0 on success. If the input column is not long long integer, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified column. The size of the
 * input array is not checked in any way, and the data values are all
 * considered valid: invalid values should be marked using the functions
 * @c cpl_column_set_invalid() and @c cpl_column_fill_invalid(). If @em N
 * is the length of the column, the first @em N values of the input data
 * buffer would be copied to the column buffer. If the column had length
 * zero, no values would be copied.
 */

cpl_error_code cpl_column_copy_data_long_long(cpl_column *column,
                                              const long long *data)
{

    cpl_type     type = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_LONG_LONG)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->ll, data, column->length * sizeof(long long));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy existing data to a @em cpl_size column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return 0 on success. If the input column is not long long integer, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified column. The size of the
 * input array is not checked in any way, and the data values are all
 * considered valid: invalid values should be marked using the functions
 * @c cpl_column_set_invalid() and @c cpl_column_fill_invalid(). If @em N
 * is the length of the column, the first @em N values of the input data
 * buffer would be copied to the column buffer. If the column had length
 * zero, no values would be copied.
 */

cpl_error_code cpl_column_copy_data_cplsize(cpl_column *column,
                                            const cpl_size *data)
{

    cpl_type    type = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_SIZE)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->sz, data, column->length * sizeof(cpl_size));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy existing data to a @em float column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return 0 on success.  If the input column is not float, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * See documentation of function @c cpl_column_copy_data_int().
 */

cpl_error_code cpl_column_copy_data_float(cpl_column *column, 
                                          const float *data)
{

    cpl_type     type = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_FLOAT)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->f, data, column->length * sizeof(float));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy existing data to a @em float complex column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return 0 on success.  If the input column is not float complex, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * See documentation of function @c cpl_column_copy_data_int().
 */

cpl_error_code cpl_column_copy_data_float_complex(cpl_column *column,
                                                  const float complex *data)
{

    cpl_type    type = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_FLOAT_COMPLEX)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->cf, data, column->length * sizeof(float complex));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy existing data to a @em double column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *   
 * @return 0 on success. If the input column is not double, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * See documentation of function @c cpl_column_copy_data_int().
 */

cpl_error_code cpl_column_copy_data_double(cpl_column *column, 
                                           const double *data)
{

    cpl_type     type = cpl_column_get_type(column); 


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_DOUBLE)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->d, data, column->length * sizeof(double));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy existing data to a @em double complex column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *
 * @return 0 on success.  If the input column is not double complex, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * See documentation of function @c cpl_column_copy_data_int().
 */

cpl_error_code cpl_column_copy_data_double_complex(cpl_column *column,
                                                   const double complex *data)
{

    cpl_type    type = cpl_column_get_type(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_DOUBLE_COMPLEX)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->length == 0)
        return CPL_ERROR_NONE;

    memcpy(column->values->cd, data, column->length * sizeof(double complex));

    if (column->null)
        cpl_free(column->null);
    column->null = NULL;
    column->nullcount = 0;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy existing data to a @em string column.
 *
 * @param column    Existing column.
 * @param data      Existing data buffer.
 *   
 * @return 0 on success. If the input column is not of type string, a
 *   @c CPL_ERROR_TYPE_MISMATCH is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * See documentation of function @c cpl_column_copy_data_int().
 * 
 * The input data are copied into the specified column. The size of the
 * input array is not checked in any way. The strings pointed by the input
 * buffer are all duplicated, while the strings contained in the column 
 * are released before being overwritten. If @em N is the length of the 
 * column, the first @em N values of the input data buffer would be copied 
 * to the column buffer. If the column had length zero, no values would be 
 * copied.
 */

cpl_error_code cpl_column_copy_data_string(cpl_column *column, 
                                           const char **data)
{

    cpl_type     type   = cpl_column_get_type(column); 
    cpl_size     length = cpl_column_get_size(column);


    if (data == NULL || column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type != CPL_TYPE_STRING)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);


    while (length--) {
        if (column->values->s[length])
            cpl_free(column->values->s[length]);
        if (data[length])
            column->values->s[length] = cpl_strdup(data[length]);
        else
            column->values->s[length] = NULL;
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Delete a column.
 *
 * @param column  Column to be deleted.
 *
 * @return Nothing.
 *
 * This function deletes a column. If the input column is @c NULL, 
 * nothing is done, and no error is set.
 */

void cpl_column_delete(cpl_column *column)
{

    cpl_type           type;
    cpl_column_values *values;
    cpl_size           length;


    if (column != NULL) {
        type   = cpl_column_get_type(column);
        values = cpl_column_get_values(column);
        length = cpl_column_get_size(column);

        cpl_column_values_delete(values, length, type);
        if (column->name)
            cpl_free(column->name);
        if (column->unit)
            cpl_free(column->unit);
        if (column->format)
            cpl_free(column->format);
        if (column->null)
            cpl_free(column->null);
        if (column->dimensions)
            cpl_array_delete(column->dimensions);
        cpl_free(column);
    }

}


/* 
 * @brief
 *   Delete a column, without losing the data.
 *
 * @param column  Column to be deleted.
 *
 * @return Pointer to the internal data buffer.
 *
 * This function deletes a column, but the data buffer is not destroyed.
 * Supposedly, the developer knows that the data are static, or the
 * developer holds the pointer to the data obtained with the functions
 * @c cpl_column_get_data_int(), @c cpl_column_get_data_float(), etc.
 * If the input column is @c NULL, nothing is done, and no error is set.
 */

void *cpl_column_unwrap(cpl_column *column)
{
 
    void *d = NULL;
 
    if (column != NULL) {
        cpl_column_values *values = cpl_column_get_values(column);
        d = (void *)values->i;
        cpl_free(values);
        if (column->name)
            cpl_free(column->name);
        if (column->unit)
            cpl_free(column->unit);
        if (column->format)
            cpl_free(column->format);
        if (column->null)
            cpl_free(column->null);
        if (column->dimensions)
            cpl_array_delete(column->dimensions);
        cpl_free(column);
    }

    return d;

}


/* 
 * @brief
 *   Delete a string column, without losing the single strings.
 *
 * @param column  Column to be deleted.
 *
 * @return Nothing, but if the column is not of string type, a 
 * @c CPL_ERROR_INVALID_TYPE is set.
 *
 * This function deletes a string column, but the single strings pointed
 * by the data buffer are not destroyed. Supposedly, the developer knows
 * that the strings are static, or the developer holds the pointers to the
 * strings somewhere else. If the input column is @c NULL, or the input
 * column is not a string column, nothing is done.
 */

void cpl_column_delete_but_strings(cpl_column *column)
{

    cpl_type           type;
    cpl_column_values *values;


    if (column == NULL)
        return;

    type = cpl_column_get_type(column);

    if (type == CPL_TYPE_STRING) {
        values = cpl_column_get_values(column);
        if (values) {
            if (values->s)
                cpl_free(values->s);
            cpl_free(values);
        }
        if (column->name)
            cpl_free(column->name);
        if (column->unit)
            cpl_free(column->unit);
        if (column->format)
            cpl_free(column->format);
        if (column->null)
            cpl_free(column->null);
        cpl_free(column);
    }
    else
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);

}


/*
 * @brief
 *   Delete an array column, without losing the single arrays.
 *
 * @param column  Column to be deleted.
 *
 * @return Nothing, but if the column is not of array type, a
 * @c CPL_ERROR_INVALID_TYPE is set.
 *   
 * This function deletes an array column, but the single arrays pointed
 * by the data buffer are not destroyed. Supposedly, the developer knows
 * that the arrays are static, or the developer holds the pointers to the
 * arrays somewhere else. If the input column is @c NULL, or the input
 * column is not an array column, nothing is done.
 */

void cpl_column_delete_but_arrays(cpl_column *column)
{

    cpl_type           type;
    cpl_column_values *values;


    if (column == NULL)
        return;

    type = cpl_column_get_type(column);

    if (type & CPL_TYPE_POINTER) {
        values = cpl_column_get_values(column);
        if (values) {
            if (values->array)
                cpl_free(values->array);
            cpl_free(values);
        }
        if (column->name)
            cpl_free(column->name);
        if (column->unit)
            cpl_free(column->unit);
        if (column->format)
            cpl_free(column->format);
        if (column->null)
            cpl_free(column->null);
        if (column->dimensions)
            cpl_array_delete(column->dimensions);
        cpl_free(column);
    }
    else
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);

}


/* 
 * @brief
 *   Give a new name to a column.
 *
 * @param column  Column to be named.
 * @param name    New column name.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is @c NULL,
 *   a @c CPL_ERROR_NULL_INPUT is returned.
 *
 * The input name is duplicated before being used as the column name. 
 * If the new name is a @c NULL pointer the column will be nameless.
 */

cpl_error_code cpl_column_set_name(cpl_column *column, const char *name)
{



    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (column->name)
        cpl_free(column->name);

    if (name)
        column->name = cpl_strdup(name);
    else
        column->name = NULL;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Get the name of a column.
 *
 * @param column  Column to get the name from.
 *
 * @return Name of column, or @em NULL if the column is nameless, or 
 *   if the input column is a @c NULL pointer. In the latter case, a 
 *   @c CPL_ERROR_NULL_INPUT is set.
 *
 * Return the name of a column, if present. Note that the returned 
 * string is a pointer to the column name, not its copy. Its manipulation
 * will directly affect the column name, while changing the column name
 * using @c cpl_column_set_name() will turn it into garbage. Therefore,
 * if a real copy of a column name is required, this function should be
 * called as an argument of the function @c cpl_strdup(). If the input column 
 * has no name, a @c NULL is returned.
 */

const char *cpl_column_get_name(const cpl_column *column)
{

    if (column)
        return column->name;

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return NULL;

}


/* 
 * @brief
 *   Give a new unit to a column.
 * 
 * @param column  Column to access.
 * @param unit    New column unit.
 *
 * @return 0 on success. Currently this function always succeeds.
 * 
 * The input unit is duplicated before being used as the column unit.
 * If the new unit is a @c NULL pointer the column will be unitless. 
 * The unit string associated to a column has no effect on any operation 
 * performed on columns, and it must be considered just an optional 
 * description of the content of a column.
 */ 

cpl_error_code cpl_column_set_unit(cpl_column *column, const char *unit)
{

    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (column->unit)
        cpl_free(column->unit);

    if (unit)
        column->unit = cpl_strdup(unit);
    else
        column->unit = NULL;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Get the unit of a column.
 *
 * @param column  Column to get the unit from.
 *
 * @return Unit of column, or @c NULL if the column is unitless, or if the
 * input column is a @c NULL. In the latter case, a @c CPL_ERROR_NULL_INPUT is
 * set.
 *
 * Return the unit of a column, if present. Note that the returned
 * string is a pointer to the column unit, not its copy. Its manipulation
 * will directly affect the column unit, while changing the column unit
 * using @c cpl_column_set_unit() will turn it into garbage. Therefore,
 * if a real copy of a column unit is required, this function should be
 * called as an argument of the function @c cpl_strdup(). If the input column
 * has no unit, a @c NULL is returned.
 */

const char *cpl_column_get_unit(const cpl_column *column)
{

    if (column)
        return column->unit;

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return NULL;

}


/* 
 * @brief
 *   Give a new format to a column.
 *
 * @param column  Column to access.
 * @param format  New column format.
 *
 * @return 0 on success. Currently this function always succeeds.
 *
 * The input format is duplicated before being used as the column format.
 * If a @c NULL @em format is given, "%s" will be used if the column is
 * of type string, "% 1.5e" if the column is of type float or double, and 
 * "% 7d" if it is of type integer, long integer, or long long integer.
 * The format string associated to a column has no effect on any operation
 * performed on columns, and it is used just while printing a column.
 * The given format string must conform to the legal standard C formats.
 */

cpl_error_code cpl_column_set_format(cpl_column *column, const char *format)
{

    cpl_type type;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    type = cpl_column_get_type(column);

    if (column->format)
        cpl_free(column->format);

    if (format)
        column->format = cpl_strdup(format);
    else
        switch (type & ~CPL_TYPE_POINTER) {
        case CPL_TYPE_INT:
            column->format = cpl_strdup(INT_FORM);
            break;
        case CPL_TYPE_LONG:
            column->format = cpl_strdup(LONG_FORM);
            break;
        case CPL_TYPE_LONG_LONG:
            column->format = cpl_strdup(LONG_LONG_FORM);
            break;
        case CPL_TYPE_SIZE:
            column->format = cpl_strdup(SIZE_TYPE_FORM);
            break;
        case CPL_TYPE_FLOAT:
            column->format = cpl_strdup(FLOAT_FORM);
            break;
        case CPL_TYPE_DOUBLE:
            column->format = cpl_strdup(DOUBLE_FORM);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            column->format = cpl_strdup(FLOAT_FORM);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            column->format = cpl_strdup(DOUBLE_FORM);
            break;
        case CPL_TYPE_STRING:
            column->format = cpl_strdup(STRING_FORM);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }

    return CPL_ERROR_NONE;
    
}


/*  
 * @brief
 *   Get the format of a column.
 *  
 * @param column  Column to get the format from.
 *
 * @return Column format, or @c NULL. The latter case can occur only if a
 *   @c NULL column is passed to the function, and a @c CPL_ERROR_NULL_INPUT 
 *   is set.
 *
 * Return the format string of a column. Note that the returned string 
 * is a pointer to the column format, not its copy. Its manipulation
 * will directly affect the column format, while changing the column format
 * using @c cpl_column_set_format() will turn it into garbage. Therefore,
 * if a real copy of a column format is required, this function should be
 * called as an argument of the function @c cpl_strdup(). The function accepts 
 * also a @c NULL input column.
 */

const char *cpl_column_get_format(const cpl_column *column)
{

    if (column)
        return column->format;

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return NULL;

}


/* 
 * @internal
 * @brief
 *   Get the length of a column.
 *
 * @param self  Column to get the length from.
 *
 * @return Length of column, or zero.
 *
 * No @c NULL check is performed.
 */
static inline cpl_size cpl_column_get_size_(const cpl_column *self)
{
    return self->length;
}

/* 
 * @brief
 *   Get the length of a column.
 *
 * @param column  Column to get the length from.
 *
 * @return Length of column, or zero. The latter case can occur either
 * with a column having zero length, or if a NULL column is passed to 
 * the function, but in the latter case a @c CPL_ERROR_NULL_INPUT is set. 
 *
 * If the column is @em NULL, zero is returned.
 */

cpl_size cpl_column_get_size(const cpl_column *column)
{

    if (column)
        return cpl_column_get_size_(column);

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return 0;

}

/*
 * @brief
 *   Get the depth of a column.
 *
 * @param column  Column to get the depth from.
 *
 * @return Depth of column, or zero. The latter case can occur either
 * with a non-array column, or if a NULL column is passed to this
 * function, but in the latter case a @c CPL_ERROR_NULL_INPUT is set.
 *
 * If the column is @em NULL, zero is returned.
 */

cpl_size cpl_column_get_depth(const cpl_column *column)
{

    if (column)
        return column->depth;

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return 0;

}


/*
 * @brief
 *   Get the number of dimensions of a column.
 *
 * @param column  Column to get the number of dimensions from.
 *
 * @return Number of dimensions of column, or zero. The latter case can 
 * occur if a NULL column is passed to this function, and in that case
 * @c CPL_ERROR_NULL_INPUT is also set. If a column is not an array
 * column, or if it has no dimensions, 1 is returned.
 *
 * If the column is @em NULL, zero is returned. 
 */

cpl_size cpl_column_get_dimensions(const cpl_column *column)
{

    if (column) {
        if (cpl_column_get_type(column) & CPL_TYPE_POINTER)
            if (column->dimensions)
                return cpl_array_get_size(column->dimensions);
        return 1;
    }

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return 0;

}


/*
 * @brief
 *   Set the dimensions of a column.
 *
 * @param column      Column to be assigned the dimensions.
 * @param dimensions  Integer array containing the sizes of the column
 *
 * @return The column must be of type array, or a @c CPL_ERROR_TYPE_MISMATCH 
 * is returned.  A @c CPL_ERROR_INCOMPATIBLE_INPUT is returned if the 
 * specified sizes are incompatible with the total number of elements in 
 * the column. The input array @em dimensions must be an integer array  
 * and all of its elements must be valid, or a @c CPL_ERROR_ILLEGAL_INPUT 
 * is returned. If this array has size less than 2, nothing is done and 
 * no error is returned. If a NULL column is passed to this function, a 
 * @c CPL_ERROR_NULL_INPUT is set. 
 *
 * Set the dimensions of a column. 
 */

cpl_error_code cpl_column_set_dimensions(cpl_column *column, 
                                         const cpl_array *dimensions)
{

    cpl_type type;
    cpl_size ndim;
    cpl_size size = 1;

    if (column == NULL || dimensions == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    type = cpl_column_get_type(column);

    if (type & CPL_TYPE_POINTER) {
        type = cpl_array_get_type(dimensions);

        if (!(type & CPL_TYPE_INT))
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

        ndim = cpl_array_get_size(dimensions);

        if (ndim < 2)
            return CPL_ERROR_NONE;

        if (cpl_array_has_invalid(dimensions))
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

        while (ndim--)
            size *= cpl_array_get(dimensions, ndim, NULL);

        if (size != column->depth)
            return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

        if (column->dimensions)
            cpl_array_delete(column->dimensions);

        column->dimensions = cpl_array_duplicate(dimensions);
    }
    else
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Get size of one dimension of the column.
 *
 * @param column  Column.
 * @param indx    Indicate dimension to query (0 = x, 1 = y, 2 = z, etc.)
 *
 * @return Size of queried dimension of the column, or zero in case of error.
 *
 * If the column is not of type array, a @c CPL_ERROR_UNSUPPORTED_MODE is set.
 * A @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set if the specified @em indx
 * is not compatible with the column dimensions. If the column has no
 * dimensions, 1 is returned.
 *
 * Get size of one dimension of the column.
 */

cpl_size cpl_column_get_dimension(const cpl_column *column, cpl_size indx)
{
    
    cpl_type type;
    cpl_size ndim;
    

    if (column == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    if (indx < 0) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return 0;
    }

    type = cpl_column_get_type(column);

    if (type & CPL_TYPE_POINTER) {

        if (column->dimensions)
            ndim = cpl_array_get_size(column->dimensions);
        else
            ndim = 1;

        if (indx >= ndim) {
            cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
            return 0;
        }

        if (ndim > 1)
            return cpl_array_get(column->dimensions, indx, NULL);
        else
            return column->depth;

    }

    return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);



}


/*
 * @brief
 *   Get the type of a column.
 *
 * @param self  Column to get the type from.
 *
 * @return Type of column
 *
 * No @c NULL check is performed.
 */
static inline cpl_type cpl_column_get_type_(const cpl_column *self)
{
    return self->type;
}

/*
 * @brief
 *   Get the type of a column.
 *
 * @param column  Column to get the type from.
 *
 * @return Type of column, or @c CPL_TYPE_INVALID if a @c NULL column is 
 *   passed to the function.
 *
 * If the column is @c NULL, @c CPL_ERROR_NULL_INPUT is set.
 */

cpl_type cpl_column_get_type(const cpl_column *column)
{

    if (column)
        return cpl_column_get_type_(column);

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return CPL_TYPE_INVALID;

}


/* 
 * @brief
 *   Check if a basic type column element is invalid.
 *
 * @param self  Column to inquire.
 * @param indx    Column element to inquire.
 * @param length  Length of column
 *
 * @return 1 if the column element is invalid, 0 if not
 *
 * No error checks are performed, only works on primitive types.
 * That excludes CPL_TYPE_STRING and CPL_TYPE_POINTER.
 */

static inline int cpl_column_base_type_invalid_(const cpl_column * self,
                                                cpl_size indx,
                                                cpl_size length)
{
    if (self->nullcount == 0)
        return 0;
    if (self->nullcount == length)
        return 1;

    return self->null[indx];
}


/* 
 * @brief
 *   Check if a column element is invalid.
 *
 * @param column  Column to inquire.
 * @param indx    Column element to inquire.
 *
 * @return 1 if the column element is invalid, 0 if not, or in case
 *   of error.
 *
 * Check if a column element is invalid. Column elements are counted 
 * starting from zero. An out of range index would cause an error 
 * @c CPL_ERROR_ACCESS_OUT_OF_RANGE to be set. If the input column
 * has zero length, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE would always be
 * set. If the input column is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT 
 * is set.
 */

int cpl_column_is_invalid(const cpl_column *column, cpl_size indx)
{

    cpl_type    type;
    cpl_size    length;


    if (column == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    length = cpl_column_get_size_(column);

    if (indx < 0 || indx >= length) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return 0;
    }

    type = cpl_column_get_type_(column);

    if (type == CPL_TYPE_STRING)
        return (column->values->s[indx] == NULL);
    if (type & CPL_TYPE_POINTER)
        return (column->values->array[indx] == NULL);

    return cpl_column_base_type_invalid_(column, indx, length);
}


/* 
 * @brief
 *   Check if a column contains at least one invalid element.
 *
 * @param column  Column to inquire.
 *
 * @return 1 if the column contains at least one invalid element, 0 if not,
 *   -1 in case of error.
 *
 * Check if there are invalid elements in a column. This function does
 * not report the presence of invalid elements within an array type
 * column, but in that case reports just if an array is allocated or
 * not (just like is done for strings). If the input column is a
 * @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is set.
 */

int cpl_column_has_invalid(const cpl_column *column)
{

    cpl_type type   = cpl_column_get_type(column);
    cpl_size length = cpl_column_get_size(column);


    if (column == NULL) {
        cpl_error_set_where_();
        return -1;
    }

    if (type == CPL_TYPE_STRING) {
        while (length--)
            if (column->values->s[length] == NULL)
                return 1;
        return 0;
    }
    if (type & CPL_TYPE_POINTER) {
        while (length--)
            if (column->values->array[length] == NULL)
                return 1;
        return 0;
    }

    return (column->nullcount > 0 ? 1 : 0);

}


/* 
 * @brief
 *   Check if a column contains at least one valid value.
 *
 * @param column  Column to inquire.
 *
 * @return 1 if the column contains at least one valid value, 0 if not
 *   -1 in case of error.
 *
 * Check if there are valid values in a column. If the input column is a
 * @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is set.
 */

int cpl_column_has_valid(const cpl_column *column)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);


    if (column == NULL) {
        cpl_error_set_where_();
        return -1;
    }

    if (type == CPL_TYPE_STRING) {

        while (length--)
            if (column->values->s[length])
                return 1;
        return 0;

    }
    if (type & CPL_TYPE_POINTER) {

        while (length--)
            if (column->values->array[length])
                return 1;
        return 0;

    }

    return (column->nullcount < length ? 1 : 0);

}


/* 
 * @internal
 * @brief
 *   Count number of invalid elements in a numerical column.
 *
 * @param self  Column to inquire
 *
 * @return Number of invalid elements in a column.
 *
 * @note No @c NULL check is performed, type checks performed with assert()
 */

static inline cpl_size cpl_column_get_invalid_numerical(const cpl_column *self)
{
#ifndef NDEBUG
    const cpl_type type = cpl_column_get_type(self);
#endif

    assert(type != CPL_TYPE_STRING);
    assert((type & CPL_TYPE_POINTER) == 0);

    return self->nullcount;
}

/* 
 * @brief
 *   Count number of invalid elements in a column.
 *
 * @param column  Column to inquire and possibly update with that count
 *
 * @return Number of invalid elements in a column. -1 is always returned
 *   in case of error.
 *
 * Count number of invalid elements in a column. This function does
 * not report the presence of invalid elements within an array type
 * column, but in that case reports just if an array is allocated or
 * not (just like is done for strings). If the column itself is 
 * a @c NULL pointer, an error @c CPL_ERROR_NULL_INPUT is set.
 */

cpl_size cpl_column_count_invalid(cpl_column *column)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    char      **p;
    cpl_array **a;


    if (column == NULL) {
        cpl_error_set_where_();
        return -1;
    }

    if (type == CPL_TYPE_STRING) {

        p = column->values->s;

        column->nullcount = 0;
        while (length--)
            if (*p++ == NULL)
                column->nullcount++;

    }

    if (type & CPL_TYPE_POINTER) {

        a = column->values->array;

        column->nullcount = 0;
        while (length--)
            if (*a++ == NULL)
                column->nullcount++;
 
    }

    return column->nullcount;

}


/* 
 * @brief
 *   Count number of invalid elements in a column.
 *
 * @param column  Column to inquire and possibly update with that count
 *
 * @return Number of invalid elements in a column. -1 is always returned
 *   in case of error.
 * @see cpl_column_count_invalid()
 * @note This function will _not_ update the internal null-counter
 *
 * Count number of invalid elements in a column. This function does
 * not report the presence of invalid elements within an array type
 * column, but in that case reports just if an array is allocated or
 * not (just like is done for strings). If the column itself is 
 * a @c NULL pointer, an error @c CPL_ERROR_NULL_INPUT is set.
 */

cpl_size cpl_column_count_invalid_const(const cpl_column *column)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    char      **p;
    cpl_array **a;
    cpl_size nullcount;


    if (column == NULL) {
        cpl_error_set_where_();
        return -1;
    }

    nullcount = column->nullcount;

    if (type == CPL_TYPE_STRING) {

        p = column->values->s;

        nullcount = 0;
        while (length--)
            if (*p++ == NULL)
                nullcount++;

    }

    if (type & CPL_TYPE_POINTER) {

        a = column->values->array;

        nullcount = 0;
        while (length--)
            if (*a++ == NULL)
                nullcount++;
 
    }

    return nullcount;

}


/*
 * @brief
 *   Get a pointer to @c integer column data. 
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @c integer column data. If @em column contains
 *   no data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_INT, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules
 *   out any check performed by the column object interface, and may
 *   introduce inconsistencies between the column information maintained
 *   internally and the actual column data.
 */

int *cpl_column_get_data_int(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, i, CPL_TYPE_INT);
}


/*
 * @brief
 *   Get a pointer to constant @c integer column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @c integer column data. If @em column contains 
 *   no data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_INT, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const int *cpl_column_get_data_int_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, i, CPL_TYPE_INT);
}


/*
 * @brief
 *   Get a pointer to @c long @c integer column data.
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @c long @c integer column data. If @em column contains
 *   no data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules
 *   out any check performed by the column object interface, and may
 *   introduce inconsistencies between the column information maintained
 *   internally and the actual column data.
 */

long *cpl_column_get_data_long(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, l, CPL_TYPE_LONG);
}


/*
 * @brief
 *   Get a pointer to constant @c long @c integer column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @c long @c integer column data. If @em column
 *   contains no data (zero length), a @c NULL is returned. If @em column is
 *   a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const long *cpl_column_get_data_long_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, l, CPL_TYPE_LONG);
}


/*
 * @brief
 *   Get a pointer to @c long @c long @c integer column data.
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @c long @c long @c integer column data. If @em column
 *   contains no data (zero length), a @c NULL is returned. If @em column is
 *   a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_LONG_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules
 *   out any check performed by the column object interface, and may
 *   introduce inconsistencies between the column information maintained
 *   internally and the actual column data.
 */

long long *cpl_column_get_data_long_long(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, ll, CPL_TYPE_LONG_LONG);
}


/*
 * @brief
 *   Get a pointer to constant @c long @c long @c integer column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @c long @c long @c integer column data. If @em
 *   column contains no data (zero length), a @c NULL is returned. If @em
 *   column is a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_LONG_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const long long *cpl_column_get_data_long_long_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, ll, CPL_TYPE_LONG_LONG);
}


/*
 * @brief
 *   Get a pointer to @c cpl_size column data.
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @c long @c long @c integer column data. If @em column
 *   contains no data (zero length), a @c NULL is returned. If @em column is
 *   a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_SIZE, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules
 *   out any check performed by the column object interface, and may
 *   introduce inconsistencies between the column information maintained
 *   internally and the actual column data.
 */

cpl_size *cpl_column_get_data_cplsize(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, sz, CPL_TYPE_SIZE);
}


/*
 * @brief
 *   Get a pointer to constant @c cpl_size column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @c long @c long @c integer column data. If @em
 *   column contains no data (zero length), a @c NULL is returned. If @em
 *   column is a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_SIZE, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const cpl_size *cpl_column_get_data_cplsize_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, sz, CPL_TYPE_SIZE);
}


/* 
 * @brief
 *   Get a pointer to @em float column data. 
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @em float column data. If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 * 
 * If the column is not of type @c CPL_TYPE_FLOAT, a 
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

float *cpl_column_get_data_float(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, f, CPL_TYPE_FLOAT);
}


/*
 * @brief
 *   Get a pointer to constant @em float column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @em float column data. If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_FLOAT, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

const float *cpl_column_get_data_float_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, f, CPL_TYPE_FLOAT);
}


/* 
 * @brief
 *   Get a pointer to @em float complex column data. 
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @em float complex column data. If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 * 
 * If the column is not of type @c CPL_TYPE_FLOAT_COMPLEX, a 
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

float complex *cpl_column_get_data_float_complex(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, cf, CPL_TYPE_FLOAT_COMPLEX);
}


/*
 * @brief
 *   Get a pointer to constant @em float complex column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @em float complex column data. 
 *   If @em column contains no data (zero length), a @c NULL 
 *   is returned. If @em column is a @c NULL, a @c NULL is 
 *   returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_FLOAT_COMPLEX, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

const float complex *
cpl_column_get_data_float_complex_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, cf, CPL_TYPE_FLOAT_COMPLEX);
}


/* 
 * @brief
 *   Get a pointer to @em double column data. 
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @em double column data. If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set. 
 *
 * If the column is not of type @c CPL_TYPE_DOUBLE, 
 * a @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

double *cpl_column_get_data_double(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, d, CPL_TYPE_DOUBLE);
}


/*
 * @brief
 *   Get a pointer to constant @em double column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @em double column data. If @em column contains 
 *   no data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_DOUBLE,
 * a @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

const double *cpl_column_get_data_double_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, d, CPL_TYPE_DOUBLE);
}


/* 
 * @brief
 *   Get a pointer to @em double complex column data. 
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to @em double complex column data. If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 * 
 * If the column is not of type @c CPL_TYPE_DOUBLE_COMPLEX, a 
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

double complex *cpl_column_get_data_double_complex(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, cd, CPL_TYPE_DOUBLE_COMPLEX);
}


/*
 * @brief
 *   Get a pointer to constant @em double complex column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant @em double complex column data. 
 *   If @em column contains no data (zero length), a @c NULL 
 *   is returned. If @em column is a @c NULL, a @c NULL is 
 *   returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_DOUBLE_COMPLEX, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

const double complex *
cpl_column_get_data_double_complex_const(const cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, cd, CPL_TYPE_DOUBLE_COMPLEX);
}


/* 
 * @brief
 *   Get a pointer to string column data. 
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to string column data.  If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set. 
 *
 * If the column is not of type @c CPL_TYPE_STRING, 
 * a @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

char **cpl_column_get_data_string(cpl_column *self)
{
    CPL_COLUMN_GET_POINTER(self, s, CPL_TYPE_STRING);
}


/*
 * @brief
 *   Get a pointer to constant string column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant string column data.  If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the column is not of type @c CPL_TYPE_STRING,
 * a @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

const char **cpl_column_get_data_string_const(const cpl_column *self)
{

    cpl_type type = cpl_column_get_type(self);


    if (self == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type != CPL_TYPE_STRING) {
        (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        return NULL;
    }

    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return (const char **)(self->values->s);
    CPL_DIAG_PRAGMA_POP;
}


/*
 * @brief
 *   Get a pointer to array column data.
 *
 * @param self  Column to get the data from.
 *
 * @return Pointer to array column data.  If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned and an error is set.
 *
 * If the column is not an array type
 * a @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

cpl_array **cpl_column_get_data_array(cpl_column *self)
{
    /* Function body modified from below */
    const cpl_type type = cpl_column_get_type(self);


    if (self == NULL) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER)) {
        (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        return NULL;
    }

    return self->values->array;
}


/*
 * @brief
 *   Get a pointer to constant array column data.
 *
 * @param self  Constant column to get the data from.
 *
 * @return Pointer to constant array column data.  If @em column contains no
 *   data (zero length), a @c NULL is returned. If @em column is a @c NULL,
 *   a @c NULL is returned and an error is set.
 *
 * If the column is not an array type a @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_column_get_data_int().
 */

const cpl_array **cpl_column_get_data_array_const(const cpl_column *self)
{

    /* Function body modified from above */
    const cpl_type type = cpl_column_get_type(self);


    if (self == NULL) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER)) {
        (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        return NULL;
    }

    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    return (const cpl_array **)(self->values->array);
    CPL_DIAG_PRAGMA_POP;
}


/* 
 * @brief
 *   Get a pointer to a column invalid flags buffer.
 *
 * @param column  Column from where to get the data.
 *
 * @return Pointer to the column buffer containing the flagging of the
 *   invalid elements, or @c NULL if it is missing.
 *
 * If all or no column elements are invalid, a @c NULL pointer is returned. 
 * A @c NULL is returned also if the column is of type @c CPL_TYPE_STRING 
 * or of type array (no buffer marking the invalid elements is present in 
 * this case).
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out 
 *   any check performed by the column object interface, and may introduce 
 *   inconsistencies between the column information maintained internally
 *   and the actual column data.
 */

cpl_column_flag *cpl_column_get_data_invalid(cpl_column *column)
{

    if (column == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return column->null;

}


/*
 * @brief
 *   Get a pointer to a constant column invalid flags buffer.
 *
 * @param column  Constant column from where to get the data.
 *
 * @return Pointer to the constant column buffer containing the flagging of 
 *   the invalid elements, or @c NULL if it is missing.
 *
 * If all or no column elements are invalid, a @c NULL pointer is returned.
 * A @c NULL is returned also if the column is of type @c CPL_TYPE_STRING
 * or of type array (no buffer marking the invalid elements is present in
 * this case).
 */

const cpl_column_flag *
cpl_column_get_data_invalid_const(const cpl_column *column)
{

    if (column == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return column->null;

}


/* 
 * @brief
 *   Plug a new invalid flag buffer into a numerical column.
 *
 * @param column      Column to access.
 * @param nulls       Array of null flags.
 * @param nullcount   Total number of nulls.
 *
 * @return CPL_ERROR_NONE on success. If the column type is not
 *   numerical, an invalid flag buffer is not used, and an attempt 
 *   to plug it in will cause a @c CPL_ERROR_INVALID_TYPE to be 
 *   returned. A @c CPL_ERROR_INCOMPATIBLE_INPUT is returned if 
 *   an impossible @em nullcount value is passed. If the input 
 *   @em nulls buffer does not contains just 1s or 0s, a 
 *   @c CPL_ERROR_ILLEGAL_INPUT is returned. A @c CPL_ERROR_NULL_INPUT
 *   is returned if a @c NULL column pointer is passed.
 *
 * This function is used to assign a new set of flags marking invalid
 * elements within a column. If a flag buffer already exists, this is 
 * deallocated before being replaced by the new one. If @em nullcount 
 * is either 0 or equal to the column length, the passed flag buffer 
 * is ignored, but the internal buffer is deallocated. If @em nullcount 
 * is negative, it will be evaluated internally, or set to zero if no 
 * flag buffer was passed. This function cannot be use either for array
 * columns or @c CPL_TYPE_STRING columns.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out 
 *   any check performed by the column object interface, and may introduce 
 *   inconsistencies between the column information maintained internally 
 *   and the actual column data.
 */

cpl_error_code cpl_column_set_data_invalid(cpl_column *column, 
                             cpl_column_flag *nulls, cpl_size nullcount)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (type == CPL_TYPE_STRING)
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    if (type & CPL_TYPE_POINTER)
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    if (nullcount > length)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    if (nulls)
        while (length--)
            if (nulls[length] != 0)
                if (nulls[length] != 1)
                    return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    length = cpl_column_get_size(column);

    if (nullcount < 0) {

        /*
         *  The total number of NULLs must be evaluated internally. If a 
         *  null flag buffer was not passed, the total number of NULLs is
         *  assumed to be zero.
         */

        if (nulls) {
            nullcount = 0;
            while (length--)
                nullcount += nulls[length];
        }
        else {
            nullcount = 0;
        }
    }


    /*
     *  If no null buffer was passed, and a wrong null count was specified,
     *  return a failure (wrong input).
     */

    if (!nulls)
        if (nullcount != length && nullcount != 0)
            return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);


    /*
     *  Replace the old buffer with the new one if necessary.
     */

    if (nullcount == length || nullcount == 0)
        nulls = NULL;

    if (column->null)
        cpl_free(column->null);

    column->null = nulls;
    column->nullcount = nullcount;

    return 0;

}


/*
 * @brief
 *   Reallocate a column of arrays to a new depth.
 *
 * @param column  Column to be resized.
 * @param depth   New depth of column.
 *
 * @return @c CPL_ERROR_NONE on success. The new column depth must not 
 *   be negative, or a @c CPL_ERROR_ILLEGAL_INPUT is returned. The input
 *   column pointer should not be @c NULL, or a @c CPL_ERROR_NULL_INPUT
 *   is returned. The column should be an array type, or a 
 *   @c CPL_ERROR_TYPE_MISMATCH is returned.
 *
 * Reallocate a column of arrays to a new depth. The contents of the
 * arrays data buffer will be unchanged up to the lesser of the new and
 * old sizes. If the column depth is increased, the new arrays elements
 * are flagged as invalid. The pointer to array data may change, therefore
 * pointers previously retrieved by calling @c cpl_column_get_data_array(),
 * @c cpl_column_get_data_string(), etc. should be discarded). Resizing
 * to zero is allowed, and would produce a zero-depth column. In case
 * of failure, the old data buffer is left intact. Changing column
 * depth implies removing any information about column dimensions.
 */

cpl_error_code cpl_column_set_depth(cpl_column *column, cpl_size depth)
{


    cpl_array  *array;
    cpl_column *acolumn;
    cpl_type    type      = cpl_column_get_type(column);
    cpl_size    length    = cpl_column_get_size(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (depth < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (!(type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    while (length--) {
        array = cpl_column_get_array(column, length);
        if (!array)
            continue;
        acolumn = cpl_array_get_column(array);
        if (cpl_column_set_size(acolumn, depth)) {
            return cpl_error_set_where_();
        }
    }

    column->depth = depth;
    if (column->dimensions) {
        cpl_array_delete(column->dimensions);
        column->dimensions = NULL;
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Reallocate a column to a new number of elements.
 *
 * @param column      Column to be resized.
 * @param new_length  New number of elements in column.
 *
 * @return @c CPL_ERROR_NONE on success. The new column size must not be
 *   negative, or a @c CPL_ERROR_ILLEGAL_INPUT is returned. The input
 *   column pointer should not be @c NULL, or a @c CPL_ERROR_NULL_INPUT
 *   is returned.
 *
 * Reallocate a column to a new number of elements. The contents of the 
 * column data buffer will be unchanged up to the lesser of the new and 
 * old sizes. If the column size is increased, the new column elements 
 * are flagged as invalid. The pointer to data may change, therefore 
 * pointers previously retrieved by calling @c cpl_column_get_data_int(), 
 * @c cpl_column_get_data_string(), etc. should be discarded). Resizing 
 * to zero is allowed, and would produce a zero-length column. In case 
 * of failure, the old data buffer is left intact.
 */

cpl_error_code cpl_column_set_size(cpl_column *column, cpl_size new_length)
{

    cpl_type     type         = cpl_column_get_type(column);
    cpl_size     old_length   = cpl_column_get_size(column);
    size_t       element_size = cpl_column_type_size(type);
    size_t       null_size    = old_length * sizeof(cpl_column_flag);
    size_t       new_size     = element_size * new_length;
    cpl_size     i            = 0;

    cpl_column_flag *np       = NULL;
    char           **sp;
    cpl_array      **ap;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (new_length < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (new_length == old_length)     /* Resizing is unnecessary */
        return CPL_ERROR_NONE;

    if (new_length == 0) {            /* Resizing to zero        */

        if (type & CPL_TYPE_POINTER) {
            switch (type & ~CPL_TYPE_POINTER) {
            case CPL_TYPE_INT:
            case CPL_TYPE_LONG:
            case CPL_TYPE_LONG_LONG:
            case CPL_TYPE_SIZE:
            case CPL_TYPE_FLOAT:
            case CPL_TYPE_DOUBLE:
            case CPL_TYPE_FLOAT_COMPLEX:
            case CPL_TYPE_DOUBLE_COMPLEX:
            case CPL_TYPE_STRING:

                for (i = 0; i < old_length; i++)
                    if (column->values->array[i])
                        cpl_array_delete(column->values->array[i]);

                cpl_free((void *)column->values->array);

                break;

            default:
                break;
            }
        } else {
            if (type == CPL_TYPE_STRING) {
                for (i = 0; i < old_length; i++)
                    cpl_free(column->values->s[i]);
            }
            cpl_free((void *)column->values->s);
        }

        column->values->i = NULL;
        if (column->null)
            cpl_free(column->null);
        column->null = NULL;
        column->nullcount = 0;
        column->length = 0;

        return CPL_ERROR_NONE;
    }

    if (old_length == 0) {            /* Resizing from zero      */

        if (type & CPL_TYPE_POINTER) {
            switch (type & ~CPL_TYPE_POINTER) {
            case CPL_TYPE_INT:
            case CPL_TYPE_LONG:
            case CPL_TYPE_LONG_LONG:
            case CPL_TYPE_SIZE:
            case CPL_TYPE_FLOAT:
            case CPL_TYPE_DOUBLE:
            case CPL_TYPE_FLOAT_COMPLEX:
            case CPL_TYPE_DOUBLE_COMPLEX:
            case CPL_TYPE_STRING:
                column->values->array =
                    cpl_calloc(new_length, sizeof(cpl_array *));
                break;
            default:
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        } else {
            switch (type) {
            case CPL_TYPE_INT:
                column->values->i = cpl_malloc(new_length * sizeof(int));
                break;
            case CPL_TYPE_LONG:
                column->values->l = cpl_malloc(new_length * sizeof(long));
                break;
            case CPL_TYPE_LONG_LONG:
                column->values->ll = cpl_malloc(new_length * sizeof(long long));
                break;
            case CPL_TYPE_SIZE:
                column->values->sz = cpl_malloc(new_length * sizeof(cpl_size));
                break;
            case CPL_TYPE_FLOAT:
                column->values->f = cpl_malloc(new_length * sizeof(float));
                break;
            case CPL_TYPE_DOUBLE:
                column->values->d = cpl_malloc(new_length * sizeof(double));
                break;
            case CPL_TYPE_FLOAT_COMPLEX:
                column->values->cf = 
                    cpl_malloc(new_length * sizeof(float complex));
                break;
            case CPL_TYPE_DOUBLE_COMPLEX:
                column->values->cd = 
                    cpl_malloc(new_length * sizeof(double complex));
                break;
            case CPL_TYPE_STRING:
                column->values->s = cpl_calloc(new_length, sizeof(char *));
                break;
            default:
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        }

        column->length = new_length;

        if (type != CPL_TYPE_STRING && !(type & CPL_TYPE_POINTER))
            column->nullcount = new_length;

        return CPL_ERROR_NONE;
    }


    /*
     * In the following the generic case is handled
     */

    if (column->null) {

        /*
         *  If the null flags buffer exists, it should be reallocated 
         *  first. 
         */

        null_size = new_length * sizeof(cpl_column_flag);
        column->null = cpl_realloc((void *)column->null, null_size);

        if (new_length < old_length) {

            /*
             *  If the column was shortened, check whether some 
             *  NULLs are still present in column. If no NULL is 
             *  found, then the null flags buffer can be deallocated.
             */

            column->nullcount = 0;
            np = column->null;
            for (i = 0; i < new_length; i++)
                if (*np++)
                    column->nullcount++;

            if (column->nullcount == new_length || 
                column->nullcount == 0) {
                if (column->null)
                    cpl_free(column->null);
                column->null = NULL;
            }
        }
        else {

            /*
             *  Pad with 1s the extra space (by definition, extra space
             *  in the column data buffer is not initialized, therefore
             *  it is a NULL value; the case of string columns doesn't 
             *  need to be treated, since in that case the null flags 
             *  are not used).
             */

            np = column->null + old_length;
            for (i = old_length; i < new_length; i++)
                *np++ = 1;

            column->nullcount += new_length - old_length;

        }
    }
    else {

        /*
         *  We don't need to take care of string columns here, because
         *  they take automatically care of their own NULLs.
         */

        if (type != CPL_TYPE_STRING) {

            if (new_length > old_length) {

                /*
                 *  If the null flags buffer doesn't exist, it is 
                 *  either because there were no NULLs, or there 
                 *  were just NULLs. In the latter case we do not 
                 *  need to allocate any new buffer, because the
                 *  expansion just added extra NULLs. In the former 
                 *  case, we have to allocate a null flags column 
                 *  buffer in case the column is expanded, because 
                 *  the expanded part contains NULLs:
                 */

                if (column->nullcount == 0) {

                    column->null = cpl_calloc(new_length, 
                                              sizeof(cpl_column_flag));

                    np = column->null + old_length;
                    for (i = old_length; i < new_length; i++)
                        *np++ = 1;

                    column->nullcount = new_length - old_length;
                }
                else                       /* There were just nulls     */
                    column->nullcount = new_length;
            }
            else if (column->nullcount)    /* Column of nulls shortened */
                column->nullcount = new_length;

        }
    }

    /*
     *  If a column of character strings or of arrays is shortened,
     *  we must explicitly free the extra strings.
     */

    if (type == CPL_TYPE_STRING)
        for (i = new_length; i < old_length; i++)
            if (column->values->s[i])
                cpl_free(column->values->s[i]);

    if (type & CPL_TYPE_POINTER)
        for (i = new_length; i < old_length; i++)
            if (column->values->array[i])
                cpl_array_delete(column->values->array[i]);

    /*
     *  Finally, we realloc the column data buffer:
     */

    column->values->i = cpl_realloc((void *)column->values->i, new_size);
    column->length = new_length;

    /*
     *  We don't initialize the extra memory, since it is NULL flagged,
     *  with the exception of string and array columns:
     */

    if (type == CPL_TYPE_STRING) {
        sp = column->values->s + old_length;
        for (i = old_length; i < new_length; i++)
            *sp++ = NULL;
    }

    if (type & CPL_TYPE_POINTER) {
        ap = column->values->array + old_length;
        for (i = old_length; i < new_length; i++)
            *ap++ = NULL;
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Read a value from a numerical column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Value read. In case of an invalid column element, or in
 *   case of error, 0.0 is always returned. 
 *
 * Read a value from a numerical column. A @c CPL_ERROR_NULL_INPUT is set in
 * case @em column is a @c NULL pointer. A @c CPL_ERROR_INVALID_TYPE is set 
 * in case a non-numerical column is accessed. @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 * is set if the @em indx is outside the column range. Indexes are 
 * counted starting from 0. If the input column has length zero, 
 * @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. The @em null 
 * flag is used to indicate whether the accessed column element is 
 * valid (0) or invalid (1). The null flag also signals an error 
 * condition (-1). The @em null argument can be left to @c NULL.
 * This function cannot be used for the access of array elements in
 * array columns.
 */

double cpl_column_get(const cpl_column *column, cpl_size indx, int *null)
{

    const cpl_type    type = cpl_column_get_type(column);

    if (type & CPL_TYPE_POINTER) {
        switch (type & ~CPL_TYPE_POINTER) {
        case CPL_TYPE_INT:
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG:
        case CPL_TYPE_SIZE:
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
        case CPL_TYPE_STRING:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;
        default:
            cpl_error_set_(CPL_ERROR_NULL_INPUT);
            break;
        }
    } else {
        switch (type) {
        case CPL_TYPE_INT:
            return (double)cpl_column_get_int(column, indx, null);
        case CPL_TYPE_LONG:
            return (double)cpl_column_get_long(column, indx, null);
        case CPL_TYPE_LONG_LONG:
            return (double)cpl_column_get_long_long(column, indx, null);
        case CPL_TYPE_SIZE:
            return (double)cpl_column_get_cplsize(column, indx, null);
        case CPL_TYPE_FLOAT:
            return (double)cpl_column_get_float(column, indx, null);
        case CPL_TYPE_DOUBLE:
            return cpl_column_get_double(column, indx, null);
        case CPL_TYPE_STRING:
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;
        default:
            cpl_error_set_(CPL_ERROR_NULL_INPUT);
            break;
        }
    }

    if (null)
        *null = -1;

    return 0.0;

}


/* 
 * @brief
 *   Read a value from a complex column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Value read. In case of an invalid column element, or in
 *   case of error, 0.0 is always returned. 
 *
 * Read a value from a complex column. A @c CPL_ERROR_NULL_INPUT is set in
 * case @em column is a @c NULL pointer. A @c CPL_ERROR_INVALID_TYPE is set 
 * in case a non-complex column is accessed. @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 * is set if the @em indx is outside the column range. Indexes are 
 * counted starting from 0. If the input column has length zero, 
 * @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. The @em null 
 * flag is used to indicate whether the accessed column element is 
 * valid (0) or invalid (1). The null flag also signals an error 
 * condition (-1). The @em null argument can be left to @c NULL.
 * This function cannot be used for the access of array elements in
 * array columns.
 */

double complex cpl_column_get_complex(const cpl_column *column, 
                                      cpl_size indx, int *null)
{

    const cpl_type    type = cpl_column_get_type(column);

    if (type & CPL_TYPE_POINTER) {
        switch (type & ~CPL_TYPE_POINTER) {
        case CPL_TYPE_INT:
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG:
        case CPL_TYPE_SIZE:
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
        case CPL_TYPE_STRING:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;
        default:
            cpl_error_set_(CPL_ERROR_NULL_INPUT);
            break;
        }
    } else {
        switch (type) {
        case CPL_TYPE_FLOAT_COMPLEX:
            return (double complex)cpl_column_get_float_complex(column, indx, null);
        case CPL_TYPE_DOUBLE_COMPLEX:
            return cpl_column_get_double_complex(column, indx, null);
        case CPL_TYPE_INT:
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG:
        case CPL_TYPE_SIZE:
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_STRING:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;
        default:
            cpl_error_set_(CPL_ERROR_NULL_INPUT);
            break;
        }
    }

    if (null)
        *null = -1;

    return 0.0;

}


/* 
 * @internal
 * @brief
 *   Error checks for get functions
 *
 * @param fcall    caller function
 * @param self     column to be accessed
 * @param indx     Position of element to be read.
 * @param null     Flag indicating null values, or error condition.
 * @param e_type   Expected type of column
 *
 * @return 0 on error 1 on success
 *
 */
static inline int cpl_get_error_check_(const char * fcall,
                                       const cpl_column * self,
                                       cpl_size indx, int * null,
                                       cpl_type e_type)
{
    cpl_size    length;
    cpl_type    type;
    int         isnull;

    if (null)
        *null = -1;

    if (self == NULL) {
        cpl_error_set(fcall, CPL_ERROR_NULL_INPUT);
        return 0; 
    }

    length = cpl_column_get_size_(self);

    if (indx < 0 || indx >= length) {
        cpl_error_set(fcall, CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return 0; 
    }

    type = cpl_column_get_type_(self);

    if (type != e_type) {
        cpl_error_set(fcall, CPL_ERROR_TYPE_MISMATCH);
        return 0;
    }

    isnull = cpl_column_base_type_invalid_(self, indx, length);
    
    if (null)
        *null = isnull;

    if (isnull)
        return 0;

    return 1;
}


/* 
 * @brief
 *   Read a value from an @em integer column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Integer value read. In case of an invalid column element, or in
 *   case of error, 0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_INT. If @em column is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx 
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set. 
 * Indexes are counted starting from 0. If the input column has length zero, 
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null 
 * flag is a valid pointer, it is used to indicate whether the accessed 
 * column element is valid (0) or invalid (1). The null flag also signals 
 * an error condition (-1).
 */

int cpl_column_get_int(const cpl_column *column, cpl_size indx, int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_INT) == 0)
        return 0;

    return column->values->i[indx];

}


/* 
 * @brief
 *   Read a value from a @em long @em integer column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Long integer value read. In case of an invalid column element, or in
 *   case of error, 0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_LONG. If @em column is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input column has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * column element is valid (0) or invalid (1). The null flag also signals
 * an error condition (-1).
 */

long cpl_column_get_long(const cpl_column *column, cpl_size indx, int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_LONG) == 0)
        return 0;

    return column->values->l[indx];

}


/*
 * @brief
 *   Read a value from a @em long @em long @em integer column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Long long integer value read. In case of an invalid column element,
 *   or in case of error, 0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_LONG_LONG. If @em column
 * is a @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is
 * not of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input column has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * column element is valid (0) or invalid (1). The null flag also signals
 * an error condition (-1).
 */

long long
cpl_column_get_long_long(const cpl_column *column, cpl_size indx, int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_LONG_LONG) == 0)
        return 0;

    return column->values->ll[indx];

}


/*
 * @brief
 *   Read a value from a @em cpl_size column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return The cpl_size value read. In case of an invalid column element,
 *   or in case of error, 0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_SIZE. If @em column
 * is a @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is
 * not of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input column has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * column element is valid (0) or invalid (1). The null flag also signals
 * an error condition (-1).
 */

cpl_size cpl_column_get_cplsize(const cpl_column *column, cpl_size indx,
                                int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_SIZE) == 0)
        return 0;

    return column->values->sz[indx];

}


/*
 * @brief
 *   Read a value from a @em float column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Float value read. In case of an invalid column element, or in
 *   case of error, 0.0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_FLOAT. See the
 * documentation of the function cpl_column_get_int().
 */

float cpl_column_get_float(const cpl_column *column, cpl_size indx, int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_FLOAT) == 0)
        return 0;

    return column->values->f[indx];

}


/* 
 * @brief
 *   Read a value from a @em float complex column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Float complex value read. In case of an invalid column element, 
 *   or in case of error, 0.0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_FLOAT_COMPLEX. See the
 * documentation of the function cpl_column_get_int().
 */

float complex cpl_column_get_float_complex(const cpl_column *column, 
                                           cpl_size indx, int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_FLOAT_COMPLEX) == 0)
        return 0;

    return column->values->cf[indx];

}

/* 
 * @brief
 *   Read a value from a @em double column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Double value read. In case of an invalid column element, or in
 *   case of error, 0.0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_DOUBLE. See the
 * documentation of the function cpl_column_get_int().
 */

double cpl_column_get_double(const cpl_column *column, cpl_size indx, int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_DOUBLE) == 0)
        return 0;

    return column->values->d[indx];

}


/* 
 * @brief
 *   Read a value from a @em double complex column.
 *
 * @param column  Column to be accessed.
 * @param indx    Position of element to be read.
 * @param null    Flag indicating null values, or error condition.
 *
 * @return Double value read. In case of an invalid column element, or in
 *   case of error, 0.0 is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_DOUBLE_COMPLEX. See the
 * documentation of the function cpl_column_get_int().
 */

double complex cpl_column_get_double_complex(const cpl_column *column, 
                                             cpl_size indx, int *null)
{

    if (cpl_get_error_check_(cpl_func, column, indx, null,
                             CPL_TYPE_DOUBLE_COMPLEX) == 0)
        return 0;

    return column->values->cd[indx];

}


/*
 * @brief
 *   Read a value from a string column.
 *
 * @param self    Column to be accessed.
 * @param indx    Position of element to be read.
 *
 * @return Character string read. In case of an invalid column element,
 *   or in case of error, a @c NULL pointer is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_STRING. If @em column is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input column has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set.
 *   
 * @note 
 *   The returned string is a pointer to a column element, not its
 *   copy. Its manipulation will directly affect that column element,
 *   while changing that column element using @c cpl_column_set_string()
 *   will turn it into garbage. Therefore, if a real copy of a string
 *   column element is required, this function should be called as an
 *   argument of the function @c cpl_strdup().
 */
    
char *cpl_column_get_string(cpl_column *self, cpl_size indx)
{
    CPL_COLUMN_GET_VALUE(self, indx, s, type == CPL_TYPE_STRING);
}


/* 
 * @brief
 *   Read a value from a constant string column.
 *
 * @param self    Constant column to be accessed.
 * @param indx    Position of element to be read.
 *
 * @return Character string read. In case of an invalid column element,
 *   or in case of error, a @c NULL pointer is always returned.
 *
 * Read a value from a column of type @c CPL_TYPE_STRING. If @em column is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input column has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set.
 */

const char *cpl_column_get_string_const(const cpl_column *self, cpl_size indx)
{
    CPL_COLUMN_GET_VALUE(self, indx, s, type == CPL_TYPE_STRING);
}


/*
 * @brief
 *   Read an array from an array column.
 *
 * @param self    Column to be accessed.
 * @param indx    Position of array to be read.
 *
 * @return CPL array read. In case of an invalid column element,
 *   or in case of error, a @c NULL pointer is always returned.
 *
 * Read an array from a column of type array. If @em column is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input column has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set.
 *
 * @note
 *   The returned array is a pointer to a column element, not its
 *   copy. Its manipulation will directly affect that column element,
 *   while changing that column element using @c cpl_column_set_array()
 *   will turn it into garbage. Therefore, if a real copy of an array
 *   column element is required, this function should be called as an
 *   argument of the function @c cpl_array_duplicate().
 */

cpl_array *cpl_column_get_array(cpl_column *self, cpl_size indx)
{
    CPL_COLUMN_GET_VALUE(self, indx, array, type & CPL_TYPE_POINTER);
}


/*
 * @brief
 *   Read a constant array from an array column.
 *
 * @param self    constant column to be accessed.
 * @param indx    Position of array to be read.
 *
 * @return CPL array read. In case of an invalid column element,
 *   or in case of error, a @c NULL pointer is always returned.
 *
 * Read a constant array from a column of type array. If @em column is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the column is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input column has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set.
 */

const cpl_array *cpl_column_get_array_const(const cpl_column *self, 
                                            cpl_size indx)
{
    CPL_COLUMN_GET_VALUE(self, indx, array, type & CPL_TYPE_POINTER);
}


/* 
 * @brief
 *   Write a value to a numerical column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer 
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of numerical 
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em indx is outside 
 *   the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If 
 *   the input column has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is always returned.
 *
 * Write a value to a numerical column element. The value is cast to the
 * accessed column type. The written value is automatically flagged as
 * valid. To invalidate a column value use @c cpl_column_set_invalid().
 * Column elements are counted starting from 0. This function cannot
 * be used on array columns.
 */

cpl_error_code cpl_column_set(cpl_column *column, cpl_size indx, double value)
{

    const cpl_type type = cpl_column_get_type(column);
    cpl_error_code code;


    if (column == NULL) return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    switch (type) {
    case CPL_TYPE_INT:
        code = cpl_column_set_int(column, indx, value);
        break;
    case CPL_TYPE_LONG:
        code = cpl_column_set_long(column, indx, value);
        break;
    case CPL_TYPE_LONG_LONG:
        code = cpl_column_set_long_long(column, indx, value);
        break;
    case CPL_TYPE_SIZE:
        code = cpl_column_set_cplsize(column, indx, value);
        break;
    case CPL_TYPE_FLOAT:
        code = cpl_column_set_float(column, indx, value);
        break;
    case CPL_TYPE_DOUBLE:
        code = cpl_column_set_double(column, indx, value);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        code = cpl_column_set_float_complex(column, indx, value);
        break;
    case CPL_TYPE_DOUBLE_COMPLEX:
        code = cpl_column_set_double_complex(column, indx, value);
        break;
    default:
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "%s",
                                      cpl_type_get_name(type));
    }

    return code ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/* 
 * @brief
 *   Write a complex value to a complex column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer 
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of complex 
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em indx is outside 
 *   the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If 
 *   the input column has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is always returned.
 *
 * Write a complex value to a complex column element. The value is cast to the
 * accessed column type. The written value is automatically flagged as
 * valid. To invalidate a column value use @c cpl_column_set_invalid().
 * Column elements are counted starting from 0. This function cannot
 * be used on array columns.
 */

cpl_error_code cpl_column_set_complex(cpl_column *column, 
                                      cpl_size indx, double complex value)
{

    const cpl_type type = cpl_column_get_type(column);

    if (column == NULL) return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    switch (type) {
    case CPL_TYPE_FLOAT_COMPLEX:
        return cpl_column_set_float_complex(column, indx, value);
    case CPL_TYPE_DOUBLE_COMPLEX:
        return cpl_column_set_double_complex(column, indx, value);
    default:
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "%s",
                                      cpl_type_get_name(type));
    }

}

/* 
 * @internal
 * @brief
 *   Error checks for set functions
 *
 * @param fcall    caller function
 * @param self     column to be accessed
 * @param indx     Position of element to be read.
 * @param e_type   Expected type of column
 *
 * @return 0 on error 1 on success
 *
 */
inline static int
cpl_set_error_check_(const char *fcall, const cpl_column *self,
                     cpl_size indx, cpl_type e_type)
{
    cpl_size    length;
    cpl_type    type;

    if (self == NULL) {
        cpl_error_set(fcall, CPL_ERROR_NULL_INPUT);
        return 0; 
    }

    length = cpl_column_get_size_(self);

    if (indx < 0 || indx >= length) {
        cpl_error_set(fcall, CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return 0; 
    }

    type = cpl_column_get_type_(self);

    if (type != e_type) {
        cpl_error_set(fcall, CPL_ERROR_TYPE_MISMATCH);
        return 0;
    }

    return 1;
}


/* 
 * @brief
 *   Write a value to an @em integer column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer 
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the 
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is 
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is always returned.
 *
 * Write a value to an @em integer column element. The written value 
 * is automatically flagged as valid. To invalidate a column value use 
 * @c cpl_column_set_invalid(). Column elements are counted starting 
 * from 0.
 */

cpl_error_code cpl_column_set_int(cpl_column *column, cpl_size indx, int value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_INT) == 0) {
        return cpl_error_get_code();
    }

    column->values->i[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a value to a @em long @em integer column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em long @em integer column element. The written value
 * is automatically flagged as valid. To invalidate a column value use
 * @c cpl_column_set_invalid(). Column elements are counted starting
 * from 0.
 */

cpl_error_code
cpl_column_set_long(cpl_column *column, cpl_size indx, long value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_LONG) == 0) {
        return cpl_error_get_code();
    }

    column->values->l[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write a value to a @em long @em long @em integer column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em long @em long @em integer column element. The
 * written value is automatically flagged as valid. To invalidate a column
 * value use @c cpl_column_set_invalid(). Column elements are counted starting
 * from 0.
 */

cpl_error_code
cpl_column_set_long_long(cpl_column *column, cpl_size indx, long long value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_LONG_LONG) == 0) {
        return cpl_error_get_code();
    }

    column->values->ll[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write a value to a @em cpl_size column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em cpl_size column element. The written value is
 * automatically flagged as valid. To invalidate a column value use
 * @c cpl_column_set_invalid(). Column elements are counted starting
 * from 0.
 */

cpl_error_code
cpl_column_set_cplsize(cpl_column *column, cpl_size indx, cpl_size value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_SIZE) == 0) {
        return cpl_error_get_code();
    }

    column->values->sz[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write a value to a @em float column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em float column element. The written value
 * is automatically flagged as valid. To invalidate a column value use
 * @c cpl_column_set_invalid(). Column elements are counted starting
 * from 0.
 */

cpl_error_code cpl_column_set_float(cpl_column *column, 
                                    cpl_size indx, float value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_FLOAT) == 0) {
        return cpl_error_get_code();
    }

    column->values->f[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a complex value to a @em float complex column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a complex value to a @em float complex column element. The written 
 * value is automatically flagged as valid. To invalidate a column value use
 * @c cpl_column_set_invalid(). Column elements are counted starting
 * from 0.
 */

cpl_error_code cpl_column_set_float_complex(cpl_column *column, 
                                            cpl_size indx, float complex value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_FLOAT_COMPLEX) == 0) {
        return cpl_error_get_code();
    }

    column->values->cf[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a value to a @em double column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em double column element. The written value
 * is automatically flagged as valid. To invalidate a column value use
 * @c cpl_column_set_invalid(). Column elements are counted starting
 * from 0.
 */

cpl_error_code cpl_column_set_double(cpl_column *column, cpl_size indx, 
                                     double value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_DOUBLE) == 0) {
        return cpl_error_get_code();
    }

    column->values->d[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a complex value to a @em double complex column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a complex value to a @em double complex column element. The written 
 * value is automatically flagged as valid. To invalidate a column value use
 * @c cpl_column_set_invalid(). Column elements are counted starting
 * from 0.
 */

cpl_error_code cpl_column_set_double_complex(cpl_column *column, cpl_size indx,
                                             double complex value)
{

    if (cpl_set_error_check_(cpl_func, column, indx,
                             CPL_TYPE_DOUBLE_COMPLEX) == 0) {
        return cpl_error_get_code();
    }

    column->values->cd[indx] = value;
    cpl_column_unset_null(column, indx);

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a character string to a string column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write character string.
 * @param string  Character string to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em indx is
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Copy a character string to a @em string column element. The written 
 * value can also be a @c NULL pointer. Note that the input character 
 * string is copied, therefore the original can be modified without 
 * affecting the column content. To "plug" a character string directly 
 * into a column element, use the function @c cpl_column_get_data_string(). 
 * Column elements are counted starting from zero.
 */

cpl_error_code cpl_column_set_string(cpl_column *column, 
                                     cpl_size indx, const char *string)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (indx < 0 || indx >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (type != CPL_TYPE_STRING)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (column->values->s[indx])
        cpl_free(column->values->s[indx]);

    if (string)
        column->values->s[indx] = cpl_strdup(string);
    else
        column->values->s[indx] = NULL;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write an array to an array column element.
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write array.
 * @param array   CPL array to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL 
 *   pointer a @c CPL_ERROR_NULL_INPUT is returned. If the column 
 *   does not match the type of the input @em array, or if the column 
 *   is not made of arrays, a @c CPL_ERROR_TYPE_MISMATCH is returned. 
 *   If the input @em array size is different from the column depth, 
 *   a @c CPL_ERROR_INCOMPATIBLE_INPUT is returned. If @em indx is 
 *   outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is 
 *   returned.
 *
 * Copy a CPL array to an array column element. The written value 
 * can also be a @c NULL pointer. Note that the input array
 * is copied, therefore the original can be modified without
 * affecting the column content. To "plug" an array directly
 * into a column element, use the function @c cpl_column_get_data_array().
 * Column elements are counted starting from zero.
 */

cpl_error_code cpl_column_set_array(cpl_column *column,
                                    cpl_size indx, const cpl_array *array)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    cpl_type    atype;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (indx < 0 || indx >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (!(type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (array) {
        atype = cpl_array_get_type(array);
        if (type != (atype | CPL_TYPE_POINTER))
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        if (cpl_column_get_depth(column) != cpl_array_get_size(array))
            return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    }

    if (column->values->array[indx])
        cpl_array_delete(column->values->array[indx]);

    if (array)
        column->values->array[indx] = cpl_array_duplicate(array);
    else
        column->values->array[indx] = NULL;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Set a column element to NULL;
 *
 * @param column  Column to be accessed.
 * @param indx    Position where to write a NULL.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If @em indx is outside the 
 *   column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set. If the input 
 *   column has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set.
 *
 * In the case of a string or array column, the string or array is set free 
 * and its pointer is set to @c NULL; for other data types, the corresponding
 * element of the null flags buffer is flagged. Column elements are counted 
 * starting from zero.
 */

cpl_error_code cpl_column_set_invalid(cpl_column *column, cpl_size indx)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (indx < 0 || indx >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);


    if (type == CPL_TYPE_STRING) {
        if (column->values->s[indx]) {
            cpl_free(column->values->s[indx]);
            column->values->s[indx] = NULL;
        }
        return CPL_ERROR_NONE;
    }

    if (type & CPL_TYPE_POINTER) {
        if (column->values->array[indx]) {
            cpl_array_delete(column->values->array[indx]);
            column->values->array[indx] = NULL;
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == column->length)
        return CPL_ERROR_NONE;

    if (!column->null)
        column->null = cpl_calloc(column->length, sizeof(cpl_column_flag));

    if (column->null[indx] == 0) {

        column->null[indx] = 1;
        column->nullcount++;

        if (column->nullcount == column->length) {
            if (column->null)
                cpl_free(column->null);
            column->null = NULL;
        }

    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write the same a value within a numerical column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of numerical
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em start is outside
 *   the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input column has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write the same value to a numerical column segment. The value is cast to
 * the accessed column type. The written values are automatically flagged as
 * valid. To invalidate a column interval use @c cpl_column_fill_invalid().
 */

cpl_error_code cpl_column_fill(cpl_column *column, 
                               cpl_size start, cpl_size count, double value)
{

    const cpl_type    type = cpl_column_get_type(column);


    if (column == NULL) return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    switch (type) {
    case CPL_TYPE_INT:
        return cpl_column_fill_int(column, start, count, value);
    case CPL_TYPE_LONG:
        return cpl_column_fill_long(column, start, count, value);
    case CPL_TYPE_LONG_LONG:
        return cpl_column_fill_long_long(column, start, count, value);
    case CPL_TYPE_SIZE:
        return cpl_column_fill_cplsize(column, start, count, value);
    case CPL_TYPE_FLOAT:
        return cpl_column_fill_float(column, start, count, value);
    case CPL_TYPE_DOUBLE:
        return cpl_column_fill_double(column, start, count, value);
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

}


/*
 * @brief
 *   Write the same complex value within a complex column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of complex
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em start is outside
 *   the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input column has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write the same complex value to a complex column segment. The value is 
 * cast to the accessed column type. The written values are automatically 
 * flagged as valid. To invalidate a column interval use 
 * @c cpl_column_fill_invalid().
 */

cpl_error_code cpl_column_fill_complex(cpl_column *column, cpl_size start, 
                                       cpl_size count, double complex value)
{

    const cpl_type    type = cpl_column_get_type(column);


    if (column == NULL) return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    switch (type) {
    case CPL_TYPE_FLOAT_COMPLEX:
        return cpl_column_fill_float_complex(column, start, count, value);
    case CPL_TYPE_DOUBLE_COMPLEX:
        return cpl_column_fill_double_complex(column, start, count, value);
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

}


/* 
 * @brief
 *   Write the same value within an @em integer column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start 
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is returned. If the input column has length zero, the error 
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to an @em integer column segment. The written 
 * values are automatically flagged as valid. To invalidate a column 
 * interval use @c cpl_column_fill_invalid(). The @em count argument
 * can go beyond the column end, and in that case the specified @em value
 * will be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_int(cpl_column *column, 
                                   cpl_size start, cpl_size count, int value)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    int        *ip;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_INT)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    ip = column->values->i + start;

    while (count--)
        *ip++ = value;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write the same value within a @em long @em integer column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em long @em integer column segment. The written
 * values are automatically flagged as valid. To invalidate a column
 * interval use @c cpl_column_fill_invalid(). The @em count argument
 * can go beyond the column end, and in that case the specified @em value
 * will be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_long(cpl_column *column,
                                    cpl_size start, cpl_size count, long value)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    long       *lp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_LONG)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    lp = column->values->l + start;

    while (count--)
        *lp++ = value;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write the same value within a @em long @em long @em integer column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em long @em long @em integer column segment.
 * The written values are automatically flagged as valid. To invalidate a
 * column interval use @c cpl_column_fill_invalid(). The @em count argument
 * can go beyond the column end, and in that case the specified @em value
 * will be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code
cpl_column_fill_long_long(cpl_column *column,
                          cpl_size start, cpl_size count, long long value)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    long long  *llp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_LONG_LONG)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    llp = column->values->ll + start;

    while (count--)
        *llp++ = value;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write the same value within a @em cpl_size column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em cpl_size column segment. The written values
 * are automatically flagged as valid. To invalidate a column interval
 * use @c cpl_column_fill_invalid(). The @em count argument can go beyond
 * the column end, and in that case the specified @em value will
 * be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_cplsize(cpl_column *column,
                                       cpl_size start, cpl_size count,
                                       cpl_size value)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    cpl_size   *szp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_SIZE)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    szp = column->values->sz + start;

    while (count--)
        *szp++ = value;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write a value to a @em float column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em float column segment. The written
 * values are automatically flagged as valid. To invalidate a column
 * interval use @c cpl_column_fill_invalid(). The @em count argument
 * can go beyond the column end, and in that case the specified @em value
 * will be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_float(cpl_column *column, cpl_size start, 
                                     cpl_size count, float value)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    float      *fp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_FLOAT)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    fp = column->values->f + start;

    while (count--)
        *fp++ = value;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a value to a @em float complex column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em float complex column segment. The written
 * values are automatically flagged as valid. To invalidate a column
 * interval use @c cpl_column_fill_invalid(). The @em count argument
 * can go beyond the column end, and in that case the specified @em value
 * will be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_float_complex(cpl_column *column, 
                                             cpl_size start, cpl_size count, 
                                             float complex value)
{

    cpl_size       length = cpl_column_get_size(column);
    cpl_type       type   = cpl_column_get_type(column);
    float complex *fp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_FLOAT_COMPLEX)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    fp = column->values->cf + start;

    while (count--)
        *fp++ = value;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a value to a @em double column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em double column segment. The written
 * values are automatically flagged as valid. To invalidate a column
 * interval use @c cpl_column_fill_invalid(). The @em count argument
 * can go beyond the column end, and in that case the specified @em value
 * will be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_double(cpl_column *column, cpl_size start, 
                                      cpl_size count, double value)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    double     *dp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_DOUBLE)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    dp = column->values->d + start;

    while (count--)
        *dp++ = value;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a value to a @em double complex column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em double complex column segment. The written
 * values are automatically flagged as valid. To invalidate a column
 * interval use @c cpl_column_fill_invalid(). The @em count argument
 * can go beyond the column end, and in that case the specified @em value
 * will be written just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_double_complex(cpl_column *column, 
                                              cpl_size start, cpl_size count, 
                                              double complex value)
{

    cpl_size        length = cpl_column_get_size(column);
    cpl_type        type   = cpl_column_get_type(column);
    double complex *dp;
    

    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_DOUBLE_COMPLEX)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    dp = column->values->cd + start;

    while (count--)
        *dp++ = value;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Write a string to a string column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Copy the same string to a string column segment. If the input string 
 * is not a @c NULL pointer, it is duplicated for each accessed column 
 * element. If the input string is @c NULL, this call is equivalent to 
 * @c cpl_column_fill_invalid(). The @em count argument can go beyond 
 * the column end, and in that case the specified @em value will be 
 * copied just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set.
 */

cpl_error_code cpl_column_fill_string(cpl_column *column, cpl_size start,
                                      cpl_size count, const char *value)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    char      **sp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_STRING)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    if (value == NULL)
        return cpl_column_fill_invalid(column, start, count);

    sp = column->values->s + start;
    while (count--) {
        if (*sp)
            cpl_free(*sp);
        *sp++ = cpl_strdup(value);
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Write the same array to an array column segment.
 *   
 * @param column  Column to be accessed.
 * @param start   Position where to begin write value.
 * @param count   Number of values to write.
 * @param array   Array to write.
 * 
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL
 *   pointer a @c CPL_ERROR_NULL_INPUT is returned. If the column
 *   does not match the type of the input @em array, or if the column
 *   is not made of arrays, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   If the input @em array size is different from the column depth,
 *   a @c CPL_ERROR_INCOMPATIBLE_INPUT is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is returned. If the input column has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *   
 * Copy the same array to an array column segment. If the input array
 * is not a @c NULL pointer, it is duplicated for each accessed column 
 * element. If the input array is @c NULL, this call is equivalent to
 * @c cpl_column_fill_invalid(). The @em count argument can go beyond 
 * the column end, and in that case the specified @em value will be 
 * copied just up to the end of the column. If @em count is zero,
 * the column is not modified and no error is set. 
 */

cpl_error_code cpl_column_fill_array(cpl_column *column, cpl_size start,
                                     cpl_size count, const cpl_array *array)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);
    cpl_type    atype;
    cpl_array **ap;
    

    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (!(type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (array) {
        atype = cpl_array_get_type(array);
        if (type != (atype | CPL_TYPE_POINTER))
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        if (cpl_column_get_depth(column) != cpl_array_get_size(array))
            return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    }

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    if (array == NULL)
        return cpl_column_fill_invalid(column, start, count);

    ap = column->values->array + start;
    while (count--) {
        if (*ap)
            cpl_array_delete(*ap);
        *ap++ = cpl_array_duplicate(array);
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Set a column segment to NULL.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to start writing NULLs.
 * @param count   Number of column elements to set to NULL.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If @em start is outside the 
 *   column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If the 
 *   input column has length zero, the error @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is always returned. If @em count is negative, a @c CPL_ERROR_ILLEGAL_INPUT 
 *   is returned.
 *
 * Invalidate values contained in a column segment. The @em count argument 
 * can go beyond the column end, and in that case the values will be
 * invalidated up to the end of the column. If @em count is zero, the 
 * column is not modified and no error is set. In the case of a @em string 
 * or @em array column, the invalidated strings or arrays are set free and 
 * their pointers are set to @c NULL; for other data types, the corresponding 
 * elements are flagged as invalid.
 */

cpl_error_code cpl_column_fill_invalid(cpl_column *column, 
                                       cpl_size start, cpl_size count)
{

    cpl_type          type   = cpl_column_get_type(column);
    cpl_size          length = cpl_column_get_size(column);
    cpl_size          c;
    cpl_column_flag  *np;
    char            **sp;
    cpl_array       **ap;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count < 0) 
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count > length - start)
        count = length - start;

    if (count == length) {
        if (type != CPL_TYPE_STRING && !(type & CPL_TYPE_POINTER)) {
            if (column->null)
                cpl_free(column->null);
            column->null = NULL;
            column->nullcount = length;
            return CPL_ERROR_NONE;
        }
    }

    c = count;

    if (type == CPL_TYPE_STRING) {

        sp = column->values->s + start;

        while (c--) {
            if (*sp)
                cpl_free(*sp);
            sp++;
        }

        memset(column->values->s + start, 0, count * sizeof(char *));

        return CPL_ERROR_NONE;
    }

    if (type & CPL_TYPE_POINTER) {
 
        ap = column->values->array + start;
 
        while (c--) { 
            if (*ap)
                cpl_array_delete(*ap);
            ap++;
        }

        memset(column->values->array + start, 0, count * sizeof(cpl_array *));

        return CPL_ERROR_NONE;
    }

    if (column->nullcount == length)
        return CPL_ERROR_NONE;

    if (!column->null)
        column->null = cpl_calloc(length, sizeof(cpl_column_flag));

    np = column->null + start;

    if (column->nullcount == 0) {
        column->nullcount = count;
        while (c--)
            *np++ = 1;
    }
    else {
        while (c--) {
            if (*np == 0) {
                *np = 1;
                column->nullcount++;
            }
            np++;
        }

        if (column->nullcount == length) {
            if (column->null)
                cpl_free(column->null);
            column->null = NULL;
        }
    }

    return CPL_ERROR_NONE;
    
}


/*
 * @brief
 *   Copy an array of values to a numerical column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of numerical
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em start is outside
 *   the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input column has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned. If @em count is not greater than zero, a 
 *   @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the values to a numerical column segment. The values are cast to
 * the accessed column type. The written values are automatically flagged 
 * as valid. The @em count argument can go beyond the column end, and in 
 * that case values will be transferred just up to the end of the column.
 * This function doesn't apply to array column types.
 */

cpl_error_code cpl_column_copy_segment(cpl_column *column, 
                                       cpl_size start, cpl_size count, 
                                       double *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *idata = cpl_column_get_data_int(column);
            idata += start;
            while (count--)
                *idata++ = *values++;
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *ldata = cpl_column_get_data_long(column);
            ldata += start;
            while (count--)
                *ldata++ = *values++;
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata = cpl_column_get_data_long_long(column);
            lldata += start;
            while (count--)
                *lldata++ = *values++;
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szdata = cpl_column_get_data_cplsize(column);
            szdata += start;
            while (count--)
                *szdata++ = *values++;
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fdata = cpl_column_get_data_float(column);
            fdata += start;
            while (count--)
                *fdata++ = *values++;
            break;
        }
        case CPL_TYPE_DOUBLE:
            memcpy(column->values->d + start, values, count * sizeof(double));
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy an array of complex values to a complex column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of complex
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em start is outside
 *   the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input column has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned. If @em count is not greater than zero, a 
 *   @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the values to a complex column segment. The values are cast to
 * the accessed column type. The written values are automatically flagged 
 * as valid. The @em count argument can go beyond the column end, and in 
 * that case values will be transferred just up to the end of the column.
 * This function doesn't apply to array column types.
 */

cpl_error_code cpl_column_copy_segment_complex(cpl_column *column, 
                                               cpl_size start, cpl_size count, 
                                               double complex *values)
{

    cpl_size       length = cpl_column_get_size(column);
    cpl_type       type   = cpl_column_get_type(column);
    float complex *fdata;
    

    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (count > length - start)
        count = length - start;

    cpl_column_unset_null_segment(column, start, count);

    switch (type) {
    case CPL_TYPE_FLOAT_COMPLEX:
        fdata = cpl_column_get_data_float_complex(column);
        fdata += start;
        while (count--)
            *fdata++ = *values++;
        break;
    case CPL_TYPE_DOUBLE_COMPLEX:
        memcpy(column->values->cd + start, values, 
               count * sizeof(double complex));
        break;
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy a list of integer values to an @em integer column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the 
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start 
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is returned. If the input column has length zero, the 
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count 
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of integer values to an @em integer column segment. 
 * The written values are automatically flagged as valid. The @em count 
 * argument can go beyond the column end, and in that case values will 
 * be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_int(cpl_column *column, 
                                           cpl_size start, cpl_size count, 
                                           int *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_INT)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->i + start, values, count * sizeof(int));

    cpl_column_unset_null_segment(column, start, count);

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy a list of long integer values to a @em long @em integer column
 *   segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of long integer values to a @em long @em integer column
 * segment. The written values are automatically flagged as valid. The
 * @em count argument can go beyond the column end, and in that case
 * values will be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_long(cpl_column *column,
                                            cpl_size start, cpl_size count,
                                            long *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_LONG)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->l + start, values, count * sizeof(long));

    cpl_column_unset_null_segment(column, start, count);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy a list of long integer values to a @em long @em long @em integer
 *   column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of long long integer values to a @em long @em long @em integer
 * column segment. The written values are automatically flagged as valid.
 * The @em count argument can go beyond the column end, and in that case
 * values will be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_long_long(cpl_column *column,
                                                 cpl_size start, cpl_size count,
                                                 long long *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_LONG_LONG)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->ll + start, values, count * sizeof(long long));

    cpl_column_unset_null_segment(column, start, count);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy a list of cpl_size values to a @em cpl_size column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of cpl_size values to a @em cpl_size column segment.
 * The written values are automatically flagged as valid.
 * The @em count argument can go beyond the column end, and in that case
 * values will be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_cplsize(cpl_column *column,
                                               cpl_size start, cpl_size count,
                                               cpl_size *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_SIZE)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->sz + start, values, count * sizeof(cpl_size));

    cpl_column_unset_null_segment(column, start, count);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy a list of float values to a @em float column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of float values to a @em float column segment.
 * The written values are automatically flagged as valid. The @em count 
 * argument can go beyond the column end, and in that case values will 
 * be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_float(cpl_column *column, cpl_size start,
                                             cpl_size count, float *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_FLOAT)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->f + start, values, count * sizeof(float));

    cpl_column_unset_null_segment(column, start, count);

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy a list of float complex values to a @em float complex column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of float complex values to a @em float complex column segment.
 * The written values are automatically flagged as valid. The @em count 
 * argument can go beyond the column end, and in that case values will 
 * be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_float_complex(cpl_column *column, 
                                                     cpl_size start, 
                                                     cpl_size count, 
                                                     float complex *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_FLOAT_COMPLEX)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->cf + start, values, count * sizeof(float complex));

    cpl_column_unset_null_segment(column, start, count);

    return CPL_ERROR_NONE;
}


/*
 * @brief
 *   Copy list of double complex values to a @em double complex column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of double complex values to a @em double complex column segment.
 * The written values are automatically flagged as valid. The @em count 
 * argument can go beyond the column end, and in that case values will 
 * be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_double_complex(cpl_column *column,
                                                      cpl_size start,
                                                      cpl_size count,
                                                      double complex *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_DOUBLE_COMPLEX)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->cd + start, values, count * sizeof(double complex));

    cpl_column_unset_null_segment(column, start, count);

    return CPL_ERROR_NONE;
}


/* 
 * @brief
 *   Copy a list of @em double values to a @em double column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write values.
 * @param count   Number of values to write.
 * @param values  Values to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of double values to a @em double column segment.
 * The written values are automatically flagged as valid. The @em count 
 * argument can go beyond the column end, and in that case values will 
 * be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_double(cpl_column *column,
                                              cpl_size start, cpl_size count,
                                              double *values)
{

    cpl_size    length = cpl_column_get_size(column);
    cpl_type    type   = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_DOUBLE)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    memcpy(column->values->d + start, values, count * sizeof(double));
 
    cpl_column_unset_null_segment(column, start, count);
 
    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Copy a list of strings to a @em string column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write strings.
 * @param count   Number of strings to write.
 * @param strings Strings to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the column is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of strings to a @em string column segment. The written 
 * values are automatically flagged as valid. The input strings are
 * duplicated before being assigned to the corresponding column elements.
 * The @em count argument can go beyond the column end, and in that case 
 * values will be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_string(cpl_column *column, 
                                              cpl_size start, cpl_size count,
                                              char **strings)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    cpl_size    i;
    char      **sp;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (type != CPL_TYPE_STRING)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    sp = column->values->s + start;

    if (strings) {
        for (i = 0; i < count; i++) {
            if (*sp)
                cpl_free(*sp);
            if (*strings)
                *sp++ = cpl_strdup(*strings++);
            else
                *sp++ = NULL;
        }
    }
    else {
        for (i = 0; i < count; i++) {
            if (*sp)
                cpl_free(*sp);
            *sp++ = NULL;
        }
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Copy a list of CPL arrays to an @em array column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position where to begin write strings.
 * @param count   Number of strings to write.
 * @param arrays  Arrays to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em column is a @c NULL 
 *   pointer a @c CPL_ERROR_NULL_INPUT is returned. If the column
 *   does not match the type of any of the input arrays, or if the column
 *   is not made of arrays, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   If the size of any of the input arrays is different from the column 
 *   depth a @c CPL_ERROR_INCOMPATIBLE_INPUT is returned. If @em start
 *   is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input column has length zero, the
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write a list of arrays to an @em array column segment. The written
 * values are automatically flagged as valid. The input arrays are
 * duplicated before being assigned to the corresponding column elements.
 * The @em count argument can go beyond the column end, and in that case
 * values will be transferred just up to the end of the column.
 */

cpl_error_code cpl_column_copy_segment_array(cpl_column *column, 
                                             cpl_size start, cpl_size count,
                                             cpl_array **arrays)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_type    atype;
    cpl_size    length = cpl_column_get_size(column);
    cpl_size    i;
    cpl_array **ap;
    cpl_array **keep;
    

    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count <= 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (!(type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (count > length - start)
        count = length - start;

    ap = column->values->array + start;

    if (arrays) {

        /*
         * Check the compatibility of all the input arrays first
         */

        keep = arrays;

        for (i = 0; i < count; i++) {
            if (*arrays) {
                atype = cpl_array_get_type(*arrays);
                if (type != (atype | CPL_TYPE_POINTER))
                    return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
                if (cpl_column_get_depth(column) != cpl_array_get_size(*arrays))
                    return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
                arrays++;
            }
        }

        arrays = keep;

        for (i = 0; i < count; i++) {
            if (*ap)
                cpl_array_delete(*ap);
            if (*arrays)
                *ap++ = cpl_array_duplicate(*arrays++);
            else
                *ap++ = NULL;
        }
    }
    else {
        for (i = 0; i < count; i++) {
            if (*ap)
                cpl_array_delete(*ap);
            *ap++ = NULL;
        }
    }

    return CPL_ERROR_NONE;

}

/*************** TODO ******************/
/*  
 * @brief
 *   Copy a segment of a column into another column.
 *
 * @param to_column   Column to be modified.
 * @param to_start    Position where to begin write values.
 * @param from_column Source column.
 * @param from_start  Position where to begin copy values.
 * @param count       Number of values to copy.
 *
 * @return 0 on success.
 *
 * Both columns must be of the same type.
 */

/*************** TODO ******************

cpl_error_code cpl_column_extract_from_column(cpl_column *to_column, 
                                              size_t to_start,
                                              cpl_column *from_column, 
                                              size_t from_start, 
                                              size_t count)
{

    size_t   i           = 0;
    size_t   nullcount   = 0;
    size_t   to_length   = cpl_column_get_size(to_column);
    size_t   from_length = cpl_column_get_size(from_column);
    cpl_type to_type     = cpl_column_get_type(to_column);
    cpl_type from_type   = cpl_column_get_type(from_column);


    assert(to_column != NULL);
    assert(from_column != NULL);


    if (to_type != from_type)
        return 1;

    if (to_start > to_length || from_start > from_length)
        return 1;

    if (to_start + count > to_length || from_start + count > from_length)
        return 1;

    if (from_column->nullcount == from_length)
        return cpl_column_fill_invalid(to_column, to_start, count);

    if (from_column->nullcount) {
        while (i < count) {
            nullcount += from_column->null[from_start + i];
            i++;
        }
    }

    if (nullcount == count)
        return cpl_column_fill_invalid(to_column, to_start, count);

    if (nullcount) {
        if (to_column->nullcount == to_length) {
            to_column->null = cpl_malloc(to_length * sizeof(cpl_column_flag));
        }
    }

    switch (to_type) {

    case CPL_TYPE_INT:
        memcpy(to_column->values->i + to_start, 
               from_column->values->i + from_start, count * sizeof(int));
    case CPL_TYPE_FLOAT:
    case CPL_TYPE_DOUBLE:
    case CPL_TYPE_STRING:
    default:

    }

    if (start < length) {

        if (count > length - start)
            count = length - start;

        if (type == CPL_TYPE_INT) {

            memcpy(column->values->i + start, 
                   values, count * sizeof(int));

            return cpl_column_unset_null_segment(column, start, count);

        }
    }

}

**********************************/


/* 
 * @brief
 *   Delete a column segment.
 *
 * @param column  Column to be accessed.
 * @param start   Position of first column element to delete.
 * @param count   Number of column elements to delete.
 *
 * @return @c CPL_ERROR_NONE on success. If @em start is outside the 
 *   column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If the 
 *   input column has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is
 *   always returned. If @em count is negative, a @c CPL_ERROR_ILLEGAL_INPUT 
 *   is returned.
 *
 * Delete a column segment. This is different from invalidating a column 
 * segment, as done by @c cpl_column_fill_invalid(): here a portion of 
 * the data is physically removed, and the column data buffer shortened 
 * proportionally. The pointer to data may change (therefore pointers 
 * previously retrieved by calling @c cpl_column_get_data_int(), 
 * @c cpl_column_get_data_string(), etc., should be discarded). The 
 * specified segment can go beyond the end of the column, and even all 
 * of the column data elements can be removed.
 */

cpl_error_code cpl_column_erase_segment(cpl_column *column, 
                                        cpl_size start, cpl_size count)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        old_length = cpl_column_get_size(column);
    cpl_size        new_length;
    cpl_size        end;
    cpl_size        i;
    cpl_size        j;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0 || start >= old_length)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (count > old_length - start)
        count = old_length - start;

    new_length = old_length - count; 
    end = start + count;

    if (end == old_length)
        return cpl_column_set_size(column, start);

    if (type == CPL_TYPE_STRING) {

        /*
         *  If a column of character strings is shortened, we must
         *  free explicitly the extra strings.
         */

        char **sp1 = column->values->s + start;

        for (i = start; i < end; i++, sp1++)
            if (*sp1)
                cpl_free(*sp1);
    }

    if (type & CPL_TYPE_POINTER) {

        /*
         *  If a column of arrays is shortened, we must
         *  free explicitly the extra arrays.
         */

        cpl_array **ap1 = column->values->array + start;

        for (i = start; i < end; i++, ap1++)
            if (*ap1)
                cpl_array_delete(*ap1);
    }


    /*
     *  Cut the slice from the invalid flags buffer, if present, 
     *  counting how many invalid elements will be lost.
     */

    if (column->null) {

        i = start;
        while (i < end) {
            if (column->null[i])
                column->nullcount--;
            i++;
        }

        if (column->nullcount == 0 || column->nullcount == new_length) {

            /*
             *  Since either no invalid or just invalid elements are left, 
             *  the invalid flag buffer can be released.
             */

            if (column->null)
                cpl_free(column->null);
            column->null = NULL;

        }
        else {

            /*
             *  Shift up NULL flags that are after the slice.
             */

            i = start;
            j = end;
            while (j < old_length) {
                column->null[i] = column->null[j];
                i++;
                j++;
            }

            /*
             *  Get rid of extra memory.
             */

            column->null = cpl_realloc(column->null,
                           new_length * sizeof(cpl_column_flag));

        }
    }
    else {

        /*
         *  There is no invalid flag buffer, so either there are no 
         *  invalid elements or there are just invalid elements. If 
         *  there are no invalid elements, nullcount stays zero. If 
         *  there are just invalid elements, nullcount must be set 
         *  to the new column length.
         */

        if (column->nullcount == old_length)
            column->nullcount = new_length;

    }


    /*
     *  Now take care of data values
     */

    if ((type == CPL_TYPE_STRING) || (type & CPL_TYPE_POINTER))
        column->nullcount = 0;

    if (column->nullcount != new_length) {

        /*
         *  Column has not just invalid elements. In this case we have to
         *  shift also the data to fill the gap.
         */

        switch (type) {
            case CPL_TYPE_INT:
            {
                int *ip1  = column->values->i + start;
                int *ip2  = column->values->i + end;

                for (j = end; j < old_length; j++)
                    *ip1++ = *ip2++;
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *lp1  = column->values->l + start;
                long *lp2  = column->values->l + end;

                for (j = end; j < old_length; j++)
                    *lp1++ = *lp2++;
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *llp1  = column->values->ll + start;
                long long *llp2  = column->values->ll + end;

                for (j = end; j < old_length; j++)
                    *llp1++ = *llp2++;
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *szp1  = column->values->sz + start;
                cpl_size *szp2  = column->values->sz + end;

                for (j = end; j < old_length; j++)
                    *szp1++ = *szp2++;
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *fp1  = column->values->f + start;
                float *fp2  = column->values->f + end;

                for (j = end; j < old_length; j++)
                    *fp1++ = *fp2++;
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *dp1  = column->values->d + start;
                double *dp2  = column->values->d + end;

                for (j = end; j < old_length; j++)
                    *dp1++ = *dp2++;
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *cfp1 = column->values->cf + start;
                float complex *cfp2 = column->values->cf + end;

                for (j = end; j < old_length; j++)
                    *cfp1++ = *cfp2++;
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *cdp1 = column->values->cd + start;
                double complex *cdp2 = column->values->cd + end;

                for (j = end; j < old_length; j++)
                    *cdp1++ = *cdp2++;
                break;
            }
            case CPL_TYPE_STRING:
            {
                char **sp1  = column->values->s + start;
                char **sp2  = column->values->s + end;

                for (j = end; j < old_length; j++)
                    *sp1++ = *sp2++;
                break;
            }
            default:     /* Array columns */
            {
                cpl_array **ap1  = column->values->array + start;
                cpl_array **ap2  = column->values->array + end;

                for (j = end; j < old_length; j++)
                    *ap1++ = *ap2++;
                break;
            }
        }

    }


    /*
     *  Finally, reallocate the data buffer.
     */

    column->values->i = cpl_realloc(column->values->i,
                        new_length * cpl_column_type_size(type));

    column->length = new_length;

    return 0;

}


/* 
 * @brief
 *   Delete column elements
 *
 * @param column    Column to be accessed.
 * @param pattern   Array that defines which elements (non-zero values)
 *                  to be erased. The array length should equal the column
 *                  length.
 * @return @c CPL_ERROR_NONE on success, CPL_ERROR_NULL_INPUT if any input
 *   pointer is NULL. If the column has length zero, nothing happens and
 *   CPL_ERROR_NONE is returned.
 *
 * Delete column elements according to non-zero values of the
 * provided @em pattern. Elements are physically removed, and the column
 * data buffer shortened proportionally. The pointer to data may change
 * (therefore pointers previously retrieved by calling
 * @c cpl_column_get_data_int(), @c cpl_column_get_data_string(), 
 * etc., should be discarded).
 *
 * The execution time O(n) where n is the column length.
 */

cpl_error_code cpl_column_erase_pattern(cpl_column *column, 
                                        int *pattern)
{
    
    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        old_length = cpl_column_get_size(column);
    cpl_size        new_length;
    cpl_size        i;
    cpl_size        j;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (pattern == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    new_length = 0;
    for (i = 0; i < old_length; i++)
        if (!pattern[i]) 
            new_length++;

    if (type == CPL_TYPE_STRING) {

        /*
         *  If a column of character strings is shortened, we must
         *  free explicitly the extra strings.
         */

        for(i = 0; i < old_length; i++)
            if (pattern[i])
                if (column->values->s[i])
                    cpl_free(column->values->s[i]);

    }

    if (type & CPL_TYPE_POINTER) {

        /*
         *  If a column of arrays is shortened, we must
         *  free explicitly the extra arrays.
         */

        for(i = 0; i < old_length; i++)
            if (pattern[i])
                if (column->values->array[i])
                    cpl_array_delete(column->values->array[i]);

    }


    /*
     *  Shorten the invalid flags buffer, if present, 
     *  counting how many invalid elements will be lost.
     */
    
    if (column->null) {
        
        for (i = 0; i < old_length; i++)
            if (pattern[i])
                if (column->null[i])
                    column->nullcount--;

        if (column->nullcount == 0 || column->nullcount == new_length) {
            
            /*
             *  Since either no invalid or just invalid elements are left, 
             *  the invalid flag buffer can be released.
             */
            
            if (column->null)
                cpl_free(column->null);
            column->null = NULL;
            
        }
        else {

            /*
             *  Shift up NULL flags.
             */
            
            i = 0;
            for (j = 0; j < old_length; j++)
                if (!pattern[j]) {
                    column->null[i] = column->null[j];
                    i++;
                }
            
            /*
             *  Get rid of extra memory.
             */
            
            column->null = cpl_realloc(column->null,
                                       new_length * sizeof(cpl_column_flag));
            
        }
    }
    else {

        /*
         *  There is no invalid flag buffer, so either there are no 
         *  invalid elements or there are just invalid elements. If 
         *  there are no invalid elements, nullcount stays zero. If 
         *  there are just invalid elements, nullcount must be set 
         *  to the new column length.
         */

        if (column->nullcount == old_length)
            column->nullcount = new_length;

    }


    /*
     *  Now take care of data values
     */

    if ((type == CPL_TYPE_STRING) || (type & CPL_TYPE_POINTER))
        column->nullcount = 0;

    if (column->nullcount != new_length) {

        /*
         *  Column has not just invalid elements. In this case we have to
         *  shift also the data to fill the gaps.
         */

        switch (type) {
            case CPL_TYPE_INT:
            {
                int *ip = column->values->i;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        ip[i++] = ip[j];
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *lp = column->values->l;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        lp[i++] = lp[j];
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *llp = column->values->ll;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        llp[i++] = llp[j];
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *szp = column->values->sz;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        szp[i++] = szp[j];
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *fp  = column->values->f;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        fp[i++] = fp[j];
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *dp  = column->values->d;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        dp[i++] = dp[j];
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *cfp = column->values->cf;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        cfp[i++] = cfp[j];
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *cdp = column->values->cd;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        cdp[i++] = cdp[j];
                break;
            }
            case CPL_TYPE_STRING:
            {
                char **sp  = column->values->s;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        sp[i++] = sp[j];
                break;
            }
            default:     /* Array columns */
            {
                cpl_array **ap  = column->values->array;

                i = 0;
                for(j = 0; j < old_length; j++)
                    if (!pattern[j])
                        ap[i++] = ap[j];
                break;
            }
        }

    }


    /*
     *  Finally, reallocate the data buffer.
     */

    column->values->i = cpl_realloc(column->values->i,
                        new_length * cpl_column_type_size(type));

    column->length = new_length;

    return 0;

}


/*
 * @brief
 *   Insert a segment into column data.
 *
 * @param column  Column to be accessed.
 * @param start   Column element where to insert the segment.
 * @param count   Length of segment.
 *
 * @return @c CPL_ERROR_NONE on success. If @em start is negative,
 *   a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If @em count 
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Insert a segment of new values in the column data buffer. The
 * new segment consists of a sequence of invalid (not initialised)
 * column elements. This is different from invalidating a column data 
 * segment, as done by @c cpl_column_fill_invalid(): here a portion 
 * of new data is physically inserted, and the column data buffer 
 * is made proportionally longer. Setting @em start to a number 
 * not less than the column length is legal, and has the effect of 
 * appending at the end of the column as many invalid elements as 
 * specified by the @em count argument (this is equivalent to expanding 
 * a column using @c cpl_column_set_size() ). The input @em column may
 * also have zero length. The pointer to data may change, therefore 
 * pointers previously retrieved by calling @c cpl_column_get_data_int(), 
 * @c cpl_column_get_data_string(), etc., should be discarded.
 */

cpl_error_code cpl_column_insert_segment(cpl_column *column, 
                                         cpl_size start, cpl_size count)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        old_length = cpl_column_get_size(column);
    cpl_size        new_length = old_length + count;
    cpl_size        i;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (start < 0)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (count < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (count == 0)
        return CPL_ERROR_NONE;

    if (cpl_column_set_size(column, new_length)) {
        return cpl_error_set_where_();
    }

    if (start > old_length)
        return CPL_ERROR_NONE;


    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip1  = column->values->i + new_length;
            int *ip2  = column->values->i + old_length;

            for (i = old_length; i > start; i--) {
                *--ip1 = *--ip2;
                *ip2 = 0;
            }
            break;
        }

        case CPL_TYPE_LONG:
        {
            long *lp1  = column->values->l + new_length;
            long *lp2  = column->values->l + old_length;

            for (i = old_length; i > start; i--) {
                *--lp1 = *--lp2;
                *lp2 = 0;
            }
            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *llp1  = column->values->ll + new_length;
            long long *llp2  = column->values->ll + old_length;

            for (i = old_length; i > start; i--) {
                *--llp1 = *--llp2;
                *llp2 = 0;
            }
            break;
        }

        case CPL_TYPE_SIZE:
        {
            cpl_size *szp1  = column->values->sz + new_length;
            cpl_size *szp2  = column->values->sz + old_length;

            for (i = old_length; i > start; i--) {
                *--szp1 = *--szp2;
                *szp2 = 0;
            }
            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fp1  = column->values->f + new_length;
            float *fp2  = column->values->f + old_length;

            for (i = old_length; i > start; i--) {
                *--fp1 = *--fp2;
                *fp2 = 0.0;
            }
            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *dp1  = column->values->d + new_length;
            double *dp2  = column->values->d + old_length;

            for (i = old_length; i > start; i--) {
                *--dp1 = *--dp2;
                *dp2 = 0.0;
            }
            break;
        }

        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex *cfp1 = column->values->cf + new_length;
            float complex *cfp2 = column->values->cf + old_length;

            for (i = old_length; i > start; i--) {
                *--cfp1 = *--cfp2;
                *cfp2 = 0.0;
            }
            break;
        }

        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex *cdp1 = column->values->cd + new_length;
            double complex *cdp2 = column->values->cd + old_length;

            for (i = old_length; i > start; i--) {
                *--cdp1 = *--cdp2;
                *cdp2 = 0.0;
            }
            break;
        }

        case CPL_TYPE_STRING:
        {
            char **sp1  = column->values->s + new_length;
            char **sp2  = column->values->s + old_length;

            for (i = old_length; i > start; i--) {
                *--sp1 = *--sp2;
                *sp2 = NULL;
            }
            break;
        }

        default:                   /* Array columns */
        {
            cpl_array **ap1  = column->values->array + new_length;
            cpl_array **ap2  = column->values->array + old_length;

            for (i = old_length; i > start; i--) {
                *--ap1 = *--ap2;
                *ap2 = NULL;
            }
            break;
        }
    }


    /*
     *  Handling the invalid flags it's simple: all the hard work
     *  was actually done by cpl_column_set_size(). If the invalid
     *  flags buffer exists, we have just to shift the flags to
     *  the proper place.
     */

    if (column->null) {

        int *np1 = column->null + new_length;
        int *np2 = column->null + old_length;

        for (i = old_length; i > start; i--) {
            *--np1 = *--np2;
            *np2 = 1;
        }

    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Make a copy of a column.
 *
 * @param column  Column to be duplicated.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * If the input @em column is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT
 * is returned. Copy is "in depth": in the case of a @em string column, 
 * also the string elements are duplicated.
 */

cpl_column *cpl_column_duplicate(const cpl_column *column)
{

    cpl_type    type       = cpl_column_get_type(column);
    cpl_size    length     = cpl_column_get_size(column);
    cpl_size    depth      = cpl_column_get_depth(column);
    cpl_column *new_column = NULL;


    if (column == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (type & CPL_TYPE_POINTER) {
        switch (type & ~CPL_TYPE_POINTER) {
        case CPL_TYPE_INT:
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG:
        case CPL_TYPE_SIZE:
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
        case CPL_TYPE_STRING:
            new_column = cpl_column_new_array(type, length, depth);
            break;
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            return NULL;
        }
    } else {
        switch (type) {
        case CPL_TYPE_INT:
            new_column = cpl_column_new_int(length);
            break;
        case CPL_TYPE_LONG:
            new_column = cpl_column_new_long(length);
            break;
        case CPL_TYPE_LONG_LONG:
            new_column = cpl_column_new_long_long(length);
            break;
        case CPL_TYPE_SIZE:
            new_column = cpl_column_new_cplsize(length);
            break;
        case CPL_TYPE_FLOAT:
            new_column = cpl_column_new_float(length);
            break;
        case CPL_TYPE_DOUBLE:
            new_column = cpl_column_new_double(length);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            new_column = cpl_column_new_float_complex(length);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            new_column = cpl_column_new_double_complex(length);
            break;
        case CPL_TYPE_STRING:
            new_column = cpl_column_new_string(length);
            break;
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            return NULL;
        }
    }

    cpl_column_set_name(new_column, cpl_column_get_name(column));
    cpl_column_set_unit(new_column, cpl_column_get_unit(column));
    cpl_column_set_format(new_column, cpl_column_get_format(column));

    if (length == 0)
        return new_column;

    if (type == CPL_TYPE_STRING) {
        while (length--) {
            if (column->values->s[length]) {
                new_column->values->s[length] = 
                                   cpl_strdup(column->values->s[length]);
            }
            else {
                new_column->values->s[length] = NULL;
            }
        }
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        while (length--) {
            if (column->values->array[length]) {
                new_column->values->array[length] =
                    cpl_array_duplicate(column->values->array[length]);
            }
            else {
                new_column->values->array[length] = NULL;
            }
        }

        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);

        return new_column;
    }

    memcpy(new_column->values->i, column->values->i, 
           length * cpl_column_type_size(type));

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null, 
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}


/*
 * @brief
 *   Cast a numerical column to a new @em integer column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *
 * If the accessed column is of type either string or array of strings, or
 * complex, a @c CPL_ERROR_INVALID_TYPE is set. If the input is a @c NULL 
 * pointer, a @c CPL_ERROR_NULL_INPUT is set. The output column is nameless, 
 * and with the default format for its type (see function 
 * @c cpl_column_set_format() ). However, if the input column is 
 * also @em integer, a simple copy is made and the column format 
 * is inherited. The input column units are always preserved in the
 * output column. Note that a column of arrays is always cast to 
 * another column of @em integer arrays. It is not allowed to cast 
 * a column of arrays into a column of plain integers.
 */

cpl_column *cpl_column_cast_to_int(const cpl_column *column)
{

    cpl_type    type       = cpl_column_get_type(column);
    cpl_size    length     = cpl_column_get_size(column);
    cpl_column *new_column = NULL;
    int        *np;
    int        *ip;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if ((type & CPL_TYPE_STRING) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_INT)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_INT | CPL_TYPE_POINTER,
                                          length, cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else
        new_column = cpl_column_new_int(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while (length--) {
            if (array[length]) {
                const cpl_column *acolumn =
                    cpl_array_get_column_const(array[length]);
                cpl_column *icolumn = cpl_column_cast_to_int(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(icolumn),
                                                  cpl_column_get_type(icolumn));
                cpl_array_set_column(new_array[length], icolumn);
            }
        }

        return new_column;

    }

    ip = cpl_column_get_data_int(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (column->nullcount == 0) {
                while (length--)
                    *ip++ = *lp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *ip = *lp;
                    ip++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp = column->values->ll;

            if (column->nullcount == 0) {
                while (length--)
                    *ip++ = *llp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *ip = *llp;
                    ip++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp = column->values->sz;

            if (column->nullcount == 0) {
                while (length--)
                    *ip++ = *szp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *ip = *szp;
                    ip++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--)
                    *ip++ = *fp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *ip = *fp;
                    ip++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--)
                    *ip++ = *dp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *ip = *dp;
                    ip++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null,
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_int_array(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    int         *np;
    cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if ((type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_INT | CPL_TYPE_POINTER,
                                      length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_INT);
                    cpl_array_set_int(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_INT);
                        cpl_array_set_int(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_INT);
                    cpl_array_set_int(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_INT);
                        cpl_array_set_int(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_INT);
                    cpl_array_set_int(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_INT);
                        cpl_array_set_int(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_INT);
                    cpl_array_set_int(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_INT);
                        cpl_array_set_int(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_INT);
                    cpl_array_set_int(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_INT);
                        cpl_array_set_int(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_INT);
                    cpl_array_set_int(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_INT);
                        cpl_array_set_int(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_int_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_int(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    while (length--) {
        array--;
        if (*array) {
            if (cpl_array_is_valid(*array, 0)) {
                cpl_column_set_int(new_column, length,
                                   cpl_array_get(*array, 0, NULL));
            }
        }
    }

    return new_column;

}


/*
 * @brief
 *   Cast a numerical column to a new @em long @em integer column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *
 * If the accessed column is of type either string or array of strings, or
 * complex, a @c CPL_ERROR_INVALID_TYPE is set. If the input is a @c NULL
 * pointer, a @c CPL_ERROR_NULL_INPUT is set. The output column is nameless,
 * and with the default format for its type (see function
 * @c cpl_column_set_format() ). However, if the input column is
 * also @em long @em integer, a simple copy is made and the column format
 * is inherited. The input column units are always preserved in the
 * output column. Note that a column of arrays is always cast to
 * another column of @em long @em integer arrays. It is not allowed to cast
 * a column of arrays into a column of plain long integers.
 */

cpl_column *cpl_column_cast_to_long(const cpl_column *column)
{

    cpl_type    type       = cpl_column_get_type(column);
    cpl_size    length     = cpl_column_get_size(column);
    cpl_column *new_column = NULL;
    int        *np;
    long       *lp;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if ((type & CPL_TYPE_STRING) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_LONG)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_LONG | CPL_TYPE_POINTER,
                                          length, cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else
        new_column = cpl_column_new_long(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while (length--) {
            if (array[length]) {
                const cpl_column *acolumn =
                    cpl_array_get_column_const(array[length]);
                cpl_column *lcolumn = cpl_column_cast_to_long(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(lcolumn),
                                                  cpl_column_get_type(lcolumn));
                cpl_array_set_column(new_array[length], lcolumn);
            }
        }

        return new_column;

    }

    lp = cpl_column_get_data_long(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (column->nullcount == 0) {
                while (length--)
                    *lp++ = *ip++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *lp = *ip;
                    lp++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp = column->values->ll;

            if (column->nullcount == 0) {
                while (length--)
                    *lp++ = *llp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *lp = *llp;
                    lp++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp = column->values->sz;

            if (column->nullcount == 0) {
                while (length--)
                    *lp++ = *szp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *lp = *szp;
                    lp++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--)
                    *lp++ = *fp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *lp = *fp;
                    lp++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--)
                    *lp++ = *dp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *lp = *dp;
                    lp++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null, 
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_long_array(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    int         *np;
    cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if ((type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_LONG | CPL_TYPE_POINTER,
                                      length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG);
                    cpl_array_set_long(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG);
                        cpl_array_set_long(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG);
                    cpl_array_set_long(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG);
                        cpl_array_set_long(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG);
                    cpl_array_set_long(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG);
                        cpl_array_set_long(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG);
                    cpl_array_set_long(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG);
                        cpl_array_set_long(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG);
                    cpl_array_set_long(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG);
                        cpl_array_set_long(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG);
                    cpl_array_set_long(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG);
                        cpl_array_set_long(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_long_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_long(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    while (length--) {
        array--;
        if (*array) {
            if (cpl_array_is_valid(*array, 0)) {
                cpl_column_set_long(new_column, length,
                                    cpl_array_get(*array, 0, NULL));
            }
        }
    }

    return new_column;

}


/*
 * @brief
 *   Cast a numerical column to a new @em long @em long @em integer column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *
 * If the accessed column is of type either string or array of strings, or
 * complex, a @c CPL_ERROR_INVALID_TYPE is set. If the input is a @c NULL
 * pointer, a @c CPL_ERROR_NULL_INPUT is set. The output column is nameless,
 * and with the default format for its type (see function
 * @c cpl_column_set_format() ). However, if the input column is
 * also @em long @em long @em integer, a simple copy is made and the column
 * format is inherited. The input column units are always preserved in the
 * output column. Note that a column of arrays is always cast to
 * another column of @em long @em long @em integer arrays. It is not allowed
 * to cast a column of arrays into a column of plain long long integers.
 */

cpl_column *cpl_column_cast_to_long_long(const cpl_column *column)
{

    cpl_type    type       = cpl_column_get_type(column);
    cpl_size    length     = cpl_column_get_size(column);
    cpl_column *new_column = NULL;
    int        *np;
    long long  *llp;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if ((type & CPL_TYPE_STRING) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_LONG_LONG)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_LONG_LONG | CPL_TYPE_POINTER,
                                          length, cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else
        new_column = cpl_column_new_long_long(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while (length--) {
            if (array[length]) {
                const cpl_column *acolumn =
                    cpl_array_get_column_const(array[length]);
                cpl_column *llcolumn = cpl_column_cast_to_long_long(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(llcolumn),
                                                  cpl_column_get_type(llcolumn));
                cpl_array_set_column(new_array[length], llcolumn);
            }
        }

        return new_column;

    }

    llp = cpl_column_get_data_long_long(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (column->nullcount == 0) {
                while (length--)
                    *llp++ = *ip++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *llp = *ip;
                    llp++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (column->nullcount == 0) {
                while (length--)
                    *llp++ = *lp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *llp = *lp;
                    llp++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp = column->values->sz;

            if (column->nullcount == 0) {
                while (length--)
                    *llp++ = *szp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *llp = *szp;
                    llp++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--)
                    *llp++ = *fp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *llp = *fp;
                    llp++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--)
                    *llp++ = *dp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *llp = *dp;
                    llp++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null,
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_long_long_array(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    int         *np;
    cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if ((type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_LONG_LONG | CPL_TYPE_POINTER,
                                      length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                    cpl_array_set_long_long(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                        cpl_array_set_long_long(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                    cpl_array_set_long_long(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                        cpl_array_set_long_long(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                    cpl_array_set_long_long(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                        cpl_array_set_long_long(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                    cpl_array_set_long_long(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                        cpl_array_set_long_long(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                    cpl_array_set_long_long(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                        cpl_array_set_long_long(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                    cpl_array_set_long_long(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_LONG_LONG);
                        cpl_array_set_long_long(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_long_long_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_long_long(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    while (length--) {
        array--;
        if (*array) {
            if (cpl_array_is_valid(*array, 0)) {
                cpl_column_set_long_long(new_column, length,
                                         cpl_array_get(*array, 0, NULL));
            }
        }
    }

    return new_column;

}


/*
 * @brief
 *   Cast a numerical column to a new @em cpl_size column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *
 * If the accessed column is of type either string or array of strings, or
 * complex, a @c CPL_ERROR_INVALID_TYPE is set. If the input is a @c NULL
 * pointer, a @c CPL_ERROR_NULL_INPUT is set. The output column is nameless,
 * and with the default format for its type (see function
 * @c cpl_column_set_format() ). However, if the input column is
 * also @em cpl_size, a simple copy is made and the column format
 * is inherited. The input column units are always preserved in the
 * output column. Note that a column of arrays is always cast to
 * another column of @em cpl_size arrays. It is not allowed
 * to cast a column of arrays into a column of plain cpl_size.
 */

cpl_column *cpl_column_cast_to_cplsize(const cpl_column *column)
{

    cpl_type    type       = cpl_column_get_type(column);
    cpl_size    length     = cpl_column_get_size(column);
    cpl_column *new_column = NULL;
    int        *np;
    cpl_size   *szp;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if ((type & CPL_TYPE_STRING) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_SIZE)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_SIZE | CPL_TYPE_POINTER,
                                          length, cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else
        new_column = cpl_column_new_cplsize(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while (length--) {
            if (array[length]) {
                const cpl_column *acolumn =
                    cpl_array_get_column_const(array[length]);
                cpl_column *szcolumn = cpl_column_cast_to_cplsize(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(szcolumn),
                                                  cpl_column_get_type(szcolumn));
                cpl_array_set_column(new_array[length], szcolumn);
            }
        }

        return new_column;

    }

    szp = cpl_column_get_data_cplsize(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (column->nullcount == 0) {
                while (length--)
                    *szp++ = *ip++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *szp = *ip;
                    szp++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (column->nullcount == 0) {
                while (length--)
                    *szp++ = *lp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *szp = *lp;
                    szp++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp = column->values->ll;

            if (column->nullcount == 0) {
                while (length--)
                    *szp++ = *llp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *szp = *llp;
                    szp++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--)
                    *szp++ = *fp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *szp = *fp;
                    szp++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--)
                    *szp++ = *dp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *szp = *dp;
                    szp++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null,
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_cplsize_array(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    int         *np;
    cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if ((type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_SIZE | CPL_TYPE_POINTER,
                                      length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_SIZE);
                    cpl_array_set_cplsize(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_SIZE);
                        cpl_array_set_cplsize(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_SIZE);
                    cpl_array_set_cplsize(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_SIZE);
                        cpl_array_set_cplsize(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_SIZE);
                    cpl_array_set_cplsize(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_SIZE);
                        cpl_array_set_cplsize(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_SIZE);
                    cpl_array_set_cplsize(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_SIZE);
                        cpl_array_set_cplsize(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_SIZE);
                    cpl_array_set_cplsize(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_SIZE);
                        cpl_array_set_cplsize(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_SIZE);
                    cpl_array_set_cplsize(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_SIZE);
                        cpl_array_set_cplsize(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}


cpl_column *cpl_column_cast_to_cplsize_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_cplsize(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    while (length--) {
        array--;
        if (*array) {
            if (cpl_array_is_valid(*array, 0)) {
                cpl_column_set_cplsize(new_column, length,
                                       cpl_array_get(*array, 0, NULL));
            }
        }
    }

    return new_column;

}


/*
 * @brief
 *   Cast a numeric column to a new @em float column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *
 * If the accessed column is of type either string or array of strings,
 * a @c CPL_ERROR_INVALID_TYPE is set. If the input is a @c NULL 
 * pointer, a @c CPL_ERROR_NULL_INPUT is set. The output column is 
 * nameless, and with the default format for its type (see function
 * @c cpl_column_set_format() ). However, if the input column is
 * also @em float, a simple copy is made and the column format
 * is inherited. The input column units are always preserved in the
 * output column. Note that a column of arrays is always cast to
 * another column of @em float arrays. It is not allowed to cast
 * a column of arrays into a column of plain floats.
 */

cpl_column *cpl_column_cast_to_float(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    const cpl_column *acolumn  = NULL;
    cpl_column     *fcolumn    = NULL;
    int            *np;
    float          *fp;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if ((type & CPL_TYPE_STRING) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_FLOAT)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_FLOAT | CPL_TYPE_POINTER,
                                          length, cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else {
        new_column = cpl_column_new_float(length);
    }

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;
    
    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while(length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column_const(array[length]);
                fcolumn = cpl_column_cast_to_float(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(fcolumn),
                                                  cpl_column_get_type(fcolumn));
                cpl_array_set_column(new_array[length], fcolumn);
            }
        }

        return new_column;

    }

    fp  = cpl_column_get_data_float(new_column);
    np  = column->null;

    switch (type) {
    case CPL_TYPE_INT:
    {
        int *ip  = column->values->i;

        if (column->nullcount == 0) {
            while (length--)
                *fp++ = *ip++;
        }
        else {
            while (length--) {
                if (*np++ == 0)
                    *fp = *ip;
                fp++;
                ip++;
            }
        }
        break;
    }

    case CPL_TYPE_LONG:
    {
        long *lp = column->values->l;

        if (column->nullcount == 0) {
            while (length--)
                *fp++ = *lp++;
        }
        else {
            while (length--) {
                if (*np++ == 0)
                    *fp = *lp;
                fp++;
                lp++;
            }
        }
        break;
    }

    case CPL_TYPE_LONG_LONG:
    {
        long long *llp = column->values->ll;

        if (column->nullcount == 0) {
            while (length--)
                *fp++ = *llp++;
        }
        else {
            while (length--) {
                if (*np++ == 0)
                    *fp = *llp;
                fp++;
                llp++;
            }
        }
        break;
    }

    case CPL_TYPE_SIZE:
    {
        cpl_size *szp = column->values->sz;

        if (column->nullcount == 0) {
            while (length--)
                *fp++ = *szp++;
        }
        else {
            while (length--) {
                if (*np++ == 0)
                    *fp = *szp;
                fp++;
                szp++;
            }
        }
        break;
    }

    case CPL_TYPE_DOUBLE:
    {
        double *dp  = column->values->d;

        if (column->nullcount == 0) {
            while (length--)
                *fp++ = *dp++;
        }
        else {
            while (length--) {
                if (*np++ == 0)
                    *fp = *dp;
                fp++;
                dp++;
            }
        }
        break;
    }

    default:
        cpl_error_set_(CPL_ERROR_UNSPECIFIED);
        cpl_column_delete(new_column);
        return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null, 
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_float_array(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    int            *np;
    cpl_array     **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if ((type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_FLOAT | CPL_TYPE_POINTER, 
                                      length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np  = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip  = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                    cpl_array_set_float(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                        cpl_array_set_float(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp  = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                    cpl_array_set_float(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                        cpl_array_set_float(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp  = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                    cpl_array_set_float(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                        cpl_array_set_float(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp  = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                    cpl_array_set_float(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                        cpl_array_set_float(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp  = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                    cpl_array_set_float(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                        cpl_array_set_float(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp  = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                    cpl_array_set_float(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT);
                        cpl_array_set_float(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_float_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_float(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    while (length--) {
        array--;
        if (array && *array) {
            if (cpl_array_is_valid(*array, 0)) {
                cpl_column_set_float(new_column, length,
                                     cpl_array_get(*array, 0, NULL));
            }
        }
    }

    return new_column;

}


/* 
 * @brief
 *   Cast a numeric column to a new @em double column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *  
 * If the accessed column is of type string, a @c CPL_ERROR_INVALID_TYPE
 * is set. If the input is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT
 * is set. The output column is nameless, and with the default format for
 * its type (see function @c cpl_column_set_format() ). However, if the
 * input column is also @em double, a simple copy is made and the column
 * format is inherited. The input column units are always preserved in
 * the output column. Note that a column of arrays is always cast to
 * another column of @em double arrays. It is not allowed to cast
 * a column of arrays into a column of plain doubles.
 */

cpl_column *cpl_column_cast_to_double(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    const cpl_column*acolumn   = NULL;
    cpl_column     *dcolumn    = NULL;
    int            *np;
    double         *dp;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if ((type & CPL_TYPE_STRING) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_DOUBLE)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_DOUBLE | CPL_TYPE_POINTER,
                                          length, cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else {
        new_column = cpl_column_new_double(length);
    }

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while(length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column_const(array[length]);
                dcolumn = cpl_column_cast_to_double(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(dcolumn),
                                                  cpl_column_get_type(dcolumn));
                cpl_array_set_column(new_array[length], dcolumn);
            }
        }

        return new_column;

    }

    dp  = cpl_column_get_data_double(new_column);
    np  = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip  = column->values->i;

            if (column->nullcount == 0) {
                while (length--)
                    *dp++ = *ip++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *dp = *ip;
                    dp++;
                    ip++;
                }
            }
            break;
        }

        case CPL_TYPE_LONG:
        {
            long *lp  = column->values->l;

            if (column->nullcount == 0) {
                while (length--)
                    *dp++ = *lp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *dp = *lp;
                    dp++;
                    lp++;
                }
            }
            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *llp  = column->values->ll;

            if (column->nullcount == 0) {
                while (length--)
                    *dp++ = *llp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *dp = *llp;
                    dp++;
                    llp++;
                }
            }
            break;
        }

        case CPL_TYPE_SIZE:
        {
            cpl_size *szp  = column->values->sz;

            if (column->nullcount == 0) {
                while (length--)
                    *dp++ = *szp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *dp = *szp;
                    dp++;
                    szp++;
                }
            }
            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fp  = column->values->f;

            if (column->nullcount == 0) {
                while (length--)
                    *dp++ = *fp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *dp = *fp;
                    dp++;
                    fp++;
                }
            }
            break;
        }

        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null, 
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_double_array(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    int            *np;
    cpl_array     **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if ((type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_DOUBLE | CPL_TYPE_POINTER, 
                                      length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np  = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip  = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                    cpl_array_set_double(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                        cpl_array_set_double(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp  = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                    cpl_array_set_double(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                        cpl_array_set_double(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp  = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                    cpl_array_set_double(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                        cpl_array_set_double(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp  = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                    cpl_array_set_double(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                        cpl_array_set_double(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp  = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                    cpl_array_set_double(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                        cpl_array_set_double(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp  = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                    cpl_array_set_double(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE);
                        cpl_array_set_double(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_double_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) || (type & CPL_TYPE_COMPLEX)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_double(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    while (length--) {
        array--;
        if (array && *array) {
            if (cpl_array_is_valid(*array, 0)) {
                cpl_column_set_double(new_column, length,
                                      cpl_array_get(*array, 0, NULL));
            }
        }
    }

    return new_column;
    
}


/*
 * @brief
 *   Cast a complex column to a new @em float complex column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *
 * If the accessed column is of type either string or array of strings,
 * a @c CPL_ERROR_INVALID_TYPE is set. If the input is a @c NULL 
 * pointer, a @c CPL_ERROR_NULL_INPUT is set. The output column is 
 * nameless, and with the default format for its type (see function
 * @c cpl_column_set_format() ). However, if the input column is
 * also @em float complex, a simple copy is made and the column format
 * is inherited. The input column units are always preserved in the
 * output column. Note that a column of arrays is always cast to
 * another column of @em float complex arrays. It is not allowed to cast
 * a column of arrays into a column of plain floats.
 */

cpl_column *cpl_column_cast_to_float_complex(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    const cpl_column*acolumn   = NULL;
    cpl_column     *icolumn    = NULL;
    int            *np;
    float complex  *cfp;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_FLOAT) &&
            (type & CPL_TYPE_COMPLEX)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_FLOAT_COMPLEX | 
                                          CPL_TYPE_POINTER, length, 
                                          cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else {
        new_column = cpl_column_new_float_complex(length);
    }

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;
    
    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while(length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column_const(array[length]);
                icolumn = cpl_column_cast_to_float_complex(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(icolumn),
                                                  cpl_column_get_type(icolumn));
                cpl_array_set_column(new_array[length], icolumn);
            }
        }

        return new_column;

    }

    cfp = cpl_column_get_data_float_complex(new_column);
    np  = column->null;

    switch (type) {

        case CPL_TYPE_INT:
        {
            int *ip  = column->values->i;

            if (column->nullcount == 0) {
                while (length--)
                    *cfp++ = *ip++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cfp = *ip;
                    cfp++;
                    ip++;
                }
            }
            break;
        }

        case CPL_TYPE_LONG:
        {
            long *lp  = column->values->l;

            if (column->nullcount == 0) {
                while (length--)
                    *cfp++ = *lp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cfp = *lp;
                    cfp++;
                    lp++;
                }
            }
            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *llp  = column->values->ll;

            if (column->nullcount == 0) {
                while (length--)
                    *cfp++ = *llp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cfp = *llp;
                    cfp++;
                    llp++;
                }
            }
            break;
        }

        case CPL_TYPE_SIZE:
        {
            cpl_size *szp  = column->values->sz;

            if (column->nullcount == 0) {
                while (length--)
                    *cfp++ = *szp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cfp = *szp;
                    cfp++;
                    szp++;
                }
            }
            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fp  = column->values->f;

            if (column->nullcount == 0) {
                while (length--)
                    *cfp++ = *fp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cfp = *fp;
                    cfp++;
                    fp++;
                }
            }
            break;
        }

        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex *cdp = column->values->cd;

            if (column->nullcount == 0) {
                while (length--)
                    *cfp++ = *cdp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cfp = *cdp;
                    cfp++;
                    cdp++;
                }
            }
            break;
        }

        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null, 
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_float_complex_array(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    int            *np;
    cpl_array     **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (type & CPL_TYPE_POINTER) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_FLOAT_COMPLEX |
                                      CPL_TYPE_POINTER, length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np    = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip    = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp    = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp    = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp    = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp    = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp    = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex *cfp   = column->values->cf;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *cfp);
                    array++;
                    cfp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *cfp);
                    }
                    array++;
                    cfp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex *cdp   = column->values->cd;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                    cpl_array_set_float_complex(*array, 0, *cdp);
                    array++;
                    cdp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_FLOAT_COMPLEX);
                        cpl_array_set_float_complex(*array, 0, *cdp);
                    }
                    array++;
                    cdp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_float_complex_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_float_complex(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    if (type & CPL_TYPE_COMPLEX) {
        while (length--) {
            array--;
            if (array && *array) {
                if (cpl_array_is_valid(*array, 0)) {
                    cpl_column_set_float_complex(new_column, length,
                                   cpl_array_get_complex(*array, 0, NULL));
                }
            }
        }
     }
     else {
        while (length--) {
            array--;
            if (array && *array) {
                if (cpl_array_is_valid(*array, 0)) {
                    cpl_column_set_float_complex(new_column, length,
                                   cpl_array_get(*array, 0, NULL));
                }
            }
        }
    }

    return new_column;

}


/* 
 * @brief
 *   Cast a numeric column to a new @em double column.
 *
 * @param column  Column to be cast.
 *
 * @return Pointer to the new column, or @em NULL in case of error.
 *  
 * If the accessed column is of type string, a @c CPL_ERROR_INVALID_TYPE
 * is set. If the input is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT
 * is set. The output column is nameless, and with the default format for
 * its type (see function @c cpl_column_set_format() ). However, if the
 * input column is also @em double, a simple copy is made and the column
 * format is inherited. The input column units are always preserved in
 * the output column. Note that a column of arrays is always cast to
 * another column of @em double arrays. It is not allowed to cast
 * a column of arrays into a column of plain doubles.
 */

cpl_column *cpl_column_cast_to_double_complex(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    const cpl_column*acolumn   = NULL;
    cpl_column     *icolumn    = NULL;
    int            *np;
    double complex *cdp;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER) && (type & CPL_TYPE_DOUBLE) &&
            (type & CPL_TYPE_COMPLEX)) {
        new_column = cpl_column_duplicate(column);
        cpl_column_set_name(new_column, NULL);
        return new_column;
    }

    if (type & CPL_TYPE_POINTER) {
        new_column = cpl_column_new_array(CPL_TYPE_DOUBLE_COMPLEX | 
                                          CPL_TYPE_POINTER, length, 
                                          cpl_column_get_depth(column));
        if (column->dimensions)
            new_column->dimensions = cpl_array_duplicate(column->dimensions);
    }
    else {
        new_column = cpl_column_new_double_complex(length);
    }

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    if (column->nullcount == length)   /* No need to copy just NULLs */
        return new_column;

    if (type & CPL_TYPE_POINTER) {

        const cpl_array **array = cpl_column_get_data_array_const(column);
        cpl_array **new_array = cpl_column_get_data_array(new_column);

        while(length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column_const(array[length]);
                icolumn = cpl_column_cast_to_double_complex(acolumn);
                new_array[length] = cpl_array_new(cpl_column_get_size(icolumn),
                                                  cpl_column_get_type(icolumn));
                cpl_array_set_column(new_array[length], icolumn);
            }
        }

        return new_column;

    }

    cdp  = cpl_column_get_data_double_complex(new_column);
    np  = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip  = column->values->i;

            if (column->nullcount == 0) {
                while (length--)
                    *cdp++ = *ip++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cdp = *ip;
                    cdp++;
                    ip++;
                }
            }
            break;
        }

        case CPL_TYPE_LONG:
        {
            long *lp  = column->values->l;

            if (column->nullcount == 0) {
                while (length--)
                    *cdp++ = *lp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cdp = *lp;
                    cdp++;
                    lp++;
                }
            }
            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *llp  = column->values->ll;

            if (column->nullcount == 0) {
                while (length--)
                    *cdp++ = *llp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cdp = *llp;
                    cdp++;
                    llp++;
                }
            }
            break;
        }

        case CPL_TYPE_SIZE:
        {
            cpl_size *szp  = column->values->sz;

            if (column->nullcount == 0) {
                while (length--)
                    *cdp++ = *szp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cdp = *szp;
                    cdp++;
                    szp++;
                }
            }
            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fp  = column->values->f;

            if (column->nullcount == 0) {
                while (length--)
                    *cdp++ = *fp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cdp = *fp;
                    cdp++;
                    fp++;
                }
            }
            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *dp  = column->values->d;

            if (column->nullcount == 0) {
                while (length--)
                    *cdp++ = *dp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cdp = *dp;
                    cdp++;
                    dp++;
                }
            }
            break;
        }

        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex *cfp  = column->values->cf;

            if (column->nullcount == 0) {
                while (length--)
                    *cdp++ = *cfp++;
            }
            else {
                while (length--) {
                    if (*np++ == 0)
                        *cdp = *cfp;
                    cdp++;
                    cfp++;
                }
            }
            break;
        }

        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    length = cpl_column_get_size(column);

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null, 
               length * sizeof(cpl_column_flag));
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_double_complex_array(const cpl_column *column)
{

    cpl_type        type       = cpl_column_get_type(column);
    cpl_size        length     = cpl_column_get_size(column);
    cpl_column     *new_column = NULL;
    int            *np;
    cpl_array     **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (type & CPL_TYPE_POINTER) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_array(CPL_TYPE_DOUBLE_COMPLEX | 
                                      CPL_TYPE_POINTER, length, 1);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array(new_column);
    np  = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip  = column->values->i;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *ip);
                    array++;
                    ip++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *ip);
                    }
                    array++;
                    ip++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp  = column->values->l;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *lp);
                    array++;
                    lp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *lp);
                    }
                    array++;
                    lp++;
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *llp  = column->values->ll;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *llp);
                    array++;
                    llp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *llp);
                    }
                    array++;
                    llp++;
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *szp  = column->values->sz;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *szp);
                    array++;
                    szp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *szp);
                    }
                    array++;
                    szp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp  = column->values->f;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *fp);
                    array++;
                    fp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *fp);
                    }
                    array++;
                    fp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp  = column->values->d;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *dp);
                    array++;
                    dp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *dp);
                    }
                    array++;
                    dp++;
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex *cfp = column->values->cf;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *cfp);
                    array++;
                    cfp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *cfp);
                    }
                    array++;
                    cfp++;
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex *cdp = column->values->cd;

            if (column->nullcount == 0) {
                while (length--) {
                    *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                    cpl_array_set_double_complex(*array, 0, *cdp);
                    array++;
                    cdp++;
                }
            }
            else {
                while (length--) {
                    if (*np++ == 0) {
                        *array = cpl_array_new(1, CPL_TYPE_DOUBLE_COMPLEX);
                        cpl_array_set_double_complex(*array, 0, *cdp);
                    }
                    array++;
                    cdp++;
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            cpl_column_delete(new_column);
            return NULL;
    }

    return new_column;

}

cpl_column *cpl_column_cast_to_double_complex_flat(const cpl_column *column)
{

    cpl_type     type       = cpl_column_get_type(column);
    cpl_size     length     = cpl_column_get_size(column);
    cpl_column  *new_column = NULL;
    const cpl_array  **array;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (type & CPL_TYPE_STRING) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    if (!(type & CPL_TYPE_POINTER)) {
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return NULL;
    }

    new_column = cpl_column_new_double(length);

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));

    if (length == 0)                   /* No need to cast a 0 length column */
        return new_column;

    array = cpl_column_get_data_array_const(column) + length;

    if (type & CPL_TYPE_COMPLEX) {
        while (length--) {
            array--;
            if (array && *array) {
                if (cpl_array_is_valid(*array, 0)) {
                    cpl_column_set_double_complex(new_column, length,
                            cpl_array_get_complex(*array, 0, NULL));
                }
            }
        }
    }
    else {
        while (length--) {
            array--;
            if (array && *array) {
                if (cpl_array_is_valid(*array, 0)) {
                    cpl_column_set_double_complex(new_column, length,
                            cpl_array_get(*array, 0, NULL));
                }
            }
        }
    }

    return new_column;
    
}


/* 
 * @brief
 *   Create a column from an interval of another column.
 *
 * @param column  Input column.
 * @param start   First element to be copied to new column.
 * @param count   Number of elements to be copied.
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * If the input column is @c NULL, @c CPL_ERROR_NULL_INPUT is set. If 
 * @em start is outside the column range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 * is set. If the input column has length zero, this error is always 
 * set. If @em count is negative, a @c CPL_ERROR_ILLEGAL_INPUT is set. 
 * If @em count is zero, a zero-length column is created. 
 * If @em start + @em count goes beyond the end of the input column, 
 * elements are always copied up to the end. The new column is nameless, 
 * but it inherits the unit and the format of the source column. Existing 
 * invalid element flags are also transferred to the new column.
 */

cpl_column *cpl_column_extract(cpl_column *column, 
                               cpl_size start, cpl_size count)
{
    cpl_column *new_column = NULL;
    cpl_type    type       = cpl_column_get_type(column);
    cpl_size    length     = cpl_column_get_size(column);
    cpl_size    depth      = cpl_column_get_depth(column);
    size_t      byte_count;
    cpl_size    i;
    cpl_size    j;


    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    if (start < 0 || start >= length) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return NULL;
    }

    if (count < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    if (count > length - start)
        count = length - start;

    byte_count  = (size_t)count * cpl_column_type_size(type);

    if (type & CPL_TYPE_POINTER) {
        switch (type & ~CPL_TYPE_POINTER) {
        case CPL_TYPE_INT:
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG:
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
        case CPL_TYPE_STRING:

            new_column = cpl_column_new_array(type, count, depth);
            for (i = 0, j = start; i < count; i++, j++) {
                if (column->values->array[j])
                    new_column->values->array[i] =
                        cpl_array_duplicate(column->values->array[j]);
                else
                    new_column->values->array[i] = NULL;
            }

            if (column->dimensions)
                new_column->dimensions = cpl_array_duplicate(column->dimensions);

            break;

        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            return NULL;
        }
    } else {
        switch (type) {

        case CPL_TYPE_INT:
            new_column = cpl_column_new_int(count);
            if (count)
                memcpy(new_column->values->i,
                       column->values->i + start, byte_count);
            break;

        case CPL_TYPE_LONG:
            new_column = cpl_column_new_long(count);
            if (count)
                memcpy(new_column->values->l,
                       column->values->l + start, byte_count);
            break;

        case CPL_TYPE_LONG_LONG:
            new_column = cpl_column_new_long_long(count);
            if (count)
                memcpy(new_column->values->ll,
                       column->values->ll + start, byte_count);
            break;

        case CPL_TYPE_SIZE:
            new_column = cpl_column_new_cplsize(count);
            if (count)
                memcpy(new_column->values->sz,
                       column->values->sz + start, byte_count);
            break;

        case CPL_TYPE_FLOAT:
            new_column = cpl_column_new_float(count);
            if (count)
                memcpy(new_column->values->f,
                       column->values->f + start, byte_count);
            break;

        case CPL_TYPE_DOUBLE:
            new_column = cpl_column_new_double(count);
            if (count)
                memcpy(new_column->values->d,
                       column->values->d + start, byte_count);
            break;

        case CPL_TYPE_FLOAT_COMPLEX:
            new_column = cpl_column_new_float_complex(count);
            if (count)
                memcpy(new_column->values->cf,
                       column->values->cf + start, byte_count);
            break;

        case CPL_TYPE_DOUBLE_COMPLEX:
            new_column = cpl_column_new_double_complex(count);
            if (count)
                memcpy(new_column->values->cd,
                       column->values->cd + start, byte_count);
            break;

        case CPL_TYPE_STRING:
            new_column = cpl_column_new_string(count);
            for (i = 0, j = start; i < count; i++, j++)
                if (column->values->s[j])
                    new_column->values->s[i] = cpl_strdup(column->values->s[j]);
                else
                    new_column->values->s[i] = NULL;
            break;

        default:
            cpl_error_set_(CPL_ERROR_UNSPECIFIED);
            return NULL;
        }
    }

    cpl_column_set_unit(new_column, cpl_column_get_unit(column));
    cpl_column_set_format(new_column, cpl_column_get_format(column));

    if (column->null) {

        /*
         *  If an invalid flags buffer exists in the original column,
         *  we have to take care of the (possible) duplication of
         *  the appropriate segment.
         */

        new_column->nullcount = 0;
        for (i = 0, j = start; i < count; i++, j++)
            if (column->null[j])
                new_column->nullcount++;

        if (new_column->nullcount != 0 && 
            new_column->nullcount != count) {
            new_column->null = cpl_calloc(count, sizeof(cpl_column_flag));
            for (i = 0, j = start; i < count; i++, j++)
                if (column->null[j])
                    new_column->null[i] = 1;
        }

    }
    else {

      /* 
       *  An invalid flag buffer doesn't exist in the original column,
       *  and this means no invalid elements, or all elements are invalid, 
       *  also for the subcolumn.
       */

      if (column->nullcount == 0)
          new_column->nullcount = 0;
      else
          new_column->nullcount = cpl_column_get_size(new_column);
          
    }

    return new_column;

}

  
/*
 * @brief
 *   Insert a column into another column of the same type.
 *
 * @param target_column Target column.
 * @param insert_column Column to be inserted in target column.
 * @param position      Position of target where to insert.
 *
 * @return @c CPL_ERROR_NONE on success. If any input column is a @c NULL 
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the columns are 
 *   not of the same type, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   If @em position is negative, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE 
 *   is returned.
 *
 * The first element of the column to be inserted will be at the 
 * specified @em position. If the specified @em position is not 
 * less than the target column length, the second column will be 
 * appended to the target column. The output column is expanded 
 * to host the values copied from the input column, that is left 
 * untouched. In the @em target column the pointer to target data 
 * may change, therefore pointers previously retrieved by calling 
 * @c cpl_column_get_data_int(), @c cpl_column_get_data_string(), 
 * etc., should be discarded). Information about columns dimensions 
 * (in case of array columns) is ignored.
 */

cpl_error_code cpl_column_merge(cpl_column *target_column, 
                                const cpl_column *insert_column, cpl_size position)
{

    cpl_size    t_length = cpl_column_get_size(target_column);
    cpl_size    i_length = cpl_column_get_size(insert_column);
    cpl_type    t_type   = cpl_column_get_type(target_column);
    cpl_type    i_type   = cpl_column_get_type(insert_column);
    size_t      i_size   = (size_t)i_length * cpl_column_type_size(i_type);
    cpl_size    i;
    cpl_size    j;


    if (target_column == NULL || insert_column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (position < 0) 
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (t_type != i_type)
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (i_length == 0)
        return CPL_ERROR_NONE;

    if (position > t_length)
        position = t_length;


    /*
     *  First of all, a segment of invalid elements is inserted at the
     *  right position.
     */

    cpl_column_insert_segment(target_column, position, i_length);


    /*
     *  If the column to be inserted just consists of invalid elements, 
     *  the job is already completed.
     */

    if (cpl_column_count_invalid_const(insert_column) == i_length)
        return CPL_ERROR_NONE;


    /*
     *  Transferring the data from the column to be inserted,
     *  to the expanded part of the target column:
     */

    t_length = cpl_column_get_size(target_column);     /* It got longer */

    if (i_type & CPL_TYPE_POINTER) {
        switch (i_type & ~CPL_TYPE_POINTER) {
        case CPL_TYPE_INT:
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG:
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
        case CPL_TYPE_STRING:

            for (i = 0, j = position; i < i_length; i++, j++)
                if (insert_column->values->array[i])
                    target_column->values->array[j] =
                        cpl_array_duplicate(insert_column->values->array[i]);
            break;

        default:
            return cpl_error_set_(CPL_ERROR_UNSPECIFIED);

        }
    } else {

        switch (i_type) {

        case CPL_TYPE_INT:
            memcpy(target_column->values->i + position,
                   insert_column->values->i, i_size);
            break;

        case CPL_TYPE_LONG:
            memcpy(target_column->values->l + position,
                   insert_column->values->l, i_size);
            break;

        case CPL_TYPE_LONG_LONG:
            memcpy(target_column->values->ll + position,
                   insert_column->values->ll, i_size);
            break;

        case CPL_TYPE_SIZE:
            memcpy(target_column->values->sz + position,
                   insert_column->values->sz, i_size);
            break;

        case CPL_TYPE_FLOAT:
            memcpy(target_column->values->f + position,
                   insert_column->values->f, i_size);
            break;

        case CPL_TYPE_DOUBLE:
            memcpy(target_column->values->d + position,
                   insert_column->values->d, i_size);
            break;

        case CPL_TYPE_FLOAT_COMPLEX:
            memcpy(target_column->values->cf + position,
                   insert_column->values->cf, i_size);
            break;

        case CPL_TYPE_DOUBLE_COMPLEX:
            memcpy(target_column->values->cd + position,
                   insert_column->values->cd, i_size);
            break;

        case CPL_TYPE_STRING:
            for (i = 0, j = position; i < i_length; i++, j++)
                if (insert_column->values->s[i])
                    target_column->values->s[j] =
                        cpl_strdup(insert_column->values->s[i]);
            break;

        default:
            return cpl_error_set_(CPL_ERROR_UNSPECIFIED);

        }
    }

    if ((i_type == CPL_TYPE_STRING) || (i_type & CPL_TYPE_POINTER))
        return CPL_ERROR_NONE;


    /*
     *  Now also the null flags buffer must be upgraded.
     */

    if (target_column->null) {

        if (insert_column->null) {

            /*
             *  Both null flags buffers exist. The expanded part of
             *  the target column already consists of invalid elements, 
             *  so just overwrite valid elements from the first column:
             */

            for (i = 0, j = position; i < i_length; i++, j++) {
                if (!insert_column->null[i]) {
                    target_column->null[j] = 0;
                    target_column->nullcount--;
                } 
            } 
        }
        else {


            /*
             *  Just the target column has the invalid flag buffer. This
             *  means necessarily that the column to insert has no invalid
             *  elements (the other possibility, that the column to insert
             *  just consists of invalid elements, was already handled at
             *  the beginning). Then we must turn all the invalid elements
             *  of the target expanded interval of the null flag buffer 
             *  into valid.
             */

            if (target_column->nullcount > i_length) {


                /*
                 *  Surely target_column->nullcount is at least i_length,
                 *  i.e., the number of invalid elements added by the
                 *  expansion. If target_column->nullcount is equal to 
                 *  i_length, then these are the only invalid elements 
                 *  existing, and they would be all unset, therefore it 
                 *  would be enough to free the null flags buffer skipping 
                 *  this loop.
                 */

                for (i = 0, j = position; i < i_length; i++, j++)
                    target_column->null[j] = 0;
            }

            target_column->nullcount -= i_length;

        }
    }
    else {

        /*
         *  The target column invalid flags buffer doesn't exist. 
         *  Excluding the case of array or string columns, this is
         *  necessarily because it just contains invalid elements, 
         *  since to expand it means just to add extra invalid elements. 
         *  The inserted column instead has some valid elements, that 
         *  must be transferred - therefore the target column invalid 
         *  flag buffer must be allocated.
         */

        target_column->null = cpl_malloc(t_length * sizeof(cpl_column_flag));

        i = t_length;
        while (i--)
            target_column->null[i] = 1;

        if (insert_column->null) {

           /*
            *  Some invalid elements:
            */

            for (i = 0, j = position; i < i_length; i++, j++) {
                if (!insert_column->null[i]) {
                    target_column->null[j] = 0;
                    target_column->nullcount--;
                }
            }

        }
        else {

            /*
             *  No invalid elements:
             */

            for (i = 0, j = position; i < i_length; i++, j++)
                target_column->null[j] = 0;

            target_column->nullcount -= i_length;
        }
    }


    /*
     *  If no invalid elements are left, free the invalid flags buffer
     */

    if (target_column->nullcount == 0) {
        if (target_column->null)
            cpl_free(target_column->null);
        target_column->null = NULL;
    }

    return 0;

}


/*
 * @brief
 *   Add two numerical or complex columns.
 *
 * @param to_column   Target column.
 * @param from_column Column to be summed to target column.
 * 
 * @return @c CPL_ERROR_NONE on success. If any of the input columns
 *   is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If 
 *   any of the input columns is of type string, a @c CPL_ERROR_INVALID_TYPE
 *   is returned. If the input columns do not have the same length, 
 *   a @c CPL_ERROR_INCOMPATIBLE_INPUT is returned. 
 *
 * The result of the sum is stored in the target column. The type of the
 * columns may differ, and in that case the operation would be performed 
 * according to the C upcasting rules, with a final cast of the result to
 * the target column type. Consistently, if at least one column is complex 
 * the math is complex, and if the target column is not complex then only
 * the real part of the result will be obtained. 
 * Invalid elements are propagated consistently: if either or both members 
 * of the sum are invalid, the result will be invalid too. Underflows and 
 * overflows are ignored. This function is not applicable to array columns.
 */

cpl_error_code cpl_column_add(cpl_column *to_column, cpl_column *from_column)
{

    cpl_size        i;
    cpl_size        to_length   = cpl_column_get_size(to_column);
    cpl_size        from_length = cpl_column_get_size(from_column);
    cpl_type        to_type     = cpl_column_get_type(to_column);
    cpl_type        from_type   = cpl_column_get_type(from_column);
    int            *tnp;


    if (to_column == NULL || from_column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if ((to_type == CPL_TYPE_STRING) || (from_type == CPL_TYPE_STRING) ||
        (to_type & CPL_TYPE_POINTER) || (from_type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    if (to_length != from_length)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    if (to_length == 0)
        return CPL_ERROR_NONE;

    if (to_column->nullcount == to_length)
        return CPL_ERROR_NONE;

    if (from_column->nullcount == from_length)
        return cpl_column_fill_invalid(to_column, 0, to_length);


    if (to_column->nullcount == 0 && from_column->nullcount == 0) {

        /*
         * If there are no invalid elements in both columns, the operation 
         * can be performed without any check:
         */

        switch (to_type) {
            case CPL_TYPE_INT:
            {
                int *tip  = to_column->values->i;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tip++ += *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tip++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tip++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tip++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tip++ += *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tip++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tip[to_length] = tip[to_length] + fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tip[to_length] = tip[to_length] + fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *tlp  = to_column->values->l;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ += *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ += *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] + fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] + fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *tlp  = to_column->values->ll;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ += *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ += *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] + fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] + fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *tlp  = to_column->values->sz;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ += *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ += *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] + fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] + fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *tfp  = to_column->values->f;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tfp++ += (double)*fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tfp++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tfp++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tfp++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tfp++ += (double)*ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tfp++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] + fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] + fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *tdp  = to_column->values->d;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tdp++ += *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tdp++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tdp++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tdp++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tdp++ += *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tdp++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] + fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] + fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *tcfp = to_column->values->cf;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcfp++ += *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcfp++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tcfp++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tcfp++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tcfp++ += *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tcfp++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            *tcfp++ += *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            *tcfp++ += *fcdp++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *tcdp = to_column->values->cd;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcdp++ += *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcdp++ += *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tcdp++ += *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tcdp++ += *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tcdp++ += *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tcdp++ += *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            *tcdp++ += *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            *tcdp++ += *fcdp++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
    else {

        /*
         * Here there are some invalid elements, so the operation needs to be
         * performed with some checks. As a first thing, take care of the
         * invalid elements in the target column:
         */

        if (to_column->nullcount == 0 || from_column->nullcount == 0) {

            /*
             * Here there are some invalid elements in just one of the two 
             * columns. If they are just in the column to add, an identical 
             * invalid flags buffer must be created for the target column.
             */

            if (from_column->nullcount) {
                to_column->null = cpl_calloc(to_length, 
                                             sizeof(cpl_column_flag));

                i = to_length;
                while (i--)
                    if (from_column->null[i])
                        to_column->null[i] = 1;

                to_column->nullcount = from_column->nullcount;

            }
        }
        else {

            /*
             *  Here there are some invalid elements in both columns.
             */

            i = to_length;
            while (i--) {
                if (from_column->null[i]) {
                    if (!to_column->null[i]) {
                        to_column->null[i] = 1;
                        to_column->nullcount++;
                    }
                }
            }

            if (to_column->nullcount == to_length) {

                /*
                 * Just invalid elements in the result: no need to perform 
                 * any further operation, and the invalid flag buffer of the 
                 * target column can be disabled.
                 */

                if (to_column->null)
                    cpl_free(to_column->null);
                to_column->null = NULL;
                return 0;
            }

        }

        /*
         * The operation can be performed while checking the already
         * computed result invalid flags buffer of the target column.
         */

        tnp = to_column->null;

        switch (to_type) {
            case CPL_TYPE_INT:
            {
                int *tip  = to_column->values->i;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip += *fip;
                            tip++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip += *flp;
                            tip++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip += *fllp;
                            tip++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip += *fszp;
                            tip++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip += *ffp;
                            tip++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip += *fdp;
                            tip++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip = *tip + *fcfp; /* Support clang */
                            tip++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip = *tip + *fcdp; /* Support clang */
                            tip++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *tlp  = to_column->values->l;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fip;
                            tlp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fllp;
                            tlp++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fszp;
                            tlp++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *ffp;
                            tlp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fdp;
                            tlp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp + *fcfp; /* Support clang */
                            tlp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp + *fcdp; /* Support clang */
                            tlp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *tlp  = to_column->values->ll;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fip;
                            tlp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fllp;
                            tlp++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fszp;
                            tlp++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *ffp;
                            tlp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fdp;
                            tlp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp + *fcfp; /* Support clang */
                            tlp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp + *fcdp; /* Support clang */
                            tlp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *tlp  = to_column->values->sz;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fip;
                            tlp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fllp;
                            tlp++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fszp;
                            tlp++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *ffp;
                            tlp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp += *fdp;
                            tlp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp + *fcfp; /* Support clang */
                            tlp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp + *fcdp; /* Support clang */
                            tlp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *tfp  = to_column->values->f;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp += (double)*fip;
                            tfp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp += *flp;
                            tfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp += *fllp;
                            tfp++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp += *fszp;
                            tfp++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp += (double)*ffp;
                            tfp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp += *fdp;
                            tfp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp = *tfp + *fcfp; /* Support clang */
                            tfp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp = *tfp + *fcdp; /* Support clang */
                            tfp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *tdp  = to_column->values->d;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp += *fip;
                            tdp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp += *flp;
                            tdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp += *fllp;
                            tdp++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp += *fszp;
                            tdp++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp += *ffp;
                            tdp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp += *fdp;
                            tdp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp = *tdp + *fcfp; /* Support clang */
                            tdp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp = *tdp + *fcdp; /* Support clang */
                            tdp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *tcfp = to_column->values->cf;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *fip;
                            tcfp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *flp;
                            tcfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *fllp;
                            tcfp++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *fszp;
                            tcfp++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *ffp;
                            tcfp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *fdp;
                            tcfp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *fcfp;
                            tcfp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp += *fcdp;
                            tcfp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *tcdp = to_column->values->cd;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *fip;
                            tcdp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *flp;
                            tcdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *fllp;
                            tcdp++;
                            fllp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *fszp;
                            tcdp++;
                            fszp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *ffp;
                            tcdp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *fdp;
                            tcdp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *fcfp;
                            tcdp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp += *fcdp;
                            tcdp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Subtract two numeric columns.
 *
 * @param to_column   Target column.
 * @param from_column Column to subtract from target column.
 *
 * @return @c CPL_ERROR_NONE on success. If any of the input columns
 *   is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If
 *   any of the input columns is of type string, a @c CPL_ERROR_INVALID_TYPE
 *   is returned. If the input columns do not have the same length,
 *   a @c CPL_ERROR_INCOMPATIBLE_INPUT is returned.
 *
 * The result of the subtraction is stored in the target column. The type of
 * the columns may differ, and in that case the operation would be performed 
 * according to the C upcasting rules, with a final cast of the result to
 * the target column type. Consistently, if at least one column is complex 
 * the math is complex, and if the target column is not complex then only
 * the real part of the result will be obtained. 
 * Invalid elements are propagated consistently: 
 * if either or both members of the subtraction are invalid, the result 
 * will be invalid too. Underflows and overflows are ignored. This function
 * is not applicable to array columns.
 */

cpl_error_code cpl_column_subtract(cpl_column *to_column, 
                                   cpl_column *from_column)
{

    cpl_size        i;
    cpl_size        to_length   = cpl_column_get_size(to_column);
    cpl_size        from_length = cpl_column_get_size(from_column);
    cpl_type        to_type     = cpl_column_get_type(to_column);
    cpl_type        from_type   = cpl_column_get_type(from_column);
    int            *tnp;


    if (to_column == NULL || from_column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if ((to_type == CPL_TYPE_STRING) || (from_type == CPL_TYPE_STRING) ||
        (to_type & CPL_TYPE_POINTER) || (from_type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    if (to_length != from_length)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    if (to_length == 0)
        return CPL_ERROR_NONE;

    if (to_column->nullcount == to_length)
        return CPL_ERROR_NONE;

    if (from_column->nullcount == from_length)
        return cpl_column_fill_invalid(to_column, 0, to_length);


    if (to_column->nullcount == 0 && from_column->nullcount == 0) {

        /*
         * If there are no invalid elements in both columns, the operation 
         * can be performed without any check:
         */

        switch (to_type) {
            case CPL_TYPE_INT:
            {

                int *tip  = to_column->values->i;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tip++ -= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tip++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tip++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tip++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tip++ -= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tip++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            tip[to_length] = tip[to_length] - fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            tip[to_length] = tip[to_length] - fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG:
            {

                long *tlp  = to_column->values->l;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ -= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tlp++ -= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tlp++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] - fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] - fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {

                long long *tlp  = to_column->values->ll;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ -= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tlp++ -= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tlp++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] - fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] - fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_SIZE:
            {

                cpl_size *tlp  = to_column->values->sz;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ -= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tlp++ -= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tlp++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] - fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] - fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {

               float *tfp  = to_column->values->f;

               switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tfp++ -= (double)*fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tfp++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tfp++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tfp++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tfp++ -= (double)*ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tfp++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] - fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] - fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {

                double *tdp  = to_column->values->d;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tdp++ -= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tdp++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tdp++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tdp++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tdp++ -= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tdp++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] - fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] - fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {

                float complex *tcfp = to_column->values->cf;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcfp++ -= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcfp++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tcfp++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tcfp++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tcfp++ -= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tcfp++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            *tcfp++ -= *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            *tcfp++ -= *fcdp++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {

                double complex *tcdp = to_column->values->cd;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcdp++ -= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcdp++ -= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *fllp  = from_column->values->ll;

                        while (to_length--)
                            *tcdp++ -= *fllp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *fszp  = from_column->values->sz;

                        while (to_length--)
                            *tcdp++ -= *fszp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp = from_column->values->f;

                        while (to_length--)
                            *tcdp++ -= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp = from_column->values->d;

                        while (to_length--)
                            *tcdp++ -= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp  = from_column->values->cf;

                        while (to_length--)
                            *tcdp++ -= *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp  = from_column->values->cd;

                        while (to_length--)
                            *tcdp++ -= *fcdp++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
    else {

        /*
         * Here there are some invalid elements, so the operation needs to be
         * performed with some checks. As a first thing, take care of the
         * invalid elements in the target column:
         */

        if (to_column->nullcount == 0 || from_column->nullcount == 0) {

            /*
             * Here there are some invalids in just one of the two columns.
             * If they are just in the column to add, an identical invalid
             * flags buffer must be created for the target column.
             */

            if (from_column->nullcount) {
                to_column->null = cpl_calloc(to_length, 
                                             sizeof(cpl_column_flag));

                i = to_length;
                while (i--)
                    if (from_column->null[i])
                        to_column->null[i] = 1;

                to_column->nullcount = from_column->nullcount;

            }
        }
        else {

            /*
             *  Here there are some invalids in both columns.
             */

            i = to_length;
            while (i--) {
                if (from_column->null[i]) {
                    if (!to_column->null[i]) {
                        to_column->null[i] = 1;
                        to_column->nullcount++;
                    }
                }
            }

            if (to_column->nullcount == to_length) {

                /*
                 * Just invalids in the result: no need to perform any further
                 * operation, and the invalid buffer of the target column can
                 * be disabled.
                 */

                if (to_column->null)
                    cpl_free(to_column->null);
                to_column->null = NULL;
                return 0;
            }

        }

        /*
         * The operation can be performed while checking the already
         * computed result invalid flags buffer of the target column.
         */

        tnp = to_column->null;

        switch (to_type) {
        case CPL_TYPE_INT:
        {
            int *tip  = to_column->values->i;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip -= *fip;
                        tip++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip -= *flp;
                        tip++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip -= *fllp;
                        tip++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip -= *fszp;
                        tip++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip -= *ffp;
                        tip++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip -= *fdp;
                        tip++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip = *tip - *fcfp; /* Support clang */
                        tip++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tip = *tip - *fcdp; /* Support clang */
                        tip++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *tlp  = to_column->values->l;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fip;
                        tlp++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *flp;
                        tlp++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fllp;
                        tlp++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fszp;
                        tlp++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *ffp;
                        tlp++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fdp;
                        tlp++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp = *tlp - *fcfp; /* Support clang */
                        tlp++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp = *tlp - *fcdp; /* Support clang */
                        tlp++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *tlp  = to_column->values->ll;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fip;
                        tlp++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *flp;
                        tlp++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fllp;
                        tlp++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fszp;
                        tlp++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *ffp;
                        tlp++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fdp;
                        tlp++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp = *tlp - *fcfp; /* Support clang */
                        tlp++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp = *tlp - *fcdp; /* Support clang */
                        tlp++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *tlp  = to_column->values->sz;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fip;
                        tlp++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *flp;
                        tlp++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fllp;
                        tlp++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fszp;
                        tlp++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *ffp;
                        tlp++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp -= *fdp;
                        tlp++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp = *tlp - *fcfp; /* Support clang */
                        tlp++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tlp = *tlp - *fcdp; /* Support clang */
                        tlp++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *tfp  = to_column->values->f;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp -= (double)*fip;
                        tfp++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp -= *flp;
                        tfp++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp -= *fllp;
                        tfp++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp -= *fszp;
                        tfp++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp -= (double)*ffp;
                        tfp++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp -= *fdp;
                        tfp++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp = *tfp - *fcfp; /* Support clang */
                        tfp++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tfp = *tfp - *fcdp; /* Support clang */
                        tfp++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {

            double *tdp  = to_column->values->d;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp -= *fip;
                        tdp++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp -= *flp;
                        tdp++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp -= *fllp;
                        tdp++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp -= *fszp;
                        tdp++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp -= *ffp;
                        tdp++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp -= *fdp;
                        tdp++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp = *tdp - *fcfp; /* Support clang */
                        tdp++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tdp = *tdp - *fcdp; /* Support clang */
                        tdp++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        {

            float complex *tcfp = to_column->values->cf;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *fip;
                        tcfp++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *flp;
                        tcfp++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *fllp;
                        tcfp++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *fszp;
                        tcfp++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *ffp;
                        tcfp++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *fdp;
                        tcfp++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *fcfp;
                        tcfp++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcfp -= *fcdp;
                        tcfp++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break; 
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {

            double complex *tcdp = to_column->values->cd;

            switch (from_type) {
                case CPL_TYPE_INT:
                {
                    int *fip  = from_column->values->i;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *fip;
                        tcdp++;
                        fip++;
                    }
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long *flp  = from_column->values->l;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *flp;
                        tcdp++;
                        flp++;
                    }
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long *fllp  = from_column->values->ll;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *fllp;
                        tcdp++;
                        fllp++;
                    }
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size *fszp  = from_column->values->sz;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *fszp;
                        tcdp++;
                        fszp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float *ffp = from_column->values->f;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *ffp;
                        tcdp++;
                        ffp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *fdp = from_column->values->d;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *fdp;
                        tcdp++;
                        fdp++;
                    }
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex *fcfp  = from_column->values->cf;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *fcfp;
                        tcdp++;
                        fcfp++;
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex *fcdp  = from_column->values->cd;

                    while (to_length--) {
                        if (*tnp++ == 0)
                            *tcdp -= *fcdp;
                        tcdp++;
                        fcdp++;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
        }
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Multiply two numeric columns.
 *
 * @param to_column   Target column.
 * @param from_column Multiplier column.
 *
 * @return @c CPL_ERROR_NONE on success. If any of the input columns
 *   is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If
 *   any of the input columns is of type string, a @c CPL_ERROR_INVALID_TYPE
 *   is returned. If the input columns do not have the same length,
 *   a @c CPL_ERROR_INCOMPATIBLE_INPUT is returned.
 *
 * The result of the multiplication is stored in the target column. The type of
 * the columns may differ, and in that case the operation would be performed
 * according to the C upcasting rules, with a final cast of the result to
 * the target column type. Consistently, if at least one column is complex 
 * the math is complex, and if the target column is not complex then only
 * the real part of the result will be obtained. 
 * Invalid elements are propagated consistently:
 * if either or both members of the multiplication are invalid, the result
 * will be invalid too. Underflows and overflows are ignored. This function
 * is not applicable to array columns.
 */

cpl_error_code cpl_column_multiply(cpl_column *to_column, 
                                   cpl_column *from_column)
{

    cpl_size        i;
    cpl_size        to_length   = cpl_column_get_size(to_column);
    cpl_size        from_length = cpl_column_get_size(from_column);
    cpl_type        to_type     = cpl_column_get_type(to_column);
    cpl_type        from_type   = cpl_column_get_type(from_column);
    int            *tnp;


    if (to_column == NULL || from_column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if ((to_type == CPL_TYPE_STRING) || (from_type == CPL_TYPE_STRING) ||
        (to_type & CPL_TYPE_POINTER) || (from_type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    if (to_length != from_length)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    if (to_length == 0)
        return CPL_ERROR_NONE;

    if (to_column->nullcount == to_length)
        return CPL_ERROR_NONE;

    if (from_column->nullcount == from_length)
        return cpl_column_fill_invalid(to_column, 0, to_length);


    if (to_column->nullcount == 0 && from_column->nullcount == 0) {

        /*
         * If there are no invalids in both columns, the operation can
         * be performed without any check:
         */

        switch (to_type) {
            case CPL_TYPE_INT:
            {
                int *tip  = to_column->values->i;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tip++ *= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tip++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tip++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tip++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tip++ *= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tip++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tip[to_length] = tip[to_length] * fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tip[to_length] = tip[to_length] * fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *tlp  = to_column->values->l;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ *= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ *= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] * fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] * fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *tlp  = to_column->values->ll;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ *= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ *= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] * fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] * fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *tlp  = to_column->values->sz;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ *= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ *= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] * fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] * fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *tfp  = to_column->values->f;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tfp++ *= (double)*fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tfp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tfp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tfp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tfp++ *= (double)*ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tfp++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] * fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] * fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *tdp  = to_column->values->d;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tdp++ *= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tdp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tdp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tdp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tdp++ *= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tdp++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] * fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] * fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *tcfp = to_column->values->cf;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcfp++ *= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcfp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tcfp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tcfp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tcfp++ *= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tcfp++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            *tcfp++ *= *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            *tcfp++ *= *fcdp++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *tcdp = to_column->values->cd;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcdp++ *= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcdp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tcdp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tcdp++ *= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tcdp++ *= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tcdp++ *= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            *tcdp++ *= *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            *tcdp++ *= *fcdp++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
    else {

        /*
         * Here there are some invalids around, so the operation needs to be
         * performed with some checks. As a first thing, take care of the
         * invalids pattern in the target column:
         */

        if (to_column->nullcount == 0 || from_column->nullcount == 0) {

            /*
             * Here there are some invalids in just one of the two columns.
             * If they are just in the column to add, an identical invalid
             * flags buffer must be created for the target column.
             */

            if (from_column->nullcount) {
                to_column->null = cpl_calloc(to_length, 
                                             sizeof(cpl_column_flag));

                i = to_length;
                while (i--)
                    if (from_column->null[i])
                        to_column->null[i] = 1;

                to_column->nullcount = from_column->nullcount;

            }
        }
        else {

            /*
             *  Here there are some invalids in both columns.
             */

            i = to_length;
            while (i--) {
                if (from_column->null[i]) {
                    if (!to_column->null[i]) {
                        to_column->null[i] = 1;
                        to_column->nullcount++;
                    }
                }
            }

            if (to_column->nullcount == to_length) {

                /*
                 * Just invalids in the result: no need to perform any further
                 * operation, and the invalid buffer of the target column can
                 * be disabled.
                 */

                if (to_column->null)
                    cpl_free(to_column->null);
                to_column->null = NULL;
                return 0;
            }

        }

        /*
         * The operation can be performed while checking the already
         * computed result invalid flags buffer of the target column.
         */

        tnp = to_column->null;

        switch (to_type) {
            case CPL_TYPE_INT:
            {
                int *tip  = to_column->values->i;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip *= *fip;
                            tip++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip *= *flp;
                            tip++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip *= *flp;
                            tip++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip *= *flp;
                            tip++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip *= *ffp;
                            tip++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip *= *fdp;
                            tip++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip = *tip * *fcfp; /* Support clang */
                            tip++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tip = *tip * *fcdp; /* Support clang */
                            tip++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *tlp  = to_column->values->l;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *fip;
                            tlp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *ffp;
                            tlp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *fdp;
                            tlp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp * *fcfp; /* Support clang */
                            tlp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp * *fcdp; /* Support clang */
                            tlp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *tlp  = to_column->values->ll;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *fip;
                            tlp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *ffp;
                            tlp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *fdp;
                            tlp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp * *fcfp; /* Support clang */
                            tlp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp * *fcdp; /* Support clang */
                            tlp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *tlp  = to_column->values->sz;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *fip;
                            tlp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *flp;
                            tlp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *ffp;
                            tlp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp *= *fdp;
                            tlp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp * *fcfp; /* Support clang */
                            tlp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tlp = *tlp * *fcdp; /* Support clang */
                            tlp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *tfp  = to_column->values->f;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp *= (double)*fip;
                            tfp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp *= *flp;
                            tfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp *= *flp;
                            tfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp *= *flp;
                            tfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp *= (double)*ffp;
                            tfp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp *= *fdp;
                            tfp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp = *tfp * *fcfp; /* Support clang */
                            tfp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tfp = *tfp * *fcdp; /* Support clang */
                            tfp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *tdp  = to_column->values->d;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp *= *fip;
                            tdp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp *= *flp;
                            tdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp *= *flp;
                            tdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp *= *flp;
                            tdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp *= *ffp;
                            tdp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp *= *fdp;
                            tdp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp = *tdp * *fcfp; /* Support clang */
                            tdp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tdp = *tdp * *fcdp; /* Support clang */
                            tdp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *tcfp = to_column->values->cf;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *fip;
                            tcfp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *flp;
                            tcfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *flp;
                            tcfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *flp;
                            tcfp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *ffp;
                            tcfp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *fdp;
                            tcfp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *fcfp;
                            tcfp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcfp *= *fcdp;
                            tcfp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *tcdp = to_column->values->cd;

                switch (from_type) {
                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *fip;
                            tcdp++;
                            fip++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *flp;
                            tcdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *flp;
                            tcdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        long long *flp  = from_column->values->sz;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *flp;
                            tcdp++;
                            flp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *ffp;
                            tcdp++;
                            ffp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *fdp;
                            tcdp++;
                            fdp++;
                        }
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *fcfp;
                            tcdp++;
                            fcfp++;
                        }
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--) {
                            if (*tnp++ == 0)
                                *tcdp *= *fcdp;
                            tcdp++;
                            fcdp++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Divide two numeric columns.
 *
 * @param to_column   Target column.
 * @param from_column Divisor column.
 *
 * @return @c CPL_ERROR_NONE on success. If any of the input columns
 *   is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If
 *   any of the input columns is of type string, a @c CPL_ERROR_INVALID_TYPE
 *   is returned. If the input columns do not have the same length,
 *   a @c CPL_ERROR_INCOMPATIBLE_INPUT is returned.
 *
 * The result of the multiplication is stored in the target column. The type of
 * the columns may differ, and in that case the operation would be performed
 * according to the C upcasting rules, with a final cast of the result to
 * the target column type. Consistently, if at least one column is complex 
 * the math is complex, and if the target column is not complex then only
 * the real part of the result will be obtained. 
 * Invalid elements are propagated consistently:
 * if either or both members of the division are invalid, the result
 * will be invalid too. Underflows and overflows are ignored, but a division 
 * by zero is producing an invalid element in the target column. This function
 * is not applicable to array columns.
 */

cpl_error_code cpl_column_divide(cpl_column *to_column, 
                                 cpl_column *from_column)
{

    cpl_size        i;
    cpl_size        to_length   = cpl_column_get_size(to_column);
    cpl_size        from_length = cpl_column_get_size(from_column);
    cpl_type        to_type     = cpl_column_get_type(to_column);
    cpl_type        from_type   = cpl_column_get_type(from_column);
    int            *tnp;


    if (to_column == NULL || from_column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if ((to_type == CPL_TYPE_STRING) || (from_type == CPL_TYPE_STRING) ||
        (to_type & CPL_TYPE_POINTER) || (from_type & CPL_TYPE_POINTER))
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    if (to_length != from_length)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    if (to_length == 0)
        return CPL_ERROR_NONE;

    if (from_column->nullcount == from_length)
        return cpl_column_fill_invalid(to_column, 0, to_length);


    /*
     *  Look for zeroes in the divisor column, and mark them as
     *  invalid in the target column in advance.
     */

    if (from_column->nullcount == 0) {

        switch (from_type) {

            case CPL_TYPE_INT:
            {
                int *fip = from_column->values->i;

                for (i = 0; i < to_length; i++)
                    if (*fip++ == 0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *flp = from_column->values->l;

                for (i = 0; i < to_length; i++)
                    if (*flp++ == 0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *flp = from_column->values->ll;

                for (i = 0; i < to_length; i++)
                    if (*flp++ == 0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *flp = from_column->values->sz;

                for (i = 0; i < to_length; i++)
                    if (*flp++ == 0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *ffp = from_column->values->f;

                for (i = 0; i < to_length; i++)
                    if (*ffp++ == 0.0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *fdp = from_column->values->d;

                for (i = 0; i < to_length; i++)
                    if (*fdp++ == 0.0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *fcfp = from_column->values->cf;

                for (i = 0; i < to_length; i++)
                    if (*fcfp++ == 0.0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *fcdp = from_column->values->cd;

                for (i = 0; i < to_length; i++)
                    if (*fcdp++ == 0.0)
                        cpl_column_set_invalid(to_column, i);
                break;
            }
            default:
                break;

        }
    }
    else {

        cpl_column_flag *fnp = from_column->null;

        switch (from_type) {

            case CPL_TYPE_INT:
            {
                int *fip = from_column->values->i;

                for (i = 0; i < to_length; i++, fip++)
                    if (*fnp++ == 0)
                        if (*fip == 0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *flp = from_column->values->l;

                for (i = 0; i < to_length; i++, flp++)
                    if (*fnp++ == 0)
                        if (*flp == 0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *flp = from_column->values->ll;

                for (i = 0; i < to_length; i++, flp++)
                    if (*fnp++ == 0)
                        if (*flp == 0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *flp = from_column->values->sz;

                for (i = 0; i < to_length; i++, flp++)
                    if (*fnp++ == 0)
                        if (*flp == 0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *ffp = from_column->values->f;

                for (i = 0; i < to_length; i++, ffp++)
                    if (*fnp++ == 0)
                        if (*ffp == 0.0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *fdp = from_column->values->d;

                for (i = 0; i < to_length; i++, fdp++)
                    if (*fnp++ == 0)
                        if (*fdp == 0.0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *fcfp = from_column->values->cf;

                for (i = 0; i < to_length; i++, fcfp++)
                    if (*fnp++ == 0)
                        if (*fcfp == 0.0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *fcdp = from_column->values->cd;

                for (i = 0; i < to_length; i++, fcdp++)
                    if (*fnp++ == 0)
                        if (*fcdp == 0.0)
                            cpl_column_set_invalid(to_column, i);
                break;
            }
            default:
                break;

        }

    }


   /*
    * If there are just NULLs in the target column, no need to
    * perform any further operation:
    */

    if (to_column->nullcount == to_length)
        return CPL_ERROR_NONE;


    if (to_column->nullcount == 0 && from_column->nullcount == 0) {

        /*
         * If there are no invalids in both columns, the operation can
         * be performed without any check:
         */

        switch (to_type) {

            case CPL_TYPE_INT:
            {
                int *tip  = to_column->values->i;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tip++ /= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tip++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tip++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tip++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tip++ /= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tip++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tip[to_length] = tip[to_length] / fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tip[to_length] = tip[to_length] / fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *tlp  = to_column->values->l;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ /= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ /= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] / fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] / fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *tlp  = to_column->values->ll;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ /= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ /= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] / fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] / fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *tlp  = to_column->values->sz;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tlp++ /= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tlp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tlp++ /= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tlp++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] / fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tlp[to_length] = tlp[to_length] / fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *tfp  = to_column->values->f;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tfp++ /= (double)*fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tfp++ /= (double)*flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tfp++ /= (double)*flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tfp++ /= (double)*flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tfp++ /= (double)*ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tfp++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] / fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tfp[to_length] = tfp[to_length] / fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *tdp  = to_column->values->d;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tdp++ /= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tdp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tdp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tdp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tdp++ /= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tdp++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] / fcfp[to_length]; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            tdp[to_length] = tdp[to_length] / fcdp[to_length]; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *tcfp = to_column->values->cf;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcfp++ /= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcfp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tcfp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tcfp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tcfp++ /= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tcfp++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            *tcfp++ /= *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            *tcfp++ /= *fcdp++;
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *tcdp = to_column->values->cd;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        while (to_length--)
                            *tcdp++ /= *fip++;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        while (to_length--)
                            *tcdp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        while (to_length--)
                            *tcdp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        while (to_length--)
                            *tcdp++ /= *flp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        while (to_length--)
                            *tcdp++ /= *ffp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        while (to_length--)
                            *tcdp++ /= *fdp++;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        while (to_length--)
                            *tcdp++ /= *fcfp++;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        while (to_length--)
                            *tcdp++ /= *fcdp++;
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            default:
                break;

        }
    }
    else {

        /*
         * Here there are some invalids around, so the operation needs to be
         * performed with some checks. As a first thing, take care of the
         * invalids pattern in the target column:
         */

        if (to_column->nullcount == 0 || from_column->nullcount == 0) {

            /*
             * Here there are some invalids in just one of the two columns.
             * If they are just in the column to subtract, an identical
             * invalid flags buffer must be created for the target column.
             */

            if (from_column->nullcount) {
                to_column->null = cpl_calloc(to_length, 
                                             sizeof(cpl_column_flag));

                i = to_length;
                while (i--)
                    if (from_column->null[i])
                        to_column->null[i] = 1;

                to_column->nullcount = from_column->nullcount;

            }
        }
        else {

            /*
             *  Here there are some invalids in both columns.
             */

            i = to_length;
            while (i--) {
                if (from_column->null[i]) {
                    if (!to_column->null[i]) {
                        to_column->null[i] = 1;
                        to_column->nullcount++;
                    }
                }
            }

            if (to_column->nullcount == to_length) {

                /*
                 * Just invalids in the result: no need to perform any further
                 * operation, and the invalid buffer of the target column can
                 * be disabled.
                 */

                if (to_column->null)
                    cpl_free(to_column->null);
                to_column->null = NULL;
                return 0;
            }

        }

        /*
         * The operation can be performed while checking the already
         * computed result invalid flags buffer of the target column.
         */

        tnp = to_column->null;

        switch (to_type) {

            case CPL_TYPE_INT:
            {
                int *tip  = to_column->values->i;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tip++, fip++, tnp++)
                            if (*tnp == 0)
                                *tip /= *fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tip++, flp++, tnp++)
                            if (*tnp == 0)
                                *tip /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tip++, flp++, tnp++)
                            if (*tnp == 0)
                                *tip /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tip++, flp++, tnp++)
                            if (*tnp == 0)
                                *tip /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tip++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tip /= *ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tip++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tip /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tip++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tip = *tip / *fcfp; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tip++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tip = *tip / *fcdp; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_LONG:
            {
                long *tlp  = to_column->values->l;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tlp++, fip++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tlp++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tlp++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tlp++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tlp = *tlp / *fcfp; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tlp++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tlp = *tlp / *fcdp; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                long long *tlp  = to_column->values->ll;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tlp++, fip++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tlp++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tlp++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tlp++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tlp = *tlp / *fcfp; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tlp++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tlp = *tlp / *fcdp; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_SIZE:
            {
                cpl_size *tlp  = to_column->values->sz;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tlp++, fip++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tlp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tlp++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tlp++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tlp /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tlp++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tlp = *tlp / *fcfp; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tlp++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tlp = *tlp / *fcdp; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                float *tfp  = to_column->values->f;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tfp++, fip++, tnp++)
                            if (*tnp == 0)
                                *tfp /= (double)*fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tfp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tfp /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tfp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tfp /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tfp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tfp /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tfp++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tfp /= (double)*ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tfp++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tfp /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tfp++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tfp /= (double)*fcfp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tfp++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tfp = *tfp / *fcdp; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                double *tdp  = to_column->values->d;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tdp++, fip++, tnp++)
                            if (*tnp == 0)
                                *tdp /= *fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tdp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tdp /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tdp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tdp /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tdp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tdp /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tdp++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tdp /= *ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tdp++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tdp /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tdp++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tdp = *tdp / *fcfp; /* Support clang */
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tdp++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tdp = *tdp / *fcdp; /* Support clang */
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                float complex *tcfp = to_column->values->cf;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tcfp++, fip++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tcfp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tcfp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tcfp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tcfp++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tcfp++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tcfp++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *fcfp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tcfp++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tcfp /= *fcdp;
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                double complex *tcdp = to_column->values->cd;

                switch (from_type) {

                    case CPL_TYPE_INT:
                    {
                        int *fip  = from_column->values->i;

                        for (i = 0; i < to_length; i++, tcdp++, fip++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *fip;
                        break;
                    }
                    case CPL_TYPE_LONG:
                    {
                        long *flp  = from_column->values->l;

                        for (i = 0; i < to_length; i++, tcdp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *flp;
                        break;
                    }
                    case CPL_TYPE_LONG_LONG:
                    {
                        long long *flp  = from_column->values->ll;

                        for (i = 0; i < to_length; i++, tcdp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *flp;
                        break;
                    }
                    case CPL_TYPE_SIZE:
                    {
                        cpl_size *flp  = from_column->values->sz;

                        for (i = 0; i < to_length; i++, tcdp++, flp++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *flp;
                        break;
                    }
                    case CPL_TYPE_FLOAT:
                    {
                        float *ffp  = from_column->values->f;

                        for (i = 0; i < to_length; i++, tcdp++, ffp++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *ffp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE:
                    {
                        double *fdp  = from_column->values->d;

                        for (i = 0; i < to_length; i++, tcdp++, fdp++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *fdp;
                        break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex *fcfp = from_column->values->cf;

                        for (i = 0; i < to_length; i++, tcdp++, fcfp++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *fcfp;
                        break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex *fcdp = from_column->values->cd;

                        for (i = 0; i < to_length; i++, tcdp++, fcdp++, tnp++)
                            if (*tnp == 0)
                                *tcdp /= *fcdp;
                        break;
                    }
                    default:
                        break;

                }
                break;
            }
            default:
                break;

        }
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Private function to make the exponential of an integer column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_int(cpl_column *column, double base)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    int     *ip        = cpl_column_get_data_int(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, ip++)
            *ip = cpl_tools_ipow(base, *ip);
    }
    else {
        for (i = 0; i < length; i++, ip++, np++)
            if (*np == 0)
                *ip = cpl_tools_ipow(base, *ip);
    }

}


/*
 * @brief
 *   Private function to make the exponential of a long integer column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_long(cpl_column *column, double base)
{

    cpl_size         nullcount = cpl_column_count_invalid(column);
    cpl_size         length    = cpl_column_get_size(column);
    long            *lp        = cpl_column_get_data_long(column);
    cpl_column_flag *np        = cpl_column_get_data_invalid(column);
    cpl_size         i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, lp++)
            *lp = pow(base, *lp);
    }
    else {
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp = pow(base, *lp);
    }

}


/*
 * @brief
 *   Private function to make the exponential of a long long integer column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_long_long(cpl_column *column, double base)
{

    cpl_size         nullcount = cpl_column_count_invalid(column);
    cpl_size         length    = cpl_column_get_size(column);
    long long       *lp        = cpl_column_get_data_long_long(column);
    cpl_column_flag *np        = cpl_column_get_data_invalid(column);
    cpl_size         i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, lp++)
            *lp = pow(base, *lp);
    }
    else {
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp = pow(base, *lp);
    }

}


/*
 * @brief
 *   Private function to make the exponential of a cpl_size column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_cplsize(cpl_column *column, double base)
{

    cpl_size         nullcount = cpl_column_count_invalid(column);
    cpl_size         length    = cpl_column_get_size(column);
    cpl_size        *lp        = cpl_column_get_data_cplsize(column);
    cpl_column_flag *np        = cpl_column_get_data_invalid(column);
    cpl_size         i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, lp++)
            *lp = pow(base, *lp);
    }
    else {
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp = pow(base, *lp);
    }

}


/*
 * @brief
 *   Private function to make the exponential of a float column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_float(cpl_column *column, double base)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    float   *fp        = cpl_column_get_data_float(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, fp++)
            *fp = pow(base, *fp);
    }
    else {
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp = pow(base, *fp);
    }

}


static void cpl_column_conj_float(cpl_column *column)
{

    cpl_size       nullcount = cpl_column_count_invalid(column);
    cpl_size       length    = cpl_column_get_size(column);
    float complex *fp        = cpl_column_get_data_float_complex(column);
    int           *np        = cpl_column_get_data_invalid(column);
    cpl_size       i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, fp++)
            *fp = conj(*fp);
    }
    else {
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp = conj(*fp);
    }

}


/*
 * @brief
 *   Private function to make the exponential of a float complex column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_float_complex(cpl_column *column, double base)
{

    cpl_size       nullcount = cpl_column_count_invalid(column);
    cpl_size       length    = cpl_column_get_size(column);
    float complex *fp        = cpl_column_get_data_float_complex(column);
    int           *np        = cpl_column_get_data_invalid(column);
    cpl_size       i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, fp++)
            *fp = cpowf(base, *fp);
    }
    else {
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp = cpowf(base, *fp);
    }

}


/*
 * @brief
 *   Private function to make the exponential of a double column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_double(cpl_column *column, double base)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    double  *dp        = cpl_column_get_data_double(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, dp++)
            *dp = pow(base, *dp);
    }
    else {
        for (i = 0; i < length; i++, dp++, np++)
            if (*np == 0)
                *dp = pow(base, *dp);
    }

}


static void cpl_column_conj_double(cpl_column *column)
{

    cpl_size        nullcount = cpl_column_count_invalid(column);
    cpl_size        length    = cpl_column_get_size(column);
    double complex *fp        = cpl_column_get_data_double_complex(column);
    int            *np        = cpl_column_get_data_invalid(column);
    cpl_size        i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, fp++)
            *fp = conj(*fp);
    }
    else {
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp = conj(*fp);
    }

}


/*
 * @brief
 *   Private function to make the exponential of a double complex column.
 *
 * @param column  Target column.
 * @param base    Constant base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_exp_double_complex(cpl_column *column, double base)
{

    cpl_size        nullcount = cpl_column_count_invalid(column);
    cpl_size        length    = cpl_column_get_size(column);
    double complex *dp        = cpl_column_get_data_double_complex(column);
    int            *np        = cpl_column_get_data_invalid(column);
    cpl_size        i;


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, dp++)
            *dp = cpow(base, *dp);
    }
    else {
        for (i = 0; i < length; i++, dp++, np++)
            if (*np == 0)
                *dp = cpow(base, *dp);
    }

}

/*
 * @brief
 *   Private function to make the power of a float complex column.
 *
 * @param column   Target column.
 * @param exponent Constant exponent.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_pow_float_complex(cpl_column *column, double exponent)
{
    const int      myerrno   = errno;
    const cpl_size nullcount = cpl_column_count_invalid(column);
    const cpl_size length    = cpl_column_get_size(column);
    float complex      *fp       = cpl_column_get_data_float_complex(column);
    const int      *np       = nullcount == 0 ? NULL
        : cpl_column_get_data_invalid_const(column);


    if (nullcount == length)
        return;

    if ((cpl_size)((size_t)length) != length) {
        (void)cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                     "%" CPL_SIZE_FORMAT, length);
        return;
    }

    /* In spite of checks powf() may still overflow */
    errno = 0;

    if (exponent == -0.5) {
        /* Column value must be non-zero */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                float complex value;
                if (fp[i] != 0.0) {
                    value = 1.0 / csqrtf(fp[i]);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    } else if (exponent < 0.0) {
        /* Column value must be non-zero */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                float complex value;
                if (fp[i] != 0.0) {
                    value = cpowf(fp[i], exponent);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    } else if (exponent == 0.5) {
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                const float complex value = csqrtf(fp[i]);
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    } else if (exponent != 1.0) {
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                const float complex value = cpowf(fp[i], exponent);
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    }

    if (exponent != 1.0) {
        /* FIXME: Counts also bad pixels */
        cpl_tools_add_flops( length );
    }

    errno = myerrno;
}

/*
 * @brief
 *   Private function to make the power of a double complex column.
 *
 * @param column   Target column.
 * @param exponent Constant exponent.
 *
 * @return Nothing
 * @see 
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_pow_double_complex(cpl_column *column, double exponent)
{
    const int      myerrno   = errno;
    const cpl_size nullcount = cpl_column_count_invalid(column);
    const cpl_size length    = cpl_column_get_size(column);
    double complex *fp       = cpl_column_get_data_double_complex(column);
    const int      *np       = nullcount == 0 ? NULL
        : cpl_column_get_data_invalid_const(column);


    if (nullcount == length)
        return;

    if ((cpl_size)((size_t)length) != length) {
        (void)cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                     "%" CPL_SIZE_FORMAT, length);
        return;
    }

    /* In spite of checks pow() may still overflow */
    errno = 0;

    if (exponent == -0.5) {
        /* Column value must be non-zero */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                double complex value = 0; /* Init avoids (false pos.) warning */
                if (fp[i] != 0.0) {
                    value = 1.0 / csqrt(fp[i]);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    } else if (exponent < 0.0) {
        /* Column value must be non-zero */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                double complex value = 0; /* Init avoids (false pos.) warning */
                if (fp[i] != 0.0) {
                    value = cpow(fp[i], exponent);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    } else if (exponent == 0.5) {
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                const double complex value = csqrt(fp[i]);
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    } else if (exponent != 1.0) {
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                const double complex value = cpow(fp[i], exponent);
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    }

    if (exponent != 1.0) {
        /* FIXME: Counts also bad pixels */
        cpl_tools_add_flops( length );
    }

    errno = myerrno;
}


/*
 * @brief
 *   Private function to make the complex power of a float complex column.
 *
 * @param column   Target column.
 * @param exponent Constant complex exponent.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_cpow_float_complex(cpl_column *column,
                                          double complex exponent)
{
    const int      myerrno   = errno;
    const cpl_size nullcount = cpl_column_count_invalid(column);
    const cpl_size length    = cpl_column_get_size(column);
    float complex  *fp       = cpl_column_get_data_float_complex(column);
    const int      *np       = nullcount == 0 ? NULL
        : cpl_column_get_data_invalid_const(column);


    if (nullcount == length)
        return;

    if ((cpl_size)((size_t)length) != length) {
        (void)cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                     "%" CPL_SIZE_FORMAT, length);
        return;
    }

    /* In spite of checks cpowf() may still overflow */
    errno = 0;

    for (size_t i = 0; i < (size_t)length; i++) {
        if (np == NULL || np[i] == 0) {
            const float complex value = cpowf(fp[i], exponent);
            if (errno) {
                errno = 0;
                cpl_column_set_invalid(column, i);
            } else {
                fp[i] = value;
            }
        }
    }

    if (exponent != 1.0) {
        /* FIXME: Counts also bad pixels */
        cpl_tools_add_flops( length );
    }

    errno = myerrno;
}

/*
 * @brief
 *   Private function to make the complex power of a float complex column.
 *
 * @param column   Target column.
 * @param exponent Constant complex exponent.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_cpow_double_complex(cpl_column *column,
                                           double complex exponent)
{
    const int      myerrno   = errno;
    const cpl_size nullcount = cpl_column_count_invalid(column);
    const cpl_size length    = cpl_column_get_size(column);
    double complex *dp       = cpl_column_get_data_double_complex(column);
    const int      *np       = nullcount == 0 ? NULL
        : cpl_column_get_data_invalid_const(column);


    if (nullcount == length)
        return;

    if ((cpl_size)((size_t)length) != length) {
        (void)cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                     "%" CPL_SIZE_FORMAT, length);
        return;
    }

    /* In spite of checks cpowf() may still overflow */
    errno = 0;

    for (size_t i = 0; i < (size_t)length; i++) {
        if (np == NULL || np[i] == 0) {
            const double complex value = cpow(dp[i], exponent);
            if (errno) {
                errno = 0;
                cpl_column_set_invalid(column, i);
            } else {
                dp[i] = value;
            }
        }
    }

    errno = myerrno;
}

/*
 * @brief
 *   Private function to make the log of an integer column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_int(cpl_column *column, double base)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    int     *ip        = cpl_column_get_data_int(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;
    double   invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, ip++) {
            if (*ip > 0)
                *ip = floor(log(*ip) * invlog + 0.5);
            else 
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, ip++, np++) {
            if (*np == 0) {
                if (*ip > 0)
                    *ip = floor(log(*ip) * invlog + 0.5);
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to make the log of a long integer column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_long(cpl_column *column, double base)
{

    cpl_size             nullcount = cpl_column_count_invalid(column);
    cpl_size             length    = cpl_column_get_size(column);
    long                *lp        = cpl_column_get_data_long(column);
    cpl_column_flag     *np        = cpl_column_get_data_invalid(column);
    cpl_size             i;
    double               invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, lp++) {
            if (*lp > 0)
                *lp = floor(log(*lp) * invlog + 0.5);
            else
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, lp++, np++) {
            if (*np == 0) {
                if (*lp > 0)
                    *lp = floor(log(*lp) * invlog + 0.5);
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to make the log of a long long integer column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_long_long(cpl_column *column, double base)
{

    cpl_size             nullcount = cpl_column_count_invalid(column);
    cpl_size             length    = cpl_column_get_size(column);
    long  long          *lp        = cpl_column_get_data_long_long(column);
    cpl_column_flag     *np        = cpl_column_get_data_invalid(column);
    cpl_size             i;
    double               invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, lp++) {
            if (*lp > 0)
                *lp = floor(log(*lp) * invlog + 0.5);
            else
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, lp++, np++) {
            if (*np == 0) {
                if (*lp > 0)
                    *lp = floor(log(*lp) * invlog + 0.5);
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to make the log of a cpl_size column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_cplsize(cpl_column *column, double base)
{

    cpl_size             nullcount = cpl_column_count_invalid(column);
    cpl_size             length    = cpl_column_get_size(column);
    cpl_size            *lp        = cpl_column_get_data_cplsize(column);
    cpl_column_flag     *np        = cpl_column_get_data_invalid(column);
    cpl_size             i;
    double               invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, lp++) {
            if (*lp > 0)
                *lp = floor(log(*lp) * invlog + 0.5);
            else
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, lp++, np++) {
            if (*np == 0) {
                if (*lp > 0)
                    *lp = floor(log(*lp) * invlog + 0.5);
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to make the log of a float column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_float(cpl_column *column, double base)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    float   *fp        = cpl_column_get_data_float(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;
    double   invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, fp++) {
            if (*fp > 0.0)
                *fp = log(*fp) * invlog;
            else
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, fp++, np++) {
            if (*np == 0) {
                if (*fp > 0.0)
                    *fp = log(*fp) * invlog;
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to make the log of a float complex column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_float_complex(cpl_column *column, double base)
{

    cpl_size       nullcount = cpl_column_count_invalid(column);
    cpl_size       length    = cpl_column_get_size(column);
    float complex *fp        = cpl_column_get_data_float_complex(column);
    int           *np        = cpl_column_get_data_invalid(column);
    cpl_size       i;
    double         invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, fp++) {
            if (cabsf(*fp) > 0.0)
                *fp = clogf(*fp) * invlog;
            else
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, fp++, np++) {
            if (*np == 0) {
                if (cabsf(*fp) > 0.0)
                    *fp = clogf(*fp) * invlog;
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to make the log of a double column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_double(cpl_column *column, double base)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    double  *dp        = cpl_column_get_data_double(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;
    double   invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, dp++) {
            if (*dp > 0.0)
                *dp = log(*dp) * invlog;
            else
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, dp++, np++) {
            if (*np == 0) {
                if (*dp > 0.0)
                    *dp = log(*dp) * invlog;
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to make the log of a double complex column.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_log_double_complex(cpl_column *column, double base)
{

    cpl_size        nullcount = cpl_column_count_invalid(column);
    cpl_size        length    = cpl_column_get_size(column);
    double complex *dp        = cpl_column_get_data_double_complex(column);
    int            *np        = cpl_column_get_data_invalid(column);
    cpl_size        i;
    double          invlog    = 1 / log(base);


    if (nullcount == length)
        return;

    if (nullcount == 0) {
        for (i = 0; i < length; i++, dp++) {
            if (cabs(*dp) > 0.0)
                *dp = clog(*dp) * invlog;
            else
                cpl_column_set_invalid(column, i);
        }
    }
    else {
        for (i = 0; i < length; i++, dp++, np++) {
            if (*np == 0) {
                if (cabs(*dp) > 0.0)
                    *dp = clog(*dp) * invlog;
                else
                    cpl_column_set_invalid(column, i);
            }
        }
    }
}


/*
 * @brief
 *   Private function to add an @em integer constant to an @em integer column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_add_to_int(cpl_column *column, int value)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    int     *ip        = cpl_column_get_data_int(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (value == 0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *ip++ += value;
    else
        for (i = 0; i < length; i++, ip++, np++)
            if (*np == 0)
                *ip += value;

}


/*
 * @brief
 *   Private function to add a @em long @em integer constant to a @em long
 *   @em integer column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_add_to_long(cpl_column *column, long value)
{

    cpl_size             nullcount = cpl_column_count_invalid(column);
    cpl_size             length    = cpl_column_get_size(column);
    long                *lp        = cpl_column_get_data_long(column);
    cpl_column_flag     *np        = cpl_column_get_data_invalid(column);
    cpl_size             i;


    if (value == 0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *lp++ += value;
    else
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp += value;

}


/*
 * @brief
 *   Private function to add a @em long @em long @em integer constant to a
 *   @em long @em long @em integer column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_add_to_long_long(cpl_column *column, long long value)
{

    cpl_size             nullcount = cpl_column_count_invalid(column);
    cpl_size             length    = cpl_column_get_size(column);
    long long           *lp        = cpl_column_get_data_long_long(column);
    cpl_column_flag     *np        = cpl_column_get_data_invalid(column);
    cpl_size             i;


    if (value == 0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *lp++ += value;
    else
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp += value;

}


/*
 * @brief
 *   Private function to add a cpl_size constant to a cpl_size column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_add_to_cplsize(cpl_column *column, cpl_size value)
{

    cpl_size             nullcount = cpl_column_count_invalid(column);
    cpl_size             length    = cpl_column_get_size(column);
    cpl_size            *lp        = cpl_column_get_data_cplsize(column);
    cpl_column_flag     *np        = cpl_column_get_data_invalid(column);
    cpl_size             i;


    if (value == 0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *lp++ += value;
    else
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp += value;

}


/*
 * @brief
 *   Private function to add a @em float constant to a @em float column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */ 

static void cpl_column_add_to_float(cpl_column *column, double value)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    float   *fp        = cpl_column_get_data_float(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (value == 0.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0) 
        for (i = 0; i < length; i++)
            *fp++ += value;
    else
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp += value;

}


/*
 * @brief
 *   Private function to add a @em float complex constant to a 
 *   @em float complex column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_add_to_float_complex(cpl_column *column, 
                                            double complex value)
{

    cpl_size       nullcount = cpl_column_count_invalid(column);
    cpl_size       length    = cpl_column_get_size(column);
    float complex *fp        = cpl_column_get_data_float_complex(column);
    int           *np        = cpl_column_get_data_invalid(column);
    cpl_size       i;


    if (value == 0.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *fp++ += value;
    else
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp += value;

}


/*
 * @brief
 *   Private function to add a @em double constant to a @em double column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_add_to_double(cpl_column *column, double value)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    double  *dp        = cpl_column_get_data_double(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (value == 0.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0) 
        for (i = 0; i < length; i++)
            *dp++ += value;
    else
        for (i = 0; i < length; i++, dp++, np++)
            if (*np == 0)
                *dp += value;

}


/*
 * @brief
 *   Private function to add a @em double complex constant to a 
 *   @em double complex column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_add_to_double_complex(cpl_column *column, 
                                             double complex value)
{

    cpl_size        nullcount = cpl_column_count_invalid(column);
    cpl_size        length    = cpl_column_get_size(column);
    double complex *dp        = cpl_column_get_data_double_complex(column);
    int            *np        = cpl_column_get_data_invalid(column);
    cpl_size        i;


    if (value == 0.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *dp++ += value;
    else
        for (i = 0; i < length; i++, dp++, np++)
            if (*np == 0)
                *dp += value;

}


/*
 * @brief
 *   Private function to multiply a @em integer column by a @em double constant.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing.
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_mul_double_to_int(cpl_column *column, double value)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    int     *ip        = cpl_column_get_data_int(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *ip++ *= value;
    else
        for (i = 0; i < length; i++, ip++, np++)
            if (*np == 0)
                *ip *= value;

}


/*
 * @brief
 *   Private function to multiply a @em long @em integer column by a
 *   @em double constant.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing.
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_mul_double_to_long(cpl_column *column, double value)
{

    cpl_size         nullcount = cpl_column_count_invalid(column);
    cpl_size         length    = cpl_column_get_size(column);
    long            *lp        = cpl_column_get_data_long(column);
    cpl_column_flag *np        = cpl_column_get_data_invalid(column);
    cpl_size         i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *lp++ *= value;
    else
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp *= value;

}


/*
 * @brief
 *   Private function to multiply a @em long @em long @em integer column by a
 *   @em double constant.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing.
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_mul_double_to_long_long(cpl_column *column, double value)
{

    cpl_size         nullcount = cpl_column_count_invalid(column);
    cpl_size         length    = cpl_column_get_size(column);
    long long       *lp        = cpl_column_get_data_long_long(column);
    cpl_column_flag *np        = cpl_column_get_data_invalid(column);
    cpl_size         i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *lp++ *= value;
    else
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp *= value;

}


/*
 * @brief
 *   Private function to multiply a @em cpl_size column by a @em double
 *   constant.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing.
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_mul_double_to_cplsize(cpl_column *column, double value)
{

    cpl_size         nullcount = cpl_column_count_invalid(column);
    cpl_size         length    = cpl_column_get_size(column);
    cpl_size        *lp        = cpl_column_get_data_cplsize(column);
    cpl_column_flag *np        = cpl_column_get_data_invalid(column);
    cpl_size         i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *lp++ *= value;
    else
        for (i = 0; i < length; i++, lp++, np++)
            if (*np == 0)
                *lp *= value;

}


/*
 * @brief
 *   Private function to multiply a @em float column by a @em double constant.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing.
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_mul_double_to_float(cpl_column *column, double value)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    float   *fp        = cpl_column_get_data_float(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *fp++ *= value;
    else
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp *= value;

}


static void cpl_column_mul_double_to_float_complex(cpl_column *column,
                                                   double complex value)
{

    cpl_size       nullcount = cpl_column_count_invalid(column);
    cpl_size       length    = cpl_column_get_size(column);
    float complex *fp        = cpl_column_get_data_float_complex(column);
    int           *np        = cpl_column_get_data_invalid(column);
    cpl_size       i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *fp++ *= value;
    else
        for (i = 0; i < length; i++, fp++, np++)
            if (*np == 0)
                *fp *= value;

}


/*
 * @brief
 *   Private function to multiply a @em double column by a @em double constant.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_mul_to_double(cpl_column *column, double value)
{

    cpl_size nullcount = cpl_column_count_invalid(column);
    cpl_size length    = cpl_column_get_size(column);
    double  *dp        = cpl_column_get_data_double(column);
    int     *np        = cpl_column_get_data_invalid(column);
    cpl_size i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *dp++ *= value;
    else
        for (i = 0; i < length; i++, dp++, np++)
            if (*np == 0)
                *dp *= value;

}


/*
 * @brief
 *   Private function to multiply a @em double complex column by a
 *   @em double complex constant.
 *
 * @param column   Target column.
 * @param value    Value to add.
 *
 * @return Nothing
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void cpl_column_mul_to_double_complex(cpl_column *column,
                                             double complex value)
{

    cpl_size        nullcount = cpl_column_count_invalid(column);
    cpl_size        length    = cpl_column_get_size(column);
    double complex *dp        = cpl_column_get_data_double_complex(column);
    int            *np        = cpl_column_get_data_invalid(column);
    cpl_size        i;


    if (value == 1.0)
        return;

    if (nullcount == length)
        return;

    if (nullcount == 0)
        for (i = 0; i < length; i++)
            *dp++ *= value;
    else
        for (i = 0; i < length; i++, dp++, np++)
            if (*np == 0)
                *dp *= value;

}


/* 
 * @brief
 *   Compute the logarithm of column values.
 *
 * @param column   Target column.
 * @param base     Logarithm base.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL 
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column 
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned. If the
 *   @em base is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is
 *   returned.
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Non-positive column
 * values will be marked as invalid. Columns of strings or arrays are
 * not allowed.
 */

cpl_error_code cpl_column_logarithm(cpl_column *column, double base)
{

    cpl_type     type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (base <= 0.0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);


    switch (type) {

    case CPL_TYPE_INT:
        cpl_column_log_int(column, base);
        break;
    case CPL_TYPE_LONG:
        cpl_column_log_long(column, base);
        break;
    case CPL_TYPE_LONG_LONG:
        cpl_column_log_long_long(column, base);
        break;
    case CPL_TYPE_SIZE:
        cpl_column_log_cplsize(column, base);
        break;
    case CPL_TYPE_FLOAT:
        cpl_column_log_float(column, base);
        break;
    case CPL_TYPE_DOUBLE:
        cpl_column_log_double(column, base);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        cpl_column_log_float_complex(column, base);
        break;
    case CPL_TYPE_DOUBLE_COMPLEX:
        cpl_column_log_double_complex(column, base);
        break;
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Compute the absolute value of column values.
 *
 * @param column   Target column.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL 
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column 
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned.
 *
 * Columns of strings, or arrays, or complex, are not allowed.
 */

cpl_error_code cpl_column_absolute(cpl_column *column)
{

    cpl_type    type = cpl_column_get_type(column);
    cpl_size    length;
    cpl_size    nullcount;
    cpl_size    i;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    switch (type) {
    case CPL_TYPE_INT:
        break;
    case CPL_TYPE_LONG:
        break;
    case CPL_TYPE_LONG_LONG:
        break;
    case CPL_TYPE_FLOAT:
        break;
    case CPL_TYPE_DOUBLE:
        break;
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (nullcount == length)
        return CPL_ERROR_NONE;

    switch (type) {

    case CPL_TYPE_INT:
    {
        int *ip = cpl_column_get_data_int(column);
        for (i = 0; i < length; i++, ip++)
            *ip = abs(*ip);
        break;
    }
    case CPL_TYPE_LONG:
    {
        long *lp = cpl_column_get_data_long(column);
        for (i = 0; i < length; i++, lp++)
            *lp = labs(*lp);
        break;
    }
    case CPL_TYPE_LONG_LONG:
    {
        long long *lp = cpl_column_get_data_long_long(column);

#if defined(HAVE_LLABS) && defined(HAVE_DECL_LLABS)

        for (i = 0; i < length; i++, lp++)
            *lp = llabs(*lp);

#else

        for (i = 0; i < length; i++, lp++)
            *lp = *lp < 0LL ? -(*lp) : *lp;

#endif

        break;
    }
    case CPL_TYPE_SIZE:
    {
        cpl_size *lp = cpl_column_get_data_cplsize(column);

#if defined(HAVE_LLABS) && defined(HAVE_DECL_LLABS)

        for (i = 0; i < length; i++, lp++)
            *lp = llabs(*lp);

#else

        for (i = 0; i < length; i++, lp++)
            *lp = *lp < 0LL ? -(*lp) : *lp;

#endif

        break;
    }
    case CPL_TYPE_FLOAT:
    {
        float *fp = cpl_column_get_data_float(column);
        for (i = 0; i < length; i++, fp++)
            *fp = fabsf(*fp);
        break;
    }
    case CPL_TYPE_DOUBLE:
    {
        double *dp = cpl_column_get_data_double(column);
        for (i = 0; i < length; i++, dp++)
            *dp = fabs(*dp);
        break;
    }
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Compute the absolute value of column complex values.
 *
 * @param column   Target column.
 *
 * @return New column of type CPL_TYPE_(FLOAT|DOUBLE) if input type
 *   is CPL_TYPE_(FLOAT|DOUBLE)_COMPLEX, NULL in case of error.
 *
 * Columns of strings, of integers, or arrays, are not allowed.
 */

cpl_column *cpl_column_absolute_complex(cpl_column *column)
{

    cpl_type        type = cpl_column_get_type(column);
    cpl_column     *new_column;
    cpl_size        length;
    cpl_size        nullcount;
    cpl_size        i;


    if (column == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    length = cpl_column_get_size(column);

    switch (type) {
        case CPL_TYPE_FLOAT_COMPLEX:
            new_column = cpl_column_new_float(length);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            new_column = cpl_column_new_double(length);
            break;
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    nullcount = cpl_column_count_invalid(column);

    if (nullcount == length)
        return new_column;

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null,
               length * sizeof(cpl_column_flag));
    }

    switch (type) {

        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex *cfp = cpl_column_get_data_float_complex(column);
            float *fp = cpl_column_get_data_float(new_column);

            for (i = 0; i < length; i++, fp++, cfp++)
                *fp = cabsf(*cfp);
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex *cdp = cpl_column_get_data_double_complex(column);
            double *dp = cpl_column_get_data_double(new_column);

            for (i = 0; i < length; i++, dp++, cdp++)
                *dp = cabs(*cdp);
            break;
        }
        default:
        {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            cpl_column_delete(new_column);
            return NULL;
        }

    }

    return new_column;

}


/* 
 * @brief
 *   Compute the phase of column complex values.
 *
 * @param column   Target column.
 *
 * @return New column of type CPL_TYPE_(FLOAT|DOUBLE) if input type
 *   is CPL_TYPE_(FLOAT|DOUBLE)_COMPLEX, NULL in case of error.
 *
 * The computed phase angles are in the range [-pi,pi].
 * Columns of strings, of integers, or arrays, are not allowed.
 */

cpl_column *cpl_column_phase_complex(cpl_column *column)
{

    cpl_type        type = cpl_column_get_type(column);
    cpl_column     *new_column;
    cpl_size        length;
    cpl_size        nullcount;
    cpl_size        i;


    if (column == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    length = cpl_column_get_size(column);

    switch (type) {
        case CPL_TYPE_FLOAT_COMPLEX:
            new_column = cpl_column_new_float(length);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            new_column = cpl_column_new_double(length);
            break;
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    nullcount = cpl_column_count_invalid(column);

    if (nullcount == length)
        return new_column;

    new_column->nullcount = column->nullcount;

    if (column->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, column->null,
               length * sizeof(cpl_column_flag));
    }

    switch (type) {

        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex *cfp = cpl_column_get_data_float_complex(column);
            float *fp = cpl_column_get_data_float(new_column);

            for (i = 0; i < length; i++, fp++, cfp++)
                *fp = cargf(*cfp);
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex *cdp = cpl_column_get_data_double_complex(column);
            double *dp = cpl_column_get_data_double(new_column);

            for (i = 0; i < length; i++, dp++, cdp++)
                *dp = carg(*cdp);
            break;
        }
        default:
        {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            cpl_column_delete(new_column);
            return NULL;
        }

    }

    return new_column;

}


/* 
 * @brief
 *   Extract the real part of column complex values.
 *
 * @param self   Target column.
 *
 * @return New column of type CPL_TYPE_(FLOAT|DOUBLE) if input type
 *   is CPL_TYPE_(FLOAT|DOUBLE)_COMPLEX, NULL in case of error.
 *
 * Columns of strings, integers, or arrays, are not allowed.
 */

cpl_column *cpl_column_extract_real(const cpl_column *self)
{

    const cpl_type  type = cpl_column_get_type(self);
    cpl_column     *new_column;
    cpl_size        length;
    cpl_size        nullcount;
    cpl_size        i;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    length = cpl_column_get_size(self);

    switch (type) {
        case CPL_TYPE_FLOAT_COMPLEX:
            new_column = cpl_column_new_float(length);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            new_column = cpl_column_new_double(length);
            break;
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    nullcount = cpl_column_get_invalid_numerical(self);

    if (nullcount == length)
        return new_column;

    new_column->nullcount = self->nullcount;

    if (self->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, self->null,
               length * sizeof(cpl_column_flag));
    }

    switch (type) {

        case CPL_TYPE_FLOAT_COMPLEX:
        {
            const float complex *cfp
                = cpl_column_get_data_float_complex_const(self);
            float *fp = cpl_column_get_data_float(new_column);

            for (i = 0; i < length; i++, fp++, cfp++)
                *fp = crealf(*cfp);
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            const double complex *cdp
                = cpl_column_get_data_double_complex_const(self);
            double *dp = cpl_column_get_data_double(new_column);

            for (i = 0; i < length; i++, dp++, cdp++)
                *dp = creal(*cdp);
            break;
        }
        default:
        {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            cpl_column_delete(new_column);
            return NULL;
        }

    }

    return new_column;

}


/* 
 * @brief
 *   Extract the imaginary part of column complex values.
 *
 * @param self   Target column.
 *
 * @return New column of type CPL_TYPE_(FLOAT|DOUBLE) if input type
 *   is CPL_TYPE_(FLOAT|DOUBLE)_COMPLEX, NULL in case of error.
 *
 * Columns of strings, integers, or arrays, are not allowed.
 */

cpl_column *cpl_column_extract_imag(const cpl_column *self)
{

    cpl_type        type = cpl_column_get_type(self);
    cpl_column     *new_column;
    cpl_size        length;
    cpl_size        nullcount;
    cpl_size        i;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    length = cpl_column_get_size(self);

    switch (type) {
        case CPL_TYPE_FLOAT_COMPLEX:
            new_column = cpl_column_new_float(length);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            new_column = cpl_column_new_double(length);
            break;
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    nullcount = cpl_column_get_invalid_numerical(self);

    if (nullcount == length)
        return new_column;

    new_column->nullcount = self->nullcount;

    if (self->null) {
        new_column->null = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_column->null, self->null,
               length * sizeof(cpl_column_flag));
    }

    switch (type) {

        case CPL_TYPE_FLOAT_COMPLEX:
        {
            const float complex *cfp
                = cpl_column_get_data_float_complex_const(self);
            float *fp = cpl_column_get_data_float(new_column);

            for (i = 0; i < length; i++, fp++, cfp++)
                *fp = cimagf(*cfp);
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            const double complex *cdp
                = cpl_column_get_data_double_complex_const(self);
            double *dp = cpl_column_get_data_double(new_column);

            for (i = 0; i < length; i++, dp++, cdp++)
                *dp = cimag(*cdp);
            break;
        }
        default:
        {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            cpl_column_delete(new_column);
            return NULL;
        }

    }

    return new_column;

}


/* 
 * @brief
 *   Compute the exponential of column values.
 *
 * @param column   Target column.
 * @param base     Exponential base.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned. If the
 *   @em base is not greater than zero, a @c CPL_ERROR_ILLEGAL_INPUT is
 *   returned.
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Columns of strings or 
 * arrays are not allowed.
 */

cpl_error_code cpl_column_exponential(cpl_column *column, double base)
{

    cpl_type     type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (base <= 0.0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);


    switch (type) {

        case CPL_TYPE_INT:
            cpl_column_exp_int(column, base);
            break;
        case CPL_TYPE_LONG:
            cpl_column_exp_long(column, base);
            break;
        case CPL_TYPE_LONG_LONG:
            cpl_column_exp_long_long(column, base);
            break;
        case CPL_TYPE_SIZE:
            cpl_column_exp_cplsize(column, base);
            break;
        case CPL_TYPE_FLOAT:
            cpl_column_exp_float(column, base);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_exp_double(column, base);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_exp_float_complex(column, base);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_exp_double_complex(column, base);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


cpl_error_code cpl_column_conjugate(cpl_column *column)
{

    cpl_type    type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    
    switch (type) {

        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_conj_float(column);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_conj_double(column);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Compute a power of numerical column values.
 *
 * @param column   Column of a numerical type
 * @param exponent Constant exponent.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned.
 *
 * For float and float complex the operation is performed in single precision,
 * otherwise it is performed in double precision and then rounded if the column
 * is of an integer type. Results that would or do cause domain errors or
 * overflow are marked as invalid. Columns of strings or arrays are 
 * not allowed.
 */

cpl_error_code cpl_column_power(cpl_column *column, double exponent)
{
    if (column == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else {
        const cpl_type type = cpl_column_get_type(column);

        switch (type) {

        case CPL_TYPE_INT:
            cpl_column_pow_int(column, exponent);
            break;
        case CPL_TYPE_LONG:
            cpl_column_pow_long(column, exponent);
            break;
        case CPL_TYPE_LONG_LONG:
            cpl_column_pow_long_long(column, exponent);
            break;
        case CPL_TYPE_SIZE:
            cpl_column_pow_cplsize(column, exponent);
            break;
        case CPL_TYPE_FLOAT:
            cpl_column_pow_float(column, exponent);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_pow_double(column, exponent);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_pow_float_complex(column, exponent);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_pow_double_complex(column, exponent);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

        }
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Compute a complex power of complex, numerical column values.
 *
 * @param column   Column of a complex, numerical type
 * @param exponent Constant exponent.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned.
 *
 * For float and float complex the operation is performed in single precision,
 * otherwise it is performed in double precision.
 * Results that would or do cause domain errors or overflow are marked as
 * invalid. Columns of strings or arrays are not allowed.
 */

cpl_error_code cpl_column_power_complex(cpl_column *column,
                                        double complex exponent)
{
    if (column == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (exponent != (double)exponent) {
        /* Only call complex version when needed */
        const cpl_type type = cpl_column_get_type(column);

        switch (type) {

        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_cpow_float_complex(column, exponent);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_cpow_double_complex(column, exponent);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

        }
    } else if (cpl_column_power(column, (double)exponent)) {
        return cpl_error_set_where_();
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Add a constant value to a numerical column.
 *
 * @param column   Target column.
 * @param value    Value to add.
 * 
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned.
 * 
 * The operation is always performed in double precision, with a final   
 * cast of the result to the target column type. Invalid flags are not
 * modified by this operation. Columns of strings or arrays are not allowed.
 */

cpl_error_code cpl_column_add_scalar(cpl_column *column, double value)
{

    cpl_type    type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    switch (type) {

        case CPL_TYPE_INT:
            cpl_column_add_to_int(column, value);
            break;
        case CPL_TYPE_LONG:
            cpl_column_add_to_long(column, value);
            break;
        case CPL_TYPE_LONG_LONG:
            cpl_column_add_to_long_long(column, value);
            break;
        case CPL_TYPE_SIZE:
            cpl_column_add_to_cplsize(column, value);
            break;
        case CPL_TYPE_FLOAT:
            cpl_column_add_to_float(column, value);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_add_to_double(column, value);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_add_to_float_complex(column, value);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_add_to_double_complex(column, value);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


cpl_error_code cpl_column_add_scalar_complex(cpl_column *column, 
                                             double complex value)
{

    cpl_type    type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    switch (type) {

        case CPL_TYPE_FLOAT:
            cpl_column_add_to_float(column, value);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_add_to_double(column, value);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_add_to_float_complex(column, value);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_add_to_double_complex(column, value);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Subtract a constant value from a numerical column.
 *
 * @param column   Target column.
 * @param value    Value to subtract.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned.
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid flags are not
 * modified by this operation. Columns of strings or arrays are not allowed.
 */

cpl_error_code cpl_column_subtract_scalar(cpl_column *column, double value)
{

    return cpl_column_add_scalar(column, -value);

}

cpl_error_code cpl_column_subtract_scalar_complex(cpl_column *column, 
                                                  double complex value)
{

    return cpl_column_add_scalar_complex(column, -value);

}


/* 
 * @brief
 *   Multiply by a constant a numerical column.
 *
 * @param column   Target column.
 * @param value    Multiplier.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned.
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid flags are not
 * modified by this operation. Columns of strings or arrays are not allowed.
 */

cpl_error_code cpl_column_multiply_scalar(cpl_column *column, double value)
{

    cpl_type    type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    switch (type) {

        case CPL_TYPE_INT:
            cpl_column_mul_double_to_int(column, value);
            break;
        case CPL_TYPE_LONG:
            cpl_column_mul_double_to_long(column, value);
            break;
        case CPL_TYPE_LONG_LONG:
            cpl_column_mul_double_to_long_long(column, value);
            break;
        case CPL_TYPE_SIZE:
            cpl_column_mul_double_to_cplsize(column, value);
            break;
        case CPL_TYPE_FLOAT:
            cpl_column_mul_double_to_float(column, value);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_mul_to_double(column, value);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_mul_double_to_float_complex(column, value);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_mul_to_double_complex(column, value);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


cpl_error_code cpl_column_multiply_scalar_complex(cpl_column *column, 
                                                  double complex value)
{

    cpl_type    type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    switch (type) {

        case CPL_TYPE_FLOAT:
            cpl_column_mul_double_to_float(column, value);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_mul_to_double(column, value);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_mul_double_to_float_complex(column, value);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_mul_to_double_complex(column, value);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Divide by a constant a numerical column.
 *
 * @param column   Target column.
 * @param value    Divisor.
 *
 * @return @c CPL_ERROR_NONE on success. If the input column is a @c NULL
 *   pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the accessed column
 *   is not numerical, a @c CPL_ERROR_INVALID_TYPE is returned. If the input 
 *   @em value is zero, a @c CPL_ERROR_DIVISION_BY_ZERO is returned.
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid flags are not
 * modified by this operation. Columns of strings or arrays are not allowed.
 */

cpl_error_code cpl_column_divide_scalar(cpl_column *column, double value)
{

    cpl_type    type = cpl_column_get_type(column);

    
    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    if (value == 0.0)
        return cpl_error_set_(CPL_ERROR_DIVISION_BY_ZERO);

    value = 1 / value;

    switch (type) {

        case CPL_TYPE_INT:
            cpl_column_mul_double_to_int(column, value);
            break;
        case CPL_TYPE_LONG:
            cpl_column_mul_double_to_long(column, value);
            break;
        case CPL_TYPE_LONG_LONG:
            cpl_column_mul_double_to_long_long(column, value);
            break;
        case CPL_TYPE_SIZE:
            cpl_column_mul_double_to_cplsize(column, value);
            break;
        case CPL_TYPE_FLOAT:
            cpl_column_mul_double_to_float(column, value);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_mul_to_double(column, value);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_mul_double_to_float_complex(column, value);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_mul_to_double_complex(column, value);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


cpl_error_code cpl_column_divide_scalar_complex(cpl_column *column, 
                                                double complex value)
{

    cpl_type    type = cpl_column_get_type(column);


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    if (value == 0.0)
        return cpl_error_set_(CPL_ERROR_DIVISION_BY_ZERO);

    value = 1 / value;

    switch (type) {

        case CPL_TYPE_FLOAT:
            cpl_column_mul_double_to_float(column, value);
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_mul_to_double(column, value);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            cpl_column_mul_double_to_float_complex(column, value);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_column_mul_to_double_complex(column, value);
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Compute the mean value of a numeric column.
 *
 * @param column   Input column.
 *
 * @return Mean value. In case of error, this is set to 0.0.
 * 
 * If the input @em column is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is
 * set. If the accessed column is not numerical, a @c CPL_ERROR_INVALID_TYPE 
 * is set. If the input column elements are all invalid, or the column has
 * zero length, a @c CPL_ERROR_DATA_NOT_FOUND is set.
 *
 * Column values marked as invalid are excluded from the computation. 
 * The column must contain at least one valid value. Columns of strings or
 * arrays are not allowed.
 */

double cpl_column_get_mean(const cpl_column *column)
{

    cpl_size    length    = cpl_column_get_size(column);
    cpl_size    nullcount = cpl_column_count_invalid_const(column);
    cpl_type    type      = cpl_column_get_type(column);
    double      mean      = 0.0;
    cpl_size    count     = length - nullcount;
    cpl_size    i;
    int        *np;

    
    if (column == NULL) {
        cpl_error_set_where_();
        return 0.0;
    }


    if (nullcount == length || length == 0) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 0.0;
    }

    np = column->null;

    switch (type) {

        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *ip++;
            else
                for (i = 0; i < length; i++, ip++, np++)
                    if (*np == 0)
                        mean += *ip;
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *lp++;
            else
                for (i = 0; i < length; i++, lp++, np++)
                    if (*np == 0)
                        mean += *lp;
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lp = column->values->ll;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *lp++;
            else
                for (i = 0; i < length; i++, lp++, np++)
                    if (*np == 0)
                        mean += *lp;
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *lp = column->values->sz;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *lp++;
            else
                for (i = 0; i < length; i++, lp++, np++)
                    if (*np == 0)
                        mean += *lp;
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *fp++;
            else
                for (i = 0; i < length; i++, fp++, np++)
                    if (*np == 0)
                        mean += *fp;
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *dp++;
            else
                for (i = 0; i < length; i++, dp++, np++)
                    if (*np == 0)
                        mean += *dp;
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;

    }

    return (mean / count);

}


/* It works with numeric types too... */

double complex cpl_column_get_mean_complex(const cpl_column *column)
{

    cpl_size        length    = cpl_column_get_size(column);
    cpl_size        nullcount = cpl_column_count_invalid_const(column);
    cpl_type        type      = cpl_column_get_type(column);
    double complex  mean      = 0.0;
    cpl_size        count     = length - nullcount;
    cpl_size        i;
    int            *np;

    
    if (column == NULL) {
        cpl_error_set_where_();
        return 0.0;
    }


    if (nullcount == length || length == 0) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 0.0;
    }

    np = column->null;

    switch (type) {

        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *ip++;
            else
                for (i = 0; i < length; i++, ip++, np++)
                    if (*np == 0)
                        mean += *ip;
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *lp++;
            else
                for (i = 0; i < length; i++, lp++, np++)
                    if (*np == 0)
                        mean += *lp;
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lp = column->values->ll;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *lp++;
            else
                for (i = 0; i < length; i++, lp++, np++)
                    if (*np == 0)
                        mean += *lp;
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *lp = column->values->sz;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *lp++;
            else
                for (i = 0; i < length; i++, lp++, np++)
                    if (*np == 0)
                        mean += *lp;
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *fp++;
            else
                for (i = 0; i < length; i++, fp++, np++)
                    if (*np == 0)
                        mean += *fp;
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *dp++;
            else
                for (i = 0; i < length; i++, dp++, np++)
                    if (*np == 0)
                        mean += *dp;
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex *cfp = column->values->cf;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *cfp++;
            else
                for (i = 0; i < length; i++, cfp++, np++)
                    if (*np == 0)
                        mean += *cfp;
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex *cdp = column->values->cd;

            if (nullcount == 0)
                for (i = 0; i < length; i++)
                    mean += *cdp++;
            else
                for (i = 0; i < length; i++, cdp++, np++)
                    if (*np == 0)
                        mean += *cdp;
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;

    }

    return (mean / count);

}


double cpl_column_get_stdev(const cpl_column *column)
{

    cpl_size    length    = cpl_column_get_size(column);
    cpl_size    nullcount = cpl_column_count_invalid_const(column);
    cpl_type    type      = cpl_column_get_type(column);
    double      stdev     = 0.0;
    double      mean;
    double      delta;
    cpl_size    count     = length - nullcount;
    cpl_size    i;
    int        *np;


    if (column == NULL) {
        cpl_error_set_where_();
        return 0.0;
    }


    if (nullcount == length || length == 0) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 0.0;
    }

    if (count == 1)
      return 0.0;

    np = column->null;

    switch (type) {

        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (nullcount == 0)
                stdev = cpl_tools_get_variancesum_int(ip, length, NULL);
            else {
                for (mean = 0, count = 0, i = 0; i < length; i++, ip++, np++) {
                    if (*np == 0) {
                        delta = *ip - mean;
                        stdev += (double)count / (count + 1) * delta * delta;
                        count++;
                        mean += delta / count;
                    }
                }
            }

            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (nullcount == 0)
                stdev = cpl_tools_get_variancesum_long(lp, length, NULL);
            else {
                for (mean = 0, count = 0, i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        delta = *lp - mean;
                        stdev += (double)count / (count + 1) * delta * delta;
                        count++;
                        mean += delta / count;
                    }
                }
            }

            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lp = column->values->ll;

            if (nullcount == 0)
                stdev = cpl_tools_get_variancesum_long_long(lp, length, NULL);
            else {
                for (mean = 0, count = 0, i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        delta = *lp - mean;
                        stdev += (double)count / (count + 1) * delta * delta;
                        count++;
                        mean += delta / count;
                    }
                }
            }

            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *lp = column->values->sz;

            if (nullcount == 0)
                stdev = cpl_tools_get_variancesum_cplsize(lp, length, NULL);
            else {
                for (mean = 0, count = 0, i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        delta = *lp - mean;
                        stdev += (double)count / (count + 1) * delta * delta;
                        count++;
                        mean += delta / count;
                    }
                }
            }

            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (nullcount == 0)
                stdev = cpl_tools_get_variancesum_float(fp, length, NULL);
            else {
                for (mean = 0, count = 0, i = 0; i < length; i++, fp++, np++) {
                    if (*np == 0) {
                        delta = *fp - mean;
                        stdev += (double)count / (count + 1) * delta * delta;
                        count++;
                        mean += delta / count;
                    }
                }
            }

            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (nullcount == 0)
                stdev = cpl_tools_get_variancesum_double(dp, length, NULL);
            else {
                for (mean = 0, count = 0, i = 0; i < length; i++, dp++, np++) {
                    if (*np == 0) {
                        delta = *dp - mean;
                        stdev += (double)count / (count + 1) * delta * delta;
                        count++;
                        mean += delta / count;
                    }
                }
            }

            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;

    }

    return sqrt(stdev / (count - 1));

}


/* 
 * @brief
 *   Get maximum value in a numerical column.
 *
 * @param column   Input column.
 *
 * @return Maximum value. In case of error, this is set to 0.0.
 *
 * If the input @em column is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is
 * set. If the accessed column is not numerical, a @c CPL_ERROR_INVALID_TYPE
 * is set. If the input column elements are all invalid, or the column has
 * zero length, a @c CPL_ERROR_DATA_NOT_FOUND is set.
 *
 * Column values marked as invalid are excluded from the search. 
 * The column must contain at least one valid value. Columns of strings or
 * arrays are not allowed.
 */

double cpl_column_get_max(const cpl_column *column)
{

    cpl_size    length    = cpl_column_get_size(column);
    cpl_size    nullcount = cpl_column_count_invalid_const(column);
    cpl_type    type      = cpl_column_get_type(column);
    int         first     = 1;
    int        *np;
    double      maximum = 0;
    cpl_size    i;


    if (column == NULL) {
        cpl_error_set_where_();
        return 0.0;
    }

    if (nullcount == length || length == 0) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 0.0;
    }

    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (length == 1)
                return *ip;
            if (nullcount == 0) {
                maximum = *ip++;
                for (i = 1; i < length; i++, ip++)
                    if (maximum < *ip)
                        maximum = *ip;
            }
            else {
                for (i = 0; i < length; i++, ip++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *ip;
                        }
                        else
                            if (maximum < *ip)
                                maximum = *ip;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (length == 1)
                return *lp;
            if (nullcount == 0) {
                maximum = *lp++;
                for (i = 1; i < length; i++, lp++)
                    if (maximum < *lp)
                        maximum = *lp;
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *lp;
                        }
                        else
                            if (maximum < *lp)
                                maximum = *lp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lp = column->values->ll;

            if (length == 1)
                return *lp;
            if (nullcount == 0) {
                maximum = *lp++;
                for (i = 1; i < length; i++, lp++)
                    if (maximum < *lp)
                        maximum = *lp;
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *lp;
                        }
                        else
                            if (maximum < *lp)
                                maximum = *lp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *lp = column->values->sz;

            if (length == 1)
                return *lp;
            if (nullcount == 0) {
                maximum = *lp++;
                for (i = 1; i < length; i++, lp++)
                    if (maximum < *lp)
                        maximum = *lp;
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *lp;
                        }
                        else
                            if (maximum < *lp)
                                maximum = *lp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (length == 1)
                return *fp;
            if (nullcount == 0) {
                maximum = *fp++;
                for (i = 1; i < length; i++, fp++)
                    if (maximum < *fp)
                        maximum = *fp;
            }
            else {
                for (i = 0; i < length; i++, fp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *fp;
                        }
                        else
                            if (maximum < *fp)
                                maximum = *fp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (length == 1)
                return *dp;
            if (nullcount == 0) {
                maximum = *dp++;
                for (i = 1; i < length; i++, dp++)
                    if (maximum < *dp)
                        maximum = *dp;
            }
            else {
                for (i = 0; i < length; i++, dp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *dp;
                        }
                        else
                            if (maximum < *dp)
                                maximum = *dp;
                    }
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return 0.0;
    }

    return maximum;

}


/* 
 * @brief
 *   Get minimum value in a numerical column.
 *
 * @param column   Input column.
 *
 * @return Minimum value. In case of error, this is set to 0.0.
 *
 * If the input @em column is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is
 * set. If the accessed column is not numerical, a @c CPL_ERROR_INVALID_TYPE
 * is set. If the input column elements are all invalid, or the column has
 * zero length, a @c CPL_ERROR_DATA_NOT_FOUND is set.
 *
 * Column values marked as invalid are excluded from the search.
 * The column must contain at least one valid value. Columns of strings or
 * arrays are not allowed.
 */ 

double cpl_column_get_min(const cpl_column *column)
{

    cpl_size    length    = cpl_column_get_size(column);
    cpl_size    nullcount = cpl_column_count_invalid_const(column);
    cpl_type    type      = cpl_column_get_type(column);
    int         first     = 1;
    int        *np;
    double      minimum = 0;
    cpl_size    i;


    if (column == NULL) {
        cpl_error_set_where_();
        return 0.0;
    }

    if (nullcount == length || length == 0) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 0.0;
    }

    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (length == 1)
                return *ip;
            if (nullcount == 0) {
                minimum = *ip++;
                for (i = 1; i < length; i++, ip++)
                    if (minimum > *ip)
                        minimum = *ip;
            }
            else {
                for (i = 0; i < length; i++, ip++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *ip;
                        }
                        else
                            if (minimum > *ip)
                                minimum = *ip;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (length == 1)
                return *lp;
            if (nullcount == 0) {
                minimum = *lp++;
                for (i = 1; i < length; i++, lp++)
                    if (minimum > *lp)
                        minimum = *lp;
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *lp;
                        }
                        else
                            if (minimum > *lp)
                                minimum = *lp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lp = column->values->ll;

            if (length == 1)
                return *lp;
            if (nullcount == 0) {
                minimum = *lp++;
                for (i = 1; i < length; i++, lp++)
                    if (minimum > *lp)
                        minimum = *lp;
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *lp;
                        }
                        else
                            if (minimum > *lp)
                                minimum = *lp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *lp = column->values->sz;

            if (length == 1)
                return *lp;
            if (nullcount == 0) {
                minimum = *lp++;
                for (i = 1; i < length; i++, lp++)
                    if (minimum > *lp)
                        minimum = *lp;
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *lp;
                        }
                        else
                            if (minimum > *lp)
                                minimum = *lp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (length == 1)
                return *fp;
            if (nullcount == 0) {
                minimum = *fp++;
                for (i = 1; i < length; i++, fp++)
                    if (minimum > *fp)
                        minimum = *fp;
            }
            else {
                for (i = 0; i < length; i++, fp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *fp;
                        }
                        else
                            if (minimum > *fp)
                                minimum = *fp;
                    }
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (length == 1)
                return *dp;
            if (nullcount == 0) {
                minimum = *dp++;
                for (i = 1; i < length; i++, dp++)
                    if (minimum > *dp)
                        minimum = *dp;
            }
            else {
                for (i = 0; i < length; i++, dp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *dp;
                        }
                        else
                            if (minimum > *dp)
                                minimum = *dp;
                    }
                }
            }
            break;
        }
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return 0.0;
    }

    return minimum;

}


/* 
 * @brief
 *   Get position of maximum in a numerical column.
 *
 * @param column   Input column.
 * @param indx     Returned position of maximum value.
 *
 * @return @c CPL_ERROR_NONE on success. If any input argument is a 
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the 
 *   accessed column is not numerical, a @c CPL_ERROR_INVALID_TYPE
 *   is returned. If the input column elements are all invalid, or 
 *   the column has zero length, a @c CPL_ERROR_DATA_NOT_FOUND is 
 *   returned.
 *
 * Column values marked as invalid are excluded from the search.
 * The @em indx argument will be assigned the position of the maximum 
 * value. Indexes are counted starting from 0. If more than one column 
 * element correspond to the max value, the position with the lowest 
 * indx is returned. In case of error, @em indx is set to zero. 
 * Columns of strings or arrays are not allowed.
 */ 
    
cpl_error_code cpl_column_get_maxpos(const cpl_column *column, cpl_size *indx)
{

    cpl_size    length    = cpl_column_get_size(column);
    cpl_size    nullcount = cpl_column_count_invalid_const(column);
    cpl_type    type      = cpl_column_get_type(column);
    int         first     = 1;
    int        *np;
    double      maximum = 0;
    cpl_size    i;


    if (column == NULL || indx == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (nullcount == length || length == 0)
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    *indx = 0;

    if (length == 1)
        return CPL_ERROR_NONE;

    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (nullcount == 0) {
                maximum = *ip++;
                for (i = 1; i < length; i++, ip++) {
                    if (maximum < *ip) {
                        maximum = *ip;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, ip++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *ip;
                            *indx = i;
                        }
                        else {
                            if (maximum < *ip) {
                                maximum = *ip;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (nullcount == 0) {
                maximum = *lp++;
                for (i = 1; i < length; i++, lp++) {
                    if (maximum < *lp) {
                        maximum = *lp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *lp;
                            *indx = i;
                        }
                        else {
                            if (maximum < *lp) {
                                maximum = *lp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lp = column->values->ll;

            if (nullcount == 0) {
                maximum = *lp++;
                for (i = 1; i < length; i++, lp++) {
                    if (maximum < *lp) {
                        maximum = *lp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *lp;
                            *indx = i;
                        }
                        else {
                            if (maximum < *lp) {
                                maximum = *lp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *lp = column->values->sz;

            if (nullcount == 0) {
                maximum = *lp++;
                for (i = 1; i < length; i++, lp++) {
                    if (maximum < *lp) {
                        maximum = *lp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *lp;
                            *indx = i;
                        }
                        else {
                            if (maximum < *lp) {
                                maximum = *lp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (nullcount == 0) {
                maximum = *fp++;
                for (i = 1; i < length; i++, fp++) {
                    if (maximum < *fp) {
                        maximum = *fp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, fp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *fp;
                            *indx = i;
                        }
                        else {
                            if (maximum < *fp) {
                                maximum = *fp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (nullcount == 0) {
                maximum = *dp++;
                for (i = 1; i < length; i++, dp++) {
                    if (maximum < *dp) {
                        maximum = *dp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, dp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            maximum = *dp;
                            *indx = i;
                        }
                        else {
                            if (maximum < *dp) {
                                maximum = *dp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Get position of minimum in a numerical column.
 *
 * @param column   Input column.
 * @param indx     Returned position of minimum value.
 *
 * @return @c CPL_ERROR_NONE on success. If any input argument is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not numerical, a @c CPL_ERROR_INVALID_TYPE
 *   is returned. If the input column elements are all invalid, or
 *   the column has zero length, a @c CPL_ERROR_DATA_NOT_FOUND is
 *   returned.
 *
 * Column values marked as invalid are excluded from the search.
 * The @em indx argument will be assigned the position of the minimum
 * value. Indexes are counted starting from 0. If more than one column
 * element correspond to the min value, the position with the lowest
 * index is returned. In case of error, @em indx is set to zero.
 * Columns of strings or arrays are not allowed.
 */

cpl_error_code cpl_column_get_minpos(const cpl_column *column, cpl_size *indx)
{

    cpl_size    length    = cpl_column_get_size(column);
    cpl_size    nullcount = cpl_column_count_invalid_const(column);
    cpl_type    type      = cpl_column_get_type(column);
    int         first     = 1;
    int        *np;
    double      minimum = 0;
    cpl_size    i;


    if (column == NULL || indx == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (nullcount == length || length == 0)
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    *indx = 0;

    if (length == 1)
        return CPL_ERROR_NONE;

    np = column->null;

    switch (type) {
        case CPL_TYPE_INT:
        {
            int *ip = column->values->i;

            if (nullcount == 0) {
                minimum = *ip++;
                for (i = 1; i < length; i++, ip++) {
                    if (minimum > *ip) {
                        minimum = *ip;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, ip++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *ip;
                            *indx = i;
                        }
                        else {
                            if (minimum > *ip) {
                                minimum = *ip;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            long *lp = column->values->l;

            if (nullcount == 0) {
                minimum = *lp++;
                for (i = 1; i < length; i++, lp++) {
                    if (minimum > *lp) {
                        minimum = *lp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *lp;
                            *indx = i;
                        }
                        else {
                            if (minimum > *lp) {
                                minimum = *lp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            long long *lp = column->values->ll;

            if (nullcount == 0) {
                minimum = *lp++;
                for (i = 1; i < length; i++, lp++) {
                    if (minimum > *lp) {
                        minimum = *lp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *lp;
                            *indx = i;
                        }
                        else {
                            if (minimum > *lp) {
                                minimum = *lp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            cpl_size *lp = column->values->sz;

            if (nullcount == 0) {
                minimum = *lp++;
                for (i = 1; i < length; i++, lp++) {
                    if (minimum > *lp) {
                        minimum = *lp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, lp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *lp;
                            *indx = i;
                        }
                        else {
                            if (minimum > *lp) {
                                minimum = *lp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            float *fp = column->values->f;

            if (nullcount == 0) {
                minimum = *fp++;
                for (i = 1; i < length; i++, fp++) {
                    if (minimum > *fp) {
                        minimum = *fp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, fp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *fp;
                            *indx = i;
                        }
                        else {
                            if (minimum > *fp) {
                                minimum = *fp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            double *dp = column->values->d;

            if (nullcount == 0) {
                minimum = *dp++;
                for (i = 1; i < length; i++, dp++) {
                    if (minimum > *dp) {
                        minimum = *dp;
                        *indx = i;
                    }
                }
            }
            else {
                for (i = 0; i < length; i++, dp++, np++) {
                    if (*np == 0) {
                        if (first) {
                            first = 0;
                            minimum = *dp;
                            *indx = i;
                        }
                        else {
                            if (minimum > *dp) {
                                minimum = *dp;
                                *indx = i;
                            }
                        }
                    }
                }
            }
            break;
        }
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Find the median of an integer column.
 *
 * @param column   Input column.
 *
 * @return Median.
 *
 * Invalid column values are excluded from the search. 
 * The column must contain at least one valid value.
 * In case of failure, zero is returned. Columns of strings or
 * arrays are not allowed.
 */

static int cpl_column_median_int(const cpl_column *column)
{

    cpl_size length    = cpl_column_get_size(column);
    cpl_size nullcount = cpl_column_count_invalid_const(column);
    int      median    = 0;
    cpl_size i, n;
    int     *np;
    int     *ip;
    int     *cip;
    int     *copybuf;


    n = length - nullcount;
    cip = copybuf = cpl_malloc(n * sizeof(int));
    ip = column->values->i;
    np = column->null;

    if (nullcount) {
        for (i = 0; i < length; i++, np++, ip++) {
            if (*np == 0) {
                *cip = *ip;
                cip++;
            }
        }
    }
    else
        memcpy(copybuf, ip, length * sizeof(int));

    median = cpl_tools_get_median_int(copybuf, n);
    cpl_free(copybuf);

    return median;

}


/*
 * @brief
 *   Find the median of a long integer column.
 *
 * @param column   Input column.
 *
 * @return Median.
 *
 * Invalid column values are excluded from the search.
 * The column must contain at least one valid value.
 * In case of failure, zero is returned. Columns of strings or
 * arrays are not allowed.
 */

static long cpl_column_median_long(const cpl_column *column)
{

    cpl_size length    = cpl_column_get_size(column);
    cpl_size nullcount = cpl_column_count_invalid_const(column);
    long     median    = 0;
    cpl_size i, n;
    int     *np;
    long    *lp;
    long    *clp;
    long    *copybuf;


    n = length - nullcount;
    clp = copybuf = cpl_malloc(n * sizeof(long));
    lp = column->values->l;
    np = column->null;

    if (nullcount) {
        for (i = 0; i < length; i++, np++, lp++) {
            if (*np == 0) {
                *clp = *lp;
                clp++;
            }
        }
    }
    else
        memcpy(copybuf, lp, length * sizeof(long));

    median = cpl_tools_get_median_long(copybuf, n);
    cpl_free(copybuf);

    return median;

}


/*
 * @brief
 *   Find the median of a long long integer column.
 *
 * @param column   Input column.
 *
 * @return Median.
 *
 * Invalid column values are excluded from the search.
 * The column must contain at least one valid value.
 * In case of failure, zero is returned. Columns of strings or
 * arrays are not allowed.
 */

static long long cpl_column_median_long_long(const cpl_column *column)
{

    cpl_size length    = cpl_column_get_size(column);
    cpl_size nullcount = cpl_column_count_invalid_const(column);
    long long   median    = 0;
    cpl_size i, n;
    int        *np;
    long long  *lp;
    long long  *clp;
    long long  *copybuf;


    n = length - nullcount;
    clp = copybuf = cpl_malloc(n * sizeof(long long));
    lp = column->values->ll;
    np = column->null;

    if (nullcount) {
        for (i = 0; i < length; i++, np++, lp++) {
            if (*np == 0) {
                *clp = *lp;
                clp++;
            }
        }
    }
    else
        memcpy(copybuf, lp, length * sizeof(long long));

    median = cpl_tools_get_median_long_long(copybuf, n);
    cpl_free(copybuf);

    return median;

}


/*
 * @brief
 *   Find the median of a cpl_size column.
 *
 * @param column   Input column.
 *
 * @return Median.
 *
 * Invalid column values are excluded from the search.
 * The column must contain at least one valid value.
 * In case of failure, zero is returned. Columns of strings or
 * arrays are not allowed.
 */

static cpl_size cpl_column_median_cplsize(const cpl_column *column)
{

    cpl_size length    = cpl_column_get_size(column);
    cpl_size nullcount = cpl_column_count_invalid_const(column);
    cpl_size    median    = 0;
    cpl_size i, n;
    int        *np;
    cpl_size   *lp;
    cpl_size   *clp;
    cpl_size   *copybuf;


    n = length - nullcount;
    clp = copybuf = cpl_malloc(n * sizeof(cpl_size));
    lp = column->values->sz;
    np = column->null;

    if (nullcount) {
        for (i = 0; i < length; i++, np++, lp++) {
            if (*np == 0) {
                *clp = *lp;
                clp++;
            }
        }
    }
    else
        memcpy(copybuf, lp, length * sizeof(cpl_size));

    median = cpl_tools_get_median_cplsize(copybuf, n);
    cpl_free(copybuf);

    return median;

}


/*
 * @brief
 *   Find the median of a float column.
 *
 * @param column   Input column.
 *
 * @return Median.
 *
 * Column values flagged as NULLs are excluded from the search. 
 * The column must contain at least one non-NULL value.
 * In case of failure, zero is returned. Columns of strings or
 * arrays are not allowed.
 */

static float cpl_column_median_float(const cpl_column *column)
{

    cpl_size length    = cpl_column_get_size(column);
    cpl_size nullcount = cpl_column_count_invalid_const(column);
    float    median    = 0;
    cpl_size i, n;
    int     *np;
    float   *fp;
    float   *cfp;
    float   *copybuf;


    n = length - nullcount;
    cfp = copybuf = cpl_malloc(n * sizeof(float));
    fp = column->values->f;
    np = column->null;

    if (nullcount) {
        for (i = 0; i < length; i++, np++, fp++) {
            if (*np == 0) {
                *cfp = *fp;
                cfp++;
            }
        }
    }
    else
        memcpy(copybuf, fp, length * sizeof(float));

    median = cpl_tools_get_median_float(copybuf, n);
    cpl_free(copybuf);

    return median;

}


/*
 * @brief
 *   Find the median of a double column.
 *
 * @param column   Input column.
 *
 * @return Median.
 *
 * Column values flagged as NULLs are excluded from the search. 
 * The column must contain at least one non-NULL value.
 * In case of failure, zero is returned.
 */

static double cpl_column_median_double(const cpl_column *column)
{

    cpl_size length    = cpl_column_get_size(column);
    cpl_size nullcount = cpl_column_count_invalid_const(column);
    double   median    = 0;
    cpl_size i, n;
    int     *np;
    double  *dp;
    double  *cdp;
    double  *copybuf;


    n = length - nullcount;
    cdp = copybuf = cpl_malloc(n * sizeof(double));
    dp = column->values->d;
    np = column->null;

    if (nullcount) {
        for (i = 0; i < length; i++, np++, dp++) {
            if (*np == 0) {
                *cdp = *dp;
                cdp++;
            }
        }
    }
    else
        memcpy(copybuf, dp, length * sizeof(double));

    median = cpl_tools_get_median_double(copybuf, n);
    cpl_free(copybuf);

    return median;

}


/* 
 * @brief
 *   Find the median of a numerical column.
 *
 * @param column   Input column.
 *
 * @return Median. In case of error, 0.0 is always returned.
 *
 * If the input column is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is set.
 * If the accessed column is not numerical, a @c CPL_ERROR_INVALID_TYPE is 
 * set. If the input column elements are all invalid, or the column has zero 
 * length, a @c CPL_ERROR_DATA_NOT_FOUND is returned. Invalid column elements 
 * are excluded from the computation. The column must contain at least one 
 * valid elements. Columns of strings or arrays are not allowed.
 */

double cpl_column_get_median(const cpl_column *column)
{

    cpl_type    type      = cpl_column_get_type(column);
    cpl_size    length    = cpl_column_get_size(column);
    cpl_size    nullcount = cpl_column_count_invalid_const(column);


    if (column == NULL) {
        cpl_error_set_where_();
        return 0.0;
    }

    if (nullcount == length || length == 0) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 0.0;
    }

    switch (type) {
        case CPL_TYPE_INT:
            return (double)cpl_column_median_int(column);
        case CPL_TYPE_LONG:
            return (double)cpl_column_median_long(column);
        case CPL_TYPE_LONG_LONG:
            return (double)cpl_column_median_long_long(column);
        case CPL_TYPE_SIZE:
            return (double)cpl_column_median_cplsize(column);
        case CPL_TYPE_FLOAT:
            return (double)cpl_column_median_float(column);
        case CPL_TYPE_DOUBLE:
            return cpl_column_median_double(column);
        default:
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            break;
    }

    return 0.0;

}


/*
 * @brief
 *   Shift numeric column elements.
 *
 * @param column   Input column.
 * @param shift    Shift column values by so many elements.
 *
 * @return @c CPL_ERROR_NONE on success. If the input @em column is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not numerical, a @c CPL_ERROR_INVALID_TYPE
 *   is returned. The specified shift must be in module less than 
 *   the column length, or a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * All column elements are shifted by the specified amount. If @em shift 
 * is positive, all elements will be moved toward the bottom of the column, 
 * otherwise toward its top. The column elements that are left undefined 
 * at either end of the column are marked as invalid. Columns of strings or
 * arrays are not allowed.
 */

cpl_error_code cpl_column_shift(cpl_column *column, cpl_size shift)
{
    
    cpl_type        type   = cpl_column_get_type(column);
    cpl_size        length = cpl_column_get_size(column);
    int             status;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    /*
     *  Allowed column types:
     */

    switch (type) {
        case CPL_TYPE_INT:
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG:
        case CPL_TYPE_SIZE:
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

#if defined(HAVE_LLABS) && defined(HAVE_DECL_LLABS)

    if (llabs(shift) >= length)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

#else

    if ((shift < 0 ? -shift : shift) >= length)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

#endif

    if (shift == 0)
        return CPL_ERROR_NONE;

    if (column->nullcount == length)
        return CPL_ERROR_NONE;


    /*
     *  Invalid flags buffer handling first (because it may fail...)
     */

    if (column->nullcount) {

        if (shift > 0) {

            cpl_size j = length - shift;
            cpl_column_flag *np1 = column->null + length;
            cpl_column_flag *np2 = np1 - shift;

            while (j--)
                *--np1 = *--np2;
        }
        else {

            cpl_size j = -shift;
            cpl_column_flag *np1 = column->null;
            cpl_column_flag *np2 = np1 - shift;

            while(j++ < length)
                *np1++ = *np2++;
        }

    }


   /*
    *  Invalidate the leftovers
    */

    if (shift > 0)
        status = cpl_column_fill_invalid(column, 0, shift);
    else
        status = cpl_column_fill_invalid(column, length + shift, -shift);

    if (status)
        return status;


   /*
    *  Now shift the data.
    */

    switch (type) {

        case CPL_TYPE_INT:
        {
            if (shift > 0) {

                cpl_size  j   = length - shift;
                int      *ip1 = column->values->i + length;
                int      *ip2 = ip1 - shift;

                while (j--)
                    *--ip1 = *--ip2;
            }
            else {

                cpl_size  j   = -shift;
                int      *ip1 = column->values->i;
                int      *ip2 = ip1 - shift;

                while(j++ < length)
                    *ip1++ = *ip2++;
            }
            break;
        }
        case CPL_TYPE_LONG:
        {
            if (shift > 0) {

                cpl_size  j   = length - shift;
                long     *lp1 = column->values->l + length;
                long     *lp2 = lp1 - shift;

                while (j--)
                    *--lp1 = *--lp2;
            }
            else {

                cpl_size  j    = -shift;
                long     *lp1  = column->values->l;
                long     *lp2  = lp1 - shift;

                while(j++ < length)
                    *lp1++ = *lp2++;
            }
            break;
        }
        case CPL_TYPE_LONG_LONG:
        {
            if (shift > 0) {

                cpl_size   j   = length - shift;
                long long *lp1 = column->values->ll + length;
                long long *lp2 = lp1 - shift;

                while (j--)
                    *--lp1 = *--lp2;
            }
            else {

                cpl_size   j    = -shift;
                long long *lp1  = column->values->ll;
                long long *lp2  = lp1 - shift;

                while(j++ < length)
                    *lp1++ = *lp2++;
            }
            break;
        }
        case CPL_TYPE_SIZE:
        {
            if (shift > 0) {

                cpl_size   j   = length - shift;
                cpl_size *lp1 = column->values->sz + length;
                cpl_size *lp2 = lp1 - shift;

                while (j--)
                    *--lp1 = *--lp2;
            }
            else {

                cpl_size   j    = -shift;
                cpl_size *lp1  = column->values->sz;
                cpl_size *lp2  = lp1 - shift;

                while(j++ < length)
                    *lp1++ = *lp2++;
            }
            break;
        }
        case CPL_TYPE_FLOAT:
        {
            if (shift > 0) {

                cpl_size  j   = length - shift;
                float    *fp1 = column->values->f + length;
                float    *fp2 = fp1 - shift;

                while (j--)
                    *--fp1 = *--fp2;
            }
            else {

                cpl_size  j   = -shift;
                float    *fp1 = column->values->f;
                float    *fp2 = fp1 - shift;

                while(j++ < length)
                    *fp1++ = *fp2++;
            }
            break;
        }
        case CPL_TYPE_DOUBLE:
        {
            if (shift > 0) {

                cpl_size  j   = length - shift;
                double   *dp1 = column->values->d + length;
                double   *dp2 = dp1 - shift;

                while (j--)
                    *--dp1 = *--dp2;
            }
            else {

                cpl_size  j   = -shift;
                double   *dp1 = column->values->d;
                double   *dp2 = dp1 - shift;

                while(j++ < length)
                    *dp1++ = *dp2++;
            }
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        {
            if (shift > 0) {

                cpl_size       j    = length - shift;
                float complex *cfp1 = column->values->cf + length;
                float complex *cfp2 = cfp1 - shift;

                while (j--)
                    *--cfp1 = *--cfp2;
            }
            else {

                cpl_size       j    = -shift;
                float complex *cfp1 = column->values->cf;
                float complex *cfp2 = cfp1 - shift;

                while(j++ < length)
                    *cfp1++ = *cfp2++;
            }
            break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            if (shift > 0) {

                cpl_size        j    = length - shift;
                double complex *cdp1 = column->values->cd + length;
                double complex *cdp2 = cdp1 - shift;

                while (j--)
                    *--cdp1 = *--cdp2;
            }
            else {

                cpl_size        j    = -shift;
                double complex *cdp1 = column->values->cd;
                double complex *cdp2 = cdp1 - shift;

                while(j++ < length)
                    *cdp1++ = *cdp2++;
            }
            break;
        }
        default:
            break;
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Assign to invalid @em integer column elements a numeric code.
 *
 * @param column   Input column.
 * @param code     Code to write at invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.  If the input @em column is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not @em integer or arrays of integers, 
 *   a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *
 * In general the column elements that are marked as invalid
 * may contain any value, that should not be given any meaning
 * whatsoever. In order to export the column data (using a call
 * to @c cpl_column_get_data_int() ) to procedures that are external
 * to the CPL column system, it may turn out to be appropriate
 * assigning to all the invalid elements a special code value.
 * This code value will supposedly be recognized and handled
 * properly by the foreign method. Note that only existing invalid
 * elements will be coded as indicated: new invalid column elements 
 * would still have their actual values undefined. Also, any further 
 * processing of the column would not take care of maintaining the 
 * assigned code to a given invalid column element: therefore the 
 * code should be set just before it is actually needed.
 *
 * @note
 *   Assigning a code to an invalid element doesn't make it valid.
 */

cpl_error_code cpl_column_fill_invalid_int(cpl_column *column, int code)
{

    const cpl_type type   = cpl_column_get_type(column);
    const cpl_size length = cpl_column_get_size(column);
    size_t                  i;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_INT))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {
        cpl_array **array = cpl_column_get_data_array(column);

        for (i = 0; i < (size_t)length; i++) {
            cpl_column *acolumn;
            if (array[i] == NULL)
                array[i] = cpl_array_new(column->depth, CPL_TYPE_INT);
            acolumn = cpl_array_get_column(array[i]);
            cpl_column_fill_invalid_int(acolumn, code);
        }
    } else if (column->nullcount == length) {
        cpl_column_fill_int(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
    } else if (column->nullcount != 0) {
        const int *np = cpl_column_get_data_invalid_const(column);
        int       *ip = cpl_column_get_data_int(column);

        for (i = 0; i < (size_t)length; i++)
            if (np[i] == 1)
                ip[i] = code;
    }

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Assign to invalid @em long @em integer column elements a numeric code.
 *
 * @param column   Input column.
 * @param code     Code to write at invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.  If the input @em column is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not @em long @em integer or arrays of long integers,
 *   a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *
 * In general the column elements that are marked as invalid
 * may contain any value, that should not be given any meaning
 * whatsoever. In order to export the column data (using a call
 * to @c cpl_column_get_data_long() ) to procedures that are external
 * to the CPL column system, it may turn out to be appropriate
 * assigning to all the invalid elements a special code value.
 * This code value will supposedly be recognized and handled
 * properly by the foreign method. Note that only existing invalid
 * elements will be coded as indicated: new invalid column elements
 * would still have their actual values undefined. Also, any further
 * processing of the column would not take care of maintaining the
 * assigned code to a given invalid column element: therefore the
 * code should be set just before it is actually needed.
 *
 * @note
 *   Assigning a code to an invalid element doesn't make it valid.
 */

cpl_error_code cpl_column_fill_invalid_long(cpl_column *column, long code)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    int        *np;
    long       *lp;
    cpl_size    i;
    cpl_column *acolumn;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_LONG))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {

        cpl_array **array = cpl_column_get_data_array(column);

        while (length--) {
            if (array[length] == NULL)
                array[length] = cpl_array_new(column->depth, CPL_TYPE_LONG);
            acolumn = cpl_array_get_column(array[length]);
            cpl_column_fill_invalid_long(acolumn, code);
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == 0)
        return CPL_ERROR_NONE;

    if (column->nullcount == length) {
        cpl_column_fill_long(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
        return CPL_ERROR_NONE;
    }

    lp = cpl_column_get_data_long(column);
    np = cpl_column_get_data_invalid(column);

    for (i = 0; i < length; i++, np++, lp++)
        if (*np == 1)
            *lp = code;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Assign to invalid @m long @em long @em integer column elements a
 *   numeric code.
 *
 * @param column   Input column.
 * @param code     Code to write at invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.  If the input @em column is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not @em long @em long @em integer or arrays of
 *   long long integers, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *
 * In general the column elements that are marked as invalid
 * may contain any value, that should not be given any meaning
 * whatsoever. In order to export the column data (using a call
 * to @c cpl_column_get_data_long_long() ) to procedures that are
 * external to the CPL column system, it may turn out to be appropriate
 * assigning to all the invalid elements a special code value.
 * This code value will supposedly be recognized and handled
 * properly by the foreign method. Note that only existing invalid
 * elements will be coded as indicated: new invalid column elements
 * would still have their actual values undefined. Also, any further
 * processing of the column would not take care of maintaining the
 * assigned code to a given invalid column element: therefore the
 * code should be set just before it is actually needed.
 *
 * @note
 *   Assigning a code to an invalid element doesn't make it valid.
 */

cpl_error_code cpl_column_fill_invalid_long_long(cpl_column *column,
                                                 long long code)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    int        *np;
    long long  *lp;
    cpl_size    i;
    cpl_column *acolumn;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_LONG_LONG))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {

        cpl_array **array = cpl_column_get_data_array(column);

        while (length--) {
            if (array[length] == NULL)
                array[length] = cpl_array_new(column->depth, CPL_TYPE_LONG_LONG);
            acolumn = cpl_array_get_column(array[length]);
            cpl_column_fill_invalid_long_long(acolumn, code);
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == 0)
        return CPL_ERROR_NONE;

    if (column->nullcount == length) {
        cpl_column_fill_long_long(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
        return CPL_ERROR_NONE;
    }

    lp = cpl_column_get_data_long_long(column);
    np = cpl_column_get_data_invalid(column);

    for (i = 0; i < length; i++, np++, lp++)
        if (*np == 1)
            *lp = code;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Assign to invalid @em cpl_size column elements a numeric code.
 *
 * @param column   Input column.
 * @param code     Code to write at invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.  If the input @em column is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not @em cpl_size or arrays of cpl_size,
 *   a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *
 * In general the column elements that are marked as invalid
 * may contain any value, that should not be given any meaning
 * whatsoever. In order to export the column data (using a call
 * to @c cpl_column_get_data_cplsize() ) to procedures that are
 * external to the CPL column system, it may turn out to be appropriate
 * assigning to all the invalid elements a special code value.
 * This code value will supposedly be recognized and handled
 * properly by the foreign method. Note that only existing invalid
 * elements will be coded as indicated: new invalid column elements
 * would still have their actual values undefined. Also, any further
 * processing of the column would not take care of maintaining the
 * assigned code to a given invalid column element: therefore the
 * code should be set just before it is actually needed.
 *
 * @note
 *   Assigning a code to an invalid element doesn't make it valid.
 */

cpl_error_code cpl_column_fill_invalid_cplsize(cpl_column *column,
                                               cpl_size code)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    int        *np;
    cpl_size   *lp;
    cpl_size    i;
    cpl_column *acolumn;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_SIZE))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {

        cpl_array **array = cpl_column_get_data_array(column);

        while (length--) {
            if (array[length] == NULL)
                array[length] = cpl_array_new(column->depth, CPL_TYPE_SIZE);
            acolumn = cpl_array_get_column(array[length]);
            cpl_column_fill_invalid_cplsize(acolumn, code);
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == 0)
        return CPL_ERROR_NONE;

    if (column->nullcount == length) {
        cpl_column_fill_cplsize(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
        return CPL_ERROR_NONE;
    }

    lp = cpl_column_get_data_cplsize(column);
    np = cpl_column_get_data_invalid(column);

    for (i = 0; i < length; i++, np++, lp++)
        if (*np == 1)
            *lp = code;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Assign to invalid @em float column elements a numeric code.
 *
 * @param column   Input column.
 * @param code     Code to write at invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.  If the input @em column is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not @em float or array of floats, 
 *   a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *
 * In general the column elements that are marked as invalid
 * may contain any value, that should not be given any meaning
 * whatsoever. In order to export the column data (using a call
 * to @c cpl_column_get_data_float() ) to procedures that are external
 * to the CPL column system, it may turn out to be appropriate
 * assigning to all the invalid elements a special code value.
 * This code value will supposedly be recognized and handled
 * properly by the foreign method. Note that only existing invalid
 * elements will be coded as indicated: new invalid column elements
 * would still have their actual values undefined. Also, any further
 * processing of the column would not take care of maintaining the
 * assigned code to a given invalid column element: therefore the
 * code should be set just before it is actually needed.
 *
 * @note
 *   Assigning a code to an invalid element doesn't make it valid.
 */

cpl_error_code cpl_column_fill_invalid_float(cpl_column *column, float code)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    int        *np;
    float      *fp;
    cpl_size    i;
    cpl_column *acolumn;
    cpl_array **array;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_FLOAT))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {

        array = cpl_column_get_data_array(column);

        while (length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column(array[length]);
                cpl_column_fill_invalid_float(acolumn, code);
            }
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == 0)
        return 0;

    if (column->nullcount == length) {
        cpl_column_fill_float(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
        return CPL_ERROR_NONE;
    }

    fp = cpl_column_get_data_float(column);
    np = cpl_column_get_data_invalid(column);

    for (i = 0; i < length; i++, np++, fp++)
        if (*np == 1)
            *fp = code;

    return CPL_ERROR_NONE;

}


cpl_error_code cpl_column_fill_invalid_float_complex(cpl_column *column, 
                                                     float complex code)
{

    cpl_type       type   = cpl_column_get_type(column);
    cpl_size       length = cpl_column_get_size(column);
    int           *np;
    float complex *fp;
    cpl_size       i;
    cpl_column    *acolumn;
    cpl_array    **array;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_FLOAT_COMPLEX))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {

        array = cpl_column_get_data_array(column);

        while (length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column(array[length]);
                cpl_column_fill_invalid_float_complex(acolumn, code);
            }
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == 0)
        return 0;

    if (column->nullcount == length) {
        cpl_column_fill_float_complex(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
        return CPL_ERROR_NONE;
    }

    fp = cpl_column_get_data_float_complex(column);
    np = cpl_column_get_data_invalid(column);

    for (i = 0; i < length; i++, np++, fp++)
        if (*np == 1)
            *fp = code;

    return CPL_ERROR_NONE;

}


/* 
 * @brief
 *   Assign to invalid @em double column elements a numeric code.
 *
 * @param column   Input column.
 * @param code     Code to write at invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.  If the input @em column is a
 *   @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is returned. If the
 *   accessed column is not @em doubleor array of doubles, 
 *   a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *
 * In general the column elements that are marked as invalid
 * may contain any value, that should not be given any meaning
 * whatsoever. In order to export the column data (using a call
 * to @c cpl_column_get_data_double() ) to procedures that are external
 * to the CPL column system, it may turn out to be appropriate
 * assigning to all the invalid elements a special code value.
 * This code value will supposedly be recognized and handled
 * properly by the foreign method. Note that only existing invalid
 * elements will be coded as indicated: new invalid column elements
 * would still have their actual values undefined. Also, any further
 * processing of the column would not take care of maintaining the
 * assigned code to a given invalid column element: therefore the
 * code should be set just before it is actually needed.
 *
 * @note
 *   Assigning a code to an invalid element doesn't make it valid.
 */

cpl_error_code cpl_column_fill_invalid_double(cpl_column *column, double code)
{

    cpl_type    type   = cpl_column_get_type(column);
    cpl_size    length = cpl_column_get_size(column);
    int        *np;
    double     *dp;
    cpl_size    i;
    cpl_column *acolumn;
    cpl_array **array;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_DOUBLE))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {

        array = cpl_column_get_data_array(column);

        while (length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column(array[length]);
                cpl_column_fill_invalid_double(acolumn, code);
            }
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == 0)
        return 0;

    if (column->nullcount == length) {
        cpl_column_fill_double(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
        return CPL_ERROR_NONE;
    }

    dp = cpl_column_get_data_double(column);
    np = cpl_column_get_data_invalid(column);

    for (i = 0; i < length; i++, np++, dp++)
        if (*np == 1)
            *dp = code;

    return CPL_ERROR_NONE;

}


cpl_error_code cpl_column_fill_invalid_double_complex(cpl_column *column, 
                                                      double complex code)
{

    cpl_type        type   = cpl_column_get_type(column);
    cpl_size        length = cpl_column_get_size(column);
    int            *np;
    double complex *dp;
    cpl_size        i;
    cpl_column     *acolumn;
    cpl_array     **array;


    if (column == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (!(type & CPL_TYPE_DOUBLE_COMPLEX))
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);

    if (type & CPL_TYPE_POINTER) {

        array = cpl_column_get_data_array(column);

        while (length--) {
            if (array[length]) {
                acolumn = cpl_array_get_column(array[length]);
                cpl_column_fill_invalid_double_complex(acolumn, code);
            }
        }
        return CPL_ERROR_NONE;
    }

    if (column->nullcount == 0)
        return 0;

    if (column->nullcount == length) {
        cpl_column_fill_double_complex(column, 0, length, code);
        column->nullcount = length;    /* Restore: they are still all NULL! */
        return CPL_ERROR_NONE;
    }

    dp = cpl_column_get_data_double_complex(column);
    np = cpl_column_get_data_invalid(column);

    for (i = 0; i < length; i++, np++, dp++)
        if (*np == 1)
            *dp = code;

    return CPL_ERROR_NONE;

}

/*
 * @internal
 * @brief Unset all NULL flags
 * @param self  Column to be accessed.
 * @return Nothing.
 * @see cpl_column_unset_null_segment(), cpl_array_init_perm()
 */
void cpl_column_unset_null_all(cpl_column *self)
{
    const cpl_size n = (size_t)cpl_column_get_size_(self);

    (void)cpl_column_unset_null_segment(self, 0, n);
}

/* @}*/
