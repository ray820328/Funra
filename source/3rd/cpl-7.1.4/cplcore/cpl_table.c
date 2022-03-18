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

#define ERASE_WCS_REGEXP "WCSAXES|WCSNAME|(PC|CD|PV|PS)[0-9]+_[0-9]+|"    \
                         "C(RVAL|RPIX|DELT|TYPE|UNIT|RDER|SYER)[0-9]+"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <regex.h>
#include <complex.h>
#include <assert.h>

#include <fitsio.h>

#include <cxmessages.h>
#include <cxstring.h>
#include <cxstrutils.h>
#include <cxutils.h>

#include "cpl_array.h"
#include "cpl_array_impl.h"
#include "cpl_column.h"
#include "cpl_error_impl.h"
#include "cpl_errorstate.h"
#include "cpl_memory.h"
#include "cpl_propertylist_impl.h"
#include "cpl_tools.h"
#include "cpl_type.h"
#include "cpl_msg.h"
#include "cpl_math_const.h"
#include "cpl_io_fits.h"
#include "cpl_table.h"


/**
 * @defgroup cpl_table Tables
 *
 * This module provides functions to create, use, and destroy
 * a @em cpl_table. A @em cpl_table is made of columns, and
 * a column consists of an array of elements of a given type.
 * Currently three numerical types are supported, @c CPL_TYPE_INT,
 * @c CPL_TYPE_FLOAT, and @c CPL_TYPE_DOUBLE, plus a type indicating
 * columns containing character strings, @c CPL_TYPE_STRING.
 * Moreover, it is possible to define columns of arrays, i.e.
 * columns whose elements are arrays of all the basic types
 * listed above. Within the same column all arrays must have 
 * the same type and the same length.
 *
 * A table column is accessed by specifying its name. The ordering
 * of the columns within a table is undefined: a @em cpl_table is
 * not an n-tuple of columns, but just a set of columns. The N
 * elements of a column are counted from 0 to N-1, with element
 * 0 on top. The set of all the table columns elements with the
 * same index constitutes a table row, and table rows are counted
 * according to the same convention. It is possible to flag each
 * @em cpl_table row as "selected" or "unselected", and each column
 * element as "valid" or "invalid". Selecting table rows is mainly
 * a way to extract just those table parts fulfilling any given
 * condition, while invalidating column elements is a way to
 * exclude such elements from any computation. A @em cpl_table
 * is created with all rows selected, and a column is created with
 * all elements invalidated.
 *
 * @note
 *   The @em cpl_table pointers specified in the argument list of all
 *   the @em cpl_table functions must point to valid objects: these
 *   functions do not perform any check in this sense. Only in the
 *   particular case of a @c NULL pointer the functions will set a
 *   @c CPL_ERROR_NULL_INPUT error code, unless differently specified.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_table.h>
 * @endcode
 */

/**@{*/

/*
 *  The table type;
 */

struct _cpl_table_ {
    cpl_size         nc;
    cpl_size         nr;
    cpl_column     **columns;
    cpl_column_flag *select;
    cpl_size         selectcount;
};


 /*
  * Prototypes for CPL_TYPE_LONG columns. Support for this type is implemented,
  * but disabled, since the FITS standard has no counterpart for this type.
  * This means that on saving a column of this type it needs to be converted
  * to either CPL_TYPE_INT or CPL_TYPE_LONG_LONG.
  *
  * Still, the prototypes are needed for correct compilation of the code.
  *
  * Confirmation of CPL_TYPE_LONG columns is still pending.
  */


cpl_error_code
cpl_table_wrap_long(cpl_table *, long *, const char *) CPL_INTERNAL;
long *cpl_table_get_data_long(cpl_table *, const char *name) CPL_INTERNAL;
const long *cpl_table_get_data_long_const(const cpl_table *,
                                          const char *name) CPL_INTERNAL;
long cpl_table_get_long(const cpl_table *, const char *, cpl_size,
                        int *null) CPL_INTERNAL;
cpl_error_code cpl_table_set_long(cpl_table *, const char *,
                                  cpl_size, long) CPL_INTERNAL;
cpl_error_code cpl_table_fill_column_window_long(cpl_table *, const char *,
                                                 cpl_size, cpl_size,
                                                 long) CPL_INTERNAL;
cpl_error_code cpl_table_copy_data_long(cpl_table *, const char *,
                                        const long *) CPL_INTERNAL;
cpl_error_code cpl_table_fill_invalid_long(cpl_table *, const char *,
                                           long) CPL_INTERNAL;
cpl_size cpl_table_and_selected_long(cpl_table *, const char *,
                                     cpl_table_select_operator,
                                     long) CPL_INTERNAL;
cpl_size cpl_table_or_selected_long(cpl_table *, const char *,
                                    cpl_table_select_operator,
                                    long) CPL_INTERNAL;

/*
 * Private method declarations (that have some attribute):
 */


static cpl_column *cpl_table_find_column_(cpl_table *, const char *);
static const cpl_column *cpl_table_find_column_const_(const cpl_table *,
                                                      const char *);

static cpl_column *cpl_table_find_column(cpl_table *,
                                         const char *)
    CPL_ATTR_NONNULL;

static const cpl_column *cpl_table_find_column_const(const cpl_table *,
                                                     const char *)
    CPL_ATTR_NONNULL;

static cpl_error_code cpl_table_append_column(cpl_table *,
                                              cpl_column *)
    CPL_ATTR_NONNULL;

static int strmatch(regex_t *, const char *)
    CPL_ATTR_NONNULL;


/*
 * Private methods:
 */


/*
 * Find the length of the longest string in the input array of strings
 */

inline static size_t
_cpl_strvlen_max(const char **s, cpl_size n)
{

    register size_t i;
    register size_t sz = 0;

    for (i = 0; i < (size_t)n; ++i) {

        if (s[i] != NULL) {
            sz = CPL_MAX(sz, strlen(s[i]));
        }

    }

    return sz;

}


inline static int
_cpl_columntype_is_pointer(cpl_type type)
{
    return (type & CPL_TYPE_POINTER) ? 1 : 0;
}


inline static cpl_type
_cpl_columntype_get_valuetype(cpl_type type)
{
    return type & ~CPL_TYPE_POINTER;
}


/*
 * Translate the CPL type code into corresponding native FITS type. If
 * fmtcode is not NULL, the associated format character is stored in the
 * character variable it points to.
 *
 * In case the input type cannot be converted, i.e. is not supported, the
 * returned native type is set to -1.
 */

inline static int
_cpl_columntype_get_nativetype(cpl_type type, char *fmtcode) {

    char fmt = '\0';

    int native_type = -1;


    type = _cpl_columntype_get_valuetype(type);

    switch (type) {

        case CPL_TYPE_BOOL:
            fmt = 'L';
            native_type = TLOGICAL;
            break;

        case CPL_TYPE_UCHAR:
            fmt = 'B';
            native_type = TBYTE;
            break;

        case CPL_TYPE_CHAR:
            fmt = 'S';
            native_type = TSBYTE;
            break;

        case CPL_TYPE_SHORT:
            fmt = 'I';
            native_type = TSHORT;
            break;

        case CPL_TYPE_INT:
            fmt = 'J';
            native_type = TINT;
            break;

        case CPL_TYPE_LONG_LONG:
            fmt = 'K';
            native_type = TLONGLONG;
            break;

        case CPL_TYPE_FLOAT:
            fmt = 'E';
            native_type = TFLOAT;
            break;

        case CPL_TYPE_DOUBLE:
            fmt = 'D';
            native_type = TDOUBLE;
            break;

        case CPL_TYPE_FLOAT_COMPLEX:
            fmt = 'C';
            native_type = TCOMPLEX;
            break;

        case CPL_TYPE_DOUBLE_COMPLEX:
            fmt = 'M';
            native_type = TDBLCOMPLEX;
            break;

        case CPL_TYPE_STRING:
            fmt = 'A';
            native_type = TSTRING;
            break;

        default:
            break;

    }

    if ((native_type != -1) && (fmtcode != NULL)) {
        *fmtcode = fmt;
    }

    return native_type;

}


inline static int
_cpl_column_find_null(const cpl_column *column, cpl_type type, cpl_size nrows,
                      unsigned int *null_flag, LONGLONG *null_value)
{
    const cpl_column_flag *is_invalid;

    /*
     * If the column does not have any invalid elements just
     * clear the flag and exit.
     */

    if (!cpl_column_has_invalid(column)) {

        *null_flag  = 0;
        *null_value = 0;
        return 0;

    }


    is_invalid = cpl_column_get_data_invalid_const(column);


    /*
     * The column has invalid elements, but the validity flag buffer
     * does not exist. This means that all elements are invalid. Thus
     * the null value is taken from the first entry. If all other rows
     * contain the same value, a consistent null value is found.
     *
     * If a validity flag buffer exists, there are some invalid elements.
     * Choose the first data value marked as invalid and verify that all
     * other invalid elements have the same value.
     */

    if (!is_invalid) {

        switch (type) {

            case CPL_TYPE_INT:
            {
                register int _null_flag = 1;

                register cpl_size irow;

                const int *data = cpl_column_get_data_int_const(column);


                for (irow = 1; irow < nrows; ++irow) {
                    if (data[irow] != data[0]) {
                        _null_flag = 0;
                        break;
                    }
                }

                if (_null_flag) {
                    *null_flag  = 1;
                    *null_value = data[0];
                }

                break;
            }

            case CPL_TYPE_LONG_LONG:
            {

                register int _null_flag = 1;

                register cpl_size irow;

                const long long *data =
                        cpl_column_get_data_long_long_const(column);


                for (irow = 1; irow < nrows; ++irow) {
                    if (data[irow] != data[0]) {
                        _null_flag = 0;
                        break;
                    }
                }

                if (_null_flag) {
                    *null_flag  = 1;
                    *null_value = data[0];
                }

                break;
            }

            default:
                return 2;
                break;

        }

    }
    else {

        register cpl_size first = 0;


        while (!is_invalid[first] && (first < nrows)) {
            ++first;
        }

        if (first >= nrows) {
            return -1;
        }

        switch (type) {
            case CPL_TYPE_INT:
            {

                register int _null_flag = 1;

                register cpl_size irow;

                const int *data = cpl_column_get_data_int_const(column);


                for (irow = first + 1; irow < nrows; ++irow) {
                    if (is_invalid[irow] && (data[irow] != data[first])) {
                        _null_flag = 0;
                        break;
                    }
                }

                if (_null_flag) {
                    *null_flag  = 1;
                    *null_value = data[first];
                }

                break;
            }

            case CPL_TYPE_LONG_LONG:
            {
                register int _null_flag = 1;

                register cpl_size irow;

                const long long *data =
                        cpl_column_get_data_long_long_const(column);

                for (irow = first + 1; irow < nrows; ++irow) {
                    if (is_invalid[irow] && (data[irow] != data[first])) {
                        _null_flag = 0;
                        break;
                    }
                }

                if (_null_flag) {
                    *null_flag  = 1;
                    *null_value = data[first];
                }

                break;
            }

            default:
                return 2;
                break;
        }

    }

    return 0;

}


/*
 * @internal
 * @brief
 *   Find a column or set the specified error
 *
 * @param table      Pointer to table where to search for the column.
 * @param name       Column name.
 *
 * @return Pointer to found column, or @c NULL on error
 * @see cpl_table_find_column()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 */

static const cpl_column *cpl_table_find_column_const_(const cpl_table *table,
                                                      const char *name)
{

    const cpl_column *column = NULL;


    if (name == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "name is NULL");
    } else if (table == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "table is NULL, "
                                     "name='%s'", name);
    } else {

        column = cpl_table_find_column_const(table, name);

        if (!column) {
            (void)cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "name='%s'",
                                         name);
        }
    }

    return column;
}


static cpl_column *cpl_table_find_column_(cpl_table *table,
                                          const char *name)
{

    cpl_column *column = NULL;


    if (name == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "name is NULL");
    } else if (table == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "table is NULL, "
                                     "name='%s'", name);
    } else {

        column = cpl_table_find_column(table, name);

        if (!column) {
            (void)cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "name='%s'",
                                         name);
        }
    }

    return column;
}


/*
 * @internal
 * @brief
 *   Find a column of the specified type or set the specified error
 *
 * @param table      Pointer to table where to search for the column.
 * @param name       Column name.
 * @param exptype    The expected type of the column, CPL_TYPE_POINTER means
                     any cpl_type of type pointer
 *
 * @return Pointer to found column, or @c NULL on error
 * @see cpl_table_find_column_()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of the specified type.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 */

static cpl_column *cpl_table_find_column_type(cpl_table *table,
                                              const char *name,
                                              cpl_type exptype)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
    } else {

        const cpl_type type = cpl_column_get_type(column);

        if (type != exptype &&
            (exptype != CPL_TYPE_POINTER || !(type & CPL_TYPE_POINTER))) {
            column = NULL;
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH, "name='%s', "
                                         "type=%s != %s", name,
                                         cpl_type_get_name(type),
                                         cpl_type_get_name(exptype));
        }
    }

    return column;
}


/*
 * @internal
 * @brief
 *   Find a column of the specified type or set the specified error
 *
 * @param table      Pointer to table where to search for the column.
 * @param name       Column name.
 * @param exptype    The expected type of the column, CPL_TYPE_POINTER means
                     any cpl_type of type pointer
 *
 * @return Pointer to found column, or @c NULL on error
 * @see cpl_table_find_column_(), cpl_table_find_column_type()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of the specified type.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 */

static
const cpl_column *cpl_table_find_column_type_const(const cpl_table *table,
                                                   const char *name,
                                                   cpl_type exptype)
{

    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
    } else {

        const cpl_type type = cpl_column_get_type(column);

        if (type != exptype &&
            (exptype != CPL_TYPE_POINTER || !(type & CPL_TYPE_POINTER))) {
            column = NULL;
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH, "name='%s', "
                                         "type=%s != %s", name,
                                         cpl_type_get_name(type),
                                         cpl_type_get_name(exptype));
        }
    }

    return column;
}


/*
 * @brief
 *   Find a column.
 *
 * @param table  Pointer to table where to search for the column.
 * @param name   Column name.
 *
 * @return Pointer to found column, or @c NULL.
 *
 * This private function looks for a column with a given name in
 * @em table. A @c NULL is returned if no column with the specified
 * @em name is found. The input @em table and @em name are assumed
 * not to be @c NULL.
 */

static const cpl_column *cpl_table_find_column_const(const cpl_table *table,
                                                     const char *name)
{

    const cpl_column * const *column =
        (const cpl_column * const *)table->columns;

    for (cpl_size i = 0; i < table->nc; i++, column++) {
        const char *column_name = cpl_column_get_name(*column);
        if (strcmp(name, column_name) == 0) {
            return *column;
        }
    }

    return NULL;

}

/*
 * @brief
 *   Find a column.
 *
 * @param table  Pointer to table where to search for the column.
 * @param name   Column name.
 *
 * @return Pointer to found column, or @c NULL.
 *
 * This private function looks for a column with a given name in
 * @em table. A @c NULL is returned if no column with the specified
 * @em name is found. The input @em table and @em name are assumed
 * not to be @c NULL.
 */

static cpl_column *cpl_table_find_column(cpl_table *table,
                                         const char *name)
{

    cpl_column **column = table->columns;

    for (cpl_size i = 0; i < table->nc; i++, column++) {
        const char *column_name = cpl_column_get_name(*column);
        if (strcmp(name, column_name) == 0) {
            return *column;
        }
    }

    return NULL;

}


/*
 * @brief
 *   Extract a column from a table.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to extracted column, or @c NULL.
 *
 * This private function looks for a column with a given name in
 * @em table. If found, the column is extracted from the table, and the
 * table will have one column less. If no columns are left in table,
 * also the selection flags are lost. A @c NULL is returned if no column
 * with the specified @em name is found. The input @em table and @em name
 * are expected not to be @c NULL.
 */

static cpl_column *cpl_table_extract_column(cpl_table *table, const char *name)
{

    cpl_column  *column  = cpl_table_find_column(table, name);


    if (column) {

        const cpl_size width   = cpl_table_get_ncol(table);
        cpl_size       i, j;

        for (i = 0; i < width; i++)
            if (column == table->columns[i])
                break;

        j = i + 1;
        while(j < width)
            table->columns[i++] = table->columns[j++];

        table->nc--;
        if (table->nc) {
            table->columns = cpl_realloc(table->columns,
                             table->nc * sizeof(column));
        }
        else {
            cpl_free(table->columns);
            table->columns = NULL;
        }

        if (table->nc == 0)          /* Last column deleted         */
            cpl_table_select_all(table);

    }

    return column;

}


/*
 * @brief
 *   Append a column to a table.
 *
 * @param table  Pointer to table where to append the column.
 * @param column Column to append.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * This private function appends a column to the column list of a table.
 * It is assumed that a column with the same name does not exist already
 * in the table. The input @em table and @em column are expected not to
 * be @c NULL, and the input column is expected not to be nameless. Also,
 * the input column is assumed to have the same length as all the other
 * columns in the table.
 */

static cpl_error_code cpl_table_append_column(cpl_table *table,
                                              cpl_column *column)
{

    if (table->columns) {
        table->columns = cpl_realloc(table->columns,
                         (table->nc + 1) * sizeof(column));
    }
    else
        table->columns = cpl_malloc(sizeof(column));

    table->columns[table->nc] = column;

    table->nc++;

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Check if a string matches a compiled regular expression.
 *
 * @param pattern   Compiled regular expression.
 * @param string    String to test.
 *
 * @return 1 on match, 0 on mismatch.
 *
 * This private function tests a string against an extended regular expression.
 * The inputs are assumed not to be @c NULL. It is assumed also that the
 * check on the availability of regexec() are done elsewhere.
 */

static int strmatch(regex_t *pattern, const char *string)
{

    if (regexec(pattern, string, (size_t)0, NULL, 0) == REG_NOMATCH)
        return 0;

    return 1;

}


/*
 * @brief
 *   Sort table rows according to columns values.
 *
 * @param table   Pointer to table.
 * @param reflist Names of reference columns with corresponding sorting mode.
 * @param n       Use only n first elements of @em reflist
 * @param sort_pattern       Work space
 * @param sort_null_pattern  Work space
 *
 * @return See @c cpl_table_sort()
 *
 * The table is sorted after each of the n columns, less significant columns
 * before more significant columns. This method depends on the sorting
 * algorithm being stable (i.e. it must conserve the order of equal elements).
 */

static cpl_error_code
table_sort(cpl_table *table, const cpl_propertylist *reflist, unsigned n,
           cpl_size *sort_pattern, cpl_size *sort_null_pattern)
{
    const cpl_property *info;
    const char *name;
    int reverse;
    const int stable = 1;
    cpl_column *column;
    cpl_column *column_sorted_null;
    cpl_type type;
    cpl_size i, j;

    cpl_size nullcount;
    int             *idata;
    int             *work_idata;
    long            *ldata;
    long            *work_ldata;
    long long       *lldata;
    long long       *work_lldata;
    float           *fdata;
    float           *work_fdata;
    double          *ddata;
    double          *work_ddata;
    float complex   *cfdata;
    float complex   *work_cfdata;
    double complex  *cddata;
    double complex  *work_cddata;
    char           **sdata;
    char           **work_sdata;
    cpl_array      **adata;
    cpl_array      **work_adata;
    cpl_column_flag *ndata;
    cpl_column_flag *work_ndata;


    if (n == 0) {
        return CPL_ERROR_NONE;
    }


    /*
     * Sort after least significant column
     */

    info = cpl_propertylist_get_const(reflist, n - 1);
    name = cpl_property_get_name(info);
    if (cpl_property_get_type(info) == CPL_TYPE_BOOL) {
        reverse = cpl_property_get_bool(info);
    }
    else {
        return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
    }

    if (name == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }
    if ((column = cpl_table_find_column(table, name)) == NULL) {
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
    }

    type = cpl_column_get_type(column);
    if (type & CPL_TYPE_POINTER) {   /* Arrays not as reference */
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }


    /*
     * Independent of the reverse flag, put rows with invalid values at the
     * beginning of the table
     */

    nullcount = cpl_column_count_invalid(column);
    ndata = cpl_column_get_data_invalid(column);
    
    j = 0;
    for (i = 0; i < table->nr; i++) {
        if (nullcount == table->nr || 
            (ndata != NULL && ndata[i])) {
            sort_null_pattern[j++] = i;
        }
    }

/*
    assert( j == nullcount );
*/

    for (i = 0; i < table->nr; i++) {
        if (!
            (nullcount == table->nr || 
             (ndata != NULL && ndata[i]))
            ) {
            sort_null_pattern[j++] = i;
        }
    }

/*
    assert( j == table->nr );
*/


    /*
     *  Apply NULL pattern
     */

    column_sorted_null = cpl_column_duplicate(column);
    for (i = 0; i < nullcount; i++) {
        cpl_column_set_invalid(column_sorted_null, i);
    }
    for (i = nullcount; i < table->nr; i++) {
        int isnull;

        switch (type) {
        case CPL_TYPE_INT:
            cpl_column_set_int(
                column_sorted_null, i,
                cpl_column_get_int(column, sort_null_pattern[i], &isnull));
            break;
        case CPL_TYPE_LONG:
            cpl_column_set_long(
                column_sorted_null, i,
                cpl_column_get_long(column, sort_null_pattern[i], &isnull));
            break;
        case CPL_TYPE_LONG_LONG:
            cpl_column_set_long_long(
                column_sorted_null, i,
                cpl_column_get_long_long(column, sort_null_pattern[i], &isnull));
            break;
        case CPL_TYPE_FLOAT:
            cpl_column_set_float(
                column_sorted_null, i,
                cpl_column_get_float(column, sort_null_pattern[i], &isnull));
            break;
        case CPL_TYPE_DOUBLE:
            cpl_column_set_double(
                column_sorted_null, i,
                cpl_column_get_double(column, sort_null_pattern[i], &isnull));
            break;
        case CPL_TYPE_STRING:
            cpl_column_set_string(
                column_sorted_null, i,
                cpl_column_get_string(column, sort_null_pattern[i]));
            break;
        default:

            /* You should never get here */
/*
            assert( 0 );
*/
            break;
        }
    }

    /*
     * Get remaining sort pattern
     */

    for (i = 0; i < nullcount; i++) {
        sort_pattern[i] = i;
    }
    switch (type) {
    case CPL_TYPE_INT:
        cpl_tools_sort_stable_pattern_int(
            cpl_column_get_data_int(column_sorted_null) + nullcount,
            table->nr - nullcount,
            reverse,
            stable,
            sort_pattern + nullcount);
        break;
    case CPL_TYPE_LONG:
        cpl_tools_sort_stable_pattern_long(
            cpl_column_get_data_long(column_sorted_null) + nullcount,
            table->nr - nullcount,
            reverse,
            stable,
            sort_pattern + nullcount);
        break;
    case CPL_TYPE_LONG_LONG:
        cpl_tools_sort_stable_pattern_long_long(
            cpl_column_get_data_long_long(column_sorted_null) + nullcount,
            table->nr - nullcount,
            reverse,
            stable,
            sort_pattern + nullcount);
        break;
    case CPL_TYPE_FLOAT:
        cpl_tools_sort_stable_pattern_float(
            cpl_column_get_data_float(column_sorted_null) + nullcount,
            table->nr - nullcount,
            reverse,
            stable,
            sort_pattern + nullcount);
        break;
    case CPL_TYPE_DOUBLE:
        cpl_tools_sort_stable_pattern_double(
            cpl_column_get_data_double(column_sorted_null) + nullcount,
            table->nr - nullcount,
            reverse,
            stable,
            sort_pattern + nullcount);
        break;
    case CPL_TYPE_STRING:
        cpl_tools_sort_stable_pattern_string(
            cpl_column_get_data_string(column_sorted_null) + nullcount,
            table->nr - nullcount,
            reverse,
            stable,
            sort_pattern + nullcount);
        break;
    default:

        /* You should never get here */
/*
        assert( 0 );
*/
        break;
    }

    cpl_column_delete(column_sorted_null);


    /*
     *  Transform sort pattern
     */

    for (i = nullcount; i < table->nr; i++) {
        sort_pattern[i] += nullcount;
    }


    /*
     * Now apply the combined sort pattern to each column in the table:
     */

    j = 0;
    while (j < table->nc) {

        cpl_type type_id = cpl_column_get_type(table->columns[j]);


        if (_cpl_columntype_is_pointer(type_id)) {

            switch (_cpl_columntype_get_valuetype(type_id)) {

                case CPL_TYPE_INT:
                case CPL_TYPE_LONG:
                case CPL_TYPE_LONG_LONG:
                case CPL_TYPE_FLOAT:
                case CPL_TYPE_DOUBLE:
                case CPL_TYPE_STRING:
                {
                    cpl_column *work_column =
                    cpl_column_new_array(cpl_column_get_type(table->columns[j]),
                                         table->nr,
                                         cpl_column_get_depth(table->columns[j]));

                    work_adata = cpl_column_get_data_array(work_column);
                    adata = cpl_column_get_data_array(table->columns[j]);
    
                    for (i = 0; i < table->nr; i++)
                        work_adata[i] = adata[i];

                    for (i = 0; i < table->nr; i++)
                        adata[i] = work_adata[sort_null_pattern[sort_pattern[i]]];

                    cpl_column_delete_but_arrays(work_column);
                    break;
                }

                default:

                    break;      /* Should never get here */

            }

        }
        else {

            switch (_cpl_columntype_get_valuetype(type_id)) {

            case CPL_TYPE_INT:
            {
                cpl_column *work_column =
                        cpl_column_duplicate(table->columns[j]);

                nullcount = cpl_column_count_invalid(table->columns[j]);

                if (nullcount != table->nr) {

                    ndata = cpl_column_get_data_invalid(table->columns[j]);

                    if (ndata) {
                        work_ndata = cpl_column_get_data_invalid(work_column);

                        for (i = 0; i < table->nr; i++)
                            ndata[i] =
                            work_ndata[sort_null_pattern[sort_pattern[i]]];

                    }

                }

                work_idata = cpl_column_get_data_int(work_column);
                idata = cpl_column_get_data_int(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    idata[i] = work_idata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete(work_column);
                break;
            }

            case CPL_TYPE_LONG:
            {
                cpl_column *work_column = cpl_column_duplicate(table->columns[j]);

                nullcount = cpl_column_count_invalid(table->columns[j]);

                if (nullcount != table->nr) {

                    ndata = cpl_column_get_data_invalid(table->columns[j]);

                    if (ndata) {
                        work_ndata = cpl_column_get_data_invalid(work_column);

                        for (i = 0; i < table->nr; i++)
                            ndata[i] =
                            work_ndata[sort_null_pattern[sort_pattern[i]]];

                    }

                }

                work_ldata = cpl_column_get_data_long(work_column);
                ldata = cpl_column_get_data_long(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    ldata[i] = work_ldata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete(work_column);
                break;
            }

            case CPL_TYPE_LONG_LONG:
            {
                cpl_column *work_column = cpl_column_duplicate(table->columns[j]);

                nullcount = cpl_column_count_invalid(table->columns[j]);

                if (nullcount != table->nr) {

                    ndata = cpl_column_get_data_invalid(table->columns[j]);

                    if (ndata) {
                        work_ndata = cpl_column_get_data_invalid(work_column);

                        for (i = 0; i < table->nr; i++)
                            ndata[i] =
                            work_ndata[sort_null_pattern[sort_pattern[i]]];

                    }

                }

                work_lldata = cpl_column_get_data_long_long(work_column);
                lldata = cpl_column_get_data_long_long(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    lldata[i] = work_lldata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete(work_column);
                break;
            }

            case CPL_TYPE_FLOAT:
            {
                cpl_column *work_column = cpl_column_duplicate(table->columns[j]);

                nullcount = cpl_column_count_invalid(table->columns[j]);

                if (nullcount != table->nr) {

                    ndata = cpl_column_get_data_invalid(table->columns[j]);

                    if (ndata) {
                        work_ndata = cpl_column_get_data_invalid(work_column);

                        for (i = 0; i < table->nr; i++)
                            ndata[i] =
                            work_ndata[sort_null_pattern[sort_pattern[i]]];

                    }

                }

                work_fdata = cpl_column_get_data_float(work_column);
                fdata = cpl_column_get_data_float(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    fdata[i] = work_fdata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete(work_column);
                break;
            }

            case CPL_TYPE_DOUBLE:
            {
                cpl_column *work_column = cpl_column_duplicate(table->columns[j]);

                nullcount = cpl_column_count_invalid(table->columns[j]);

                if (nullcount != table->nr) {

                    ndata = cpl_column_get_data_invalid(table->columns[j]);

                    if (ndata) {
                        work_ndata = cpl_column_get_data_invalid(work_column);

                        for (i = 0; i < table->nr; i++)
                            ndata[i] =
                            work_ndata[sort_null_pattern[sort_pattern[i]]];

                    }

                }

                work_ddata = cpl_column_get_data_double(work_column);
                ddata = cpl_column_get_data_double(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    ddata[i] = work_ddata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete(work_column);
                break;
            }

            case CPL_TYPE_FLOAT_COMPLEX:
            {
                cpl_column *work_column = cpl_column_duplicate(table->columns[j]);

                nullcount = cpl_column_count_invalid(table->columns[j]);

                if (nullcount != table->nr) {

                    ndata = cpl_column_get_data_invalid(table->columns[j]);

                    if (ndata) {
                        work_ndata = cpl_column_get_data_invalid(work_column);

                        for (i = 0; i < table->nr; i++)
                            ndata[i] =
                            work_ndata[sort_null_pattern[sort_pattern[i]]];

                    }

                }

                work_cfdata = cpl_column_get_data_float_complex(work_column);
                cfdata = cpl_column_get_data_float_complex(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    cfdata[i] = work_cfdata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete(work_column);
                break;
            }

            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                cpl_column *work_column = cpl_column_duplicate(table->columns[j]);

                nullcount = cpl_column_count_invalid(table->columns[j]);

                if (nullcount != table->nr) {

                    ndata = cpl_column_get_data_invalid(table->columns[j]);

                    if (ndata) {
                        work_ndata = cpl_column_get_data_invalid(work_column);

                        for (i = 0; i < table->nr; i++)
                            ndata[i] =
                            work_ndata[sort_null_pattern[sort_pattern[i]]];

                    }

                }

                work_cddata = cpl_column_get_data_double_complex(work_column);
                cddata = cpl_column_get_data_double_complex(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    cddata[i] = work_cddata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete(work_column);
                break;
            }

            case CPL_TYPE_STRING:
            {
                cpl_column *work_column = cpl_column_new_string(table->nr);

                work_sdata = cpl_column_get_data_string(work_column);
                sdata = cpl_column_get_data_string(table->columns[j]);

                for (i = 0; i < table->nr; i++)
                    work_sdata[i] = sdata[i];

                for (i = 0; i < table->nr; i++)
                    sdata[i] = work_sdata[sort_null_pattern[sort_pattern[i]]];

                cpl_column_delete_but_strings(work_column);
                break;
            }

            default:

                break;      /* Should never get here */

            }

        }

        j++;

    }


    /*
     * Finished sorting after least significant column.
     * Sort after remaining columns
     */

    return table_sort(table, reflist, n - 1, sort_pattern, sort_null_pattern);
}


/*
 * Public methods:
 */


/**
 * @brief
 *   Create an empty table structure.
 *
 * @param length  Number of rows in table.
 *
 * @return Pointer to new table, or @c NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified <i>length</i> is negative.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function allocates and initialises memory for a table data container.
 * A new table is created with no columns, but the size of the columns that
 * will be created is defined in advance, to ensure that all columns will
 * be created with the same length. All table rows are marked a priori as
 * selected. This should be considered the normal status of a table, as
 * long as no row selection has been applied to it.
 */

cpl_table *cpl_table_new(cpl_size length)
{

    cpl_table *table;


    if (length < 0) {
        cpl_error_set("cpl_table_new", CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    table = cpl_calloc(1, sizeof(cpl_table));
    table->nc = 0;
    table->nr = length;
    table->columns = NULL;
    table->select = NULL;
    table->selectcount = length;

    return table;

}


/**
 * @brief
 *   Give to a table the same structure of another table.
 *
 * @param table   Pointer to empty table.
 * @param mtable  Pointer to model table.
 *
 * @return @c CPL_ERROR_NONE in case of success.
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
 *         <i>table</i> contains columns.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function assignes to a columnless table the same column structure
 * (names, types, units) of a given model table. All columns are physically
 * created in the new table, and they are initialised to contain just
 * invalid elements.
 */

cpl_error_code cpl_table_copy_structure(cpl_table *table,
                                        const cpl_table *mtable)
{

    const char  *unit = NULL;
    const char  *form = NULL;
    cpl_column **column;
    cpl_type     type;
    cpl_size     width = cpl_table_get_ncol(mtable);
    cpl_size     i     = 0;


    if (table == NULL || mtable == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (table->nc > 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    column = mtable->columns;

    for (i = 0; i < width; i++, column++) {
        type = cpl_column_get_type(*column);
        unit = cpl_column_get_unit(*column);
        form = cpl_column_get_format(*column);
        if (type & CPL_TYPE_POINTER) {
            cpl_table_new_column_array(table, cpl_column_get_name(*column),
                                       type, cpl_column_get_depth(*column));
        }
        else {
            cpl_table_new_column(table, cpl_column_get_name(*column), type);
        }
        cpl_table_set_column_unit(table, cpl_column_get_name(*column), unit);
        cpl_table_set_column_format(table, cpl_column_get_name(*column), form);
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create an empty column in a table.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param type   Type of the new column.
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
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>type</i> is not supported.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function allocates memory for a new column of specified @em type,
 * excluding @em array types (for creating a column of arrays use the
 * function @c cpl_table_new_column_array(), where the column depth
 * must also be specified). The new column name must be different from
 * any other column name in the table. All the elements of the new column
 * are marked as invalid.
 */

cpl_error_code cpl_table_new_column(cpl_table *table,
                                    const char *name, cpl_type type)
{

    cpl_column  *column;


    if (table == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    switch(type) {
    case CPL_TYPE_INT:
        column = cpl_column_new_int(table->nr);
        break;
#if defined(USE_COLUMN_TYPE_LONG)
    case CPL_TYPE_LONG:
        column = cpl_column_new_long(table->nr);
        break;
#endif
    case CPL_TYPE_LONG_LONG:
        column = cpl_column_new_long_long(table->nr);
        break;
    case CPL_TYPE_FLOAT:
        column = cpl_column_new_float(table->nr);
        break;
    case CPL_TYPE_DOUBLE:
        column = cpl_column_new_double(table->nr);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        column = cpl_column_new_float_complex(table->nr);
        break;
    case CPL_TYPE_DOUBLE_COMPLEX:
        column = cpl_column_new_double_complex(table->nr);
        break;
    case CPL_TYPE_STRING:
        column = cpl_column_new_string(table->nr);
        break;
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create an empty column of arrays in a table.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param type   Type of the new column.
 * @param depth  Depth of the new column.
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
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>type</i> is not supported.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified <i>depth</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function allocates memory for a new column of specified array
 * @em type, (for creating a column of simple scalars or character strings
 * use the function @c cpl_table_new_column() instead). It doesn't make
 * any difference if a simple or an array @em type is specified, the
 * corresponding array type will always be created (e.g., specifying
 * a @em type @c CPL_TYPE_INT or a type @c CPL_TYPE_INT | @c CPL_TYPE_POINTER
 * would always create a column of type @c CPL_TYPE_INT | @c CPL_TYPE_POINTER).
 * The new column name must be different from any other column name in the
 * table. All the elements of the new column are marked as invalid.
 */

cpl_error_code cpl_table_new_column_array(cpl_table *table, const char *name,
                                          cpl_type type, cpl_size depth)
{

    cpl_column  *column;


    if (table == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

#if !defined(USE_COLUMN_TYPE_LONG)
    if (type == CPL_TYPE_LONG)
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
#endif

    if (depth < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_new_array(type, table->nr, depth);

    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Change saving type for a given column.
 *
 * @param table  Pointer to table.
 * @param name   Name of existing column.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> does not exist</i>.
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
 * their matching CFITSIO and FITS types:
 *
 * @code
 *
 *  CPL_TYPE_INT:            CPL_TYPE_INT            TINT        'J' (default)
 *                           CPL_TYPE_BOOL           TLOGICAL    'L'
 *                           CPL_TYPE_CHAR           TSBYTE      'B' with TZERO = -128)
 *                           CPL_TYPE_UCHAR          TBYTE       'B'
 *                           CPL_TYPE_SHORT          TSHORT      'I'
 *  CPL_TYPE_LONG_LONG:      CPL_TYPE_LONG_LONG      TLONGLONG   'K' (default)
 *                           CPL_TYPE_INT            TINT        'J'
 *                           CPL_TYPE_BOOL           TLOGICAL    'L'
 *                           CPL_TYPE_CHAR           TSBYTE      'B' with TZERO = -128)
 *                           CPL_TYPE_UCHAR          TBYTE       'B'
 *                           CPL_TYPE_SHORT          TSHORT      'I'
 *  CPL_TYPE_FLOAT:          CPL_TYPE_FLOAT          TFLOAT      'E' (default)
 *  CPL_TYPE_DOUBLE:         CPL_TYPE_DOUBLE         TDOUBLE     'D' (default)
 *  CPL_TYPE_FLOAT_COMPLEX:  CPL_TYPE_FLOAT_COMPLEX  TCOMPLEX    'C' (default)
 *  CPL_TYPE_DOUBLE_COMPLEX: CPL_TYPE_DOUBLE_COMPLEX TDBLCOMPLEX 'M' (default)
 *
 * @endcode
 *
 * @note
 *   Saving CPL_TYPE_INT into shorter integer types (such as 
 *   unsigned byte) may introduce loss of information, in case
 *   the integer values overflowed the lower datatype. Only the
 *   least significant bits will be transferred, according to
 *   the C casting rules. This would not be signalled in any way
 *   (neither as an error nor as a warning) by the @c cpl_table_save()
 *   function: it is entirely a responsibility of the programmer
 *   to ensure that integer columns to be saved to lower types 
 *   would not overflow such types.
 */

cpl_error_code cpl_table_set_column_savetype(cpl_table *table,
                                             const char *name, cpl_type type)
{
    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_save_type(column, type)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/*
 * @brief
 *   Get the saving type of a given table column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Column saving type, or @c CPL_TYPE_INVALID in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the saving type of a column. See function 
 * @c cpl_table_set_column_save_type()
 */

/*

cpl_type cpl_table_get_column_save_type(const cpl_table *table, 
                                        const char *name)
{
    cpl_column  *column;


    if (table == NULL || name == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return CPL_TYPE_INVALID;
    }

    column = cpl_table_find_column(table, name);

    if (!column) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return CPL_TYPE_INVALID;
    }

    return cpl_column_get_save_type(column);

}

*/


/**
 * @brief
 *   Create in table a new @em integer column obtained from existing data.
 *
 * @param table  Pointer to table where to create the new column.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_INT that will
 * encapsulate the given data. The size of the input data array is not
 * checked in any way, and it is expected to match the number of rows
 * assigned to the given table. The pointed data values are all taken
 * as valid: invalid values should be marked using the functions
 * @c cpl_table_set_invalid() and @c cpl_table_set_column_invalid().
 * The data buffer is not copied, so it should not be deallocated while
 * the table column is still in use: the functions @c cpl_table_erase_column()
 * or @c cpl_table_delete() would take care of deallocating it. To avoid
 * problems with the memory managment, the specified data buffer should
 * have been allocated using the functions of the @em cpl_memory module,
 * and statically allocated data should be avoided too. If this is not
 * possible, then the function @c cpl_table_unwrap() should be used on
 * that column before destroying the table that contains it.
 */

cpl_error_code cpl_table_wrap_int(cpl_table *table,
                                  int *data, const char *name)
{

    cpl_column  *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_int(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create in table a new @em long column obtained from existing data.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_LONG
 * that will encapsulate the given data. See the description of
 * @c cpl_table_wrap_int() for further details.
 */

cpl_error_code cpl_table_wrap_long(cpl_table *table,
                                   long *data, const char *name)
{

    cpl_column  *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_long(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create in table a new @em long @em long column obtained from existing data.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_LONG_LONG
 * that will encapsulate the given data. See the description of
 * @c cpl_table_wrap_int() for further details.
 */

cpl_error_code cpl_table_wrap_long_long(cpl_table *table,
                                        long long *data, const char *name)
{

    cpl_column  *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_long_long(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create in table a new @em float column obtained from existing data.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_FLOAT
 * that will encapsulate the given data. See the description of
 * @c cpl_table_wrap_int() for further details.
 */

cpl_error_code cpl_table_wrap_float(cpl_table *table,
                                    float *data, const char *name)
{

    cpl_column  *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_float(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create in table a new @em float complex column obtained from existing data.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_FLOAT_COMPLEX
 * that will encapsulate the given data. See the description of
 * @c cpl_table_wrap_int() for further details.
 */

cpl_error_code cpl_table_wrap_float_complex(cpl_table *table,
                                            float complex *data, 
                                            const char *name)
{

    cpl_column *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_float_complex(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create in table a new @em double column obtained from existing data.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_DOUBLE
 * that will encapsulate the given data. See the description of
 * @c cpl_table_wrap_int() for further details.
 */

cpl_error_code cpl_table_wrap_double(cpl_table *table,
                                     double *data, const char *name)
{

    cpl_column  *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_double(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create in table a new @em double complex column from existing data.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_FLOAT_COMPLEX
 * that will encapsulate the given data. See the description of
 * @c cpl_table_wrap_int() for further details.
 */

cpl_error_code cpl_table_wrap_double_complex(cpl_table *table,
                                             double complex *data, 
                                             const char *name)
{

    cpl_column *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_double_complex(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create in table a new @em string column obtained from existing data.
 *
 * @param table  Pointer to table.
 * @param name   Name of the new column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function creates a new column of type @c CPL_TYPE_STRING that will
 * encapsulate the given data. See the description of @c cpl_table_wrap_int()
 * for further details, especially with regard to memory managment. In the
 * specific case of @em string columns the described restrictions applies
 * also to the single column elements (strings). To deallocate specific
 * column elements the functions @c cpl_table_set_invalid() and
 * @c cpl_table_set_column_invalid() should be used.
 */

cpl_error_code cpl_table_wrap_string(cpl_table *table,
                                     char **data, const char *name)
{

    cpl_column  *column;


    if (table == NULL || data == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_column_wrap_string(data, table->nr);
    cpl_column_set_name(column, name);
    cpl_table_append_column(table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Unwrap a table column
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 *
 * @return Pointer to internal data buffer, @c NULL in case of error.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSUPPORTED_MODE</td>
 *       <td class="ecr">
 *         The column with the given <i>name</i> is a column of arrays.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function deallocates all the memory associated to a table column,
 * with the exception of its data buffer. This type of destructor
 * should be used on columns created with the @c cpl_table_wrap_<type>()
 * constructors, if the data buffer specified then was not allocated
 * using the functions of the @c cpl_memory module. In such a case, the
 * data buffer should be deallocated separately. See the documentation
 * of the functions @c cpl_table_wrap_<type>().
 *
 * @note
 *   Columns of arrays cannot be unwrapped. Use the function
 *   @c cpl_table_get_data_array() to directly access the column
 *   data buffer.
 */

void *cpl_table_unwrap(cpl_table *table, const char *name)
{
    cpl_column *column;


    if (table == NULL || name == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (cpl_table_get_column_type(table, name) & CPL_TYPE_POINTER) {
        cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
        return NULL;
    }

    column = cpl_table_extract_column(table, name);

    if (!column) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return NULL;
    }

    return cpl_column_unwrap(column);

}


/**
 * @brief
 *   Copy existing data to a table @em integer column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input data values are copied to the specified column. The size of the
 * input array is not checked in any way, and it is expected to be compatible
 * with the number of rows in the given table. The copied data values are
 * all taken as valid: invalid values should be marked using the functions
 * @c cpl_table_set_invalid() and @c cpl_table_set_column_invalid().
 */

cpl_error_code cpl_table_copy_data_int(cpl_table *table,
                                       const char *name, const int *data)
{

    cpl_column  *column = cpl_table_find_column_(table, name);

    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_int(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a table @em long column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of @c cpl_table_copy_data_int() for details.
 */

cpl_error_code cpl_table_copy_data_long(cpl_table *table,
                                        const char *name, const long *data)
{

    cpl_column  *column = cpl_table_find_column_(table, name);


    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_long(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a table @em long @em long column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of @c cpl_table_copy_data_int() for details.
 */

cpl_error_code cpl_table_copy_data_long_long(cpl_table *table,
                                             const char *name,
                                             const long long *data)
{

    cpl_column  *column = cpl_table_find_column_(table, name);


    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_long_long(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Copy existing data to a table @em float column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of @c cpl_table_copy_data_int() for details.
 */

cpl_error_code cpl_table_copy_data_float(cpl_table *table,
                                         const char *name, const float *data)
{

    cpl_column  *column = cpl_table_find_column_(table, name);


    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_float(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Copy existing data to a table @em float complex column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of @c cpl_table_copy_data_int() for details.
 */

cpl_error_code cpl_table_copy_data_float_complex(cpl_table *table,
                                                 const char *name, 
                                                 const float complex *data)
{

    cpl_column  *column = cpl_table_find_column_(table, name);

    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_float_complex(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Copy existing data to a table @em double column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of @c cpl_table_copy_data_int() for details.
 */

cpl_error_code cpl_table_copy_data_double(cpl_table *table,
                                          const char *name, const double *data)
{
    cpl_column  *column = cpl_table_find_column_(table, name);

    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_double(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Copy existing data to a table @em double complex column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of @c cpl_table_copy_data_int() for details.
 */

cpl_error_code cpl_table_copy_data_double_complex(cpl_table *table,
                                                  const char *name,
                                                  const double complex *data)
{

    cpl_column  *column = cpl_table_find_column_(table, name);

    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_double_complex(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Copy existing data to a table @em string column.
 *
 * @param table  Pointer to table.
 * @param name   Name of the column.
 * @param data   Existing data buffer.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of @c cpl_table_copy_data_int() for details.
 * In the particular case of a string column, it should be noted that
 * the data are copied in-depth, i.e., also the pointed strings are
 * duplicated. Strings contained in the existing table column are
 * deallocated before being replaced by the new ones.
 */

cpl_error_code cpl_table_copy_data_string(cpl_table *table, const char *name, 
                                          const char **data)
{
    cpl_column  *column = cpl_table_find_column_(table, name);

    if (data == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_copy_data_string(column, data)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Delete a table.
 *
 * @param table  Pointer to table to be deleted.
 *
 * @return Nothing.
 *
 * This function deletes a table, releasing all the memory associated
 * to it, including any existing column. If @em table is @c NULL,
 * nothing is done, and no error is set.
 */

void cpl_table_delete(cpl_table *table)
{

    int          width;
    cpl_column **column;


    if (table) {
        width = cpl_table_get_ncol(table);
        column = table->columns;
        while (width--)
            cpl_column_delete(*column++);
        if (table->columns)
            cpl_free(table->columns);
        if (table->select)
            cpl_free(table->select);
        cpl_free(table);
    }

}


/**
 * @brief
 *   Get the number of rows in a table.
 *
 * @param table  Pointer to table to examine.
 *
 * @return Number of rows in the table. If a @c NULL table pointer
 *   is passed, -1 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the number of rows in a table.
 */

cpl_size cpl_table_get_nrow(const cpl_table *table)
{

    if (table)
        return table->nr;

    cpl_error_set("cpl_table_get_nrow", CPL_ERROR_NULL_INPUT);

    return -1;

}


/**
 * @brief
 *   Get the number of columns in a table.
 *
 * @param table  Pointer to table to examine.
 *
 * @return Number of columns in the table. If a @c NULL table pointer
 *   is passed, -1 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the number of columns in a table.
 */

cpl_size cpl_table_get_ncol(const cpl_table *table)
{

    if (table)
        return table->nc;

    cpl_error_set("cpl_table_get_ncol", CPL_ERROR_NULL_INPUT);

    return -1;

}


/**
 * @brief
 *   Get the type of a table column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Column type, or @c CPL_TYPE_INVALID in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the type of a column.
 */

cpl_type cpl_table_get_column_type(const cpl_table *table, const char *name)
{
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return CPL_TYPE_INVALID;
    }

    return cpl_column_get_type(column);

}


/**
 * @brief
 *   Get the depth of a table column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Column depth, or -1 in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the depth of a column. Columns of type @em array always have positive
 * depth, while columns listing numbers or character strings have depth 0.
 */

cpl_size cpl_table_get_column_depth(const cpl_table *table, const char *name)
{

    const cpl_column  *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    return cpl_column_get_depth(column);

}


/**
 * @brief
 *   Get the number of dimensions of a table column of arrays.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Column number of dimensions, or 0 in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the number of dimensions of a column. If a column is not an array
 * column, or if it has no dimensions, 1 is returned.
 */

cpl_size cpl_table_get_column_dimensions(const cpl_table *table, 
                                         const char *name)
{

    cpl_size dim;
    const cpl_column  *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0;
    }

    dim = cpl_column_get_dimensions(column);

    if (dim == 0) {
        (void)cpl_error_set_where_();
    }

    return dim;
}


/**
 * @brief
 *   Set the dimensions of a table column of arrays.
 *
 * @param table      Pointer to table.
 * @param name       Column name.
 * @param dimensions Integer array containing the sizes of the column dimensions
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Either the specified column is not of type <i>array</i>,
 *         or the <i>dimensions</i> array contains invalid elements.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The <i>dimensions</i> array is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The specified dimensions are incompatible with the total number
 *         of elements in the column arrays.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Set the number of dimensions of a column. If the @em dimensions array
 * has size less than 2, nothing is done and no error is returned. 
 */

cpl_error_code cpl_table_set_column_dimensions(cpl_table *table,
                                               const char *name,
                                               const cpl_array *dimensions)
{
    cpl_column  *column = cpl_table_find_column_(table, name);

    if (dimensions == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return column ? cpl_column_set_dimensions(column, dimensions)
        : cpl_error_set_where_();
}


/**
 * @brief
 *   Get size of one dimension of a table column of arrays.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 * @param indx   Indicate dimension to query (0 = x, 1 = y, 2 = z, etc.).
 *
 * @return Size of queried dimension of the column, or zero in case of error.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSUPPORTED_MODE</td>
 *       <td class="ecr">
 *         The specified column is not of type <i>array</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The specified <i>indx</i> array is not compatible with
 *         the column dimensions.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The specified dimensions are incompatible with the total number
 *         of elements in the column arrays.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get the size of one dimension of a column. If a column is not an array
 * column, or if it has no dimensions, 1 is returned.
 */

cpl_size cpl_table_get_column_dimension(const cpl_table *table,
                                        const char *name, cpl_size indx)
{
    cpl_size dim;
    const cpl_column  *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0;
    }

    dim = cpl_column_get_dimension(column, indx);

    if (dim == 0) {
        (void)cpl_error_set_where_();
    }

    return dim;
}


/**
 * @brief
 *   Give a new unit to a table column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param unit    New unit.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input unit string is duplicated before being used as the column
 * unit. If @em unit is a @c NULL pointer, the column will be unitless.
 * The unit associated to a column has no effect on any operation performed
 * on columns, and it must be considered just an optional description of
 * the content of a column. It is however saved to a FITS file when using
 * cpl_table_save().
 */

cpl_error_code cpl_table_set_column_unit(cpl_table *table, const char *name,
                                         const char *unit)
{

    cpl_column  *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_unit(column, unit)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Get the unit of a table column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Unit of column, or @c NULL if no unit can be returned.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Return the unit of a column if present, otherwise a NULL pointer is 
 * returned. Note that the returned string is a pointer to the column 
 * unit, not its copy. Its manipulation will directly affect the column 
 * unit, while changing the column unit using @c cpl_column_set_unit() 
 * will turn it into garbage. Therefore it should be considered read-only, 
 * and if a real copy of a column unit is required, this function should 
 * be called as an argument of the function @c strdup().
 */

const char *cpl_table_get_column_unit(const cpl_table *table, const char *name)
{

    const cpl_column  *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    /* May return NULL but cannot fail now */
    return cpl_column_get_unit(column);
}


/**
 * @brief
 *   Give a new format to a table column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param format  New format.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input format string is duplicated before being used as the column
 * format. If @em format is a @c NULL pointer, "%%s" will be used if
 * the column is of type @c CPL_TYPE_STRING, "% 1.5e" if the column is
 * of type @c CPL_TYPE_FLOAT or @c CPL_TYPE_DOUBLE, and "% 7d" if it is
 * of type @c CPL_TYPE_INT. The format associated to a column has no
 * effect on any operation performed on columns, and it is used just
 * in the @c printf() calls made while printing a table using the
 * function @c cpl_table_dump(). This information is lost after saving
 * the table in FITS format using @c cpl_table_save().
 */

cpl_error_code cpl_table_set_column_format(cpl_table *table,
                                           const char *name, const char *format)
{

    cpl_column  *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_format(column, format)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Get the format of a table column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Format of column, or @c NULL in case of error.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Return the format of a column. Note that the returned string is
 * a pointer to the column format, not its copy. Its manipulation
 * will directly affect the column format, while changing the column
 * format using @c cpl_column_set_format() will turn it into garbage.
 * Therefore it should be considered read-only, and if a real copy of
 * a column format is required, this function should be called as an
 * argument of the function @c strdup().
 */

const char *cpl_table_get_column_format(const cpl_table *table,
                                        const char *name)
{

    const char * format;
    const cpl_column  *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    format = cpl_column_get_format(column);

    if (format == NULL) {
        (void)cpl_error_set_where_();
    }

    return format;
}


/**
 * @brief
 *   Get a pointer to @em integer column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table> 
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_INT includes an array of
 * values of type @em int. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, @c cpl_table_fill_invalid_int()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_int() for further details.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

int *cpl_table_get_data_int(cpl_table *table, const char *name)
{
    /* Modified from the corresponding const accessor */
    cpl_column  *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_INT);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_int(column);
}


/**
 * @brief
 *   Get a pointer to constant @em integer column data.
 *
 * @param table  Pointer to constant table.
 * @param name   Column name.
 *
 * @return Pointer to constant column data, or @c NULL if the column 
 *   has zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_INT includes an array of
 * values of type @em int. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, @c cpl_table_fill_invalid_int()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_int() for further details.
 */

const int *cpl_table_get_data_int_const(const cpl_table *table, 
                                        const char *name)
{
    const cpl_column  *column = cpl_table_find_column_type_const(table, name,
                                                                 CPL_TYPE_INT);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_int_const(column);
}


/**
 * @brief
 *   Get a pointer to @em long column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_LONG includes an array of
 * values of type @em long. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, @c cpl_table_fill_invalid_long()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_long() for further details.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

long *cpl_table_get_data_long(cpl_table *table, const char *name)
{
    /* Modified from the corresponding const accessor */
    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_long(column);
}


/**
 * @brief
 *   Get a pointer to constant @em long column data.
 *
 * @param table  Pointer to constant table.
 * @param name   Column name.
 *
 * @return Pointer to constant column data, or @c NULL if the column
 *   has zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_LONG includes an array of
 * values of type @em long. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, @c cpl_table_fill_invalid_long()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_long() for further details.
 */

const long *cpl_table_get_data_long_const(const cpl_table *table,
                                          const char *name)
{
    const cpl_column *column = cpl_table_find_column_type_const(table, name,
                                                                CPL_TYPE_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_long_const(column);

}


/**
 * @brief
 *   Get a pointer to @em long @em long column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_LONG_LONG includes an array of
 * values of type @em long long. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this,
 * @c cpl_table_fill_invalid_long_long() should be called just before this
 * function, assigning to all the invalid column elements an @em ad @em hoc
 * numerical value. See the description of function
 * @c cpl_table_fill_invalid_long_long() for further details.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

long long *cpl_table_get_data_long_long(cpl_table *table, const char *name)
{
    /* Modified from the corresponding const accessor */
    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_LONG_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_long_long(column);
}


/**
 * @brief
 *   Get a pointer to constant @em long long column data.
 *
 * @param table  Pointer to constant table.
 * @param name   Column name.
 *
 * @return Pointer to constant column data, or @c NULL if the column
 *   has zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_LONG_LONG includes an array of
 * values of type @em long long. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this,
 * @c cpl_table_fill_invalid_long_long() should be called just before this
 * function, assigning to all the invalid column elements an @em ad @em hoc
 * numerical value. See the description of function
 * @c cpl_table_fill_invalid_long_long() for further details.
 */

const long long *cpl_table_get_data_long_long_const(const cpl_table *table,
                                                    const char *name)
{
    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_LONG_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_long_long_const(column);

}


/**
 * @brief
 *   Get a pointer to @em float column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_FLOAT includes an array
 * of values of type @em float. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, @c cpl_table_fill_invalid_float()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_float() for further details.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

float *cpl_table_get_data_float(cpl_table *table, const char *name)
{
    /* Modified from the corresponding const accessor */
    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_FLOAT);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_float(column);
}


/**
 * @brief
 *   Get a pointer to constant @em float column data.
 *
 * @param table  Pointer to constant table.
 * @param name   Column name.
 *
 * @return Pointer to constant column data, or @c NULL if the column 
 *   has zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_FLOAT includes an array
 * of values of type @em float. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, @c cpl_table_fill_invalid_float()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_float() for further details.
 */

const float *cpl_table_get_data_float_const(const cpl_table *table, 
                                            const char *name)
{

    const cpl_column *column = cpl_table_find_column_type_const(table, name,
                                                                CPL_TYPE_FLOAT);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_float_const(column);

}


/**
 * @brief
 *   Get a pointer to @em float complex column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_FLOAT_COMPLEX includes an array
 * of values of type @em float complex. This function returns a pointer to 
 * this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, 
 * @c cpl_table_fill_invalid_float_complex()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_float_complex() for further details.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

float complex *cpl_table_get_data_float_complex(cpl_table *table, 
                                                const char *name)
{
    /* Modified from the corresponding const accessor */

    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_FLOAT_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_float_complex(column);
}


/**
 * @brief
 *   Get a pointer to constant @em float complex column data.
 *
 * @param table  Pointer to constant table.
 * @param name   Column name.
 *
 * @return Pointer to constant column data, or @c NULL if the column 
 *   has zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_FLOAT_COMPLEX includes an array
 * of values of type @em float complex. This function returns a pointer to 
 * this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, 
 * @c cpl_table_fill_invalid_float_complex()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_float_complex() for further details.
 */ 

const float complex *
cpl_table_get_data_float_complex_const(const cpl_table *table,
                                       const char *name)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_FLOAT_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_float_complex_const(column);

}


/**
 * @brief
 *   Get a pointer to @em double column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_DOUBLE includes an array of
 * values of type @em double. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would in
 * general contain garbage. To avoid this, @c cpl_table_fill_invalid_double()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_double() for further details.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

double *cpl_table_get_data_double(cpl_table *table, const char *name)
{
    /* Modified from the corresponding const accessor */
    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_DOUBLE);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_double(column);
}


/**
 * @brief
 *   Get a pointer to constant @em double column data.
 * 
 * @param table  Pointer to constant table.
 * @param name   Column name.
 * 
 * @return Pointer to constant column data, or @c NULL if the column 
 *   has zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td> 
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_DOUBLE includes an array of
 * values of type @em double. This function returns a pointer to this array.
 * The data buffer elements corresponding to invalid column elements would in
 * general contain garbage. To avoid this, @c cpl_table_fill_invalid_double()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_double() for further details.
 */

const double *cpl_table_get_data_double_const(const cpl_table *table, 
                                              const char *name)
{
    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_DOUBLE);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_double_const(column);

}


/**
 * @brief
 *   Get a pointer to @em double complex column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_DOUBLE_COMPLEX includes an array
 * of values of type @em double complex. This function returns a pointer to 
 * this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, 
 * @c cpl_table_fill_invalid_double_complex()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_double_complex() for further details.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

double complex *cpl_table_get_data_double_complex(cpl_table *table, 
                                                  const char *name)
{
    /* Modified from the corresponding const accessor */

    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_DOUBLE_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_double_complex(column);

}


/**
 * @brief
 *   Get a pointer to constant @em double complex column data.
 *
 * @param table  Pointer to constant table.
 * @param name   Column name.
 *
 * @return Pointer to constant column data, or @c NULL if the column 
 *   has zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A @em cpl_table column of type @c CPL_TYPE_DOUBLE_COMPLEX includes an array
 * of values of type @em double complex. This function returns a pointer to 
 * this array.
 * The data buffer elements corresponding to invalid column elements would
 * in general contain garbage. To avoid this, 
 * @c cpl_table_fill_invalid_double_complex()
 * should be called just before this function, assigning to all the invalid
 * column elements an @em ad @em hoc numerical value. See the description
 * of function @c cpl_table_fill_invalid_double_complex() for further details.
 */ 

const double complex *
cpl_table_get_data_double_complex_const(const cpl_table *table,
                                        const char *name)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_DOUBLE_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_double_complex_const(column);

}


/**
 * @brief
 *   Get a pointer to @em string column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A table column of type @c CPL_TYPE_STRING includes an array of values
 * of type @em char*. This function returns a pointer to this array.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

char **cpl_table_get_data_string(cpl_table *table, const char *name)
{
    /* Modified from the corresponding const acessor */
    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_STRING);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_string(column);
}


/**
 * @brief
 *   Get a pointer to constant @em string column data.
 *
 * @param table  Pointer to constant table.
 * @param name   Column name.
 *
 * @return Pointer to constant column data, or @c NULL if the column has 
 *   zero length, or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A table column of type @c CPL_TYPE_STRING includes an array of values
 * of type @em char*. This function returns a pointer to this array.
 */

const char **cpl_table_get_data_string_const(const cpl_table *table, 
                                                    const char *name)
{
    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_STRING);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_string_const(column);

}


/**
 * @brief
 *   Get a pointer to @em array column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A table column of type @em array includes an array of values
 * of type @em cpl_array*. This function returns a pointer to this array.
 *
 * @note
 *   Use at your own risk: direct manipulation of column data rules out
 *   any check performed by the table object interface, and may introduce
 *   inconsistencies between the information maintained internally, and
 *   the actual column data and structure.
 */

cpl_array **cpl_table_get_data_array(cpl_table *table, const char *name)
{
    /* Modified from the corresponding const acessor */
    cpl_column  *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_POINTER);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_array(column);
}


/**
 * @brief
 *   Get a pointer to @em array column data.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Pointer to column data, or @c NULL if the column has zero length,
 *   or in case of failure.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A table column of type @em array includes an array of values
 * of type @em cpl_array*. This function returns a pointer to this array.
 */

const cpl_array **cpl_table_get_data_array_const(const cpl_table *table, 
                                                        const char *name)
{
    const cpl_column  *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_POINTER);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_data_array_const(column);

}


/**
 * @brief
 *   Delete a column from a table.
 *
 * @param table  Pointer to table.
 * @param name   Name of table column to delete.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Delete a column from a table. If the table is left without columns,
 * also the selection flags are lost.
 */

cpl_error_code cpl_table_erase_column(cpl_table *table, const char *name)
{

    cpl_column  *column;


    if (table == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    column = cpl_table_extract_column(table, name);

    if (!column)
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    cpl_column_delete(column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Delete a table segment.
 *
 * @param table   Pointer to table.
 * @param start   First row to delete.
 * @param count   Number of rows to delete.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input table has length zero, or <i>start</i> is
 *         outside the table range.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A portion of the table data is physically removed. The pointers to column
 * data may change, therefore pointers previously retrieved by calling
 * @c cpl_table_get_data_int(), @c cpl_table_get_data_string(), etc.,
 * should be discarded. The table selection flags are set back to 
 * "all selected". The specified segment can extend beyond the end of 
 * the table, and in that case rows will be removed up to the end of 
 * the table.
 */

cpl_error_code cpl_table_erase_window(cpl_table *table, 
                                      cpl_size start, cpl_size count)
{

    cpl_size     width = cpl_table_get_ncol(table);
    cpl_column **column;


    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count > table->nr - start)
        count = table->nr - start;

    column = table->columns;

    /*
     * If it will fail, it will be at the first column: the table
     * will never be returned half-done.
     */

    while (width--) {
        if (cpl_column_erase_segment(*column++, start, count)) {
            return cpl_error_set_where_();
        }
    }

    table->nr -= count;

    return cpl_table_select_all(table);

}


/**
 * @brief
 *   Delete the selected rows of a table.
 *
 * @param table   Pointer to table
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A portion of the table data is physically removed. The pointer to
 * column data may change, therefore pointers previously retrieved by
 * calling @c cpl_table_get_data_int(), @c cpl_table_get_data_string(),
 * etc., should be discarded. The table selection flags are set back to 
 * "all selected".
 */

cpl_error_code cpl_table_erase_selected(cpl_table *table)
{

    cpl_size    length = cpl_table_get_nrow(table);
    cpl_size    i, width;

    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (table->selectcount == 0)
        return cpl_table_select_all(table);

    if (table->selectcount == length)
        return cpl_table_set_size(table, 0);

    width = cpl_table_get_ncol(table);

    for (i = 0; i < width; i++)
        if (cpl_column_erase_pattern(table->columns[i], table->select)) {
            return cpl_error_set_where_();
        }

    table->nr -= table->selectcount;

    return cpl_table_select_all(table);
}


/**
 * @brief
 *   Insert a segment of rows into table data.
 *
 * @param table   Pointer to table
 * @param start   Row where to insert the segment.
 * @param count   Length of the segment.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         <i>start</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Insert a segment of empty rows, just containing invalid elements.
 * Setting @em start to a number greater than the column length is legal,
 * and has the effect of appending extra rows at the end of the table:
 * this is equivalent to expanding the table using @c cpl_table_set_size().
 * The input @em column may also have zero length. The pointers to column
 * data values may change, therefore pointers previously retrieved by
 * calling @c cpl_table_get_data_int(), @c cpl_table_get_data_string(),
 * etc., should be discarded. The table selection flags are set back to 
 * "all selected".
 */

cpl_error_code cpl_table_insert_window(cpl_table *table, 
                                       cpl_size start, cpl_size count)
{

    cpl_size     width = cpl_table_get_ncol(table);
    cpl_column **column;


    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);


    column = table->columns;

    /*
     * If it will fail, it will be at the first column: the table
     * will never be returned half-done.
     */

    while (width--) {
        if (cpl_column_insert_segment(*column++, start, count)) {
            return cpl_error_set_where_();
        }
    }

    table->nr += count;

    return cpl_table_select_all(table);

}


/**
 * @brief
 *   Compare the structure of two tables.
 *
 * @param table1  Pointer to a table.
 * @param table2  Pointer to another table.
 *
 * @return 0 if the tables have the same structure, 1 otherwise.
 *   In case of error, -1 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any argument is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Two tables have the same structure if they have the same number
 * of columns, with the same names, the same types, and the same units. 
 * The order of the columns is not relevant.
 */

int cpl_table_compare_structure(const cpl_table *table1,
                                const cpl_table *table2)
{

    cpl_size    width1 = cpl_table_get_ncol(table1);
    cpl_size    width2 = cpl_table_get_ncol(table2);
    const char *name   = NULL;


    if (table1 == NULL || table2 == NULL) {
        cpl_error_set_where_();
        return -1;
    }


    if (width1 == width2) {

        cpl_array *names = cpl_table_get_column_names(table1);
        cpl_size   i;
        int result = 0;

        for (i = 0; i < width1; i++) {
            name = cpl_array_get_string(names, i);

            if (!cpl_table_has_column(table2, name)) {
                result = 1;
                break;
            }

            if (cpl_table_get_column_type(table1, name) !=
                cpl_table_get_column_type(table2, name)) {
                result = 1;
                break;
            }

            if (cpl_table_get_column_depth(table1, name) !=
                cpl_table_get_column_depth(table2, name)) {
                result = 1;
                break;
            }

            if (cpl_table_get_column_unit(table1, name) &&
                cpl_table_get_column_unit(table2, name)) {
                if (strcmp(cpl_table_get_column_unit(table1, name),
                           cpl_table_get_column_unit(table2, name)) != 0) {
                    result = 1;
                    break;
                }
            }

            if (cpl_table_get_column_unit(table1, name) == NULL &&
                cpl_table_get_column_unit(table2, name)) {
                result = 1;
                break;
            }

            if (cpl_table_get_column_unit(table2, name) == NULL &&
                cpl_table_get_column_unit(table1, name)) {
                result = 1;
                break;
            }
        }

        cpl_array_delete(names);

        return result;

    }

    return 1;

}


/**
 * @brief
 *   Merge two tables.
 *
 * @param target_table Target table.
 * @param insert_table Table to be inserted in the target table.
 * @param row          Row where to insert the insert table.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input table is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         <i>row</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The input tables do not have the same structure.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input tables must have the same structure, as defined by the function
 * @c cpl_table_compare_structure() . Data from the @em insert_table are
 * duplicated and inserted at the specified position of the @em target_table.
 * If the specified @em row is not less than the target table length, the
 * second table will be appended to the target table. The selection flags
 * of the target table are always set back to "all selected". The pointers 
 * to column data in the target table may change, therefore pointers 
 * previously retrieved by calling @c cpl_table_get_data_int(), 
 * @c cpl_table_get_data_string(), etc., should be discarded.
 */

cpl_error_code cpl_table_insert(cpl_table *target_table,
                                const cpl_table *insert_table, cpl_size row)
{

    cpl_size     width = cpl_table_get_ncol(target_table);
    cpl_column **column;


    if (cpl_table_compare_structure(target_table, insert_table))
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    column = target_table->columns;

    while (width--) {

        if (cpl_column_merge(*column, cpl_table_find_column_const(insert_table,
                             cpl_column_get_name(*column)), row)) {
            return cpl_error_set_where_();
        }
        column++;

    }

    target_table->nr += insert_table->nr;
    return cpl_table_select_all(target_table);

}


/**
 * @brief
 *   Read a value from a numerical column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to be accessed.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Value read. In case of invalid table element, or in case of
 *   error, 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical, or is a column of arrays.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Rows are counted starting from 0. The @em null flag is used to
 * indicate whether the accessed table element is valid (0) or
 * invalid (1). The @em null flag also signals an error condition (-1).
 * The @em null argument can be left to @c NULL.
 */

double cpl_table_get(const cpl_table *table,
                     const char *name, cpl_size row, int *null)
{

    cpl_type    type;
    const cpl_column  *column  = cpl_table_find_column_const_(table, name);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    type = cpl_column_get_type(column);

    if ((type == CPL_TYPE_STRING) 
     || (type & CPL_TYPE_POINTER) 
     || (type & CPL_TYPE_COMPLEX)) {
        (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "name='%s', row=%"
                                     CPL_SIZE_FORMAT ", type='%s'", name, row,
                                     cpl_type_get_name(type));
        return 0.0;
    }

    return cpl_column_get(column, row, null);

}


/**
 * @brief
 *   Read a value from a complex column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to be accessed.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Value read. In case of invalid table element, or in case of
 *   error, 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not complex, or is a column of arrays.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Rows are counted starting from 0. The @em null flag is used to
 * indicate whether the accessed table element is valid (0) or
 * invalid (1). The @em null flag also signals an error condition (-1).
 * The @em null argument can be left to @c NULL.
 */

double complex cpl_table_get_complex(const cpl_table *table,
                                     const char *name, cpl_size row, int *null)
{

    cpl_type    type;
    const cpl_column  *column = cpl_table_find_column_const_(table, name);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    type = cpl_column_get_type(column);

    if (!(type & CPL_TYPE_COMPLEX) || (type & CPL_TYPE_POINTER)) {
        (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "name='%s', row=%"
                                     CPL_SIZE_FORMAT ", type='%s'", name, row,
                                     cpl_type_get_name(type));
        return 0.0;
    }

    return cpl_column_get_complex(column, row, null);

}


/**
 * @brief 
 *   Read a value from an @em integer column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Integer value read. In case of an invalid table element, or in
 *   case of error, 0 is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_INT. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * column element is valid (0) or invalid (1). The @em null flag also signals
 * an error condition (-1). The @em null flag pointer can also be @c NULL,
 * and in that case this option will be disabled. Rows are counted starting
 * from 0.
 *
 * @note
 *   For automatic conversion (always to type @em double), use the function
 *   @c cpl_table_get().
 */

int cpl_table_get_int(const cpl_table *table,
                      const char *name, cpl_size row, int *null)
{

    const cpl_column  *column;
    cpl_type     type;


    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (name == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "name is NULL");
        return 0;
    }

    if (table == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "table is NULL");
        return 0;
    }

    column = cpl_table_find_column_const(table, name);

    if (!column) {
        (void)cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "name='%s', "
                                     "row=%" CPL_SIZE_FORMAT, name, row);
        return 0;
    }

    type = cpl_column_get_type(column);

    if (type != CPL_TYPE_INT) {
        (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH, "name='%s', row=%"
                                     CPL_SIZE_FORMAT ", Non-int type='%s'",
                                     name, row, cpl_type_get_name(type));
        return 0;
    }

    return cpl_column_get_int(column, row, null);

}


/**
 * @brief
 *   Read a value from a @em long column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Long integer value read. In case of an invalid table element, or in
 *   case of error, 0 is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_LONG. See the documentation
 * of function cpl_table_get_int().
 */

long cpl_table_get_long(const cpl_table *table, const char *name,
                        cpl_size row, int *null)
{

    const cpl_column  *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_LONG);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0L;
    }

    return cpl_column_get_long(column, row, null);

}


/**
 * @brief
 *   Read a value from a @em long @em long column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Long long integer value read. In case of an invalid table element,
 *   or in case of error, 0 is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_LONG_LONG. See the
 * documentation of function cpl_table_get_int().
 */

long long cpl_table_get_long_long(const cpl_table *table, const char *name,
                                  cpl_size row, int *null)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_LONG_LONG);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0L;
    }

    return cpl_column_get_long_long(column, row, null);

}


/**
 * @brief
 *   Read a value from a @em float column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Float value read. In case of an invalid table element, or in
 *   case of error, 0.0 is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_FLOAT. See the documentation
 * of function cpl_table_get_int().
 */

float cpl_table_get_float(const cpl_table *table, const char *name,
                          cpl_size row, int *null)
{

    const cpl_column  *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_FLOAT);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    return cpl_column_get_float(column, row, null);

}


/**
 * @brief
 *   Read a value from a @em float complex column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Float complex value read. In case of an invalid table element, or in
 *   case of error, 0.0 is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_FLOAT_COMPLEX. 
 * See the documentation of function cpl_table_get_int().
 */

float complex cpl_table_get_float_complex(const cpl_table *table, 
                                          const char *name, cpl_size row, 
                                          int *null)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_FLOAT_COMPLEX);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    return cpl_column_get_float_complex(column, row, null);

}


/**
 * @brief
 *   Read a value from a @em double column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Double value read. In case of an invalid table element, or in
 *   case of error, 0.0 is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_DOUBLE. See the
 * documentation of function cpl_table_get_int().
 */

double cpl_table_get_double(const cpl_table *table, const char *name,
                            cpl_size row, int *null)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_DOUBLE);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    return cpl_column_get_double(column, row, null);

}


/**
 * @brief
 *   Read a value from a @em double complex column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 * @param null    Flag indicating @em null values, or error condition.
 *
 * @return Double complex value read. In case of an invalid table element, or in
 *   case of error, 0.0 is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_DOUBLE_COMPLEX. See the
 * documentation of function cpl_table_get_int().
 */

double complex cpl_table_get_double_complex(const cpl_table *table, 
                                            const char *name,
                                            cpl_size row, int *null)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_DOUBLE_COMPLEX);

    if (null)
        *null = -1; /* Ensure initialization in case of an error */

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    return cpl_column_get_double_complex(column, row, null);

}


/**
 * @brief
 *   Read a value from a @em string column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 *
 * @return Pointer to string. In case of an invalid column element, or in
 *   case of error, a @c NULL pointer is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of type @c CPL_TYPE_STRING.
 * Rows are counted starting from 0.
 *
 * @note
 *   The returned string is a pointer to a table element, not its copy.
 *   Its manipulation will directly affect that element, while changing
 *   that element using @c cpl_table_set_string() will turn it into garbage.
 *   Therefore, if a real copy of a string column element is required, this
 *   function should be called as an argument of the function @c strdup().
 */

const char *cpl_table_get_string(const cpl_table *table,
                                 const char *name, cpl_size row)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_STRING);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_string_const(column, row);

}


/**
 * @brief
 *   Read an array from an @em array column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position of element to be read.
 *
 * @return Pointer to array. In case of an invalid column element, or in
 *   case of error, a @c NULL pointer is always returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Read a value from a column of any array type.
 * Rows are counted starting from 0.
 *
 * @note
 *   The returned array is a pointer to a table element, not its copy.
 *   Its manipulation will directly affect that element, while changing
 *   that element using @c cpl_table_set_array() will turn it into garbage.
 *   Therefore, if a real copy of an array column element is required,
 *   this function should be called as an argument of the function
 *   @c cpl_array_duplicate().
 */

const cpl_array *cpl_table_get_array(const cpl_table *table,
                                     const char *name, cpl_size row)
{

    const cpl_column *column =
        cpl_table_find_column_type_const(table, name, CPL_TYPE_POINTER);

    if (!column) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    return cpl_column_get_array_const(column, row);

}


/**
 * @brief
 *   Write a value to a numerical table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical, or it is of type
 *         <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a numerical column element. The value is cast to the
 * accessed column type according to the C casting rules. The written value
 * is automatically marked as valid. To invalidate a column value use
 * @c cpl_table_set_invalid(). Table rows are counted starting from 0.
 */

cpl_error_code cpl_table_set(cpl_table *table,
                             const char *name, cpl_size row, double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a complex value to a complex numerical table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not complex, or it is of type
 *         <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a complex column element. The value is cast to the
 * accessed column type according to the C casting rules. The written value
 * is automatically marked as valid. To invalidate a column value use
 * @c cpl_table_set_invalid(). Table rows are counted starting from 0.
 */

cpl_error_code cpl_table_set_complex(cpl_table *table,
                                     const char *name, 
                                     cpl_size row, double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_complex(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to an @em integer table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a table column of type @c CPL_TYPE_INT. The written
 * value is automatically marked as valid. To invalidate a column value use
 * @c cpl_table_set_invalid(). Table rows are counted starting from 0.
 *
 * @note
 *   For automatic conversion to the column type, use the function
 *   @c cpl_table_set().
 */

cpl_error_code cpl_table_set_int(cpl_table *table,
                                 const char *name, cpl_size row, int value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_int(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to an @em long table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a table column of type @c CPL_TYPE_LONG. The written
 * value is automatically marked as valid. To invalidate a column value use
 * @c cpl_table_set_invalid(). Table rows are counted starting from 0.
 *
 * @note
 *   For automatic conversion to the column type, use the function
 *   @c cpl_table_set().
 */

cpl_error_code cpl_table_set_long(cpl_table *table,
                                  const char *name, cpl_size row, long value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_long(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to an @em long @em long table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a table column of type @c CPL_TYPE_LONG_LONG. The written
 * value is automatically marked as valid. To invalidate a column value use
 * @c cpl_table_set_invalid(). Table rows are counted starting from 0.
 *
 * @note
 *   For automatic conversion to the column type, use the function
 *   @c cpl_table_set().
 */

cpl_error_code cpl_table_set_long_long(cpl_table *table,
                                       const char *name, cpl_size row,
                                       long long value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_long_long(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to a @em float table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a table column of type @c CPL_TYPE_FLOAT. The written
 * value is automatically marked as valid. To invalidate a column value use
 * @c cpl_table_set_invalid(). Table rows are counted starting from 0.
 *
 * @note
 *   For automatic conversion to the column type, use the function
 *   @c cpl_table_set().
 */

cpl_error_code cpl_table_set_float(cpl_table *table, const char *name,
                                   cpl_size row, float value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_float(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em float complex table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a table column of type @c CPL_TYPE_FLOAT_COMPLEX. 
 * The written value is automatically marked as valid. To invalidate 
 * a column value use @c cpl_table_set_invalid(). Table rows are counted 
 * starting from 0.
 *
 * @note
 *   For automatic conversion to the column type, use the function
 *   @c cpl_table_set_complex().
 */

cpl_error_code cpl_table_set_float_complex(cpl_table *table, const char *name,
                                           cpl_size row, float complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_float_complex(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em double table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a table column of type @c CPL_TYPE_DOUBLE. The written
 * value is automatically marked as valid. To invalidate a column value use
 * @c cpl_table_set_invalid(). Table rows are counted starting from 0.
 *
 * @note
 *   For automatic conversion to the column type, use the function
 *   @c cpl_table_set().
 */

cpl_error_code cpl_table_set_double(cpl_table *table, const char *name,
                                    cpl_size row, double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_double(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em double complex table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write value.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a value to a table column of type @c CPL_TYPE_DOUBLE_COMPLEX. 
 * The written value is automatically marked as valid. To invalidate 
 * a column value use @c cpl_table_set_invalid(). Table rows are counted 
 * starting from 0.
 *
 * @note
 *   For automatic conversion to the column type, use the function
 *   @c cpl_table_set_complex().
 */

cpl_error_code cpl_table_set_double_complex(cpl_table *table, const char *name,
                                            cpl_size row, double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_double_complex(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a character string to a @em string table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write the character string.
 * @param value   Character string to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write a string to a table column of type @c CPL_TYPE_STRING. The written
 * value can also be a @c NULL pointer, that is equivalent to a call to
 * @c cpl_table_set_invalid(). Note that the character string is copied,
 * therefore the original can be modified without affecting the table
 * element. To "plug" a character string directly into a table element,
 * use the function @c cpl_table_get_data_string().
 */

cpl_error_code cpl_table_set_string(cpl_table *table, const char *name,
                                    cpl_size row, const char *value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_string(column, row, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write an array to an @em array table column element.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Position where to write the array.
 * @param array   Array to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <i>array</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *          The size of the input <i>array</i> is different from the
 *          depth of the specified column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write an array to a table column of type @em array. The written
 * value can also be a @c NULL pointer, that is equivalent to a call
 * to @c cpl_table_set_invalid(). Note that the array is copied,
 * therefore the original can be modified without affecting the
 * table element. To "plug" an array directly into a table element,
 * use the function @c cpl_table_get_data_array(). Beware that the 
 * "plugged" array must have the same type and depth declared for 
 * the column.
 */

cpl_error_code cpl_table_set_array(cpl_table *table, const char *name,
                                   cpl_size row, const cpl_array *array)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_array(column, row, array)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Flag a column element as invalid.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Row where to write a @em null.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * In the case of either a @em string or an @em array column, the
 * corresponding string or array is set free and its pointer is set
 * to @c NULL. For other data types, the corresponding table element
 * is flagged internally as invalid.
 */

cpl_error_code cpl_table_set_invalid(cpl_table *table, const char *name,
                                     cpl_size row)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_set_invalid(column, row)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to a numerical column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical, or is of type <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a numerical column segment. The value is cast
 * to the type of the accessed column according to the C casting rules.
 * The written values are automatically marked as valid. To invalidate
 * a column interval use @c cpl_table_set_column_invalid() instead.
 * If the sum of @em start and @em count exceeds the number of table
 * rows, the column is filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window(cpl_table *table, const char *name,
                                            cpl_size start, cpl_size count, 
                                            double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to a complex column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not complex, or is of type <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a complex column segment. The value is cast
 * to the type of the accessed column according to the C casting rules.
 * The written values are automatically marked as valid. To invalidate
 * a column interval use @c cpl_table_set_column_invalid() instead.
 * If the sum of @em start and @em count exceeds the number of table
 * rows, the column is filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window_complex(cpl_table *table, 
                                                    const char *name,
                                                    cpl_size start, 
                                                    cpl_size count, 
                                                    double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_complex(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to an @em integer column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to an @em integer column segment. The written
 * values are automatically marked as valid. To invalidate a column
 * interval use @c cpl_table_set_column_invalid() instead. If the sum
 * of @em start and @em count exceeds the number of table rows, the
 * column is filled up to its end.
 *
 * @note
 *   For automatic conversion to the accessed column type use the function
 *   @c cpl_table_fill_column_window().
 */

cpl_error_code cpl_table_fill_column_window_int(cpl_table *table,
                                                const char *name, 
                                                cpl_size start,
                                                cpl_size count, int value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_int(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to an @em long column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to an @em long column segment. The written
 * values are automatically marked as valid. To invalidate a column
 * interval use @c cpl_table_set_column_invalid() instead. If the sum
 * of @em start and @em count exceeds the number of table rows, the
 * column is filled up to its end.
 *
 * @note
 *   For automatic conversion to the accessed column type use the function
 *   @c cpl_table_fill_column_window().
 */

cpl_error_code cpl_table_fill_column_window_long(cpl_table *table,
                                                 const char *name,
                                                 cpl_size start,
                                                 cpl_size count, long value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_long(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to an @em long @em long column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to an @em long @em long column segment. The written
 * values are automatically marked as valid. To invalidate a column
 * interval use @c cpl_table_set_column_invalid() instead. If the sum
 * of @em start and @em count exceeds the number of table rows, the
 * column is filled up to its end.
 *
 * @note
 *   For automatic conversion to the accessed column type use the function
 *   @c cpl_table_fill_column_window().
 */

cpl_error_code cpl_table_fill_column_window_long_long(cpl_table *table,
                                                      const char *name,
                                                      cpl_size start,
                                                      cpl_size count,
                                                      long long value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_long_long(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to a @em float column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a @em float column segment. The written
 * values are automatically marked as valid. To invalidate a column
 * interval use @c cpl_table_set_column_invalid() instead. If the sum
 * of @em start and @em count exceeds the number of table rows, the
 * column is filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window_float(cpl_table *table,
                                                  const char *name, 
                                                  cpl_size start, 
                                                  cpl_size count, float value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_float(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to a @em float complex column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a @em float complex column segment. The written
 * values are automatically marked as valid. To invalidate a column
 * interval use @c cpl_table_set_column_invalid() instead. If the sum
 * of @em start and @em count exceeds the number of table rows, the
 * column is filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window_float_complex(cpl_table *table,
                                                          const char *name, 
                                                          cpl_size start, 
                                                          cpl_size count, 
                                                          float complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_float_complex(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to a @em double column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a @em double column segment. The written
 * values are automatically marked as valid. To invalidate a column
 * interval use @c cpl_table_set_column_invalid() instead. If the sum
 * of @em start and @em count exceeds the number of table rows, the
 * column is filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window_double(cpl_table *table,
                                                   const char *name, 
                                                   cpl_size start,
                                                   cpl_size count, 
                                                   double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_double(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a value to a @em double complex column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the value.
 * @param count   Number of values to write.
 * @param value   Value to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a @em double complex column segment. The written
 * values are automatically marked as valid. To invalidate a column
 * interval use @c cpl_table_set_column_invalid() instead. If the sum
 * of @em start and @em count exceeds the number of table rows, the
 * column is filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window_double_complex(cpl_table *table,
                                                           const char *name, 
                                                           cpl_size start, 
                                                           cpl_size count,
                                                           double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_double_complex(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a character string to a @em string column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the character string.
 * @param count   Number of strings to write.
 * @param value   Character string to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same value to a @em string column segment. If the input string
 * is not a @c NULL pointer, it is duplicated for each accessed column
 * element. If the input string is a @c NULL pointer, this call is equivalent
 * to a call to @c cpl_table_set_column_invalid(). If the sum of @em start
 * and @em count exceeds the number of rows in the table, the column is
 * filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window_string(cpl_table *table,
                                                   const char *name, 
                                                   cpl_size start, 
                                                   cpl_size count,
                                                   const char *value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_string(column, start, count, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write an array to an @em array column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin to write the array.
 * @param count   Number of arrays to write.
 * @param array   Array to write.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column does not match the type of the input
 *         <i>array</i>, or it is  not made of arrays.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *          The size of the input <i>array</i> is different from the
 *          depth of the specified column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Write the same array to a segment of an array column. If the input array
 * is not a @c NULL pointer, it is duplicated for each accessed column
 * element. If the input array is a @c NULL pointer, this call is equivalent
 * to a call to @c cpl_table_set_column_invalid(). If the sum of @em start
 * and @em count exceeds the number of rows in the table, the column is
 * filled up to its end.
 */

cpl_error_code cpl_table_fill_column_window_array(cpl_table *table,
                                                  const char *name, 
                                                  cpl_size start, 
                                                  cpl_size count,
                                                  const cpl_array *array)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_array(column, start, count, array)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Invalidate a column segment.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param start   Position where to begin invalidation.
 * @param count   Number of column elements to invalidate.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * All the column elements in the specified interval are invalidated.
 * In the case of either a @em string or an @em array column, the
 * corresponding strings or arrays are set free. If the sum of @em start
 * and @em count exceeds the number of rows in the table, the column is
 * invalidated up to its end.
 */

cpl_error_code cpl_table_set_column_invalid(cpl_table *table, const char *name,
                                            cpl_size start, cpl_size count)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid(column, start, count)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Check if a column element is valid.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to access.
 * @param row     Column element to examine.
 *
 * @return 1 if the column element is valid, 0 if invalid, -1 in case of
 *    error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Check if a column element is valid.
 */

int cpl_table_is_valid(const cpl_table *table, const char *name, cpl_size row)
{

    int          validity;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    validity = cpl_column_is_invalid(column, row) ? 0 : 1;

    if (!cpl_errorstate_is_equal(prevstate)) {
        cpl_error_set_where_();
        return -1;
    }

    return validity;

}


/**
 * @brief
 *   Check if a column contains at least one invalid value.
 *
 * @param table  Pointer to table.
 * @param name   Name of table column to access.
 *
 * @return 1 if the column contains at least one invalid element, 0 if not,
 *   -1 in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Check if there are invalid elements in a column. In case of columns
 * of arrays, invalid values within an array are not considered: an
 * invalid element here means that an array element is not allocated,
 * i.e., it is a @c NULL pointer. In order to detect invalid elements
 * within an array element, this element must be extracted using
 * the function @c cpl_table_get_array(), and then use the function
 * @c cpl_array_has_invalid().
 */

int cpl_table_has_invalid(const cpl_table *table, const char *name)
{

    int          answer;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    answer = cpl_column_has_invalid(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return answer;

}


/**
 * @brief
 *   Check if a column contains at least one valid value.
 *
 * @param table  Pointer to table.
 * @param name   Name of table column to access.
 *
 * @return 1 if the column contains at least one valid value, 0 if not,
 *   -1 in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Check if there are valid elements in a column. In case of columns
 * of arrays, invalid values within an array are not considered: an
 * invalid element here means that an array element is not allocated,
 * i.e., it is a @c NULL pointer. In order to detect valid elements
 * within an array element, this element must be extracted using
 * the function @c cpl_table_get_array(), and then use the function
 * @c cpl_array_has_valid().
 */

int cpl_table_has_valid(const cpl_table *table, const char *name)
{

    int          answer;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    answer = cpl_column_has_valid(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return answer;

}


/**
 * @brief
 *   Count number of invalid values in a table column.
 *
 * @param table   Pointer to table.
 * @param name    Name of table column to examine.
 *
 * @return Number of invalid elements in a table column, or -1 in case of
 *   error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Count number of invalid elements in a table column.
 */

cpl_size cpl_table_count_invalid(const cpl_table *table, const char *name)
{

    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    return cpl_column_count_invalid_const(column);

}


/**
 * @brief
 *   Move a column from a table to another.
 *
 * @param to_table   Target table.
 * @param name       Name of column to move.
 * @param from_table Source table.
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
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The input tables do not have the same number of rows.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Source and target tables are the same table.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the given <i>name</i> is not found in the
 *         source table.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the same <i>name</i> already exists in the
 *         target table.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Move a column from a table to another.
 */

cpl_error_code cpl_table_move_column(cpl_table *to_table,
                                     const char *name, cpl_table *from_table)
{

    cpl_column  *column;


    if (to_table == NULL || from_table == NULL || name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (to_table == from_table)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (to_table->nr != from_table->nr)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_table_find_column(to_table, name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_table_extract_column(from_table, name);

    if (!column)
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    cpl_table_append_column(to_table, column);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy a column from a table to another.
 *
 * @param to_table   Target table.
 * @param to_name    New name of copied column.
 * @param from_table Source table.
 * @param from_name  Name of column to copy.
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
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The input tables do not have the same number of rows.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>from_name</i> is not found in the
 *         source table.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the specified <i>to_name</i> already exists in the
 *         target table.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Copy a column from a table to another. The column is duplicated. A column
 * may be duplicated also within the same table.
 */

cpl_error_code cpl_table_duplicate_column(cpl_table *to_table,
                                          const char *to_name,
                                          const cpl_table *from_table,
                                          const char *from_name)
{

    const cpl_column  *column_from;
    cpl_column  *column_to;


    if (to_table == NULL || from_table == NULL ||
        to_name == NULL || from_name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (to_table->nr != from_table->nr)
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_table_find_column_const(to_table, to_name))
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_OUTPUT,
                                      "Destination name='%s'", to_name);

    column_from = cpl_table_find_column_const(from_table, from_name);

    if (!column_from)
        return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND,
                                      "Source name='%s'", from_name);

    column_to = cpl_column_duplicate(column_from);
    cpl_column_set_name(column_to, to_name);
    cpl_table_append_column(to_table, column_to);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Rename a table column.
 *
 * @param table     Pointer to table.
 * @param from_name Name of table column to rename.
 * @param to_name   New name of column.
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
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>from_name</i> is not found
 *         in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the specified <i>to_name</i> already exists in
 *         <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function is used to change the name of a column.
 */

cpl_error_code cpl_table_name_column(cpl_table *table, const char *from_name,
                                       const char *to_name)
{

    cpl_column  *column;


    if (from_name == NULL || to_name == NULL || table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_table_find_column(table, to_name))
        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    column = cpl_table_find_column(table, from_name);

    if (!column)
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    return cpl_column_set_name(column, to_name);

}


/**
 * @brief
 *   Check if a column with a given name exists.
 *
 * @param table Pointer to table.
 * @param name  Name of table column.
 *
 * @return 1 if column exists, 0 if column doesn't exist, -1 in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any argument is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Check if a column with a given name exists in the specified table.
 */

int cpl_table_has_column(const cpl_table *table, const char *name)
{

    if (table == NULL || name == NULL) {
        cpl_error_set("cpl_table_has_column", CPL_ERROR_NULL_INPUT);
        return -1;
    }

    return (cpl_table_find_column_const(table, name) != NULL ? 1 : 0);

}


/**
 * @brief
 *   Get table columns names.
 *
 * @param table  Pointer to table.
 *
 * @return Name of a table column.
 *
 * If this function is not called with a @c NULL pointer the name of
 * the first table column will be returned. Further calls made with
 * a @c NULL pointer would return the next columns names, till the
 * end of the list of columns when a @c NULL would be returned. This
 * function only guarantees that all the table column names would be
 * returned by subsequent calls to this function, but the order in which
 * the column names are returned is undefined. The table structure must
 * not be modified (e.g. by deleting, creating, moving, or renaming
 * columns) between a sequence of calls to @c cpl_table_get_column_name()
 * related to the same table, or this function behaviour will be
 * undetermined. This function returns a pointer to the table column
 * name, and not to its copy, therefore the pointed string shouldn't
 * be deallocated or manipulated in any way. Its manipulation would
 * directly affect the column name, while changing the column name
 * using @c cpl_table_name_column() would turn it into garbage.
 * Therefore, if a real copy of a column name is required, this
 * function should be called as an argument of the function @c strdup().
 *
 * @deprecated
 *   This function is deprecated, because its usage could create 
 *   serious problems in case it is attempted to get names from 
 *   different tables simultaneously. For instance, a programmer 
 *   may call cpl_table_get_column_name() in a loop, and in the 
 *   same loop call a CPL function that calls as well the same 
 *   function. The behaviour in this case would be unpredictable.
 *   The function cpl_table_get_column_names() should be used
 *   instead.
 */

const char *cpl_table_get_column_name(const cpl_table *table)
{

    static const cpl_table *looking = NULL;
    static cpl_size   width   = 0;
    static cpl_size   i       = 0;


    if (table) {
        looking = table;
        width = cpl_table_get_ncol(table);
        i = 0;
    }
    else {
        if (looking)
            i++;
        else
            return NULL;
    }

    if (i < width)
        return cpl_column_get_name(looking->columns[i]);

    return NULL;

}


/**
 * @brief
 *   Get table columns names.
 *
 * @param table  Pointer to table.
 *
 * @return Array of table columns names.
 *
 * The returned CPL array of strings should be finally destroyed
 * using cpl_array_delete().
 */
    
cpl_array *cpl_table_get_column_names(const cpl_table *table)
{
    cpl_array  *names;
    cpl_size    i;


    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    names = cpl_array_new(table->nc, CPL_TYPE_STRING);
    for (i = 0; i < table->nc; i++)
       cpl_array_set_string(names, i, cpl_column_get_name(table->columns[i]));

    return names;

}


/**
 * @brief
 *   Resize a table to a new number of rows.
 *
 * @param table      Pointer to table.
 * @param new_length New number of rows in table.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified new length is negative.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The contents of the columns will be unchanged up to the lesser of the
 * new and old sizes. If the table is expanded, the extra table rows would
 * just contain invalid elements. The table selection flags are set back 
 * to "all selected". The pointer to column data may change, therefore 
 * pointers previously retrieved by calling @c cpl_table_get_data_int(), 
 * @c cpl_table_get_data_string(), etc. should be discarded.
 */

cpl_error_code cpl_table_set_size(cpl_table *table, cpl_size new_length)
{

    cpl_size     width = cpl_table_get_ncol(table);
    cpl_column **column;


    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (new_length < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    column = table->columns;


    /*
     * If it will fail, it will be at the first column: the table
     * will never be returned half-done.
     */

    while (width--) {
        if (cpl_column_set_size(*column++, new_length)) {
            return cpl_error_set_where_();
        }
    }

    table->nr = new_length;

    return cpl_table_select_all(table);

}


/**
 * @brief
 *   Modify depth of a column of arrays
 *
 * @param table      Pointer to table.
 * @param name       Column name.
 * @param depth      New column depth.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input argument is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified new depth is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found
 *         in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The column with the specified <i>name</i> is not an
 *         <i>array</i> type.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function is applicable just to columns of arrays. The contents
 * of the arrays in the specified column will be unchanged up to the
 * lesser of the new and old depths. If the depth is increased, the
 * extra array elements would be flagged as invalid. The pointers to
 * array data may change, therefore pointers previously retrieved by
 * calling @c cpl_array_get_data_int(), @c cpl_array_get_data_string(),
 * etc. should be discarded.
 */

cpl_error_code cpl_table_set_column_depth(cpl_table *table, const char *name,
                                          cpl_size depth)
{

    cpl_size     length;
    cpl_column *column =
        cpl_table_find_column_type(table, name, CPL_TYPE_POINTER);

    if (!column) {
        return cpl_error_set_where_();
    }

    if (depth < 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    length = table->nr;

    /*
     * If it will fail, it will be at the first array: the column
     * will never be returned half-done.
     */

    while (length--) {
        if (cpl_column_set_depth(column, depth)) {
            return cpl_error_set_where_();
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Make a copy of a table.
 *
 * @param table  Pointer to table.
 *
 * @return Pointer to the new table, or @c NULL in case of @c NULL input,
 *   or in case of error.
 *
 * The copy operation is done "in depth": columns data are duplicated
 * too, not just their pointers. Also the selection flags of the original
 * table are transferred to the new table.
 */

cpl_table *cpl_table_duplicate(const cpl_table *table)
{

    cpl_size    length    = cpl_table_get_nrow(table);
    cpl_size    width     = cpl_table_get_ncol(table);
    cpl_size    i;
    cpl_table  *new_table = NULL;
    cpl_column *column;


    if (table)
        new_table = cpl_table_new(length);
    else
        return NULL;

    if (table->select) {

        new_table->select = cpl_malloc(length * sizeof(cpl_column_flag));
        memcpy(new_table->select, table->select,
               length * sizeof(cpl_column_flag));

    }

    new_table->selectcount = table->selectcount;

    for (i = 0; i < width; i++) {
        column = cpl_column_duplicate(table->columns[i]);
        cpl_table_append_column(new_table, column);
    }

    return new_table;

}


/**
 * @brief
 *   Create a table from a section of another table.
 *
 * @param table  Pointer to table.
 * @param start  First row to be copied to new table.
 * @param count  Number of rows to be copied.
 *
 * @return Pointer to the new table, or @c NULL in case or error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A number of consecutive rows are copied from an input table to a
 * newly created table. The new table will have the same structure of
 * the original table (see function @c cpl_table_compare_structure() ).
 * If the sum of @em start and @em count goes beyond the end of the
 * input table, rows are copied up to the end. All the rows of the
 * new table are selected, i.e., existing selection flags are not
 * transferred from the old table to the new one.
 */

cpl_table *cpl_table_extract(const cpl_table *table, 
                             cpl_size start, cpl_size count)
{

    cpl_size    width     = cpl_table_get_ncol(table);
    cpl_size    length    = cpl_table_get_nrow(table);
    cpl_size    i;
    cpl_table  *new_table = NULL;
    cpl_column *column    = NULL;


    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (count > length - start)
        count = length - start;

    new_table = cpl_table_new(count);

    for (i = 0; i < width; i++) {
        column = cpl_column_extract(table->columns[i], start, count);
        if (column) {
            cpl_column_set_name(column,
                                cpl_column_get_name(table->columns[i]));
            cpl_table_append_column(new_table, column);
        }
        else {
            cpl_error_set_where_();
            cpl_table_delete(new_table);
            return NULL;
        }
    }

    return new_table;

}


/**
 * @brief
 *   Cast a numeric or complex column to a new numeric or complex type column.
 *
 * @param table     Pointer to table.
 * @param from_name Name of table column to cast.
 * @param to_name   Name of new table column.
 * @param type      Type of new table column.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any of <i>table</i> or <i>from_name</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_OUTPUT</td>
 *       <td class="ecr">
 *         A column with the specified <i>to_name</i> already exists in
 *         <i>table</i>. Note however that <i>to_name</i> equal to 
 *         <i>from_name</i> is legal (in-place cast).
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>from_name</i> is not found
 *         in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified <i>type</i> is neither numerical nor complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A new column of the specified type is created, and the content of the
 * given numeric column is cast to the new type. If the input column type
 * is identical to the specified type the column is duplicated as is done
 * by the function @c cpl_table_duplicate_column(). Note that a column of
 * arrays is always cast to another column of arrays of the specified type,
 * unless it has depth 1. Consistently, a column of numbers can be cast 
 * to a column of arrays of depth 1.
 * Here is a complete summary of how any (legal) @em type specification 
 * would be interpreted, depending on the type of the input column:
 *
 * @code
 * from_name type = CPL_TYPE_XXX
 * specified type = CPL_TYPE_XXX
 * to_name   type = CPL_TYPE_XXX
 *
 * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER
 * specified type = CPL_TYPE_XXX | CPL_TYPE_POINTER
 * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER
 *
 * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth > 1)
 * specified type = CPL_TYPE_XXX
 * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER
 *
 * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
 * specified type = CPL_TYPE_XXX
 * to_name   type = CPL_TYPE_XXX
 *
 * from_name type = CPL_TYPE_XXX
 * specified type = CPL_TYPE_XXX | CPL_TYPE_POINTER
 * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
 *
 * from_name type = CPL_TYPE_XXX
 * specified type = CPL_TYPE_POINTER
 * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
 *
 * from_name type = CPL_TYPE_XXX
 * specified type = CPL_TYPE_YYY
 * to_name   type = CPL_TYPE_YYY
 *
 * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER
 * specified type = CPL_TYPE_YYY | CPL_TYPE_POINTER
 * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER
 *
 * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth > 1)
 * specified type = CPL_TYPE_YYY
 * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER
 *
 * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
 * specified type = CPL_TYPE_YYY
 * to_name   type = CPL_TYPE_YYY
 *
 * from_name type = CPL_TYPE_XXX
 * specified type = CPL_TYPE_YYY | CPL_TYPE_POINTER
 * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER (depth = 1)
 * @endcode
 *
 * @note
 *   If @em to_name is a NULL pointer, or it is equal to @em from_name,
 *   the cast is done in-place. The pointers to data will change, 
 *   therefore pointers previously retrieved by @c cpl_table_get_data_xxx(), 
 *   should be discarded.
 */

cpl_error_code cpl_table_cast_column(cpl_table *table, const char *from_name,
                                     const char *to_name, cpl_type type)
{

    int          in_place = 0;
    cpl_size     depth    = 0;
    int          array    = 0;
    cpl_column *column    = cpl_table_find_column_(table, from_name);

    if (!column) {
        return cpl_error_set_where_();
    }

    if (to_name) {
        if (strcmp(from_name, to_name) == 0) {
            in_place = 1;
        }
        else {
            if (cpl_table_find_column(table, to_name))
                return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
        }
    }
    else {
        in_place = 1;
    }


    /*
     * Inherit input type in case output type was not specified.
     */

    if (type == CPL_TYPE_POINTER)
        type |= cpl_table_get_column_type(table, from_name);

    /*
     * Ensure depth of input column: 0 means number, 1 means arrays
     * of one element each which can conceivably be expressed with
     * a depth 0 column, >1 means generical arrays.
     */

    depth = cpl_table_get_column_depth(table, from_name);

    /*
     * Record an explicit request that the output column should
     * be an array: useful only if the input array has depth = 0,
     * to produce a column of depth = 1.
     */

    if (type & CPL_TYPE_POINTER)
        array = 1;

    /*
     * If cast is to be done in-place, and the output type is
     * identical to the input one, no doubt there is nothing to do.
     */

    if (in_place)
        if (cpl_table_get_column_type(table, from_name) == type)
            return CPL_ERROR_NONE;

    /*
     * If cast is to be done in-place, and the _basic_ output type is
     * identical to the input one, and the input column has depth > 1,
     * no doubt there is nothing to do.
     */

    if (in_place)
        if (depth > 1)
            if (cpl_table_get_column_type(table, from_name) == 
                                                  (type | CPL_TYPE_POINTER))
                return CPL_ERROR_NONE;

    /*
     * Transform type into the basic type, no matter whether array or not.
     */

    type = _cpl_columntype_get_valuetype(type);

    /*
     * Now perform the cast
     */

    if (depth == 0 && array == 1) {
        switch (type) {
            case CPL_TYPE_INT:
                column = cpl_column_cast_to_int_array(column);
                break;
#if defined(USE_COLUMN_TYPE_LONG)
            case CPL_TYPE_LONG:
                column = cpl_column_cast_to_long_array(column);
                break;
#endif
            case CPL_TYPE_LONG_LONG:
                column = cpl_column_cast_to_long_long_array(column);
                break;
            case CPL_TYPE_FLOAT:
                column = cpl_column_cast_to_float_array(column);
                break;
            case CPL_TYPE_DOUBLE:
                column = cpl_column_cast_to_double_array(column);
                break;
            case CPL_TYPE_FLOAT_COMPLEX:
                column = cpl_column_cast_to_float_complex_array(column);
                break;
            case CPL_TYPE_DOUBLE_COMPLEX:
                column = cpl_column_cast_to_double_complex_array(column);
                break;
            default:
              return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
    }
    else if (depth == 1 && array == 0) {
        switch (type) {
            case CPL_TYPE_INT:
                column = cpl_column_cast_to_int_flat(column);
                break;
#if defined(USE_COLUMN_TYPE_LONG)
            case CPL_TYPE_LONG:
                column = cpl_column_cast_to_long_flat(column);
                break;
#endif
            case CPL_TYPE_LONG_LONG:
                column = cpl_column_cast_to_long_long_flat(column);
                break;
            case CPL_TYPE_FLOAT:
                column = cpl_column_cast_to_float_flat(column);
                break;
            case CPL_TYPE_DOUBLE:
                column = cpl_column_cast_to_double_flat(column);
                break;
            case CPL_TYPE_FLOAT_COMPLEX:
                column = cpl_column_cast_to_float_complex_flat(column);
                break;
            case CPL_TYPE_DOUBLE_COMPLEX:
                column = cpl_column_cast_to_double_complex_flat(column);
                break;
            default:
              return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
    }
    else {
        switch (type) {
            case CPL_TYPE_INT:
                column = cpl_column_cast_to_int(column);
                break;
#if defined(USE_COLUMN_TYPE_LONG)
            case CPL_TYPE_LONG:
                column = cpl_column_cast_to_long(column);
                break;
#endif
            case CPL_TYPE_LONG_LONG:
                column = cpl_column_cast_to_long_long(column);
                break;
            case CPL_TYPE_FLOAT:
                column = cpl_column_cast_to_float(column);
                break;
            case CPL_TYPE_DOUBLE:
                column = cpl_column_cast_to_double(column);
                break;
            case CPL_TYPE_FLOAT_COMPLEX:
                column = cpl_column_cast_to_float_complex(column);
                break;
            case CPL_TYPE_DOUBLE_COMPLEX:
                column = cpl_column_cast_to_double_complex(column);
                break;
            default:
              return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
    }

    if (column) {
        if (in_place) {
            cpl_table_erase_column(table, from_name);
            cpl_column_set_name(column, from_name);
        }
        else {
            cpl_column_set_name(column, to_name);
        }
        cpl_table_append_column(table, column);
    }
    else {
        return cpl_error_set_where_();
    }

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Add the values of two numeric or complex table columns.
 *
 * @param table     Pointer to table.
 * @param to_name   Name of target column.
 * @param from_name Name of source column.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or any column name are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with any specified name is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified column is neither numerical nor complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The columns are summed element by element, and the result of the sum is
 * stored in the target column. The columns' types may differ, and in that
 * case the operation would be performed using the standard C upcasting
 * rules, with a final cast of the result to the target column type.
 * Invalid elements are propagated consistently: if either or both members
 * of the sum are invalid, the result will be invalid too. Underflows and
 * overflows are ignored.
 */

cpl_error_code cpl_table_add_columns(cpl_table *table, const char *to_name,
                                     const char *from_name)
{

    cpl_column *to_column = cpl_table_find_column_(table, to_name);
    /* FIXME: Should be const */
    cpl_column *from_column = cpl_table_find_column_(table, from_name);

    return !to_column || !from_column || cpl_column_add(to_column, from_column)
        ?  cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Subtract two numeric or complex table columns.
 *
 * @param table     Pointer to table.
 * @param to_name   Name of target column.
 * @param from_name Name of column to be subtracted from target column.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or any column name are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with any specified name is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified column is neither numerical non complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The columns are subtracted element by element, and the result of the
 * subtraction is stored in the target column. See the documentation of
 * the function @c cpl_table_add_columns() for further details.
 */

cpl_error_code cpl_table_subtract_columns(cpl_table *table,
                                          const char *to_name,
                                          const char *from_name)
{

    cpl_column *to_column = cpl_table_find_column_(table, to_name);
    /* FIXME: Should be const */
    cpl_column *from_column = cpl_table_find_column_(table, from_name);

    return !to_column || !from_column ||
        cpl_column_subtract(to_column, from_column)
        ?  cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Multiply two numeric or complex table columns.
 *
 * @param table     Pointer to table.
 * @param to_name   Name of target column.
 * @param from_name Name of column to be multiplied with target column.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or any column name are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with any specified name is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified column is neither numerical nor complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The columns are multiplied element by element, and the result of the
 * multiplication is stored in the target column. See the documentation of
 * the function @c cpl_table_add_columns() for further details.
 */

cpl_error_code cpl_table_multiply_columns(cpl_table *table,
                                          const char *to_name,
                                          const char *from_name)
{

    cpl_column *to_column = cpl_table_find_column_(table, to_name);
    /* FIXME: Should be const */
    cpl_column *from_column = cpl_table_find_column_(table, from_name);

    return !to_column || !from_column ||
        cpl_column_multiply(to_column, from_column)
        ?  cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Divide two numeric or complex table columns.
 *
 * @param table     Pointer to table.
 * @param to_name   Name of target column.
 * @param from_name Name of column dividing the target column.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or any column name are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with any specified name is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified column is neither numerical nor complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The columns are divided element by element, and the result of the
 * division is stored in the target column. The columns' types may
 * differ, and in that case the operation would be performed using
 * the standard C upcasting rules, with a final cast of the result
 * to the target column type. Invalid elements are propagated consistently:
 * if either or both members of the division are invalid, the result
 * will be invalid too. Underflows and overflows are ignored, but a
 * division by exactly zero will set an invalid column element.
 */

cpl_error_code cpl_table_divide_columns(cpl_table *table, const char *to_name,
                                        const char *from_name)
{

    cpl_column *to_column = cpl_table_find_column_(table, to_name);
    /* FIXME: Should be const */
    cpl_column *from_column = cpl_table_find_column_(table, from_name);

    return !to_column || !from_column ||
        cpl_column_divide(to_column, from_column)
        ?  cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Add a constant value to a numerical or complex column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Value to add.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_table_add_scalar(cpl_table *table, const char *name,
                                    double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_add_scalar(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Add a constant complex value to a numerical or complex column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Value to add.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_table_add_scalar_complex(cpl_table *table, const char *name,
                                            double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_add_scalar_complex(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Subtract a constant value from a numerical or complex column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Value to subtract.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of the function @c cpl_table_add_scalar().
 */

cpl_error_code cpl_table_subtract_scalar(cpl_table *table, const char *name,
                                         double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_subtract_scalar(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Subtract a constant complex value from a numerical or complex column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Value to subtract.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, 
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of the function @c cpl_table_add_scalar_complex().
 */

cpl_error_code cpl_table_subtract_scalar_complex(cpl_table *table, 
                                                 const char *name,
                                                 double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_subtract_scalar_complex(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Multiply a numerical or complex column by a constant.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Multiplication factor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex,
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of the function @c cpl_table_add_scalar().
 */

cpl_error_code cpl_table_multiply_scalar(cpl_table *table, const char *name,
                                         double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_multiply_scalar(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Multiply a numerical or complex column by a complex constant.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Multiplication factor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex,
 *         or it is an array column.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * See the description of the function @c cpl_table_add_scalar_complex().
 */

cpl_error_code cpl_table_multiply_scalar_complex(cpl_table *table, 
                                                 const char *name,
                                                 double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_multiply_scalar_complex(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Divide a numerical or complex column by a constant.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Divisor value.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, 
 *         or it is an array column.
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
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid elements are
 * not modified by this operation.
 */

cpl_error_code cpl_table_divide_scalar(cpl_table *table, const char *name,
                                       double value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_divide_scalar(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Divide a numerical or complex column by a complex constant.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param value   Divisor value.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, 
 *         or it is an array column.
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
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid elements are
 * not modified by this operation.
 */


cpl_error_code cpl_table_divide_scalar_complex(cpl_table *table, 
                                               const char *name,
                                               double complex value)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_divide_scalar_complex(column, value)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Compute the absolute value of column values.
 *
 * @param table   Pointer to table.
 * @param name    Table column name.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical, or has type array.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each column element is replaced by its absolute value.
 * Invalid elements are not modified by this operation.
 * If the column is complex, its type will be turned to 
 * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
 * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
 * and any pointer retrieved by calling @c cpl_table_get_data_float_complex() 
 * and @c cpl_array_get_data_double_complex() should be discarded.
 */

cpl_error_code cpl_table_abs_column(cpl_table *table, const char *name)
{

    cpl_type    type;
    cpl_column *column = cpl_table_find_column_(table, name);

    if (!column) {
        return cpl_error_set_where_();
    }

    type = cpl_table_get_column_type(table, name);

    if (type & CPL_TYPE_COMPLEX) {
        cpl_column *new_column = cpl_column_absolute_complex(column);
        if (new_column) {
            cpl_column_set_name(new_column, name);
            cpl_table_extract_column(table, name);
            cpl_table_append_column(table, new_column);
            cpl_column_delete(column);
        }
        else
            return cpl_error_set_where_();
    }
    else {
        if (cpl_column_absolute(column))
            return cpl_error_set_where_();
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the logarithm of column values.
 *
 * @param table   Pointer to table.
 * @param name    Table column name.
 * @param base    Logarithm base.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, 
 *         or it is an array column.
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
 * Each column element is replaced by its logarithm in the specified base.
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid elements are
 * not modified by this operation, but zero or negative elements are
 * invalidated by this operation. In case of complex numbers, values
 * very close to the origin may cause an overflow. The imaginary part 
 * of the result is chosen in the interval [-pi/ln(base),pi/ln(base)],
 * so it should be kept in mind that doing the logarithm of exponential
 * of a complex number will not always express the phase angle with the
 * same number. For instance, the exponential in base 2 of (5.00, 5.00) 
 * is (-30.33, -10.19), and the logarithm in base 2 of the latter will
 * be expressed as (5.00, -4.06).
 */

cpl_error_code cpl_table_logarithm_column(cpl_table *table, const char *name,
                                          double base)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_logarithm(column, base)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Compute the exponential of column values.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 * @param base    Exponential base.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, or it
 *         is an array column.
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
 * Each column element is replaced by its exponential in the specified base.
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid elements are
 * not modified by this operation.
 */

cpl_error_code cpl_table_exponential_column(cpl_table *table,
                                            const char *name, double base)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_exponential(column, base)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Compute the complex conjugate of column values.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex, or it
 *         is integer, or it is an array column.
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
 * Each column element is replaced by its complex conjugate.
 * The operation is always performed in double precision, with a final
 * cast of the result to the target column type. Invalid elements are
 * not modified by this operation.
 */

cpl_error_code cpl_table_conjugate_column(cpl_table *table, const char *name)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_conjugate(column)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Compute the power of numerical column values.
 *
 * @param table    Pointer to table.
 * @param name     Name of column of numerical type
 * @param exponent Constant exponent.
 *
 * @return @c CPL_ERROR_NONE on success.
 * @see pow(), cpow()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each column element is replaced by its power to the specified exponent.
 * For float and float complex the operation is performed in single precision,
 * otherwise it is performed in double precision and then rounded if the column
 * is of an integer type. Results that would or do cause domain errors or
 * overflow are marked as invalid.
 */

cpl_error_code cpl_table_power_column(cpl_table *table, const char *name,
                                      double exponent)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_power(column, exponent)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Compute the phase angle value of table column elements.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each column element is replaced by its phase angle value.
 * The phase angle will be in the range of [-pi,pi].
 * Invalid elements are not modified by this operation.
 * If the column is complex, its type will be turned to 
 * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
 * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
 * and any pointer retrieved by calling @c cpl_table_get_data_float_complex(), 
 * @c cpl_table_get_data_double_complex(), etc., should be discarded.
 */

cpl_error_code cpl_table_arg_column(cpl_table *table, const char *name)
{

    cpl_type    type;
    cpl_column *column = cpl_table_find_column_(table, name);

    if (!column) {
        return cpl_error_set_where_();
    }

    type = cpl_table_get_column_type(table, name);

    if (type & CPL_TYPE_COMPLEX) {
        cpl_column *new_column = cpl_column_phase_complex(column);
        if (new_column) {
            cpl_column_set_name(new_column, name);
            cpl_table_extract_column(table, name);
            cpl_table_append_column(table, new_column);
            cpl_column_delete(column);
        }
        else
            return cpl_error_set_where_();
    }
    else {
        float  *fdata;
        double *ddata;
        int     length = cpl_table_get_nrow(table);

        switch (type) {
        case CPL_TYPE_FLOAT:
            if (length) {
                fdata = cpl_table_get_data_float(table, name);
                memset(fdata, 0.0, length * sizeof(float)); // keep NULLs
            }   
            break;
        case CPL_TYPE_DOUBLE:
            if (length) {
                ddata = cpl_table_get_data_double(table, name);
                memset(ddata, 0.0, length * sizeof(double));
            }
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the real part value of table column elements.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each column element is replaced by its real party value only.
 * Invalid elements are not modified by this operation.
 * If the column is complex, its type will be turned to 
 * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
 * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
 * and any pointer retrieved by calling @c cpl_table_get_data_float_complex(), 
 * @c cpl_table_get_data_double_complex(), etc., should be discarded.
 */

cpl_error_code cpl_table_real_column(cpl_table *table, const char *name)
{

    cpl_type    type;
    cpl_column *column = cpl_table_find_column_(table, name);

    if (!column) {
        return cpl_error_set_where_();
    }

    type = cpl_table_get_column_type(table, name);

    if (type & CPL_TYPE_COMPLEX) {
        cpl_column *new_column = cpl_column_extract_real(column);
        if (new_column) {
            cpl_column_set_name(new_column, name);
            cpl_table_extract_column(table, name);
            cpl_table_append_column(table, new_column);
            cpl_column_delete(column);
        }
        else
            return cpl_error_set_where_();
    }
    else {
        switch (type) {
        case CPL_TYPE_FLOAT:
        case CPL_TYPE_DOUBLE:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the imaginary part value of table column elements.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each column element is replaced by its imaginary party value only.
 * Invalid elements are not modified by this operation.
 * If the column is complex, its type will be turned to 
 * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
 * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
 * and any pointer retrieved by calling @c cpl_table_get_data_float_complex(), 
 * @c cpl_table_get_data_double_complex(), etc., should be discarded.
 */

cpl_error_code cpl_table_imag_column(cpl_table *table, const char *name)
{

    cpl_type    type;
    cpl_column *column = cpl_table_find_column_(table, name);

    if (!column) {
        return cpl_error_set_where_();
    }

    type = cpl_table_get_column_type(table, name);

    if (type & CPL_TYPE_COMPLEX) {
        cpl_column *new_column = cpl_column_extract_imag(column);
        if (new_column) {
            cpl_column_set_name(new_column, name);
            cpl_table_extract_column(table, name);
            cpl_table_append_column(table, new_column);
            cpl_column_delete(column);
        }
        else
            return cpl_error_set_where_();
    }
    else {
        float  *fdata;
        double *ddata;
        int     length = cpl_table_get_nrow(table);

        switch (type) {
        case CPL_TYPE_FLOAT:
            if (length) {
                fdata = cpl_table_get_data_float(table, name);
                memset(fdata, 0.0, length * sizeof(float)); // keep NULLs
            }
            break;
        case CPL_TYPE_DOUBLE:
            if (length) {
                ddata = cpl_table_get_data_double(table, name);
                memset(ddata, 0.0, length * sizeof(double));
            }
            break;
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the mean value of a numerical column.
 *
 * @param table Pointer to table.
 * @param name  Column name.
 *
 * @return Mean value. In case of error 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in
 *         <i>table</i>, or it just contains invalid elements, or
 *         the table has length zero.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Invalid column values are excluded from the computation. The table
 * selection flags have no influence on the result.
 */

double cpl_table_get_column_mean(const cpl_table *table, const char *name)
{

    double       mean;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    mean = cpl_column_get_mean(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return mean;

}


/**
 * @brief
 *   Compute the mean value of a numerical or complex column.
 *
 * @param table Pointer to table.
 * @param name  Column name.
 *
 * @return Mean value. In case of error 0.0 is returned.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in
 *         <i>table</i>, or it just contains invalid elements, or
 *         the table has length zero.
 *       </td> 
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Invalid column values are excluded from the computation. The table
 * selection flags have no influence on the result.
 */

double complex cpl_table_get_column_mean_complex(const cpl_table *table, 
                                                 const char *name)
{

    double complex mean;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    mean = cpl_column_get_mean_complex(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return mean;

}


/**
 * @brief
 *   Compute the median value of a numerical column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 *
 * @return Median value. See documentation of @c cpl_table_get_column_mean().
 *
 * See the description of the function @c cpl_table_get_column_mean().
 */

double cpl_table_get_column_median(const cpl_table *table, const char *name)
{

    double         median;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    median = cpl_column_get_median(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return median;

}


/**
 * @brief
 *   Find the standard deviation of a table column.
 *
 * @param table   Pointer to table.
 * @param name    Column name.
 *
 * @return Standard deviation. See documentation of
 *   @c cpl_table_get_column_mean().
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in
 *         <i>table</i>, or it just contains invalid elements, or
 *         the table has length zero.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Invalid column values are excluded from the computation of the
 * standard deviation. If just one valid element is found, 0.0 is
 * returned but no error is set. The table selection flags have no
 * influence on the result.
 */

double cpl_table_get_column_stdev(const cpl_table *table, const char *name)
{

    double       stdev;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    stdev = cpl_column_get_stdev(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return stdev;

}


/**
 * @brief
 *   Get maximum value in a numerical column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Maximum value. See documentation of @c cpl_table_get_column_mean().
 *
 * See the description of the function @c cpl_table_get_column_mean().
 */

double cpl_table_get_column_max(const cpl_table *table, const char *name)
{

    double       max;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    max = cpl_column_get_max(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return max;

}


/**
 * @brief
 *   Get minimum value in a numerical column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 *
 * @return Minimum value. See documentation of @c cpl_table_get_column_mean().
 *
 * See the description of the function @c cpl_table_get_column_mean().
 */

double cpl_table_get_column_min(const cpl_table *table, const char *name)
{

    double       min;
    cpl_errorstate prevstate = cpl_errorstate_get();
    const cpl_column *column = cpl_table_find_column_const_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return 0.0;
    }

    min = cpl_column_get_min(column);

    if (!cpl_errorstate_is_equal(prevstate))
        cpl_error_set_where_();

    return min;

}


/**
 * @brief
 *   Get position of maximum in a numerical column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 * @param row    Returned position of maximum value.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in
 *         <i>table</i>, or it just contains invalid elements, or
 *         the table has length zero.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Invalid column values are excluded from the search. The @em row argument
 * will be assigned the position of the maximum value, where rows are counted
 * starting from 0. If more than one column element correspond to the max
 * value, the position with the lowest row number is returned. In case of
 * error, @em row is left untouched. The table selection flags have no
 * influence on the result.
 */

cpl_error_code cpl_table_get_column_maxpos(const cpl_table *table,
                                           const char *name, cpl_size *row)
{

    const cpl_column *column = cpl_table_find_column_const_(table, name);

    return !column || cpl_column_get_maxpos(column, row)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Get position of minimum in a numerical column.
 *
 * @param table  Pointer to table.
 * @param name   Column name.
 * @param row    Returned position of minimum value.
 *
 * @return See function @c cpl_table_get_column_maxpos().
 *
 * See the description of the function @c cpl_table_get_column_maxpos().
 */

cpl_error_code cpl_table_get_column_minpos(const cpl_table *table,
                                           const char *name, cpl_size *row)
{

    const cpl_column *column = cpl_table_find_column_const_(table, name);

    return !column || cpl_column_get_minpos(column, row)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Remove from a table columns and rows just containing invalid elements.
 *
 * @param table   Pointer to table.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Table columns and table rows just containing invalid elements are deleted
 * from the table, i.e. a column or a row is deleted only if all of its
 * elements are invalid. The selection flags are set back to "all selected"
 * even if no rows or columns are removed. The pointers to data may change,
 * therefore pointers previously retrieved by @c cpl_table_get_data_int(), 
 * @c cpl_table_get_data_string(), etc., should be discarded.
 *
 * @note
 *   If the input table just contains invalid elements, all columns are
 *   deleted.
 */

cpl_error_code cpl_table_erase_invalid_rows(cpl_table *table)
{

    cpl_size    length     = cpl_table_get_nrow(table);
    cpl_size    width      = cpl_table_get_ncol(table);
    cpl_size    new_width;
    cpl_size    count      = 0;
    int         any_valid;
    int         all_valid  = 0;


    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    while (width--) {

        if (!cpl_column_has_valid(table->columns[width]))
            cpl_table_erase_column(table,
                             cpl_column_get_name(table->columns[width]));

        else if (!cpl_column_has_invalid(table->columns[width]))
            all_valid = 1;

    }

    if (all_valid)
        return cpl_table_select_all(table);

    new_width = cpl_table_get_ncol(table);

    if (new_width == 0)
        return CPL_ERROR_NONE;

    while (length--) {

        width = new_width;

        any_valid = 0;
        while (width--) {
            if (!cpl_column_is_invalid(table->columns[width], length)) {
                any_valid = 1;
                break;
            }
        }

        if (any_valid) {  /* Current row does not contain just invalids */
            if (count) {  /* End of segment of just invalids            */
                cpl_table_erase_window(table, length + 1, count);
                count = 0;
            }
        }
        else                      /* Current row just contains invalids */
            count++;

    }

    if (count)                    /* The first table row is invalid     */
        cpl_table_erase_window(table, 0, count);

    return cpl_table_select_all(table);

}


/**
 * @brief
 *   Remove from a table all columns just containing invalid elements,
 *   and then all rows containing at least one invalid element.
 *
 * @param table   Pointer to table.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Firstly, all columns consisting just of invalid elements are deleted
 * from the table. Next, the remaining table rows containing at least
 * one invalid element are also deleted from the table. The selection
 * flags are set back to "all selected" even if no rows or columns are 
 * erased. The pointers to data may change, therefore pointers previously 
 * retrieved by calling @c cpl_table_get_data_int(), etc., should be 
 * discarded.
 *
 * The function is similar to the function cpl_table_erase_invalid_rows(),
 * except for the criteria to remove rows containing invalid elements after
 * all invalid columns have been removed. While cpl_table_erase_invalid_rows()
 * requires all elements to be invalid in order to remove a row from the
 * table, this function requires only one (or more) elements to be invalid.
 *
 * @note
 *   If the input table just contains invalid elements, all columns are
 *   deleted.
 *
 * @see 
 *   cpl_table_erase_invalid_rows()
 */

cpl_error_code cpl_table_erase_invalid(cpl_table *table)
{

    cpl_size    length     = 0;
    cpl_size    width      = 0;
    cpl_size    new_width;
    cpl_size    count      = 0;
    int         any_null;


    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    length = cpl_table_get_nrow(table);
    width  = cpl_table_get_ncol(table);

    while (width--)
        if (!cpl_column_has_valid(table->columns[width]))
            cpl_table_erase_column(table,
                             cpl_column_get_name(table->columns[width]));

    new_width = cpl_table_get_ncol(table);

    if (new_width == 0)
        return CPL_ERROR_NONE;

    while (length--) {

        width = new_width;

        any_null = 0;
        while (width--) {
            if (cpl_column_is_invalid(table->columns[width], length)) {
                any_null = 1;
                break;
            }
        }

        if (any_null) {     /* Current row contains at least an invalid  */
            count++;
        }
        else {              /* Current row contains no invalids          */
            if (count) {    /* End of segment of rows with some invalids */
                cpl_table_erase_window(table, length + 1, count);
                count = 0;
            }
        }

    }

    if (count)              /* The first table row has at least a NULL */
        cpl_table_erase_window(table, 0, count);

    return cpl_table_select_all(table);

}


/**
 * @brief
 *   Flag a table row as selected.
 *
 * @param table   Pointer to table.
 * @param row     Row to select.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Flag a table row as selected. Any previous selection is kept.
 */

cpl_error_code cpl_table_select_row(cpl_table *table, cpl_size row)
{



    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (row < 0 || row >= table->nr)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (table->selectcount == table->nr)
        return CPL_ERROR_NONE;

    if (table->selectcount == 0)
        table->select = cpl_calloc(table->nr, sizeof(cpl_column_flag));

    if (table->select[row] == 0) {
        table->select[row] = 1;
        table->selectcount++;

        if (table->selectcount == table->nr) {
            cpl_free(table->select);
            table->select = NULL;
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Select all table rows.
 *
 * @param table   Pointer to table.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The table selection flags are reset, meaning that they are
 * all marked as selected. This is the initial state of any
 * table.
 */

cpl_error_code cpl_table_select_all(cpl_table *table)
{



    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (table->select)
        cpl_free(table->select);
    table->select = NULL;
    table->selectcount = table->nr;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Flag a table row as unselected.
 *
 * @param table   Pointer to table.
 * @param row     Row to unselect.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Flag a table row as unselected. Any previous selection is kept.
 */

cpl_error_code cpl_table_unselect_row(cpl_table *table, cpl_size row)
{

    int          length;


    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (row < 0 || row >= table->nr)
        return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (table->selectcount == 0)
        return CPL_ERROR_NONE;

    length = table->nr;

    if (table->selectcount == length)
        table->select = cpl_malloc(table->nr * sizeof(cpl_column_flag));

    if (table->selectcount == length)
        while (length--)
            table->select[length] = 1;

    if (table->select[row] == 1) {
        table->select[row] = 0;
        table->selectcount--;

        if (table->selectcount == 0) {
            if (table->select)
                cpl_free(table->select);
            table->select = NULL;
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Unselect all table rows.
 *
 * @param table   Pointer to table.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The table selection flags are all unset, meaning that no table
 * rows are selected.
 */

cpl_error_code cpl_table_unselect_all(cpl_table *table)
{

    if (table == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (table->select)
        cpl_free(table->select);
    table->select = NULL;
    table->selectcount = 0;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Describe the structure and the contents of a table.
 *
 * @param table    Pointer to table.
 * @param stream   The output stream
 *
 * @return Nothing.
 *
 * This function is mainly intended for debug purposes. Some information
 * about the structure of a table and its contents is printed to terminal:
 *
 * - Number of columns, with their names and types
 * - Number of invalid elements for each column
 * - Number of rows and of selected rows
 *
 * If the specified stream is @c NULL, it is set to @em stdout. The function
 * used for printing is the standard C @c fprintf().
 */

void cpl_table_dump_structure(const cpl_table *table, FILE *stream)
{

    cpl_size width = cpl_table_get_ncol(table);
    cpl_size i     = 0;


    if (stream == NULL)
        stream = stdout;

    if (table == NULL) {
        fprintf(stream, "NULL table\n\n");
        return;
    }

    if (width > 0)
        fprintf(stream, "Table with %" CPL_SIZE_FORMAT 
                "/%" CPL_SIZE_FORMAT " selected rows and %" 
                CPL_SIZE_FORMAT " columns:\n\n",
                table->selectcount, table->nr, table->nc);

    while (i < width) {
        const char *type_name = "UNDEFINED";

        const cpl_type type_id = cpl_column_get_type(table->columns[i]);

        if (cpl_column_get_unit(table->columns[i])) {
            fprintf(stream, "  %s [%s]:\n     %" CPL_SIZE_FORMAT " ",
                    cpl_column_get_name(table->columns[i]),
                    cpl_column_get_unit(table->columns[i]),
                    cpl_column_get_size(table->columns[i]));
        }
        else {
            fprintf(stream, "  %s:\n     %" CPL_SIZE_FORMAT " ",
                    cpl_column_get_name(table->columns[i]),
                    cpl_column_get_size(table->columns[i]));
        }


        switch (_cpl_columntype_get_valuetype(type_id)) {
        case CPL_TYPE_INT:
            type_name = "integer";
            break;
        case CPL_TYPE_LONG:
            type_name = "long integer";
            break;
        case CPL_TYPE_LONG_LONG:
            type_name = "long long integer";
            break;
        case CPL_TYPE_FLOAT:
            type_name = "float";
            break;
        case CPL_TYPE_DOUBLE:
            type_name = "double";
            break;
        case CPL_TYPE_STRING:
            type_name = "string";
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            type_name = "float complex";
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            type_name = "double complex";
            break;
        default:
            break;
        }

        if (_cpl_columntype_is_pointer(type_id)) {
            fprintf(stream, "arrays of ");
        }

        fprintf(stream, "%s elements, of which %" CPL_SIZE_FORMAT " are "
                "flagged invalid.\n", type_name,
                cpl_column_count_invalid(table->columns[i]));
        i++;
    }

    return;

}


/**
 * @brief
 *   Print a table.
 *
 * @param table    Pointer to table
 * @param start    First row to print
 * @param count    Number of rows to print
 * @param stream   The output stream
 *
 * @return Nothing.
 *
 * This function is mainly intended for debug purposes.
 * All column elements are printed according to the column formats,
 * that may be specified for each table column with the function
 * @c cpl_table_set_column_format(). The default column formats
 * have been chosen to provide a reasonable printout in most cases.
 * Table rows are counted from 0, and their sequence number is printed
 * at the left of each row. Invalid table elements are represented
 * as a sequence of "-" as wide as the field occupied by the column
 * to which they belong. Array elements are not resolved, and are
 * represented by a sequence of "+" as wide as the field occupied by the
 * column to which they belong. It is not shown whether a table row is
 * selected or not. Specifying a @em start beyond the table boundaries,
 * or a non-positive @em count, would generate a warning message, but no
 * error would be set. The specified number of rows to print may exceed
 * the table end, and in that case the table would be printed up to its
 * last row. If the specified stream is @c NULL, it is set to @em stdout.
 * The function used for printing is the standard C @c fprintf().
 */

void cpl_table_dump(const cpl_table *table, 
                    cpl_size start, cpl_size count, FILE *stream)
{

    cpl_size       size;
    cpl_size       nc = cpl_table_get_ncol(table);
    cpl_size       offset;
    cpl_size       i, j, k;
    cpl_size       end;
    int           *field_size;
    int           *label_len;
    int            row;
    char         **fields;
    char          *row_field;
    int            null;
    int            found;


    if (stream == NULL)
        stream = stdout;

    if (table == NULL) {
        fprintf(stream, "NULL table\n\n");
        return;
    }

    if (table->nr == 0) {
        fprintf(stream, "Zero length table\n\n");
        return;
    }

    if (start < 0 || start >= table->nr || count < 1) {
        fprintf(stream, "Illegal cpl_table_dump() arguments!\n");
        return;
    }

    if (count > table->nr - start)
        count = table->nr - start;

    end = start + count;

    row = cx_snprintf(NULL, 0, "% -" CPL_SIZE_FORMAT, end);
    row_field = cpl_malloc((row + 1) * sizeof(char));
    memset(row_field, ' ', row + 1);
    row_field[row] = '\0';

    label_len = cpl_calloc(nc, sizeof(int));
    field_size = cpl_calloc(nc, sizeof(int));
    fields = cpl_calloc(nc, sizeof(char *));

    for (j = 0; j < nc; j++) {
        const cpl_type type_id = cpl_column_get_type(table->columns[j]);

        label_len[j] = field_size[j] =
                       strlen(cpl_column_get_name(table->columns[j]));

        if (!_cpl_columntype_is_pointer(type_id)) {

            switch (_cpl_columntype_get_valuetype(type_id)) {

            case CPL_TYPE_INT:
                for (i = start; i < end; i++) {
                    int inum = cpl_column_get_int(table->columns[j], i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(table->columns[j]),
                                        inum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            case CPL_TYPE_LONG:
                for (i = start; i < end; i++) {
                    long lnum = cpl_column_get_long(table->columns[j], i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(table->columns[j]),
                                        lnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            case CPL_TYPE_LONG_LONG:
                for (i = start; i < end; i++) {
                    long long llnum = cpl_column_get_long_long(table->columns[j], i,
                                                               &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(table->columns[j]),
                                        llnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            case CPL_TYPE_FLOAT:
                for (i = start; i < end; i++) {
                    float fnum = cpl_column_get_float(table->columns[j], i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(table->columns[j]),
                                        fnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            case CPL_TYPE_DOUBLE:
                for (i = start; i < end; i++) {
                    double dnum = cpl_column_get_double(table->columns[j], i,
                                                        &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(table->columns[j]),
                                        dnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            case CPL_TYPE_FLOAT_COMPLEX:
                for (i = start; i < end; i++) {
                    float complex cfnum =
                            cpl_column_get_float_complex(table->columns[j], i,
                                                         &null);

                    if (null)
                        size = 4;
                    else
                        size = 3 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(table->columns[j]),
                                   crealf(cfnum))
                                 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(table->columns[j]),
                                   cimagf(cfnum));

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            case CPL_TYPE_DOUBLE_COMPLEX:
                for (i = start; i < end; i++) {
                    double complex cdnum =
                            cpl_column_get_double_complex(table->columns[j], i,
                                                          &null);

                    if (null)
                        size = 4;
                    else
                        size = 3 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(table->columns[j]),
                                   creal(cdnum))
                                 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(table->columns[j]),
                                   cimag(cdnum));

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            case CPL_TYPE_STRING:
                for (i = start; i < end; i++) {
                    char *string = (char *)cpl_column_get_string(table->columns[j],
                                                                 i);

                    if (string == NULL)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(table->columns[j]),
                                        string);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;

            default:
                field_size[j] = 4;
                break;
            }
        }

        field_size[j]++;
        label_len[j]++;

        fields[j] = cpl_malloc(field_size[j] * sizeof(char));

    }

    fprintf(stream, "%s ", row_field);

    for (j = 0; j < nc; j++) {
        offset = (field_size[j] - label_len[j]) / 2;
        for (i = 0; i < offset; i++)
            fields[j][i] = ' ';
        cx_snprintf(fields[j] + offset, label_len[j], "%s",
                 cpl_column_get_name(table->columns[j]));
        for (i = label_len[j] + offset - 1; i < field_size[j]; i++)
            fields[j][i] = ' ';
        fields[j][field_size[j] - 1] = '\0';
        fprintf(stream, "%-*s ", label_len[j], fields[j]);
    }

    fprintf(stream, "\n\n");


    for (i = start; i < end; i++) {

        fprintf(stream, "%*" CPL_SIZE_FORMAT " ", row, i);

        for (j = 0; j < nc; j++) {

            cpl_type type_id = cpl_column_get_type(table->columns[j]);

            if (_cpl_columntype_is_pointer(type_id)) {

                cpl_array *array = cpl_column_get_array(table->columns[j], i);

                if (!array) {
                    memset(fields[j], '-', field_size[j]);
                }
                else {
                    memset(fields[j], '+', field_size[j]);
                }

                fields[j][field_size[j] - 1] = '\0';

            }
            else {

                switch (_cpl_columntype_get_valuetype(type_id)) {

                    case CPL_TYPE_INT:
                    {
                        int inum = cpl_column_get_int(table->columns[j], i, &null);

                        if (null) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else
                            cx_snprintf(fields[j], field_size[j],
                                        cpl_column_get_format(table->columns[j]), inum);
                        break;
                    }

                    case CPL_TYPE_LONG:
                    {
                        long lnum = cpl_column_get_long(table->columns[j], i, &null);

                        if (null) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else
                            cx_snprintf(fields[j], field_size[j],
                                        cpl_column_get_format(table->columns[j]), lnum);
                        break;
                    }

                    case CPL_TYPE_LONG_LONG:
                    {
                        long long llnum = cpl_column_get_long_long(table->columns[j], i,
                                                                   &null);

                        if (null) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else
                            cx_snprintf(fields[j], field_size[j],
                                        cpl_column_get_format(table->columns[j]), llnum);
                        break;
                    }

                    case CPL_TYPE_FLOAT:
                    {
                        float fnum = cpl_column_get_float(table->columns[j], i, &null);

                        if (null) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else
                            cx_snprintf(fields[j], field_size[j],
                                        cpl_column_get_format(table->columns[j]), fnum);
                        break;
                    }

                    case CPL_TYPE_DOUBLE:
                    {
                        double dnum = cpl_column_get_double(table->columns[j], i,
                                                            &null);

                        if (null) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else
                            cx_snprintf(fields[j], field_size[j],
                                        cpl_column_get_format(table->columns[j]), dnum);
                        break;
                    }

                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        float complex cfnum =
                                cpl_column_get_float_complex(table->columns[j], i,
                                                             &null);

                        if (null) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else {
                            char *s = cpl_sprintf("(%s,%s)",
                                                  cpl_column_get_format(table->columns[j]),
                                                  cpl_column_get_format(table->columns[j]));

                            cx_snprintf(fields[j], field_size[j], s,
                                        crealf(cfnum), cimagf(cfnum));
                            cpl_free(s);
                        }
                        break;
                    }

                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        double complex cdnum =
                                cpl_column_get_double_complex(table->columns[j], i,
                                                              &null);

                        if (null) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else {
                            char *s = cpl_sprintf("(%s,%s)",
                                                  cpl_column_get_format(table->columns[j]),
                                                  cpl_column_get_format(table->columns[j]));

                            cx_snprintf(fields[j], field_size[j], s,
                                        creal(cdnum), cimag(cdnum));
                            cpl_free(s);
                        }
                        break;
                    }

                    case CPL_TYPE_STRING:
                    {
                        char *string =
                                (char *)cpl_column_get_string(table->columns[j], i);

                        if (!string) {
                            memset(fields[j], '-', field_size[j]);
                            fields[j][field_size[j] - 1] = '\0';
                        }
                        else
                            cx_snprintf(fields[j], field_size[j],
                                        cpl_column_get_format(table->columns[j]), string);
                        break;
                    }

                    default:
                    {
                        cpl_array *array = cpl_column_get_array(table->columns[j], i);

                        if (!array) {
                            memset(fields[j], '-', field_size[j]);
                        }
                        else
                            memset(fields[j], '+', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                        break;
                    }
                }
            }

            found = 0;
            for (k = 0; k < field_size[j]; k++) {
                 if (fields[j][k] == '\0')
                     found = 1;
                 if (found)
                     fields[j][k] = ' ';
            }
            fields[j][field_size[j] - 1] = '\0';
            fprintf(stream, "%-*s ", field_size[j], fields[j]);
        }
        fprintf(stream, "\n");
    }

    for (j = 0; j < nc; j++)
         cpl_free(fields[j]);
    cpl_free(fields);
    cpl_free(row_field);
    cpl_free(label_len);
    cpl_free(field_size);

    return;

}


/**
 * @brief
 *   Shift the position of numeric or complex column values.
 *
 * @param table    Pointer to table.
 * @param name     Name of table column to shift.
 * @param shift    Shift column values by so many rows.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is neither numerical nor complex.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The absolute value of <i>shift</i> is greater than the column
 *         length.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The position of all column values is shifted by the specified amount.
 * If @em shift is positive, all values will be moved toward the bottom
 * of the column, otherwise toward its top. In either case as many column
 * elements as the amount of the @em shift will be left undefined, either
 * at the top or at the bottom of the column according to the direction
 * of the shift. These column elements will be marked as invalid. This
 * function is applicable just to numeric and complex columns, and not 
 * to strings and or array types. The selection flags are always set back 
 * to "all selected" after this operation.
 */

cpl_error_code cpl_table_shift_column(cpl_table *table, const char *name,
                                      cpl_size shift)
{


    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_shift(column, shift) ||
        cpl_table_select_all(table)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a numerical value to invalid @em integer column elements.
 *
 * @param table    Pointer to table containing the column.
 * @param name     Column name.
 * @param code     Value to write to invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>,
 *         or of type <tt>CPL_TYPE_INT | CPL_TYPE_POINTER</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * In general, a numeric column element that is flagged as invalid is undefined
 * and should not be read. It is however sometimes convenient to read such
 * values, f.ex. via calls to @c cpl_table_get_data_int() and memcpy().
 * In order to avoid that such usage causes uninitialized memory to be read,
 * the invalid elements may be set to a value specified by a call to this
 * function. Note that only existing invalid elements will be filled as
 * indicated: new invalid column elements would still have their actual
 * values left undefined. Also, any further processing of the column would
 * not take care of maintaining the assigned value to a given invalid column
 * element: therefore the value should be applied just before it is actually
 * needed.
 * An invalid numerical column element remains invalid after this call,
 * and the usual method of checking whether the element is invalid or not
 * should still be used.
 * This function can be applied also to columns of arrays of integers. In
 * this case the call will cause the array element to be flagged as valid.
 * 
 * @note
 *   Assigning a value to an invalid numerical element will not make it valid,
 *   but assigning a value to an element consisting of an array of numbers
 *   will make the array element valid.
 */

cpl_error_code cpl_table_fill_invalid_int(cpl_table *table,
                                          const char *name, int code)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid_int(column, code)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a numerical value to invalid @em long column elements.
 *
 * @param table    Pointer to table containing the column.
 * @param name     Column name.
 * @param code     Value to write to invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>,
 *         or of type <tt>CPL_TYPE_LONG | CPL_TYPE_POINTER</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * @see cpl_table_fill_invalid_int()
 *
 * @note
 *   Assigning a value to an invalid numerical element will not make it valid,
 *   but assigning a value to an element consisting of an array of numbers
 *   will make the array element valid.
 */

cpl_error_code cpl_table_fill_invalid_long(cpl_table *table,
                                           const char *name, long code)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid_long(column, code)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a numerical value to invalid @em long @em long column elements.
 *
 * @param table    Pointer to table containing the column.
 * @param name     Column name.
 * @param code     Value to write to invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>,
 *         or of type <tt>CPL_TYPE_LONG_LONG | CPL_TYPE_POINTER</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * @see cpl_table_fill_invalid_int()
 *
 * @note
 *   Assigning a value to an invalid numerical element will not make it valid,
 *   but assigning a value to an element consisting of an array of numbers
 *   will make the array element valid.
 */

cpl_error_code cpl_table_fill_invalid_long_long(cpl_table *table,
                                                const char *name,
                                                long long code)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid_long_long(column, code)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a numerical value to invalid @em float column elements.
 *
 * @param table    Pointer to table containing the column.
 * @param name     Column name.
 * @param code     Value to write to invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>,
 *         or of type <tt>CPL_TYPE_FLOAT | CPL_TYPE_POINTER</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * @see cpl_table_fill_invalid_int()
 *
 * @note
 *   Assigning a value to an invalid numerical element will not make it valid,
 *   but assigning a value to an element consisting of an array of numbers
 *   will make the array element valid.
 */

cpl_error_code cpl_table_fill_invalid_float(cpl_table *table,
                                            const char *name, float code)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid_float(column, code)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a numerical value to invalid @em float complex column elements.
 *
 * @param table    Pointer to table containing the column.
 * @param name     Column name.
 * @param code     Value to write to invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>,
 *         or of type <tt>CPL_TYPE_FLOAT_COMPLEX | CPL_TYPE_POINTER</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * @see cpl_table_fill_invalid_int()
 *
 * @note
 *   Assigning a value to an invalid numerical element will not make it valid,
 *   but assigning a value to an element consisting of an array of numbers
 *   will make the array element valid.
 */

cpl_error_code cpl_table_fill_invalid_float_complex(cpl_table *table,
                                                    const char *name, 
                                                    float complex code)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid_float_complex(column, code)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a numerical value to invalid @em double column elements.
 *
 * @param table    Pointer to table containing the column.
 * @param name     Column name.
 * @param code     Value to write to invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>,
 *         or of type <tt>CPL_TYPE_DOUBLE | CPL_TYPE_POINTER</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * @see cpl_table_fill_invalid_int()
 *
 * @note
 *   Assigning a value to an invalid numerical element will not make it valid,
 *   but assigning a value to an element consisting of an array of numbers
 *   will make the array element valid.
 */

cpl_error_code cpl_table_fill_invalid_double(cpl_table *table,
                                             const char *name, double code)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid_double(column, code)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Write a numerical value to invalid @em double complex column elements.
 *
 * @param table    Pointer to table containing the column.
 * @param name     Column name.
 * @param code     Value to write to invalid column elements.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>,
 *         or of type <tt>CPL_TYPE_DOUBLE_COMPLEX | CPL_TYPE_POINTER</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * @see cpl_table_fill_invalid_int()
 *
 * @note
 *   Assigning a value to an invalid numerical element will not make it valid,
 *   but assigning a value to an element consisting of an array of numbers
 *   will make the array element valid.
 */

cpl_error_code cpl_table_fill_invalid_double_complex(cpl_table *table,
                                                    const char *name,
                                                    double complex code)
{

    cpl_column *column = cpl_table_find_column_(table, name);

    return !column || cpl_column_fill_invalid_double_complex(column, code)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Select from selected table rows, by comparing @em integer column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference value. All table rows not
 * fulfilling the comparison are unselected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the function @c cpl_table_or_selected_int().
 */

cpl_size cpl_table_and_selected_int(cpl_table *table, const char *name,
                                    cpl_table_select_operator operator,
                                    int value)
{

    cpl_column_flag *nulldata;
    int             *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_INT);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_int(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    /*
     * All invalid elements by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata)                    /* Some (not all!) invalid elements */
        while (length--)
            if (nulldata[length])
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0) 
                        if (data[length] != value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0) 
                        if (data[length] == value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0) 
                        if (data[length] <= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0) 
                        if (data[length] > value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0) 
                        if (data[length] >= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0) 
                        if (data[length] < value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_unselect_row(table, length);
            }
            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing @em integer column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified
 * column are compared with the reference value. The table rows
 * fulfilling the comparison are selected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the description of the function @c cpl_table_and_selected_int().
 */

cpl_size cpl_table_or_selected_int(cpl_table *table, const char *name,
                                   cpl_table_select_operator operator,
                                   int value)
{

    cpl_column_flag *nulldata;
    int             *data;
    int              length;
    int              nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_INT);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_int(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    if (table->selectcount == length)  /* It's already all selected */
        return length;

    if (nullcount == length)   /* Just invalids, no need to check values */
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the invalids:
     */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_select_row(table, length);
            }

            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows, by comparing @em long column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference value. All table rows not
 * fulfilling the comparison are unselected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the function @c cpl_table_or_selected_long().
 */

cpl_size cpl_table_and_selected_long(cpl_table *table, const char *name,
                                     cpl_table_select_operator operator,
                                     long value)
{

    cpl_column_flag *nulldata;
    long            *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_long(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    /*
     * All invalid elements by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata)                    /* Some (not all!) invalid elements */
        while (length--)
            if (nulldata[length])
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_unselect_row(table, length);
            }
            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing @em long column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified
 * column are compared with the reference value. The table rows
 * fulfilling the comparison are selected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the description of the function @c cpl_table_and_selected_long().
 */

cpl_size cpl_table_or_selected_long(cpl_table *table, const char *name,
                                    cpl_table_select_operator operator,
                                    long value)
{

    cpl_column_flag *nulldata;
    long            *data;
    int              length;
    int              nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_long(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    if (table->selectcount == length)  /* It's already all selected */
        return length;

    if (nullcount == length)   /* Just invalids, no need to check values */
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the invalids:
     */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_select_row(table, length);
            }

            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows, by comparing @em long @em long column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference value. All table rows not
 * fulfilling the comparison are unselected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the function @c cpl_table_or_selected_long_long().
 */

cpl_size cpl_table_and_selected_long_long(cpl_table *table, const char *name,
                                          cpl_table_select_operator operator,
                                          long long value)
{

    cpl_column_flag *nulldata;
    long long       *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_LONG_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_long_long(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    /*
     * All invalid elements by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata)                    /* Some (not all!) invalid elements */
        while (length--)
            if (nulldata[length])
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_unselect_row(table, length);
            }
            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing @em long @em long column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_LONG_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified
 * column are compared with the reference value. The table rows
 * fulfilling the comparison are selected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the description of the function @c cpl_table_and_selected_long_long().
 */

cpl_size cpl_table_or_selected_long_long(cpl_table *table, const char *name,
                                         cpl_table_select_operator operator,
                                         long long value)
{

    cpl_column_flag *nulldata;
    long long       *data;
    int              length;
    int              nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_LONG_LONG);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_long_long(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    if (table->selectcount == length)  /* It's already all selected */
        return length;

    if (nullcount == length)   /* Just invalids, no need to check values */
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the invalids:
     */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_select_row(table, length);
            }

            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows, by comparing @em float column values
 *   with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference value. All table rows not
 * fulfilling the comparison are unselected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the function @c cpl_table_or_selected_float().
 */

cpl_size cpl_table_and_selected_float(cpl_table *table, const char *name,
                                      cpl_table_select_operator operator,
                                      float value)
{

    cpl_column_flag *nulldata;
    float           *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_FLOAT);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }


    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_float(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    /*
     * All invalid elements by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata)                       /* Some (not all!) invalids */
        while (length--)
            if (nulldata[length])
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_unselect_row(table, length);
            }
            break;

    }

    return table->selectcount;
}


/**
 * @brief
 *   Select from selected table rows, by comparing @em float complex column 
 *   values with a complex constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Operator other than <tt>CPL_EQUAL_TO</tt> or 
 *         <tt>CPL_NOT_EQUAL_TO</tt> was specified.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference value. All table rows not
 * fulfilling the comparison are unselected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the function @c cpl_table_or_selected_float_complex().
 */

cpl_size cpl_table_and_selected_float_complex(cpl_table *table, 
                                        const char *name,
                                        cpl_table_select_operator operator,
                                        float complex value)
{

    cpl_column_flag *nulldata;
    float complex   *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_FLOAT_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_float_complex(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    /*
     * All invalid elements by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata)                       /* Some (not all!) invalids */
        while (length--)
            if (nulldata[length])
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

    case CPL_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] != value)
                        cpl_table_unselect_row(table, length);
        }
        else {
            while (length--)
                if (data[length] != value)
                    cpl_table_unselect_row(table, length);
        }
        break;

    case CPL_NOT_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] == value)
                        cpl_table_unselect_row(table, length);
        }
        else {
            while (length--)
                if (data[length] == value)
                    cpl_table_unselect_row(table, length);
        }
        break;

    case CPL_GREATER_THAN:
    case CPL_NOT_GREATER_THAN:
    case CPL_LESS_THAN:
    case CPL_NOT_LESS_THAN:

        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;

    }

    return table->selectcount;
}


/**
 * @brief
 *   Select from unselected table rows, by comparing @em float column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified
 * column are compared with the reference value. The table rows
 * fulfilling the comparison are selected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the description of the function @c cpl_table_and_selected_float().
 */

cpl_size cpl_table_or_selected_float(cpl_table *table, const char *name,
                                     cpl_table_select_operator operator,
                                     float value)
{

    cpl_column_flag *nulldata;
    float           *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_FLOAT);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_float(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (table->selectcount == length)
        return length;

    if (nullcount == length)
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the invalids:
     */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_select_row(table, length);
            }

            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing @em float complex column
 *   values with a complex constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Operator other than <tt>CPL_EQUAL_TO</tt> or 
 *         <tt>CPL_NOT_EQUAL_TO</tt> was specified.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified
 * column are compared with the reference value. The table rows
 * fulfilling the comparison are selected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO. If the table has no 
 * rows, no error is set, and 0 is returned. See also the description 
 * of the function @c cpl_table_and_selected_float_complex().
 */

cpl_size cpl_table_or_selected_float_complex(cpl_table *table, const char *name,
                                             cpl_table_select_operator operator,
                                             float complex value)
{

    cpl_column_flag *nulldata;
    float complex   *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_FLOAT_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_float_complex(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (table->selectcount == length)
        return length;

    if (nullcount == length)
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the invalids:
     */

    switch (operator) {

    case CPL_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] == value)
                        cpl_table_select_row(table, length);
        }
        else {
            while (length--)
                if (data[length] == value)
                    cpl_table_select_row(table, length);
        }

        break;

    case CPL_NOT_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] != value)
                        cpl_table_select_row(table, length);
        }
        else {
            while (length--)
                if (data[length] != value)
                    cpl_table_select_row(table, length);
        }

        break;

    case CPL_GREATER_THAN:
    case CPL_NOT_GREATER_THAN:
    case CPL_LESS_THAN:
    case CPL_NOT_LESS_THAN:

        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows, by comparing @em double column values
 *   with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference value. All table rows not
 * fulfilling the comparison are unselected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the function @c cpl_table_or_selected_double().
 */

cpl_size cpl_table_and_selected_double(cpl_table *table, const char *name,
                                       cpl_table_select_operator operator,
                                       double value)
{

    cpl_column_flag *nulldata;
    double          *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_DOUBLE);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }


    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_double(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    /*
     * All invalid by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata)                            /* Some (not all!) invalids */
        while (length--)
            if (nulldata[length])
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_unselect_row(table, length);
            }
            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_unselect_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_unselect_row(table, length);
            }
            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows, by comparing @em double complex column 
 *   values with a complex constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Operator other than <tt>CPL_EQUAL_TO</tt> or 
 *         <tt>CPL_NOT_EQUAL_TO</tt> was specified.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference value. All table rows not
 * fulfilling the comparison are unselected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the function @c cpl_table_or_selected_double_complex().
 */

cpl_size cpl_table_and_selected_double_complex(cpl_table *table, 
                                          const char *name,
                                          cpl_table_select_operator operator,
                                          double complex value)
{

    cpl_column_flag *nulldata;
    double complex  *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_DOUBLE_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_double_complex(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    /*
     * All invalid by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata)                            /* Some (not all!) invalids */
        while (length--)
            if (nulldata[length])
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

    case CPL_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] != value)
                        cpl_table_unselect_row(table, length);
        }
        else {
            while (length--)
                if (data[length] != value)
                    cpl_table_unselect_row(table, length);
        }
        break;

    case CPL_NOT_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] == value)
                        cpl_table_unselect_row(table, length);
        }
        else {
            while (length--)
                if (data[length] == value)
                    cpl_table_unselect_row(table, length);
        }
        break;

    case CPL_GREATER_THAN:
    case CPL_NOT_GREATER_THAN:
    case CPL_LESS_THAN:
    case CPL_NOT_LESS_THAN:

        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing @em double column
 *   values with a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified
 * column are compared with the reference value. The table rows
 * fulfilling the comparison are selected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See
 * also the description of the function @c cpl_table_and_selected_double().
 */

cpl_size cpl_table_or_selected_double(cpl_table *table, const char *name,
                                      cpl_table_select_operator operator,
                                      double value)
{

    cpl_column_flag *nulldata;
    double          *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_DOUBLE);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_double(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (table->selectcount == length)
        return length;

    if (nullcount == length)
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the invalids:
     */

    switch (operator) {

        case CPL_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] == value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] == value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_EQUAL_TO:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] != value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] != value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] > value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] > value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_GREATER_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] <= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] <= value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] < value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] < value)
                        cpl_table_select_row(table, length);
            }

            break;

        case CPL_NOT_LESS_THAN:

            if (nulldata) {
                while (length--)
                    if (nulldata[length] == 0)
                        if (data[length] >= value)
                            cpl_table_select_row(table, length);
            }
            else {
                while (length--)
                    if (data[length] >= value)
                        cpl_table_select_row(table, length);
            }

            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing @em double complex column
 *   values with a complex constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param value    Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Operator other than <tt>CPL_EQUAL_TO</tt> or 
 *         <tt>CPL_NOT_EQUAL_TO</tt> was specified.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified
 * column are compared with the reference value. The table rows
 * fulfilling the comparison are selected. An invalid element never
 * fulfills any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO. If the table has no 
 * rows, no error is set, and 0 is returned. See also the description 
 * of the function @c cpl_table_and_selected_double_complex().
 */

cpl_size cpl_table_or_selected_double_complex(cpl_table *table, 
                                         const char *name,
                                         cpl_table_select_operator operator,
                                         double complex value)
{

    cpl_column_flag *nulldata;
    double complex  *data;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_DOUBLE_COMPLEX);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    nulldata  = cpl_column_get_data_invalid(column);
    data      = cpl_column_get_data_double_complex(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (table->selectcount == length)
        return length;

    if (nullcount == length)
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the invalids:
     */

    switch (operator) {

    case CPL_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] == value)
                        cpl_table_select_row(table, length);
        }
        else {
            while (length--)
                if (data[length] == value)
                    cpl_table_select_row(table, length);
        }

        break;

    case CPL_NOT_EQUAL_TO:

        if (nulldata) {
            while (length--)
                if (nulldata[length] == 0)
                    if (data[length] != value)
                        cpl_table_select_row(table, length);
        }
        else {
            while (length--)
                if (data[length] != value)
                    cpl_table_select_row(table, length);
        }

        break;

    case CPL_GREATER_THAN:
    case CPL_NOT_GREATER_THAN:
    case CPL_LESS_THAN:
    case CPL_NOT_LESS_THAN:

        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows, by comparing @em string column values
 *   with a character string.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param string   Reference character string.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Invalid regular expression.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, the values of the specified
 * column are compared with the reference string. The comparison function
 * used is the C standard @c strcmp(), but in case the relational operators
 * @c CPL_EQUAL_TO or @c CPL_NOT_EQUAL_TO are specified, the comparison
 * string is treated as a regular expression. All table rows not fulfilling
 * the comparison are unselected. An invalid element never fulfills
 * any comparison by definition. Allowed relational operators are
 * @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN.
 * If the table has no rows, no error is set, and 0 is returned. See also
 * the function @c cpl_table_or_selected_string().
 */

cpl_size cpl_table_and_selected_string(cpl_table *table, const char *name,
                                       cpl_table_select_operator operator,
                                       const char *string)
{

    char      **data;
    cpl_size    length;
    cpl_size    nullcount;
    int         status;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_STRING);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    data      = cpl_column_get_data_string(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (length == 0)
        return 0;


    /*
     * All invalids by definition do not fulfill any comparison
     */

    if (nullcount == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nullcount)                      /* Some (not all!) invalids */
        while (length--)
            if (data[length] == NULL)
                cpl_table_unselect_row(table, length);

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);    /* Restore */

    switch (operator) {

        case CPL_EQUAL_TO:
        {
            regex_t re;

            status = regcomp(&re, string, REG_EXTENDED|REG_NOSUB);

            if (status) {
                cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                return -1;
            }

            while (length--) {
                if (data[length]) {
                    if (!strmatch(&re, data[length])) {
                        cpl_table_unselect_row(table, length);
                    }
                }
            }

            regfree(&re);
            break;
        }

        case CPL_NOT_EQUAL_TO:
        {
            regex_t re;

            status = regcomp(&re, string, REG_EXTENDED|REG_NOSUB);

            if (status) {
                cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                return -1;
            }

            while (length--) {
                if (data[length]) {
                    if (strmatch(&re, data[length])) {
                        cpl_table_unselect_row(table, length);
                    }
                }
            }

            regfree(&re);
            break;
        }

        case CPL_GREATER_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) <= 0)
                        cpl_table_unselect_row(table, length);
            break;

        case CPL_NOT_GREATER_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) > 0)
                        cpl_table_unselect_row(table, length);
            break;

        case CPL_LESS_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) >= 0)
                        cpl_table_unselect_row(table, length);
            break;

        case CPL_NOT_LESS_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) < 0)
                        cpl_table_unselect_row(table, length);
            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing column values with
 *   a constant.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 * @param operator Relational operator.
 * @param string   Reference value.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The specified column is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         Invalid regular expression.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, the values of the specified column
 * are compared with the reference value. The comparison function used
 * is the C standard @c strcmp(), but in case the relational operators
 * @c CPL_EQUAL_TO or @c CPL_NOT_EQUAL_TO are specified, the comparison
 * string is treated as a regular expression. The table rows fulfilling
 * the comparison are selected. An invalid element never fulfills any
 * comparison by definition. Allowed relational operators are @c CPL_EQUAL_TO,
 * @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN, @c CPL_NOT_GREATER_THAN,
 * @c CPL_LESS_THAN, and @c CPL_NOT_LESS_THAN. If the table has no rows,
 * no error is set, and 0 is returned. See also the description of the
 * function @c cpl_table_and_selected_string().
 */

cpl_size cpl_table_or_selected_string(cpl_table *table, const char *name,
                                      cpl_table_select_operator operator,
                                      const char *string)
{

    char      **data;
    cpl_size    length;
    cpl_size    nullcount;
    int         status;
    cpl_column  *column = cpl_table_find_column_type(table, name,
                                                     CPL_TYPE_STRING);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    data      = cpl_column_get_data_string(column);
    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);


    if (length == 0)
        return 0;

    if (table->selectcount == length)
        return length;

    if (nullcount == length)
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the NULLs:
     */

    switch (operator) {

        case CPL_EQUAL_TO:
        {
            regex_t re;

            status = regcomp(&re, string, REG_EXTENDED|REG_NOSUB);

            if (status) {
                cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                return -1;
            }

            while (length--) {
                if (data[length]) {
                    if (strmatch(&re, data[length])) {
                        cpl_table_select_row(table, length);
                    }
                }
            }

            regfree(&re);
            break;
        }

        case CPL_NOT_EQUAL_TO:
        {
            regex_t re;

            status = regcomp(&re, string, REG_EXTENDED|REG_NOSUB);

            if (status) {
                cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                return -1;
            }

            while (length--) {
                if (data[length]) {
                    if (!strmatch(&re, data[length])) {
                        cpl_table_select_row(table, length);
                    }
                }
            }

            regfree(&re);
            break;
        }

        case CPL_GREATER_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) > 0)
                        cpl_table_select_row(table, length);
            break;

        case CPL_NOT_GREATER_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) <= 0)
                        cpl_table_select_row(table, length);
            break;

        case CPL_LESS_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) < 0)
                        cpl_table_select_row(table, length);
            break;

        case CPL_NOT_LESS_THAN:

            while (length--)
                if (data[length])
                    if (strcmp(data[length], string) >= 0)
                        cpl_table_select_row(table, length);
            break;

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows all rows with an invalid value in
 *   a specified column.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the already selected table rows, all the rows containing valid
 * values at the specified column are unselected. See also the function
 * @c cpl_table_or_selected_invalid().
 */

cpl_size cpl_table_and_selected_invalid(cpl_table *table, const char *name)
{

    cpl_type         type;
    cpl_column_flag *nulldata = NULL;
    cpl_array      **adata    = NULL;
    char           **cdata    = NULL;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    type = cpl_column_get_type(column);

    if (type == CPL_TYPE_STRING)
        cdata = cpl_column_get_data_string(column);
    else if (type & CPL_TYPE_POINTER)
        adata = cpl_column_get_data_array(column);
    else
        nulldata = cpl_column_get_data_invalid(column);

    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (nullcount == length)
        return table->selectcount;

    if (nullcount == 0)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    /*
     * If this point is reached, there are some (not all!) invalid elements.
     */

    if (cdata) {                               /* String column */
        while (length--)
            if (cdata[length])
                cpl_table_unselect_row(table, length);
    }
    else if (adata) {                          /* Array column */
        while (length--)
            if (adata[length])
                cpl_table_unselect_row(table, length);
    }
    else {
        while (length--)
            if (!nulldata[length])
                cpl_table_unselect_row(table, length);
    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows all rows with an invalid value in
 *   a specified column.
 *
 * @param table    Pointer to table.
 * @param name     Column name.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column <i>name</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with the specified <i>name</i> is not found in <i>table</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * For all the unselected table rows, all the rows containing invalid
 * values at the specified column are selected. See also the function
 * @c cpl_table_and_selected_invalid().
 */

cpl_size cpl_table_or_selected_invalid(cpl_table *table, const char *name)
{

    cpl_type         type;
    cpl_column_flag *nulldata = NULL;
    cpl_array      **adata    = NULL;
    char           **cdata    = NULL;
    cpl_size         length;
    cpl_size         nullcount;
    cpl_column  *column = cpl_table_find_column_(table, name);

    if (!column) {
        (void)cpl_error_set_where_();
        return -1;
    }

    type = cpl_column_get_type(column);

    if (type == CPL_TYPE_STRING)
        cdata = cpl_column_get_data_string(column);
    else if (type & CPL_TYPE_POINTER)
        adata = cpl_column_get_data_array(column);
    else
        nulldata = cpl_column_get_data_invalid(column);

    length    = cpl_column_get_size(column);
    nullcount = cpl_column_count_invalid(column);

    if (nullcount == 0)
        return table->selectcount;

    if (nullcount == length)
        cpl_table_select_all(table);

    if (table->selectcount == length)
        return length;

    /*
     * If this point is reached, there are some (not all!) invalid elements.
     */

    if (cdata) {                               /* String column */
        while (length--)
            if (!cdata[length])
                cpl_table_select_row(table, length);
    }
    else if (adata) {                          /* Array column */
        while (length--)
            if (!adata[length])
                cpl_table_select_row(table, length);
    }
    else {
        while (length--)
            if (nulldata[length])
                cpl_table_select_row(table, length);
    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from selected rows only those within a table segment.
 *
 * @param table  Pointer to table.
 * @param start  First row of table segment.
 * @param count  Length of segment.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * All the selected table rows that are outside the specified interval are
 * unselected. If the sum of @em start and @em count goes beyond the end
 * of the input table, rows are checked up to the end of the table. See
 * also the function @c cpl_table_or_selected_window().
 */

cpl_size cpl_table_and_selected_window(cpl_table *table, 
                                       cpl_size start, cpl_size count)
{

    cpl_size    i;


    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (start < 0 || start >= table->nr) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return -1;
    }

    if (count < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }


    if (table->selectcount == 0)  /* Nothing was selected, nothing to "and" */
        return 0;

    if (count > table->nr - start)
        count = table->nr - start;

    if (count == table->nr)       /* "Anding" the whole table, no change */
        return table->selectcount;

    if (table->selectcount == table->nr) {

        table->select = cpl_calloc(table->nr, sizeof(cpl_column_flag));

        i = start;
        table->selectcount = count;
        while (count--) {
            table->select[i] = 1;
            i++;
        }

    }
    else {

        i = 0;
        while (i < start) {
            if (table->select[i] == 1) {
                table->select[i] = 0;
                table->selectcount--;
            }
            i++;
        }

        i += count;
        while (i < table->nr) {
            if (table->select[i] == 1) {
                table->select[i] = 0;
                table->selectcount--;
            }
            i++;
        }

        if (table->selectcount == 0) {
            if (table->select)
                cpl_free(table->select);
            table->select = NULL;
        }

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected rows only those within a table segment.
 *
 * @param table  Table to handle.
 * @param start  First row of table segment.
 * @param count  Length of segment.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>start</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         <i>count</i> is negative.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * All the unselected table rows that are within the specified interval are
 * selected. If the sum of @em start and @em count goes beyond the end of
 * the input table, rows are checked up to the end of the table. See also
 * the function @c cpl_table_and_selected_window().
 */

cpl_size cpl_table_or_selected_window(cpl_table *table, 
                                      cpl_size start, cpl_size count)
{

    cpl_size    i   = start;


    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (start < 0 || start >= table->nr) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return -1;
    }

    if (count < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    if (count > table->nr - start)
        count = table->nr - start;

    if (count == table->nr)              /* "Oring" the whole table */
        cpl_table_select_all(table);

    if (table->selectcount == table->nr) /* All was selected, no "or" is due */
        return table->selectcount;

    if (table->selectcount == 0) {

        table->select = cpl_calloc(table->nr, sizeof(cpl_column_flag));

        table->selectcount = count;
        while (count--) {
            table->select[i] = 1;
            i++;
        }

    }
    else {

        while (count--) {
            if (table->select[i] == 0) {
                table->select[i] = 1;
                table->selectcount++;
            }
            i++;
        }

        if (table->selectcount == table->nr) {
            if (table->select)
                cpl_free(table->select);
            table->select = NULL;
        }

    }

    return table->selectcount;

}


/**
 * @brief
 *   Select unselected table rows, and unselect selected ones.
 *
 * @param table  Pointer to table.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Select unselected table rows, and unselect selected ones.
 */

cpl_size cpl_table_not_selected(cpl_table *table)
{

    cpl_size    length;


    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (table->selectcount == 0)
        return table->selectcount = table->nr;

    if (table->selectcount == table->nr)
        return table->selectcount = 0;

    length = table->nr;

    while (length--) {
        if (table->select[length])
            table->select[length] = 0;
        else
            table->select[length] = 1;
    }

    return table->selectcount = table->nr - table->selectcount;

}


/**
 * @brief
 *   Select from selected table rows, by comparing the values of two
 *   numerical columns.
 *
 * @param table    Pointer to table.
 * @param name1    Name of first table column.
 * @param operator Relational operator.
 * @param name2    Name of second table column.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column names are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with any of the specified names is not found in
 *         <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Invalid types for comparison.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Either both columns must be numerical, or they both must be strings. 
 * The comparison between strings is lexicographical. Comparison between 
 * complex types and array types are not supported.
 *
 * For all the already selected table rows, the values of the specified 
 * columns are compared. The table rows not fulfilling the comparison 
 * are unselected. Invalid elements from either columns never fulfill 
 * any comparison by definition. Allowed relational operators are 
 * @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN, 
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, @c CPL_NOT_LESS_THAN. 
 * See also the function @c cpl_table_or_selected().
 */

cpl_size cpl_table_and_selected(cpl_table *table, const char *name1,
                                cpl_table_select_operator operator,
                                const char *name2)
{

    cpl_type         type1;
    cpl_type         type2;
    cpl_column_flag *nulldata1;
    cpl_column_flag *nulldata2;
    char           **sdata1;
    char           **sdata2;
    cpl_size         nullcount1;
    cpl_size         nullcount2;
    cpl_size         length;
    cpl_column  *column1 = cpl_table_find_column_(table, name1);
    cpl_column  *column2 = cpl_table_find_column_(table, name2);

    if (!column1 || !column2) {
        (void)cpl_error_set_where_();
        return -1;
    }

    type1 = cpl_column_get_type(column1);
    type2 = cpl_column_get_type(column2);

    if (type1 == type2) {
        if ((type1 & CPL_TYPE_COMPLEX) || (type1 & CPL_TYPE_POINTER)) {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return -1;
        }
    }
    else {
        if ((type1 == CPL_TYPE_STRING) || (type2 == CPL_TYPE_STRING) ||
            (type1 & CPL_TYPE_COMPLEX) || (type2 & CPL_TYPE_COMPLEX) ||
            (type1 & CPL_TYPE_POINTER) || (type2 & CPL_TYPE_POINTER)) {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return -1;
        }
    }

    nulldata1  = cpl_column_get_data_invalid(column1);
    nulldata2  = cpl_column_get_data_invalid(column2);
    nullcount1 = cpl_column_count_invalid(column1);
    nullcount2 = cpl_column_count_invalid(column2);
    length     = cpl_column_get_size(column1);

    if (length == 0)
        return 0;


    /*
     * All invalid elements by definition do not fulfill any comparison
     */

    if (nullcount1 == length || nullcount2 == length)
        cpl_table_unselect_all(table);

    if (table->selectcount == 0)
        return 0;

    if (nulldata1)                          /* Some (not all!) NULLs */
        while (length--)
            if (nulldata1[length])
                cpl_table_unselect_row(table, length);

    if (type1 == CPL_TYPE_STRING) {       /* In case of string comparison */
        sdata1 = cpl_column_get_data_string(column1);
        while (length--) {
            if (sdata1[length] == NULL) {
                cpl_table_unselect_row(table, length);
            }
        }
    }

    if (table->selectcount == 0)
        return 0;

    length = cpl_table_get_nrow(table);     /* Restore */

    if (nulldata2)                          /* Some (not all!) NULLs */
        while (length--)
            if (nulldata2[length])
                cpl_table_unselect_row(table, length);

    if (type2 == CPL_TYPE_STRING) {       /* In case of string comparison */
        sdata2 = cpl_column_get_data_string(column2);
        while (length--) {
            if (sdata2[length] == NULL) {
                cpl_table_unselect_row(table, length);
            }
        }
    }

    if (table->selectcount == 0)
        return 0;


    /*
     * Moreover unselect anything that does not fulfill the comparison:
     */

    length = cpl_table_get_nrow(table);     /* Restore */

    switch (type1) {

    case CPL_TYPE_INT:
    {
        int *idata1 = cpl_column_get_data_int(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (idata1[length] != idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (idata1[length] == idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (idata1[length] <= idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (idata1[length] > idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (idata1[length] >= idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (idata1[length] < idata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((long)idata1[length] != ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((long)idata1[length] == ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((long)idata1[length] <= ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((long)idata1[length] > ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((long)idata1[length] >= ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((long)idata1[length] < ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((long long)idata1[length] != lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((long long)idata1[length] == lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((long long)idata1[length] <= lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((long long)idata1[length] > lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((long long)idata1[length] >= lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((long long)idata1[length] < lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((float)idata1[length] != fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((float)idata1[length] == fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((float)idata1[length] <= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((float)idata1[length] > fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((float)idata1[length] >= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((float)idata1[length] < fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((double)idata1[length] != ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((double)idata1[length] == ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((double)idata1[length] <= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((double)idata1[length] > ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((double)idata1[length] >= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((double)idata1[length] < ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        default:

            break;

        } // switch(type2)

        break;

    } // case CPL_TYPE_INT

    case CPL_TYPE_LONG:
    {
        long *ldata1 = cpl_column_get_data_long(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (ldata1[length] != (long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (ldata1[length] == (long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (ldata1[length] <= (long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (ldata1[length] > (long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (ldata1[length] >= (long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (ldata1[length] < (long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (ldata1[length] != ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (ldata1[length] == ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (ldata1[length] <= ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (ldata1[length] > ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (ldata1[length] >= ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (ldata1[length] < ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((long long)ldata1[length] != lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((long long)ldata1[length] == lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((long long)ldata1[length] <= lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((long long)ldata1[length] > lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((long long)ldata1[length] >= lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((long long)ldata1[length] < lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((float)ldata1[length] != fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((float)ldata1[length] == fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((float)ldata1[length] <= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((float)ldata1[length] > fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((float)ldata1[length] >= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((float)ldata1[length] < fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((double)ldata1[length] != ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((double)ldata1[length] == ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((double)ldata1[length] <= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((double)ldata1[length] > ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((double)ldata1[length] >= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((double)ldata1[length] < ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        default:

            break;

        } // switch (type2)

        break;

    } // case CPL_TYPE_LONG

    case CPL_TYPE_LONG_LONG:
    {
        long long *lldata1 = cpl_column_get_data_long_long(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (lldata1[length] != (long long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (lldata1[length] == (long long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (lldata1[length] <= (long long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (lldata1[length] > (long long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (lldata1[length] >= (long long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (lldata1[length] < (long long)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (lldata1[length] != (long long)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (lldata1[length] == (long long)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (lldata1[length] <= (long long)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (lldata1[length] > (long long)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (lldata1[length] >= (long long)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (lldata1[length] < (long long)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (lldata1[length] != lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (lldata1[length] == lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (lldata1[length] <= lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (lldata1[length] > lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (lldata1[length] >= lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (lldata1[length] < lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((float)lldata1[length] != fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((float)lldata1[length] == fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((float)lldata1[length] <= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((float)lldata1[length] > fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((float)lldata1[length] >= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((float)lldata1[length] < fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((double)lldata1[length] != ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((double)lldata1[length] == ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((double)lldata1[length] <= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((double)lldata1[length] > ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((double)lldata1[length] >= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((double)lldata1[length] < ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        default:

            break;

        } // switch (type2)

        break;

    } // case CPL_TYPE_LONG_LONG

    case CPL_TYPE_FLOAT:
    {
        float *fdata1 = cpl_column_get_data_float(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (fdata1[length] != (float)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (fdata1[length] == (float)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (fdata1[length] <= (float)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (fdata1[length] > (float)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (fdata1[length] >= (float)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (fdata1[length] < (float)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (fdata1[length] != (float)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (fdata1[length] == (float)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (fdata1[length] <= (float)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (fdata1[length] > (float)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (fdata1[length] >= (float)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (fdata1[length] < (float)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (fdata1[length] != (float)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (fdata1[length] == (float)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (fdata1[length] <= (float)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (fdata1[length] > (float)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (fdata1[length] >= (float)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (fdata1[length] < (float)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (fdata1[length] != fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (fdata1[length] == fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (fdata1[length] <= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (fdata1[length] > fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (fdata1[length] >= fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (fdata1[length] < fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if ((double)fdata1[length] != ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if ((double)fdata1[length] == ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if ((double)fdata1[length] <= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if ((double)fdata1[length] > ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if ((double)fdata1[length] >= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if ((double)fdata1[length] < ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        default:

            break;

        } // switch (type2)

        break;

    } //case CPL_TYPE_FLOAT

    case CPL_TYPE_DOUBLE:
    {
        double *ddata1 = cpl_column_get_data_double(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (ddata1[length] != (double)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (ddata1[length] == (double)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (ddata1[length] <= (double)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (ddata1[length] > (double)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (ddata1[length] >= (double)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (ddata1[length] < (double)idata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (ddata1[length] != (double)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (ddata1[length] == (double)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (ddata1[length] <= (double)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (ddata1[length] > (double)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (ddata1[length] >= (double)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (ddata1[length] < (double)ldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (ddata1[length] != (double)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (ddata1[length] == (double)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (ddata1[length] <= (double)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (ddata1[length] > (double)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (ddata1[length] >= (double)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (ddata1[length] < (double)lldata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (ddata1[length] != (double)fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (ddata1[length] == (double)fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (ddata1[length] <= (double)fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (ddata1[length] > (double)fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (ddata1[length] >= (double)fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (ddata1[length] < (double)fdata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                while (length--)
                    if (ddata1[length] != ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_EQUAL_TO:

                while (length--)
                    if (ddata1[length] == ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_GREATER_THAN:

                while (length--)
                    if (ddata1[length] <= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_GREATER_THAN:

                while (length--)
                    if (ddata1[length] > ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_LESS_THAN:

                while (length--)
                    if (ddata1[length] >= ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;

            case CPL_NOT_LESS_THAN:

                while (length--)
                    if (ddata1[length] < ddata2[length])
                        cpl_table_unselect_row(table, length);
                break;
            }

            break;
        }

        default:

            break;

        }  // end switch (type2)

        break;

    } // end CPL_TYPE_DOUBLE

    case CPL_TYPE_STRING:
    {
        sdata1 = cpl_column_get_data_string(column1);
        sdata2 = cpl_column_get_data_string(column2);

        switch (operator) {

        case CPL_EQUAL_TO:

            while (length--)
                if (strcmp(sdata1[length], sdata2[length]))
                    cpl_table_unselect_row(table, length);
            break;

        case CPL_NOT_EQUAL_TO:

            while (length--)
                if (!strcmp(sdata1[length], sdata2[length]))
                    cpl_table_unselect_row(table, length);
            break;

        case CPL_GREATER_THAN:

            while (length--)
                if (strcmp(sdata1[length], sdata2[length]) <= 0)
                    cpl_table_unselect_row(table, length);
            break;

        case CPL_NOT_GREATER_THAN:

            while (length--)
                if (strcmp(sdata1[length], sdata2[length]) > 0)
                    cpl_table_unselect_row(table, length);
            break;

        case CPL_LESS_THAN:

            while (length--)
                if (strcmp(sdata1[length], sdata2[length]) >= 0)
                    cpl_table_unselect_row(table, length);
            break;

        case CPL_NOT_LESS_THAN:

            while (length--)
                if (strcmp(sdata1[length], sdata2[length]) < 0)
                    cpl_table_unselect_row(table, length);
            break;
        }

        break;

    } // end CASE_TYPE_STRING

    default:

        break;

    } // end switch (type1)

    return table->selectcount;

}


/**
 * @brief
 *   Select from unselected table rows, by comparing the values of two
 *   numerical columns.
 *
 * @param table    Pointer to table.
 * @param name1    Name of first table column.
 * @param operator Relational operator.
 * @param name2    Name of second table column.
 *
 * @return Current number of selected rows, or a negative number in case
 *   of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or column names are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         A column with any of the specified names is not found in
 *         <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Invalid types for comparison.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Either both columns must be numerical, or they both must be strings. 
 * The comparison between strings is lexicographical. Comparison between 
 * complex types and array types are not supported.
 *
 * Both columns must be numerical. For all the unselected table rows, the
 * values of the specified columns are compared. The table rows fulfilling
 * the comparison are selected. Invalid elements from either columns never
 * fulfill any comparison by definition. Allowed relational operators
 * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
 * @c CPL_NOT_GREATER_THAN, @c CPL_LESS_THAN, @c CPL_NOT_LESS_THAN.
 * See also the function @c cpl_table_and_selected().
 */

cpl_size cpl_table_or_selected(cpl_table *table, const char *name1,
                               cpl_table_select_operator operator,
                               const char *name2)
{

    cpl_type         type1;
    cpl_type         type2;
    cpl_column_flag *nulldata1;
    cpl_column_flag *nulldata2;
    char           **sdata1;
    char           **sdata2;
    cpl_size         nullcount1;
    cpl_size         nullcount2;
    cpl_size         length;
    cpl_column  *column1 = cpl_table_find_column_(table, name1);
    cpl_column  *column2 = cpl_table_find_column_(table, name2);

    if (!column1 || !column2) {
        (void)cpl_error_set_where_();
        return -1;
    }

    type1 = cpl_column_get_type(column1);
    type2 = cpl_column_get_type(column2);

    if (type1 == type2) {
        if ((type1 & CPL_TYPE_COMPLEX) || (type1 & CPL_TYPE_POINTER)) {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return -1;
        }
    }
    else {
        if ((type1 == CPL_TYPE_STRING) || (type2 == CPL_TYPE_STRING) ||
            (type1 & CPL_TYPE_COMPLEX) || (type2 & CPL_TYPE_COMPLEX) ||
            (type1 & CPL_TYPE_POINTER) || (type2 & CPL_TYPE_POINTER)) {
            cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return -1;
        }
    }

    nulldata1  = cpl_column_get_data_invalid(column1);
    nulldata2  = cpl_column_get_data_invalid(column2);
    nullcount1 = cpl_column_count_invalid(column1);
    nullcount2 = cpl_column_count_invalid(column2);
    length     = cpl_column_get_size(column1);

    if (length == 0)
        return 0;

    if (table->selectcount == length)         /* It's already all selected */
        return length;

    if (nullcount1 == length || nullcount2 == length)
        return table->selectcount;

    /*
     * Select anything that fulfills the comparison, avoiding the NULLs:
     */

    switch (type1) {

    case CPL_TYPE_INT:
    {
        int *idata1 = cpl_column_get_data_int(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (idata1[length] == idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (idata1[length] == idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (idata1[length] == idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (idata1[length] == idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (idata1[length] != idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (idata1[length] != idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (idata1[length] != idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (idata1[length] != idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (idata1[length] > idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (idata1[length] > idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (idata1[length] > idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (idata1[length] > idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (idata1[length] <= idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (idata1[length] <= idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (idata1[length] <= idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (idata1[length] <= idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (idata1[length] < idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (idata1[length] < idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (idata1[length] < idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (idata1[length] < idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (idata1[length] >= idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (idata1[length] >= idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (idata1[length] >= idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (idata1[length] >= idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long)idata1[length] == ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long)idata1[length] == ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long)idata1[length] == ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long)idata1[length] == ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long)idata1[length] != ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long)idata1[length] != ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long)idata1[length] != ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long)idata1[length] != ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long)idata1[length] > ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long)idata1[length] > ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long)idata1[length] > ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long)idata1[length] > ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long)idata1[length] <= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long)idata1[length] <= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long)idata1[length] <= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long)idata1[length] <= ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long)idata1[length] < ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long)idata1[length] < ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long)idata1[length] < ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long)idata1[length] < ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long)idata1[length] >= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long)idata1[length] >= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long)idata1[length] >= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long)idata1[length] >= ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)idata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)idata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)idata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)idata1[length] == lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)idata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)idata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)idata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)idata1[length] != lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)idata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)idata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)idata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)idata1[length] > lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)idata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)idata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)idata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)idata1[length] <= lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)idata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)idata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)idata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)idata1[length] < lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)idata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)idata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)idata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)idata1[length] >= lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)idata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)idata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)idata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)idata1[length] == fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)idata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)idata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)idata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)idata1[length] != fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)idata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)idata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)idata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)idata1[length] > fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)idata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)idata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)idata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)idata1[length] <= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)idata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)idata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)idata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)idata1[length] < fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)idata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)idata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)idata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)idata1[length] >= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)idata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)idata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)idata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)idata1[length] == ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)idata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)idata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)idata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)idata1[length] != ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)idata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)idata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)idata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)idata1[length] > ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)idata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)idata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)idata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)idata1[length] <= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)idata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)idata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)idata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)idata1[length] < ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)idata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)idata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)idata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)idata1[length] >= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        default:

            break;

        } // end switch (type2)

        break;

    } // end case CPL_TYPE_INT

    case CPL_TYPE_LONG:
    {
        long *ldata1 = cpl_column_get_data_long(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] == (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] == (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] == (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] == (long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] != (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] != (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] != (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] != (long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] > (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] > (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] > (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] > (long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] <= (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] <= (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] <= (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] <= (long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] < (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] < (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] < (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] < (long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] >= (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] >= (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] >= (long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] >= (long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] == ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] == ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] == ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] == ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] != ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] != ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] != ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] != ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] > ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] > ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] > ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] > ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] <= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] <= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] <= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] <= ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] < ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] < ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] < ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] < ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ldata1[length] >= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ldata1[length] >= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ldata1[length] >= ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ldata1[length] >= ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)ldata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)ldata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)ldata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)ldata1[length] == lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)ldata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)ldata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)ldata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)ldata1[length] != lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)ldata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)ldata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)ldata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)ldata1[length] > lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)ldata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)ldata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)ldata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)ldata1[length] <= lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)ldata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)ldata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)ldata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)ldata1[length] < lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((long long)ldata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((long long)ldata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((long long)ldata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((long long)ldata1[length] >= lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)ldata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)ldata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)ldata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)ldata1[length] == fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)ldata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)ldata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)ldata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)ldata1[length] != fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)ldata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)ldata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)ldata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)ldata1[length] > fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)ldata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)ldata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)ldata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)ldata1[length] <= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)ldata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)ldata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)ldata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)ldata1[length] < fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)ldata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)ldata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)ldata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)ldata1[length] >= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)ldata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)ldata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)ldata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)ldata1[length] == ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)ldata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)ldata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)ldata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)ldata1[length] != ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)ldata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)ldata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)ldata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)ldata1[length] > ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)ldata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)ldata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)ldata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)ldata1[length] <= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)ldata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)ldata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)ldata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)ldata1[length] < ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)ldata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)ldata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)ldata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)ldata1[length] >= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        default:

            break;

        } // end switch (type2)

        break;

    } // end case CPL_TYPE_LONG

    case CPL_TYPE_LONG_LONG:
    {
        long long *lldata1 = cpl_column_get_data_long_long(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] == (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] == (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] == (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] == (long long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] != (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] != (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] != (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] != (long long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] > (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] > (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] > (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] > (long long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] <= (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] <= (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] <= (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] <= (long long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] < (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] < (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] < (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] < (long long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] >= (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] >= (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] >= (long long)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] >= (long long)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] == (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] == (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] == (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] == (long long)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] != (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] != (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] != (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] != (long long)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] > (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] > (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] > (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] > (long long)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] <= (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] <= (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] <= (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] <= (long long)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] < (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] < (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] < (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] < (long long)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] >= (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] >= (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] >= (long long)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] >= (long long)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] == lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] == lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] != lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] != lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] > lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] > lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] <= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] <= lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] < lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] < lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (lldata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (lldata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (lldata1[length] >= lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (lldata1[length] >= lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)lldata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)lldata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)lldata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)lldata1[length] == fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)lldata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)lldata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)lldata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)lldata1[length] != fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)lldata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)lldata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)lldata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)lldata1[length] > fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)lldata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)lldata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)lldata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)lldata1[length] <= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)lldata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)lldata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)lldata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)lldata1[length] < fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((float)lldata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((float)lldata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((float)lldata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((float)lldata1[length] >= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)lldata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)lldata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)lldata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)lldata1[length] == ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)lldata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)lldata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)lldata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)lldata1[length] != ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)lldata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)lldata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)lldata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)lldata1[length] > ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)lldata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)lldata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)lldata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)lldata1[length] <= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)lldata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)lldata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)lldata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)lldata1[length] < ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)lldata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)lldata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)lldata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)lldata1[length] >= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        default:

            break;

        } // end switch (type2)

        break;

    } // end case CPL_TYPE_LONG_LONG

    case CPL_TYPE_FLOAT:
    {
        float *fdata1 = cpl_column_get_data_float(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] == (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] == (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] == (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] == (float)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] != (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] != (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] != (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] != (float)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] > (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] > (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] > (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] > (float)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] <= (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] <= (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] <= (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] <= (float)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] < (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] < (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] < (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] < (float)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] >= (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] >= (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] >= (float)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] >= (float)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;

        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] == (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] == (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] == (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] == (float)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] != (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] != (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] != (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] != (float)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] > (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] > (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] > (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] > (float)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] <= (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] <= (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] <= (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] <= (float)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] < (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] < (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] < (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] < (float)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] >= (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] >= (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] >= (float)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] >= (float)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] == (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] == (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] == (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] == (float)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] != (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] != (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] != (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] != (float)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] > (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] > (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] > (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] > (float)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] <= (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] <= (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] <= (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] <= (float)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] < (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] < (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] < (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] < (float)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] >= (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] >= (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] >= (float)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] >= (float)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] == fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] == fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] != fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] != fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] > fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] > fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] <= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] <= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] < fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] < fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (fdata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (fdata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (fdata1[length] >= fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (fdata1[length] >= fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;

        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)fdata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)fdata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)fdata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)fdata1[length] == ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)fdata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)fdata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)fdata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)fdata1[length] != ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)fdata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)fdata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)fdata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)fdata1[length] > ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)fdata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)fdata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)fdata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)fdata1[length] <= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)fdata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)fdata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)fdata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)fdata1[length] < ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if ((double)fdata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if ((double)fdata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if ((double)fdata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if ((double)fdata1[length] >= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;

        }

        default:

            break;

        } // end switch (type2)

        break;

    } // end case CPL_TYPE_FLOAT

    case CPL_TYPE_DOUBLE:
    {
        double *ddata1 = cpl_column_get_data_double(column1);

        switch (type2) {

        case CPL_TYPE_INT:
        {
            int *idata2 = cpl_column_get_data_int(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] == (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] == (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] == (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] == (double)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] != (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] != (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] != (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] != (double)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] > (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] > (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] > (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] > (double)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] <= (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] <= (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] <= (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] <= (double)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] < (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] < (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] < (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] < (double)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] >= (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] >= (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] >= (double)idata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] >= (double)idata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG:
        {
            long *ldata2 = cpl_column_get_data_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] == (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] == (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] == (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] == (double)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] != (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] != (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] != (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] != (double)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] > (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] > (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] > (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] > (double)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] <= (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] <= (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] <= (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] <= (double)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] < (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] < (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] < (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] < (double)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] >= (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] >= (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] >= (double)ldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] >= (double)ldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            long long *lldata2 = cpl_column_get_data_long_long(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] == (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] == (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] == (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] == (double)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] != (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] != (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] != (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] != (double)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] > (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] > (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] > (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] > (double)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] <= (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] <= (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] <= (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] <= (double)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] < (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] < (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] < (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] < (double)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] >= (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] >= (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] >= (double)lldata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] >= (double)lldata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_FLOAT:
        {
            float *fdata2 = cpl_column_get_data_float(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] == (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] == (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] == (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] == (double)fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] != (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] != (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] != (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] != (double)fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] > (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] > (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] > (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] > (double)fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] <= (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] <= (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] <= (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] <= (double)fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] < (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] < (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] < (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] < (double)fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] >= (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] >= (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] >= (double)fdata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] >= (double)fdata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double *ddata2 = cpl_column_get_data_double(column2);

            switch (operator) {

            case CPL_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] == ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] == ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_EQUAL_TO:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] != ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] != ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] > ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] > ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_GREATER_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] <= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] <= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] < ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] < ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            case CPL_NOT_LESS_THAN:

                if (nulldata1 && nulldata2) {
                    while (length--)
                        if (nulldata1[length] == 0 && nulldata2[length] == 0)
                            if (ddata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata1) {
                    while (length--)
                        if (nulldata1[length] == 0)
                            if (ddata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else if (nulldata2) {
                    while (length--)
                        if (nulldata2[length] == 0)
                            if (ddata1[length] >= ddata2[length])
                                cpl_table_select_row(table, length);
                }
                else {
                    while (length--)
                        if (ddata1[length] >= ddata2[length])
                            cpl_table_select_row(table, length);
                }

                break;

            }

            break;
        }

        default:

            break;

        } // end switch (type2)

        break;

    } // end case CPL_TYPE_DOUBLE

    case CPL_TYPE_STRING:
    {
        sdata1 = cpl_column_get_data_string(column1);
        sdata2 = cpl_column_get_data_string(column2);

        switch (operator) {

        case CPL_EQUAL_TO:

            while (length--)
                if (sdata1[length] && sdata2[length])
                    if (!strcmp(sdata1[length], sdata2[length]))
                        cpl_table_select_row(table, length);
            break;

        case CPL_NOT_EQUAL_TO:

            while (length--)
                if (sdata1[length] && sdata2[length])
                    if (strcmp(sdata1[length], sdata2[length]))
                        cpl_table_select_row(table, length);
            break;

        case CPL_GREATER_THAN:

            while (length--)
                if (sdata1[length] && sdata2[length])
                    if (strcmp(sdata1[length], sdata2[length]) > 0)
                        cpl_table_select_row(table, length);
            break;

        case CPL_NOT_GREATER_THAN:

            while (length--)
                if (sdata1[length] && sdata2[length])
                    if (strcmp(sdata1[length], sdata2[length]) <= 0)
                        cpl_table_select_row(table, length);
            break;

        case CPL_LESS_THAN:

            while (length--)
                if (sdata1[length] && sdata2[length])
                    if (strcmp(sdata1[length], sdata2[length]) < 0)
                        cpl_table_select_row(table, length);
            break;

        case CPL_NOT_LESS_THAN:

            while (length--)
                if (sdata1[length] && sdata2[length])
                    if (strcmp(sdata1[length], sdata2[length]) >= 0)
                        cpl_table_select_row(table, length);
            break;
        }

        break;

    } // end case CPL_TYPE_STRING

    default:

        break;

    } // end switch (type1)

    return table->selectcount;

}


/**
 * @brief
 *   Determine whether a table row is selected or not.
 *
 * @param table  Pointer to table.
 * @param row    Table row to check.
 *
 * @return 1 if row is selected, 0 if it is not selected, -1 in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>table</i> has zero length, or <i>row</i> is
 *         outside the table boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Check if a table row is selected.
 */

int cpl_table_is_selected(const cpl_table *table, cpl_size row)
{



    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (row < 0 || row >= table->nr) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return -1;
    }

    if (table->selectcount == 0)
        return 0;

    if (table->selectcount == table->nr)
        return 1;

    return table->select[row];

}


/**
 * @brief
 *   Get number of selected rows in given table.
 *
 * @param table  Pointer to table.
 *
 * @return Number of selected rows, or a negative number in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get number of selected rows in given table.
 */

cpl_size cpl_table_count_selected(const cpl_table *table)
{



    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    return table->selectcount;

}


/**
 * @brief
 *   Get array of indexes to selected table rows
 *
 * @param table  Pointer to table.
 *
 * @return Indexes to selected table rows, or NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Get array of indexes to selected table rows. If no rows are selected,
 * an array of size zero is returned. The returned array must be deleted
 * using @c cpl_array_delete().
 */

cpl_array *cpl_table_where_selected(const cpl_table *table)
{

    cpl_array *array;
    cpl_size  *flags;
    cpl_size   i;


    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    array = cpl_array_new(table->selectcount, CPL_TYPE_SIZE);

    cpl_array_fill_window(array, 0, table->selectcount, 0);
    flags = cpl_array_get_data_cplsize(array);

    if (table->selectcount == table->nr)
        for (i = 0; i < table->nr; i++)
            *flags++ = i;
    else if (table->selectcount > 0)
        for (i = 0; i < table->nr; i++)
            if (table->select[i])
                *flags++ = i;

    return array;

}


/**
 * @brief
 *   Create a new table from the selected rows of another table.
 *
 * @param table  Pointer to table.
 *
 * @return Pointer to new table, or @c NULL in case of error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A new table is created, containing a copy of all the selected
 * rows of the input table. In the output table all rows are selected.
 */

cpl_table *cpl_table_extract_selected(const cpl_table *table)
{

    cpl_table  *new_table;
    int         ivalue;
    float       fvalue;
    double      dvalue;
    char       *svalue;
    cpl_array  *avalue;
    cpl_size    from_row;
    cpl_size    to_row;
    cpl_size    count;
    cpl_size    i, j, l, m, n;
    int         isnull;


    if (table == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (table->selectcount == table->nr)
        return cpl_table_duplicate(table);

    new_table = cpl_table_new(table->selectcount);
    cpl_table_copy_structure(new_table, table);

    if (table->selectcount == 0)
        return new_table;

    from_row = 0;
    to_row = 0;
    count = 0;
    i = 0;
    while (i < table->nr) {

        if (table->select[i]) {
            if (count == 0)
                from_row = i;
            count++;
            i++;
            if (i != table->nr)
                continue;
            i--;
        }

        if (count) {

            j = 0;

            while (j < table->nc) {

                cpl_type type_id = cpl_column_get_type(table->columns[j]);

                l = 0;
                m = from_row;
                n = to_row;

                if (_cpl_columntype_is_pointer(type_id)) {

                    switch (_cpl_columntype_get_valuetype(type_id)) {

                        case CPL_TYPE_INT:
                        case CPL_TYPE_LONG_LONG:
                        case CPL_TYPE_FLOAT:
                        case CPL_TYPE_DOUBLE:
                        case CPL_TYPE_FLOAT_COMPLEX:
                        case CPL_TYPE_DOUBLE_COMPLEX:
                        case CPL_TYPE_STRING:
                        {
                            while (l < count) {
                                avalue = cpl_column_get_array(table->columns[j], m);
                                cpl_column_set_array(new_table->columns[j], n, avalue);

                                l++;
                                m++;
                                n++;
                            }
                            break;
                        }

                        default:
                            break;
                    }

                }
                else {

                    /*
                     * This is slow, but acceptable for the moment.
                     * It should be replaced by a call to a function
                     * that copies efficiently column segments into
                     * other column segments (including the NULL
                     * flags).
                     */

                    switch (_cpl_columntype_get_valuetype(type_id)) {

                        case CPL_TYPE_INT:

                            while (l < count) {
                                ivalue = cpl_column_get_int(table->columns[j],
                                                            m, &isnull);

                                if (isnull)
                                    cpl_column_set_invalid(new_table->columns[j], n);
                                else
                                    cpl_column_set_int(new_table->columns[j],
                                                       n, ivalue);

                                l++;
                                m++;
                                n++;
                            }
                            break;

                        case CPL_TYPE_LONG_LONG:

                            while (l < count) {
                                long long value =
                                        cpl_column_get_long_long(table->columns[j],
                                                                 m, &isnull);

                                if (isnull)
                                    cpl_column_set_invalid(new_table->columns[j], n);
                                else
                                    cpl_column_set_long_long(new_table->columns[j],
                                                             n, value);

                                l++;
                                m++;
                                n++;
                            }
                            break;

                        case CPL_TYPE_FLOAT:

                            while (l < count) {
                                fvalue =
                                        cpl_column_get_float(table->columns[j],
                                                             m, &isnull);
                                if (isnull)
                                    cpl_column_set_invalid(new_table->columns[j], n);
                                else
                                    cpl_column_set_float(
                                            new_table->columns[j], n, fvalue);

                                l++;
                                m++;
                                n++;
                            }
                            break;

                        case CPL_TYPE_DOUBLE:

                            while (l < count) {
                                dvalue = cpl_column_get_double(table->columns[j],
                                                               m, &isnull);

                                if (isnull)
                                    cpl_column_set_invalid(new_table->columns[j], n);
                                else
                                    cpl_column_set_double(
                                            new_table->columns[j], n, dvalue);

                                l++;
                                m++;
                                n++;
                            }
                            break;

                        case CPL_TYPE_FLOAT_COMPLEX:

                            while (l < count) {
                                float complex value =
                                        cpl_column_get_float_complex(table->columns[j],
                                                                     m, &isnull);
                                if (isnull)
                                    cpl_column_set_invalid(new_table->columns[j], n);
                                else
                                    cpl_column_set_float_complex(
                                            new_table->columns[j], n, value);

                                l++;
                                m++;
                                n++;
                            }
                            break;

                        case CPL_TYPE_DOUBLE_COMPLEX:

                            while (l < count) {
                                double complex value =
                                        cpl_column_get_double_complex(table->columns[j],
                                                                      m, &isnull);
                                if (isnull)
                                    cpl_column_set_invalid(new_table->columns[j], n);
                                else
                                    cpl_column_set_double_complex(
                                            new_table->columns[j], n, value);

                                l++;
                                m++;
                                n++;
                            }
                            break;

                        case CPL_TYPE_STRING:

                            while (l < count) {
                                svalue =
                                        (char *)cpl_column_get_string(table->columns[j], m);
                                cpl_column_set_string(new_table->columns[j], n, svalue);

                                l++;
                                m++;
                                n++;
                            }
                            break;

                        default:
                            break;
                    }

                }

                j++;
            }

            to_row += count;
            count = 0;

            if (to_row == new_table->nr)
                break;
        }

        i++;

    }

    return new_table;

}


/**
 * @brief
 *   Sort table rows according to columns values.
 *
 * @param table   Pointer to table.
 * @param reflist Names of reference columns with corresponding sorting mode.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>reflist</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         Any reference column specified in <i>reflist</i> is not found in
 *         <i>table</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The size of <i>reflist</i> exceeds the total number of columns
 *         in <i>table</i>, or <i>reflist</i> is empty.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The input <i>reflist</i> includes properties of type
 *         different from <tt>CPL_TYPE_BOOL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSUPPORTED_MODE</td>
 *       <td class="ecr">
 *         Any reference column specified in <i>reflist</i> is of type
 *         <i>array</i>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The table rows are sorted according to the values of the specified
 * reference columns. The reference column names are listed in the input
 * @em reflist, that associates to each reference column a boolean value.
 * If the associated value is @c FALSE, the table is sorted according
 * to the ascending values of the specified column, otherwise if the
 * associated value is @c TRUE, the table is sorted according to the
 * descending values of that column. The sorting will be performed by
 * comparing the values of the first reference column; if the compared
 * values are equal, the values from the next column in the list are
 * compared, and so on till the end of the reference columns list.
 * An invalid table element is always treated as if it doesn't fulfill
 * any comparison, i.e., sorting either by increasing or decreasing
 * column values would accumulate invalid elements toward the top of
 * the table. The sorting is made in-place, therefore pointers to
 * data retrieved with calls to @c cpl_table_get_data_<TYPE>()
 * remain valid.
 */

cpl_error_code cpl_table_sort(cpl_table *table, const cpl_propertylist *reflist)
{
    cpl_size      *sort_pattern, *sort_null_pattern;
    cpl_error_code ec;

    if (table == NULL || reflist == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_propertylist_get_size(reflist) == 0)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (table->nr < 2)
        return CPL_ERROR_NONE;

    sort_pattern      = cpl_malloc(table->nr * sizeof(cpl_size));
    sort_null_pattern = cpl_malloc(table->nr * sizeof(cpl_size));

    ec = table_sort(table, reflist, cpl_propertylist_get_size(reflist),
                    sort_pattern, sort_null_pattern);

    cpl_free(sort_pattern);
    cpl_free(sort_null_pattern);

    return ec;
}


static cpl_table *cpl_table_overload_window(const char *filename, int xtnum, 
                                            int check_nulls,
                                            const cpl_array *selcol,
                                            cpl_size firstrow, cpl_size nrow)
{


    fitsfile         *fptr;

    cpl_table        *table;

    char              colname[FLEN_VALUE];
    char              colunit[FLEN_VALUE];
    char              comment[FLEN_COMMENT];
    char              keyname[FLEN_KEYWORD];

    cpl_size          nullcount;

    int               extension_count;
    cpl_size          field_size;
    int               ncol;
    cpl_size          nrows;
    int               naxis;
    cpl_size         *naxes;
    int               i, k;
    cpl_size          j;
    cpl_size          depth;
    cpl_size          z, zz;
    cpl_array        *adim;
    cpl_array       **array;
    cpl_column       *acolumn;
    cpl_column_flag  *nulldata;
    char              err_text[FLEN_STATUS];
    const char      **extcol = NULL;
    char             *barray;
    int               xcol = 0;
    int               extract;
    int               colnum;
    int               status = 0;
    int               hdutype;
    int               typecode;


    err_text[0] = '\0';

    if (filename == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (access(filename, F_OK)) {
        cpl_error_set_(CPL_ERROR_FILE_NOT_FOUND);
        return NULL;
    }

    if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &status)) {

        (void)cpl_error_set_fits(status == FILE_NOT_OPENED ?
                                 CPL_ERROR_FILE_IO : CPL_ERROR_BAD_FILE_FORMAT,
                                 status, fits_open_diskfile, "filename='%s', "
                                 "position=%d", filename, xtnum);
    }

    fits_get_num_hdus(fptr, &extension_count, &status);

    if (status) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                 fits_get_num_hdus, "filename='%s', position=%d",
                                 filename, xtnum);
        status = 0;
        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }

    if (extension_count < 0) {
        cpl_error_set_(CPL_ERROR_BAD_FILE_FORMAT);
        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }

    if (extension_count == 0) {
        cpl_error_set_(CPL_ERROR_BAD_FILE_FORMAT);
        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }

    /*
     * HDUs in the CFITSIO convention are counted starting from 1
     * (the primary array). Therefore we need to adapt the input to 
     * this convention:
     */

    xtnum++;

    if (xtnum < 2 || xtnum > extension_count) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }

    fits_movabs_hdu(fptr, xtnum, NULL, &status);
    fits_get_hdu_type(fptr, &hdutype, &status);

    if (status) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                 fits_get_hdu_type, "filename='%s', position=%d",
                                 filename, xtnum);
        status = 0;
        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }

    if (hdutype != ASCII_TBL && hdutype != BINARY_TBL) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }


    /*
     * CPL table initialisation
     */

    fits_get_num_cols(fptr, &ncol, &status);

    {
        /*FIXME: debug purpose only.
         * Compatibility with cpl_size = int.
         */

        LONGLONG longnrows;
        fits_get_num_rowsll(fptr, &longnrows, &status);
        nrows = (cpl_size)longnrows;
    }

    if (status) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                 fits_get_num_rowsll, "filename='%s', position=%d",
                                 filename, xtnum);
        status = 0;
        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }

    if (ncol > 9999) {
        cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                               "Tables with more than 9999 columns "
                               "are not supported.");

        cpl_io_fits_close_file(fptr, &status);
        return NULL;
    }

    if (ncol < 1 || nrows < 0) {
        cpl_io_fits_close_file(fptr, &status);
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return NULL;
    }

    if (firstrow > nrows) {
        cpl_io_fits_close_file(fptr, &status);
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return NULL;
    }

    /*
     * nrows is the actual number of rows in table, while nrow is the
     * requested number of rows to extract.
     */

    if (nrows == 0)
        check_nulls = 0;

    if (firstrow < 0) {
        firstrow = 0;
        if (nrow < 0) {
            nrow = nrows;
        }
    }

    if (nrow > nrows - firstrow)
        nrow = nrows - firstrow;

    table = cpl_table_new(nrow);

    /*
     * In case they were given, get the names of the columns to extract.
     */

    if (selcol) {
        extcol = cpl_array_get_data_string_const(selcol);
        if (extcol == NULL) {
            cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            cpl_io_fits_close_file(fptr, &status);
            cpl_table_delete(table);
            return NULL;
        }

        xcol = cpl_array_get_size(selcol);

        /*
         * Check if all specified columns exist in table
         */

        for (i = 0; i < xcol; i++) {
            if (extcol[i] == NULL) {
                cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                cpl_io_fits_close_file(fptr, &status);
                cpl_table_delete(table);
                return NULL;
            }
            cpl_fits_get_colnum(fptr, CASEINSEN, extcol[i], &colnum,
                                &status);
            if (status) {
                cpl_table_delete(table);
                cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
                status = 0;
                cpl_io_fits_close_file(fptr, &status);
                return NULL;
            }
        }
    }

    /*
     * Columns initialisation - k counts the really created columns.
     */

    for (i = 0, k = 0; i < ncol; i++) {

        cpl_size  repeat;
        cpl_size  width;
        int       anynul;
        char     *nullarray;
        char      scolnum[5];     /* Keeping column numbers up to 9999 */


        snprintf(scolnum, 5, "%d", i + 1);
        fits_get_colname(fptr, CASEINSEN, scolnum, colname, &colnum, &status);

        if (status) {
            (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                     fits_get_colname, "filename='%s', position=%d",
                                     filename, xtnum);
            status = 0;
            cpl_io_fits_close_file(fptr, &status);
            return NULL;
        }

        if (colnum != i + 1) {

            /*
             * This has the sense of an assertion
             */

            cpl_io_fits_close_file(fptr, &status);
            cpl_error_set_message_(CPL_ERROR_UNSPECIFIED,
                                   "Unexpected column number "
                                   "(%d instead of %d)", colnum, i + 1);
            return NULL;
        }

        if (extcol) {

            /*
             * Check if this column should be extracted
             */

            extract = 0;
            for (j = 0; j < xcol; j++) {
                if (strcmp(extcol[j], colname) == 0) {
                    extract = 1;
                    break;
                }
            }

            if (extract == 0)
                continue;
        }

        {
            /*FIXME: debug purpose only.
             * Compatibility with cpl_size = int.
             */

            LONGLONG longrepeat, longwidth;
            fits_get_eqcoltypell(fptr, i + 1, &typecode, 
                                 &longrepeat, &longwidth, &status);
            repeat = (cpl_size)longrepeat;
            width = (cpl_size)longwidth;
        }

        if (status) {
            (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                     fits_get_eqcoltypell, "filename='%s', position=%d",
                                     filename, xtnum);
            status = 0;
            cpl_io_fits_close_file(fptr, &status);
            return NULL;
        }

        /*
         * Here is another place where we like to think that
         * cpl_size = LONGLONG
         */

        depth = repeat;

        if (depth < 1) {
            cpl_msg_debug(cpl_func, "Found dummy column (%d)\n", i);
            continue;
        }

        switch (typecode) {
        case TSTRING:
            if (hdutype == ASCII_TBL) {

                /*
                 * No arrays of strings in ASCII tables (i.e., repeat = 1
                 * always, it's the width that rules).
                 */

                 repeat = width;
            }

            field_size = width + 1;
            depth = repeat / width;

            if (depth > 1) {
                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_STRING, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));

                for (j = 0; j < nrow; j++) {

                    char **scolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_STRING);
                    scolumn = cpl_array_get_data_string(array[j]);

                    for (z = 0; z < depth; z++)
                        scolumn[z] = cpl_calloc(field_size, sizeof(char));

                    if (check_nulls) {
                        fits_read_colnull(fptr, TSTRING, i + 1, 
                                          firstrow + j + 1, 1, depth, scolumn, 
                                          nullarray, &anynul, &status);
                        nullcount = 0;
                        if (anynul) {
                            for (z = 0; z < depth; z++) {
                                if (nullarray[z]) {
                                    nullcount++;
                                    cpl_free(scolumn[z]);
                                    scolumn[z] = NULL;
                                }
                            }
                        }
                        if (nullcount == depth) {
                            cpl_free(array[j]);
                            array[j] = NULL;
                        }
                    }
                    else {
                        fits_read_col(fptr, TSTRING, i + 1, firstrow + j + 1, 
                                      1, depth, 0, scolumn, &anynul, &status);
                    }
                }
                cpl_free(nullarray);
            }
            else {

                char **scolumn = NULL;

                cpl_table_new_column(table, colname, CPL_TYPE_STRING);
                scolumn = cpl_table_get_data_string(table, colname);
                for (j = 0; j < nrow; j++)
                    scolumn[j] = cpl_calloc(field_size, sizeof(char));

                if (check_nulls) {
                    nullarray = cpl_calloc(nrow, sizeof(char));
                    fits_read_colnull(fptr, TSTRING, i + 1, firstrow + 1, 1, 
                                      nrow, scolumn, nullarray, &anynul, 
                                      &status);
                    if (anynul) {
                        for (j = 0; j < nrow; j++) {
                            if (nullarray[j]) {
                                cpl_free(scolumn[j]);
                                scolumn[j] = NULL;
                            }
                        }
                    }
                    cpl_free(nullarray);
                }
                else {
                    fits_read_col(fptr, TSTRING, i + 1, firstrow + 1, 1, 
                                  nrow, 0, scolumn, &anynul, &status);
                }    
            }

            break;

        case TLOGICAL:
            if (depth > 1) {
                barray = cpl_malloc(depth * sizeof(char));
                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_INT, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));

                for (j = 0; j < nrow; j++) {

                    int *icolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_INT);
                    acolumn = cpl_array_get_column(array[j]);

                    icolumn = cpl_column_get_data_int(acolumn);
                    if (check_nulls) {
                        fits_read_colnull(fptr, TLOGICAL, i + 1, 
                                          firstrow + j + 1,
                                          1, depth, barray, nullarray,
                                          &anynul, &status);
                        nullcount = 0;
                        if (anynul) {
                            for (z = 0; z < depth; z++)
                                if (nullarray[z])
                                    nullcount++;

                            if (nullcount == depth) {
                                cpl_array_delete(array[j]);
                                array[j] = NULL;
                            }
                            else {
                                nulldata =
                                cpl_malloc(depth * sizeof(cpl_column_flag));
                                for (z = 0; z < depth; z++)
                                    nulldata[z] = nullarray[z];
                                cpl_column_set_data_invalid(acolumn, nulldata,
                                                            nullcount);
                            }
                        }
                        else {
                            cpl_column_set_data_invalid(acolumn, NULL, 0);
                        }
                    }
                    else {
                        fits_read_col(fptr, TLOGICAL, i + 1, firstrow + j + 1, 
                                      1, depth, 0, barray, &anynul, &status);
                        cpl_column_set_data_invalid(acolumn, NULL, 0);
                    }
                    for (z = 0; z < depth; z++)
                        icolumn[z] = barray[z];
                }
                cpl_free(barray);
                cpl_free(nullarray);
            }
            else {

                int *icolumn = NULL;

                barray = cpl_malloc(nrow * sizeof(char));
                cpl_table_new_column(table, colname, CPL_TYPE_INT);
                icolumn = cpl_table_get_data_int(table, colname);
                if (check_nulls) {
                    nullarray = cpl_malloc(nrow * sizeof(char));
                    fits_read_colnull(fptr, TLOGICAL, i + 1, firstrow + 1, 1,
                                      nrow, barray, nullarray, &anynul,
                                      &status);
                    nullcount = 0;
                    nulldata = NULL;
                    if (anynul) {
                        for (j = 0; j < nrow; j++)
                            if (nullarray[j])
                                nullcount++;

                        if (nullcount < nrow) {
                            nulldata = cpl_malloc(nrow *
                                                  sizeof(cpl_column_flag));
                            for (j = 0; j < nrow; j++)
                                nulldata[j] = nullarray[j];
                        }
                    }
                    cpl_free(nullarray);
                    cpl_column_set_data_invalid(table->columns[k],
                                                nulldata, nullcount);
                }
                else {
                    fits_read_col(fptr, TLOGICAL, i + 1, firstrow + 1, 1,
                                  nrow, 0, barray, &anynul, &status);
                    cpl_column_set_data_invalid(table->columns[k], NULL, 0);
                }
                for (j = 0; j < nrow; j++)
                    icolumn[j] = barray[j];
                cpl_free(barray);
            }
            break;
        case TBYTE:
        case TSBYTE:
        case TSHORT:
        case TUSHORT:
        case TINT:
        case TLONG:
            if (depth > 1) {
                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_INT, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));

                for (j = 0; j < nrow; j++) {

                    int *icolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_INT);
                    acolumn = cpl_array_get_column(array[j]);

                    icolumn = cpl_column_get_data_int(acolumn);
                    if (check_nulls) {
                        fits_read_colnull(fptr, TINT, i + 1, firstrow + j + 1, 
                                          1, depth, icolumn, nullarray, 
                                          &anynul, &status);
                        nullcount = 0;
                        if (anynul) {
                            for (z = 0; z < depth; z++)
                                if (nullarray[z])
                                    nullcount++;
    
                            if (nullcount == depth) {
                                cpl_array_delete(array[j]);
                                array[j] = NULL;
                            }
                            else {
                                nulldata = 
                                cpl_malloc(depth * sizeof(cpl_column_flag));
                                for (z = 0; z < depth; z++)
                                    nulldata[z] = nullarray[z];
                                cpl_column_set_data_invalid(acolumn, nulldata, 
                                                            nullcount);
                            }
                        }
                        else {
                            cpl_column_set_data_invalid(acolumn, NULL, 0);
                        }
                    }
                    else {
                        fits_read_col(fptr, TINT, i + 1, firstrow + j + 1, 1, 
                                      depth, 0, icolumn, &anynul, &status);
                        cpl_column_set_data_invalid(acolumn, NULL, 0);
                    }
                }
                cpl_free(nullarray);
            }
            else {

                int *icolumn = NULL;

                cpl_table_new_column(table, colname, CPL_TYPE_INT);
                icolumn = cpl_table_get_data_int(table, colname);
                if (check_nulls) {
                    nullarray = cpl_malloc(nrow * sizeof(char));
                    fits_read_colnull(fptr, TINT, i + 1, firstrow + 1, 1, 
                                      nrow, icolumn, nullarray, &anynul, 
                                      &status);
                    nullcount = 0;
                    nulldata = NULL;
                    if (anynul) {
                        for (j = 0; j < nrow; j++)
                            if (nullarray[j])
                                nullcount++;
    
                        if (nullcount < nrow) {
                            nulldata = cpl_malloc(nrow * 
                                                  sizeof(cpl_column_flag));
                            for (j = 0; j < nrow; j++)
                                nulldata[j] = nullarray[j];
                        }
                    }
                    cpl_free(nullarray);
                    cpl_column_set_data_invalid(table->columns[k], 
                                                nulldata, nullcount);
                }
                else {
                    fits_read_col(fptr, TINT, i + 1, firstrow + 1, 1, 
                                  nrow, 0, icolumn, &anynul, &status);
                    cpl_column_set_data_invalid(table->columns[k], NULL, 0);
                }
            }
            break;
        case TLONGLONG:
            if (depth > 1) {
                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_LONG_LONG, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));

                for (j = 0; j < nrow; j++) {

                    long long *llcolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_LONG_LONG);
                    acolumn = cpl_array_get_column(array[j]);

                    llcolumn = cpl_column_get_data_long_long(acolumn);
                    if (check_nulls) {
                        fits_read_colnull(fptr, TLONGLONG, i + 1,
                                          firstrow + j + 1, 1, depth,
                                          llcolumn, nullarray,
                                          &anynul, &status);
                        nullcount = 0;
                        if (anynul) {
                            for (z = 0; z < depth; z++)
                                if (nullarray[z])
                                    nullcount++;

                            if (nullcount == depth) {
                                cpl_array_delete(array[j]);
                                array[j] = NULL;
                            }
                            else {
                                nulldata =
                                cpl_malloc(depth * sizeof(cpl_column_flag));
                                for (z = 0; z < depth; z++)
                                    nulldata[z] = nullarray[z];
                                cpl_column_set_data_invalid(acolumn, nulldata,
                                                            nullcount);
                            }
                        }
                        else {
                            cpl_column_set_data_invalid(acolumn, NULL, 0);
                        }
                    }
                    else {
                        fits_read_col(fptr, TLONGLONG, i + 1, firstrow + j + 1,
                                      1, depth, 0, llcolumn, &anynul, &status);
                        cpl_column_set_data_invalid(acolumn, NULL, 0);
                    }
                }
                cpl_free(nullarray);
            }
            else {

                long long *llcolumn = NULL;

                cpl_table_new_column(table, colname, CPL_TYPE_LONG_LONG);
                llcolumn = cpl_table_get_data_long_long(table, colname);
                if (check_nulls) {
                    nullarray = cpl_malloc(nrow * sizeof(char));
                    fits_read_colnull(fptr, TLONGLONG, i + 1, firstrow + 1, 1,
                                      nrow, llcolumn, nullarray, &anynul,
                                      &status);
                    nullcount = 0;
                    nulldata = NULL;
                    if (anynul) {
                        for (j = 0; j < nrow; j++)
                            if (nullarray[j])
                                nullcount++;

                        if (nullcount < nrow) {
                            nulldata = cpl_malloc(nrow *
                                                  sizeof(cpl_column_flag));
                            for (j = 0; j < nrow; j++)
                                nulldata[j] = nullarray[j];
                        }
                    }
                    cpl_free(nullarray);
                    cpl_column_set_data_invalid(table->columns[k],
                                                nulldata, nullcount);
                }
                else {
                    fits_read_col(fptr, TLONGLONG, i + 1, firstrow + 1, 1,
                                  nrow, 0, llcolumn, &anynul, &status);
                    cpl_column_set_data_invalid(table->columns[k], NULL, 0);
                }
            }
            break;
        case TFLOAT:
            if (depth > 1) {
                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_FLOAT, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));

                for (j = 0; j < nrow; j++) {

                    float *fcolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_FLOAT);
                    acolumn = cpl_array_get_column(array[j]);

                    fcolumn = cpl_column_get_data_float(acolumn);
                    if (check_nulls) {
                        fits_read_colnull(fptr, TFLOAT, i + 1, 
                                          firstrow + j + 1, 1, depth, fcolumn, 
                                          nullarray, &anynul, &status);
                        nullcount = 0;
                        if (anynul) {
                            for (z = 0; z < depth; z++)
                                if (nullarray[z])
                                    nullcount++;

                            if (nullcount == depth) {
                                cpl_array_delete(array[j]);
                                array[j] = NULL;
                            }
                            else {
                                nulldata = 
                                cpl_malloc(depth * sizeof(cpl_column_flag));
                                for (z = 0; z < depth; z++)
                                    nulldata[z] = nullarray[z];
                                cpl_column_set_data_invalid(acolumn, nulldata,
                                                            nullcount);
                            }
                        }
                        else {
                            cpl_column_set_data_invalid(acolumn, NULL, 0);
                        }
                    }
                    else {
                        fits_read_col(fptr, TFLOAT, i + 1, firstrow + j + 1, 
                                      1, depth, 0, fcolumn, &anynul, &status);
                        cpl_column_set_data_invalid(acolumn, NULL, 0);
                    }
                }
                cpl_free(nullarray);
            }
            else {

                float *fcolumn = NULL;

                cpl_table_new_column(table, colname, CPL_TYPE_FLOAT);
                fcolumn = cpl_table_get_data_float(table, colname);
                if (check_nulls) {
                    nullarray = cpl_malloc(nrow * sizeof(char));
                    fits_read_colnull(fptr, TFLOAT, i + 1, firstrow + 1, 1, 
                                      nrow, fcolumn, nullarray, &anynul, 
                                      &status);
                    nullcount = 0;
                    nulldata = NULL;
                    if (anynul) {
                        for (j = 0; j < nrow; j++)
                            if (nullarray[j])
                                nullcount++;
    
                        if (nullcount < nrow) {
                            nulldata = cpl_malloc(nrow *
                                                  sizeof(cpl_column_flag));
                            for (j = 0; j < nrow; j++)
                                nulldata[j] = nullarray[j];
                        }
                    }
                    cpl_free(nullarray);
                    cpl_column_set_data_invalid(table->columns[k],
                                                nulldata, nullcount);
                }
                else {
                    fits_read_col(fptr, TFLOAT, i + 1, firstrow + 1, 1, 
                                  nrow, 0, fcolumn, &anynul, &status);
                    cpl_column_set_data_invalid(table->columns[k], NULL, 0);
                }
            }
            break;
        case TDOUBLE: 
            if (depth > 1) {
                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_DOUBLE, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));

                for (j = 0; j < nrow; j++) {

                    double *dcolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_DOUBLE);
                    acolumn = cpl_array_get_column(array[j]);

                    dcolumn = cpl_column_get_data_double(acolumn);
                    if (check_nulls) {
                        fits_read_colnull(fptr, TDOUBLE, i + 1, 
                                          firstrow + j + 1, 1, depth, dcolumn, 
                                          nullarray, &anynul, &status);
                        nullcount = 0;
                        if (anynul) { 
                            for (z = 0; z < depth; z++)
                                if (nullarray[z])
                                    nullcount++;

                            if (nullcount == depth) {
                                cpl_array_delete(array[j]);
                                array[j] = NULL;
                            }
                            else {
                                nulldata = 
                                cpl_malloc(depth * sizeof(cpl_column_flag));
                                for (z = 0; z < depth; z++)
                                    nulldata[z] = nullarray[z];
                                cpl_column_set_data_invalid(acolumn, nulldata,
                                                            nullcount);
                            }
                        }
                        else {
                            cpl_column_set_data_invalid(acolumn, NULL, 0);
                        }
                    }
                    else {
                        fits_read_col(fptr, TDOUBLE, i + 1, firstrow + j + 1, 
                                      1, depth, 0, dcolumn, &anynul, &status);
                        cpl_column_set_data_invalid(acolumn, NULL, 0);
                    }
                }
                cpl_free(nullarray);
            }
            else {

                double *dcolumn = NULL;

                cpl_table_new_column(table, colname, CPL_TYPE_DOUBLE);
                dcolumn = cpl_table_get_data_double(table, colname);
                if (check_nulls) {
                    nullarray = cpl_malloc(nrow * sizeof(char));
                    fits_read_colnull(fptr, TDOUBLE, i + 1, firstrow + 1, 
                                      1, nrow, dcolumn, nullarray, &anynul, 
                                      &status);
                    nullcount = 0;
                    nulldata = NULL;
                    if (anynul) {
                        for (j = 0; j < nrow; j++)
                            if (nullarray[j])
                                nullcount++;
    
                        if (nullcount < nrow) {
                            nulldata = cpl_malloc(nrow *
                                                  sizeof(cpl_column_flag));
                            for (j = 0; j < nrow; j++)
                                nulldata[j] = nullarray[j];
                        }
                    }
                    cpl_free(nullarray);
                    cpl_column_set_data_invalid(table->columns[k],
                                                nulldata, nullcount);
                }
                else {
                    fits_read_col(fptr, TDOUBLE, i + 1, firstrow + 1, 1, 
                                  nrow, 0, dcolumn, &anynul, &status);
                    cpl_column_set_data_invalid(table->columns[k], NULL, 0);
                }
            }
            break;
        case TCOMPLEX:
            if (depth > 1) {

                float *fcolumn = NULL;

                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_FLOAT_COMPLEX, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));
                fcolumn = cpl_malloc(2 * depth * sizeof(float));

                for (j = 0; j < nrow; j++) {

                    float complex *cfcolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_FLOAT_COMPLEX);
                    acolumn = cpl_array_get_column(array[j]);

                    cfcolumn = cpl_column_get_data_float_complex(acolumn);
                    if (check_nulls) {
                        fits_read_colnull(fptr, TCOMPLEX, i + 1, 
                                          firstrow + j + 1, 1, depth, fcolumn, 
                                          nullarray, &anynul, &status);

                        for (zz = z = 0; z < depth; z++, zz += 2)
                            cfcolumn[z] = fcolumn[zz] + I*fcolumn[zz+1];

                        nullcount = 0;
                        if (anynul) {
                            for (z = 0; z < depth; z++)
                                if (nullarray[z])
                                    nullcount++;

                            if (nullcount == depth) {
                                cpl_array_delete(array[j]);
                                array[j] = NULL;
                            }
                            else {
                                nulldata = 
                                cpl_malloc(depth * sizeof(cpl_column_flag));
                                for (z = 0; z < depth; z++)
                                    nulldata[z] = nullarray[z];
                                cpl_column_set_data_invalid(acolumn, nulldata,
                                                            nullcount);
                            }
                        }
                        else {
                            cpl_column_set_data_invalid(acolumn, NULL, 0);
                        }
                    }
                    else {
                        fits_read_col(fptr, TCOMPLEX, i + 1, firstrow + j + 1, 
                                      1, depth, 0, fcolumn, &anynul, &status);

                        for (zz = z = 0; z < depth; z++, zz += 2)
                            cfcolumn[z] = fcolumn[zz] + I*fcolumn[zz+1];

                        cpl_column_set_data_invalid(acolumn, NULL, 0);
                    }
                }
                cpl_free(fcolumn);
                cpl_free(nullarray);
            }
            else {

                float         *fcolumn  = NULL;
                float complex *cfcolumn = NULL;

                cpl_table_new_column(table, colname, CPL_TYPE_FLOAT_COMPLEX);
                cfcolumn = cpl_table_get_data_float_complex(table, colname);
                fcolumn = cpl_malloc(2 * nrow * sizeof(float));
                if (check_nulls) {
                    nullarray = cpl_malloc(nrow * sizeof(char));
                    fits_read_colnull(fptr, TCOMPLEX, i + 1, firstrow + 1, 1, 
                                      nrow, fcolumn, nullarray, &anynul, 
                                      &status);

                    nullcount = 0;
                    nulldata = NULL;
                    if (anynul) {
                        for (j = 0; j < nrow; j++)
                            if (nullarray[j])
                                nullcount++;
    
                        if (nullcount < nrow) {
                            nulldata = cpl_malloc(nrow *
                                                  sizeof(cpl_column_flag));
                            for (j = 0; j < nrow; j++)
                                nulldata[j] = nullarray[j];
                        }
                    }
                    cpl_free(nullarray);
                    cpl_column_set_data_invalid(table->columns[k],
                                                nulldata, nullcount);
                }
                else {
                    fits_read_col(fptr, TCOMPLEX, i + 1, firstrow + 1, 1, 
                                  nrow, 0, fcolumn, &anynul, &status);
                    cpl_column_set_data_invalid(table->columns[k], NULL, 0);
                }

                for (z = zz = 0; z < nrow; z++, zz += 2)
                    cfcolumn[z] = fcolumn[zz] + I*fcolumn[zz+1];

                cpl_free(fcolumn);
            }
            break;
        case TDBLCOMPLEX:
            if (depth > 1) {

                double *dcolumn = NULL;

                cpl_table_new_column_array(table, colname,
                                           CPL_TYPE_DOUBLE_COMPLEX, depth);
                array = cpl_table_get_data_array(table, colname);
                nullarray = cpl_malloc(depth * sizeof(char));
                dcolumn = cpl_malloc(2 * depth * sizeof(double));

                for (j = 0; j < nrow; j++) {

                    double complex *cdcolumn = NULL;

                    array[j] = cpl_array_new(depth, CPL_TYPE_DOUBLE_COMPLEX);
                    acolumn = cpl_array_get_column(array[j]);

                    cdcolumn = cpl_column_get_data_double_complex(acolumn);
                    if (check_nulls) {
                        fits_read_colnull(fptr, TDBLCOMPLEX, i + 1, 
                                          firstrow + j + 1, 1, depth, dcolumn, 
                                          nullarray, &anynul, &status);

                        for (zz = z = 0; z < depth; z++, zz += 2)
                            cdcolumn[z] = dcolumn[zz] + I*dcolumn[zz+1];

                        nullcount = 0;
                        if (anynul) { 
                            for (z = 0; z < depth; z++)
                                if (nullarray[z])
                                    nullcount++;

                            if (nullcount == depth) {
                                cpl_array_delete(array[j]);
                                array[j] = NULL;
                            }
                            else {
                                nulldata = 
                                cpl_malloc(depth * sizeof(cpl_column_flag));
                                for (z = 0; z < depth; z++)
                                    nulldata[z] = nullarray[z];
                                cpl_column_set_data_invalid(acolumn, nulldata,
                                                            nullcount);
                            }
                        }
                        else {
                            cpl_column_set_data_invalid(acolumn, NULL, 0);
                        }
                    }
                    else {
                        fits_read_col(fptr, TDBLCOMPLEX, i + 1, 
                                      firstrow + j + 1, 1, depth, 0, 
                                      dcolumn, &anynul, &status);

                        for (zz = z = 0; z < depth; z++, zz += 2)
                            cdcolumn[z] = dcolumn[zz] + I*dcolumn[zz+1];

                        cpl_column_set_data_invalid(acolumn, NULL, 0);
                    }
                }
                cpl_free(dcolumn);
                cpl_free(nullarray);
            }
            else {

                double         *dcolumn  = NULL;
                double complex *cdcolumn = NULL;

                cpl_table_new_column(table, colname, CPL_TYPE_DOUBLE_COMPLEX);
                cdcolumn = cpl_table_get_data_double_complex(table, colname);
                dcolumn = cpl_malloc(2 * nrow * sizeof(double));

                if (check_nulls) {
                    nullarray = cpl_malloc(nrow * sizeof(char));
                    fits_read_colnull(fptr, TDBLCOMPLEX, i + 1, firstrow + 1, 
                                      1, nrow, dcolumn, nullarray, &anynul, 
                                      &status);
                    nullcount = 0;
                    nulldata = NULL;
                    if (anynul) {
                        for (j = 0; j < nrow; j++)
                            if (nullarray[j])
                                nullcount++;
    
                        if (nullcount < nrow) {
                            nulldata = cpl_malloc(nrow *
                                                  sizeof(cpl_column_flag));
                            for (j = 0; j < nrow; j++)
                                nulldata[j] = nullarray[j];
                        }
                    }
                    cpl_free(nullarray);
                    cpl_column_set_data_invalid(table->columns[k],
                                                nulldata, nullcount);
                }
                else {
                    fits_read_col(fptr, TDBLCOMPLEX, i + 1, firstrow + 1, 1, 
                                  nrow, 0, dcolumn, &anynul, &status);
                    cpl_column_set_data_invalid(table->columns[k], NULL, 0);
                }

                for (z = zz = 0; z < nrow; z++, zz += 2)
                    cdcolumn[z] = dcolumn[zz] + I*dcolumn[zz+1];

                cpl_free(dcolumn);
            }
            break;

        default:
            cpl_msg_debug(cpl_func, "Found unsupported type (%s)\n", colname);
/*
            cx_print("cpl_table_load(): found unsupported type (%s)\n",
                     colname);
*/
            continue;
        }

        if (status) {
            (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                     fits_read_col, "filename='%s', position=%d",
                                     filename, xtnum);
            status = 0;
            cpl_io_fits_close_file(fptr, &status);
            return NULL;
        }


        /*
         *  Get the column unit (if present).
         */

        colunit[0] = '\0';
        cx_snprintf(keyname, FLEN_KEYWORD, "%s%d", "TUNIT", colnum);
        if (fits_read_key(fptr, TSTRING, keyname, colunit, comment, &status)) {
            status = 0;
        }
        else {
            cpl_column_set_unit(table->columns[k], colunit);
        }

        if (depth > 1 && typecode != TSTRING) {

            /*
             * Read TDIM keyword. The first call is just to get the
             * number of dimensions.
             */

            fits_read_tdimll(fptr, i + 1, 0, &naxis, NULL, &status);
            naxes = cpl_malloc(naxis * sizeof(cpl_size));

            {
                /*FIXME: debug purpose only.
                 * Compatibility with cpl_size = int.
                 */

                LONGLONG *longnaxes = cpl_malloc(naxis * sizeof(LONGLONG));
                int       t;

                fits_read_tdimll(fptr, i + 1, naxis, &naxis, longnaxes, 
                                 &status);

                for (t = 0; t < naxis; t++) {
                    naxes[t] = (cpl_size)longnaxes[t];
                }

                cpl_free(longnaxes);

            }

            if (status) {
                (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                         fits_read_tdimll, "filename='%s', position=%d",
                                         filename, xtnum);
                status = 0;
                cpl_io_fits_close_file(fptr, &status);
                return NULL;
            }

            /*FIXME:
             * This is one of those places where arrays of type
             * CPL_TYPE_INT64 are wished to be available...
             */

            adim = cpl_array_new(naxis, CPL_TYPE_INT);
            for (j = 0; j < naxis; j++) 
                cpl_array_set_int(adim, j, (int) naxes[j]);
            cpl_free(naxes);
            cpl_column_set_dimensions(table->columns[k], adim);
            cpl_array_delete(adim);
        }

        k++;    /* A valid column was successfully processed */

    }

    cpl_io_fits_close_file(fptr, &status);

    if (status) {
        cpl_error_set_message_(CPL_ERROR_BAD_FILE_FORMAT, "CFITSIO: %s", 
                               err_text);
        return NULL;
    }

    return table;
}


/**
 * @brief
 *   Load a FITS table extension into a new @em cpl_table.
 *
 * @param filename    Name of FITS file with at least one table extension.
 * @param xtnum       Number of extension to read, starting from 1.
 * @param check_nulls If set to 0, identified invalid values are not marked.
 *
 * @return New table, or @c NULL in case of failure.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>filename</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_FOUND</td>
 *       <td class="ecr">
 *         A file named as specified in <i>filename</i> is not found.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_BAD_FILE_FORMAT</td>
 *       <td class="ecr">
 *         The input file is not in FITS format.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified FITS file extension is not a table, or, if it
 *         is a table, it has more than 9999 columns.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         <i>xtnum</i> is greater than the number of FITS extensions
 *         in the FITS file, or is less than 1.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The FITS table has no rows or no columns.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         Generic error condition, that should be reported to the
 *         CPL Team.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The selected FITS file table extension is just read and converted into
 * the @em cpl_table conventions.
 */

cpl_table *cpl_table_load(const char *filename, int xtnum, int check_nulls)
{

    cpl_table *table = cpl_table_overload_window(filename, xtnum, check_nulls, 
                                                 NULL, -1, -1);

    return table;
}


/**
 * @brief
 *   Load part of a FITS table extension into a new @em cpl_table.
 *
 * @param filename    Name of FITS file with at least one table extension.
 * @param xtnum       Number of extension to read, starting from 1.
 * @param check_nulls If set to 0, identified invalid values are not marked.
 * @param selcol      Array with the names of the columns to extract.
 * @param firstrow    First table row to extract.
 * @param nrow        Number of rows to extract.
 *
 * @return New table, or @c NULL in case of failure.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>filename</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_FOUND</td>
 *       <td class="ecr">
 *         A file named as specified in <i>filename</i> is not found.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_BAD_FILE_FORMAT</td>
 *       <td class="ecr">
 *         The input file is not in FITS format.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified FITS file extension is not a table.
 *         Or the specified number of rows to extract is less than zero.
 *         Or the array of column names to extract contains empty fields.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         <i>xtnum</i> is greater than the number of FITS extensions
 *         in the FITS file, or is less than 1.
 *         Or <i>firstrow</i> is either less than zero, or greater
 *         than the number of rows in the table.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The FITS table has no columns.
 *         Or <i>selcol</i> includes columns that are not found in table.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         Generic error condition, that should be reported to the
 *         CPL Team.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The selected FITS file table extension is just read in the specified
 * columns and rows intervals, and converted into the @em cpl_table 
 * conventions. If @em selcol is NULL, all columns are selected.
 */

cpl_table *cpl_table_load_window(const char *filename, int xtnum, 
                                 int check_nulls, const cpl_array *selcol,
                                 cpl_size firstrow, cpl_size nrow)
{


    cpl_table *table;


    if (firstrow < 0) {
        cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return NULL;
    }

    if (nrow < 0) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    table = cpl_table_overload_window(filename, xtnum, check_nulls,
                                      selcol, firstrow, nrow);

    return table;

}


enum {
    CPL_NULL_FLAG_NONE   = 0,
    CPL_NULL_FLAG_IVALUE = 1 << 0,
    CPL_NULL_FLAG_FVALUE = 1 << 1,
    CPL_NULL_FLAG_ARRAY  = 1 << 2
};


typedef struct cpl_table_layout cpl_table_layout;

struct cpl_table_layout
{
        int       columns;
        LONGLONG  rows;
        LONGLONG *depth;

        cpl_type *value_type;
        cpl_type *saved_type;
        int      *native_type;

        int       *naxes;
        LONGLONG **naxis;

        char **ttype;
        char **tform;
        char **tunit;

        struct
        {
                unsigned int           flag;
                LONGLONG               value;
                const cpl_column_flag *mask;
        } *tnull;

};


inline static void
_cpl_table_layout_clear(cpl_table_layout *self)
{

    register int icol;


    cpl_free(self->tnull);

    for (icol = 0; icol < self->columns; ++icol) {

        cpl_free(self->tform[icol]);
        cpl_free(self->tunit[icol]);
        cpl_free(self->ttype[icol]);
        cpl_free(self->naxis[icol]);

    }

    cpl_free(self->tform);
    cpl_free(self->tunit);
    cpl_free(self->ttype);

    cpl_free(self->naxis);
    cpl_free(self->naxes);

    cpl_free(self->native_type);
    cpl_free(self->saved_type);
    cpl_free(self->value_type);
    cpl_free(self->depth);

    return;

}


inline static int
_cpl_table_layout_init(cpl_table_layout *self, int ncolumns)
{

    self->columns = ncolumns;
    self->rows    = 0;
    self->depth   = cpl_calloc(self->columns, sizeof *self->depth);

    self->value_type  = cpl_calloc(self->columns, sizeof *self->value_type);
    self->saved_type  = cpl_calloc(self->columns, sizeof *self->saved_type);
    self->native_type = cpl_calloc(self->columns, sizeof *self->native_type);

    self->naxes = cpl_calloc(self->columns, sizeof *self->naxes);
    self->naxis = cpl_calloc(self->columns, sizeof *self->naxis);

    self->ttype = cpl_calloc(self->columns, sizeof *self->ttype);
    self->tunit = cpl_calloc(self->columns, sizeof *self->tunit);
    self->tform = cpl_calloc(self->columns, sizeof *self->tform);

    self->tnull = cpl_calloc(self->columns, sizeof *self->tnull);

    return 0;

}


/*
 * Compare the two given table layouts. The comparison is done to the extend
 * that if the both layouts are found to be equal both would result in the
 * same FITS table structure.
 */

inline static int
_cpl_table_layout_compare(const cpl_table_layout *self,
                          const cpl_table_layout *other)
{

    cpl_size icol;


    if (self->columns != other->columns) {
        return -1;
    }

    for (icol = 0; icol < self->columns; ++icol) {

        if ((strcmp(self->tform[icol], other->tform[icol])) ||
                (strcmp(self->ttype[icol], other->ttype[icol])) ||
                (strcmp(self->tunit[icol], other->tunit[icol]))) {
            return 1;
        }

        if (self->tnull[icol].flag & CPL_NULL_FLAG_IVALUE) {

            if (!(other->tnull[icol].flag & CPL_NULL_FLAG_IVALUE)) {
                return 2;
            }

            if (self->tnull[icol].value != other->tnull[icol].value) {
                return 3;
            }

        }

        if (self->naxes[icol] != other->naxes[icol]) {
            return 4;
        }
        else if (self->naxes[icol] > 0) {

            register cpl_size iaxes;

            for (iaxes = 0; iaxes < self->naxes[icol]; ++iaxes) {

                LONGLONG *dimensions  = self->naxis[icol];
                LONGLONG *_dimensions = other->naxis[icol];

                if (dimensions[iaxes] != _dimensions[iaxes]) {
                    return 5;
                }

            }

        }

    }

    return 0;

}


inline static cpl_error_code
_cpl_table_layout_create(cpl_table_layout *self, const cpl_table *table)
{

    register int icol;


    _cpl_table_layout_init(self, cpl_table_get_ncol(table));


    self->rows = cpl_table_get_nrow(table);

    for (icol = 0; icol < self->columns; ++icol) {

        char fmtcode = '\0';

        cpl_size rcount = 1;


        const cpl_column *column = table->columns[icol];

        cpl_type column_type = cpl_column_get_type(column);

        const char *unit = cpl_column_get_unit(column);



        self->value_type[icol] = _cpl_columntype_get_valuetype(column_type);
        self->saved_type[icol] = cpl_column_get_save_type(column);

        self->ttype[icol] = cpl_strdup(cpl_column_get_name(column));
        self->tunit[icol] = cpl_strdup((unit == NULL) ? "" : unit);


        if (_cpl_columntype_is_pointer(column_type)) {

            /*
             * Determine repeat count for the FITS column format description.
             * This is simply the array length, with the exception of
             * arrays of strings. In the latter case the repeat count
             * is the array length times the number of characters of
             * the longest string in the array and the organization
             * into elements is given by the TDIM shape specifier.
             */

            rcount = cpl_column_get_depth(column);

            if (rcount < 1) {
                return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
            }

            self->depth[icol] = rcount;


            /*
             * Determine axis information for the TDIM shape specifier.
             */

            if (self->value_type[icol] == CPL_TYPE_STRING) {

                register size_t sz = 1;

                register cpl_size irow;
                register cpl_size nrow = cpl_column_get_size(column);

                unsigned int bad_rows = 0;

                const cpl_array **cdata =
                        cpl_column_get_data_array_const(column);


                for (irow = 0; irow < nrow; ++irow) {

                    if (cdata[irow] != NULL) {

                        cpl_size n = cpl_array_get_size(cdata[irow]);

                        const char **s =
                                cpl_array_get_data_string_const(cdata[irow]);

                        sz = CPL_MAX(sz, _cpl_strvlen_max(s, n));

                    }
                    else {
                        bad_rows = 1;
                    }

                }

                if (bad_rows) {
                    self->tnull[icol].flag  |= CPL_NULL_FLAG_ARRAY;
                }

                self->naxes[icol] = 2;

                self->naxis[icol] = cpl_malloc(2 * sizeof(LONGLONG));
                self->naxis[icol][0] = sz;
                self->naxis[icol][1] = self->depth[icol];

                rcount *=  sz;

            }
            else {

                int iaxes;

                self->naxes[icol] = cpl_column_get_dimensions(column);
                self->naxis[icol] =
                        cpl_malloc(self->naxes[icol] * sizeof(LONGLONG));

                for (iaxes = 0; iaxes < self->naxes[icol]; ++iaxes) {

                    cpl_size sz = cpl_column_get_dimension(column, iaxes);
                    self->naxis[icol][iaxes] = (LONGLONG)sz;

                }


                /*
                 * If the columns value type is an integer type, and invalid
                 * entries are present in the column data, determine
                 * the null value needed to fill the TNULL header keyword.
                 *
                 * For array valued columns there may be invalid rows, which
                 * is interpreted as all elements of the associated array
                 * are invalid, and there may be individual elements in an
                 * associated array which can be invalid.
                 */

                if ((self->value_type[icol] == CPL_TYPE_INT) ||
                    (self->value_type[icol] == CPL_TYPE_LONG_LONG)) {

                    register int irow;
                    unsigned int flag = 0;
                    unsigned int bad_rows = 0;

                    LONGLONG value = 0;

                    const cpl_array **arrays =
                            cpl_column_get_data_array_const(column);


                    /*
                     * Check if the row is invalid, or whether there are
                     * invalid elements in the row's array value. For invalid
                     * rows just record that they are present. For invalid
                     * array elements verify that all invalid elements across
                     * all rows in this column share the same value. If and
                     * only if this is the case record the consistent null
                     * value for this column.
                     */

                    for (irow = 0; irow < self->rows; ++irow) {

                        if (arrays[irow]) {

                            unsigned int _flag = 0;

                            LONGLONG _value = 0;

                            const cpl_column *cdata =
                                    cpl_array_get_column_const(arrays[irow]);

                            _cpl_column_find_null(cdata, self->value_type[icol],
                                                  self->rows, &_flag, &_value);

                            if (_flag) {

                                if (!flag) {
                                    flag = 1;
                                    value = _value;
                                }
                                else {

                                    if (value != _value) {
                                        flag = 0;
                                        break;
                                    }

                                }

                            }

                        }
                        else {
                            bad_rows = 1;
                        }

                    }


                    /*
                     * Set up the null value state for this column. If
                     * invalid rows were seen record it in the the flag
                     * mask.
                     */

                    if (flag) {

                        self->tnull[icol].flag  = 1;
                        self->tnull[icol].value = value;

                        if (bad_rows) {
                            self->tnull[icol].flag  |= CPL_NULL_FLAG_ARRAY;
                        }

                    }

                }
                else {

                    /*
                     * For floating point data columns, just mark if there
                     * are invalid rows.
                     */

                    if (cpl_column_has_invalid(column)) {
                        self->tnull[icol].flag = CPL_NULL_FLAG_ARRAY;
                    }

                }

            }

        }
        else {


            /*
             * Dummy values for non-array type columns
             */

            self->depth[icol] = 0;
            self->naxes[icol] = 0;
            self->naxis[icol] = NULL;


            /*
             * For columns of type string determine the length of the
             * longest string in the column, which gives the repeat count
             * for the column format specification.
             */

            if (self->value_type[icol] == CPL_TYPE_STRING) {

                const char **s = cpl_column_get_data_string_const(column);

                rcount = CPL_MAX(1, _cpl_strvlen_max(s, self->rows));

            }


            /*
             * If the columns value type is an integer type, and invalid
             * entries are present in the column data, determine
             * the null value needed to fill the TNULL header keyword.
             *
             * For floating point columns the reference to the column flag
             * buffer is stored, but evaluated later during the actual write
             * operations.
             */

            switch (self->value_type[icol]) {

                case CPL_TYPE_INT:
                case CPL_TYPE_LONG_LONG:
                {
                    _cpl_column_find_null(column, self->value_type[icol],
                                          self->rows, &self->tnull[icol].flag,
                                          &self->tnull[icol].value);
                    break;
                }

                case CPL_TYPE_FLOAT:
                case CPL_TYPE_DOUBLE:
                case CPL_TYPE_FLOAT_COMPLEX:
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    if (cpl_column_has_invalid(column)) {
                        self->tnull[icol].flag = CPL_NULL_FLAG_FVALUE;
                        self->tnull[icol].mask =
                                cpl_column_get_data_invalid_const(column);
                    }
                    break;
                }

                default:
                    break;
            }

        }


        /*
         * Translate the column value_type to the FITS native type and
         * determine the format code for the saved_type.
         */

        self->native_type[icol] =
                _cpl_columntype_get_nativetype(self->value_type[icol], NULL);

        if (self->native_type[icol] == -1) {
            return cpl_error_set_(CPL_ERROR_UNSPECIFIED);
        }


        _cpl_columntype_get_nativetype(self->saved_type[icol], &fmtcode);

        if(fmtcode == '\0') {
            return cpl_error_set_(CPL_ERROR_UNSPECIFIED);
        }

        self->tform[icol] = cpl_sprintf("%" CPL_SIZE_FORMAT "%c",
                                        rcount, fmtcode);

    }

    return CPL_ERROR_NONE;

}


/*
 * Read the table layout from the current HDU of a FITS file
 */

inline static cpl_error_code
_cpl_table_layout_read(cpl_table_layout *self, fitsfile *ifile)
{

    int ic     = 0;
    int ncol   = 0;
    int status = 0;


    fits_get_num_cols(ifile, &ncol, &status);

    if (status) {
        return cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                  status, fits_get_num_cols, ".");
    }


    _cpl_table_layout_init(self, ncol);


    for (ic = 0; ic < ncol; ++ic) {
        self->tform[ic] = cpl_calloc(FLEN_VALUE, sizeof(char));
        self->ttype[ic] = cpl_calloc(FLEN_VALUE, sizeof(char));
        self->tunit[ic] = cpl_calloc(FLEN_VALUE, sizeof(char));
    }


    fits_read_btblhdrll(ifile, -1, &self->rows, NULL, self->ttype,
                        self->tform, self->tunit, NULL, NULL, &status);

    for (ic = 0; ic < ncol; ++ic) {

        int _status = 0;

        char keyname[FLEN_KEYWORD];
        char keyvalue[FLEN_VALUE];

        LONGLONG rcount;
        LONGLONG width;


        fits_get_coltypell(ifile, ic + 1, &self->native_type[ic],
                           &rcount, &width, &_status);

        if (_status) {
            status = _status;
            break;
        }

        if (self->native_type[ic] == TSTRING) {
            rcount /= width;
        }

        if (rcount > 1) {
            self->depth[ic] = rcount;
        }
        else {
            self->depth[ic] = 0;
        }


        snprintf(keyname, FLEN_KEYWORD, "TNULL%d", ic);

        fits_read_key(ifile, TLONGLONG, keyname, &self->tnull[ic].value,
                      NULL, &_status);

        if (_status == 0) {
            self->tnull[ic].flag = CPL_NULL_FLAG_IVALUE;
        }
        else if (_status == KEY_NO_EXIST) {
            _status = 0;
        }
        else {
            status = _status;
            break;
        }

        snprintf(keyname, FLEN_KEYWORD, "TDIM%d", ic + 1);

        fits_read_key(ifile, TSTRING, keyname, keyvalue, NULL, &_status);

        if (_status == 0) {

            cpl_size ndim = 0;

            char *c = keyvalue;

            while (*c) {
                if (*c == ',') {
                    ++ndim;
                }
                ++c;
            }
            ++ndim;

            self->naxis[ic] = cpl_calloc(ndim, sizeof *self->naxis[ic]);

            fits_read_tdimll(ifile, ic + 1, ndim, &self->naxes[ic],
                             self->naxis[ic], &_status);

            if (_status) {
                status = _status;
                break;
            }

        }
        else if (_status == KEY_NO_EXIST) {

            if (rcount > 1) {
                self->naxes[ic] = 1;
                self->naxis[ic] = cpl_calloc(1, sizeof *self->naxis[ic]);
                *self->naxis[ic] = rcount;
            }
            else {
                self->naxes[ic] = 0;
                self->naxis[ic] = NULL;
            }

        }
        else {
            status = _status;
            break;
        }

    }

    if (status) {
        return cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                  status, fits_read_tdimll, ".");
    }

    return CPL_ERROR_NONE;
}


/*
 * Append a new FITS binary table extension to the output stream and
 * initialize it with the given table layout.
 */

inline static cpl_error_code
_cpl_table_layout_write(const cpl_table_layout *self, fitsfile *outfile)
{

    int status = 0;


    /*
     * Create an empty binary FITS table at the end of the output stream
     */

    fits_create_tbl(outfile, BINARY_TBL, self->rows, self->columns,
                    self->ttype, self->tform, self->tunit, NULL, &status);

    if (status) {
        const cpl_error_code code =
            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                               status, fits_create_tbl, ".");
        status = 0;
        cpl_io_fits_delete_file(outfile, &status);
        return code;
    }


    /*
     * Write additional table and/or column properties, like null value
     * and array shape specification, etc (TNULL, TDIM, ...)
     */

    for (int icol = 0; icol < self->columns; ++icol) {

        if (self->tnull[icol].flag & CPL_NULL_FLAG_IVALUE) {

            char keyname[FLEN_KEYWORD];

            snprintf(keyname, FLEN_KEYWORD, "TNULL%d", icol + 1);


            switch (self->value_type[icol]) {

                case CPL_TYPE_INT:
                {
                    int ival = self->tnull[icol].value;

                    fits_write_key(outfile, TINT, keyname, &ival, NULL, &status);
                    break;
                }

                case CPL_TYPE_LONG_LONG:
                {
                    fits_write_key(outfile, TLONGLONG, keyname,
                                   &self->tnull[icol].value, NULL, &status);
                    break;
                }

                default:

                    /*
                     * The null value flag is only set for integer type
                     * columns. This should never be reached!
                     */

                    break;

            }

            if (status) {
                const cpl_error_code code =
                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                       status, fits_write_key, ".");
                status = 0;
                cpl_io_fits_delete_file(outfile, &status);
                return code;
            }

        }


        /*
         * Write data array shape specification only if more than one
         * axis is defined. For one dimensional arrays this is redundant,
         * since it is given as the repeat count of the column format (TFORM).
         */

        if (self->naxes[icol] > 1) {

            fits_write_tdimll(outfile, icol + 1, self->naxes[icol],
                              self->naxis[icol], &status);

            if (status) {
                const cpl_error_code code =
                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                       status, fits_write_tdimll, ".");
                status = 0;
                cpl_io_fits_delete_file(outfile, &status);
                return code;
            }


        }

    }


    // FIXME: Enforce flushing the buffers to the file. This guarantees that
    //        the column definitions inside the fitsfile structure are
    //        completely valid!

    /*
     * This is a workaround for a bug in CFITSIO, which somewhere misses
     * a rescan of the table structure before the data is written. This may
     * cause problems if column attributes (TNULL) are written to the
     * header after a call to fits_write_tdimll(), which initializes the
     * column structures with incomplete information for columns which
     * are not yet fully defined.
     *
     * This may lead to false 'null not defined' errors in CFITSIO, since
     * the data writing routines apparently do not make sure that the
     * internal column structures are up to date!
     */

    fits_flush_file(outfile, &status);

    if (status) {
        const cpl_error_code code =
            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                               status, fits_flush_file, ".");
        status = 0;
        cpl_io_fits_delete_file(outfile, &status);
        return code;
    }

    return CPL_ERROR_NONE;

}


/*
 * Determine the optimal number of rows to be written to the output stream
 * in one go.
 */

inline static cpl_size
_cpl_table_save_get_chunksize(fitsfile *outfile, cpl_size nrows)
{

    int status  = 0;

    long _nrows = 0;


    fits_get_rowsize(outfile, &_nrows, &status);

    if (status || (_nrows > nrows)) {
        return nrows;
    }

    return _nrows;

}


inline static cpl_error_code
_cpl_table_write_column(fitsfile *outfile, cpl_size trow,
                        const cpl_column *column, int icol, cpl_size srow,
                        cpl_size nrows, const cpl_table_layout *layout)
{

    int status = 0;


    /*
     * Prepare data buffer suitable for CFITSIO calls. If a floating point
     * column is encountered where all entries are invalid, bypass the
     * general case and just write invalid data values to the file.
     */

    if ((layout->tnull[icol].flag & CPL_NULL_FLAG_FVALUE) &&
            !layout->tnull[icol].mask) {

        fits_write_col_null(outfile, icol + 1, trow + 1, 1, nrows, &status);

    }
    else {


        void       *sdata = NULL;
        const void *tdata = NULL;
        const void *tnull = NULL;

        const char *null_string = "";

        union {
                int ival;
                LONGLONG lval;
        } null_value;

        const cpl_column_flag *null_mask = NULL;


        switch (layout->value_type[icol]) {

            case CPL_TYPE_STRING:
            {

                register cpl_size irow;

                const char **cdata =
                        &cpl_column_get_data_string_const(column)[srow];

                const char **val = cpl_malloc(nrows * sizeof *cdata);

                sdata = val;

                for (irow = 0; irow < nrows; ++irow) {
                    val[irow] = (cdata[irow] == NULL) ? null_string : cdata[irow];
                }

                tdata = sdata;
                tnull = null_string;

                break;
            }

            case CPL_TYPE_INT:
            {
                tdata = &cpl_column_get_data_int_const(column)[srow];

                if (layout->tnull[icol].flag & CPL_NULL_FLAG_IVALUE) {
                    null_value.ival = layout->tnull[icol].value;
                    tnull = &null_value.ival;
                }

                break;
            }

            case CPL_TYPE_LONG_LONG:
            {
                tdata =
                        &cpl_column_get_data_long_long_const(column)[srow];

                if (layout->tnull[icol].flag & CPL_NULL_FLAG_IVALUE) {
                    null_value.lval = layout->tnull[icol].value;
                    tnull = &null_value.lval;
                }

                break;
            }

            case CPL_TYPE_FLOAT:
            {
                tdata = &cpl_column_get_data_float_const(column)[srow];

                if (layout->tnull[icol].flag & CPL_NULL_FLAG_FVALUE) {
                    null_mask = &layout->tnull[icol].mask[srow];
                }

                break;
            }

            case CPL_TYPE_DOUBLE:
            {
                tdata = &cpl_column_get_data_double_const(column)[srow];

                if (layout->tnull[icol].flag & CPL_NULL_FLAG_FVALUE) {
                    null_mask = &layout->tnull[icol].mask[srow];
                }

                break;
            }

            case CPL_TYPE_FLOAT_COMPLEX:
            {

                register cpl_size irow;

                const float complex *cdata =
                        &cpl_column_get_data_float_complex_const(column)[srow];

                float *val = cpl_malloc(2 * nrows * sizeof(float));

                sdata = val;

                for (irow = 0; irow < nrows; ++irow) {

                    register cpl_size i = 2 * irow;

                    val[i]     = crealf(cdata[irow]);
                    val[i + 1] = cimagf(cdata[irow]);

                }

                tdata = sdata;

                if (layout->tnull[icol].flag & CPL_NULL_FLAG_FVALUE) {
                    null_mask = &layout->tnull[icol].mask[srow];
                }

                break;
            }

            case CPL_TYPE_DOUBLE_COMPLEX:
            {

                register cpl_size irow;

                const double complex *cdata =
                        &cpl_column_get_data_double_complex_const(column)[srow];

                double *val = cpl_malloc(2 * nrows * sizeof(double));

                sdata = val;

                for (irow = 0; irow < nrows; ++irow) {

                    register cpl_size i = 2 * irow;

                    val[i]     = creal(cdata[irow]);
                    val[i + 1] = cimag(cdata[irow]);

                }

                tdata = sdata;

                if (layout->tnull[icol].flag & CPL_NULL_FLAG_FVALUE) {
                    null_mask = &layout->tnull[icol].mask[srow];
                }

                break;
            }

            default:
            {
                /* This should never be reached! */
                return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                break;
            }

        }

        cpl_fits_write_colnull(outfile, layout->native_type[icol], icol + 1,
                               trow + 1, 1, nrows, tdata, tnull, &status);

        if (sdata) {
            cpl_free(sdata);
        }


        /*
         * If a null mask has been setup, overwrite masked entries with the
         * appropriate null value. Note that this applies only to floating
         * point columns
         */

        if ((status == 0) && null_mask) {

            register cpl_size irow;
            register cpl_size first = 0;
            register cpl_size count = 0;

            for (irow = 0; irow < nrows; ++irow) {

                if (null_mask[irow]) {

                    if (count == 0) {
                        first = irow;
                    }

                    ++count;

                }
                else {

                    if (count) {
                        fits_write_col_null(outfile, icol + 1, trow + first + 1,
                                            1, count, &status);

                        if (status) {
                            break;
                        }

                    }

                    count = 0;

                }

            }
            if ((status == 0) && count) {
                fits_write_col_null(outfile, icol + 1, trow + first + 1,
                                    1, count, &status);
            }

        }

    }

    if (status) {
        return cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                  status, fits_write_col_null,
                                  "column=%s, icol=%d, srow=%d, nrows=%d",
                                  cpl_column_get_name(column),
                                  (int)icol, (int)srow, (int)nrows);
    }

    return CPL_ERROR_NONE;

}


inline static cpl_error_code
_cpl_table_write_column_array(fitsfile *outfile, cpl_size trow,
                              const cpl_column *column, int icol, cpl_size srow,
                              cpl_size nrows, const cpl_table_layout *layout)
{

    int status = 0;

    cpl_size irow;


    /*
     * Prepare data buffer suitable for CFITSIO calls.
     */

    void        *sdata = NULL;
    const void **tdata = cpl_malloc(nrows * sizeof *tdata);
    const void  *tnull = NULL;

    const cpl_array **arrays =
            &cpl_column_get_data_array_const(column)[srow];

    cpl_array *null_array = NULL;

    const char *null_string = "";

    union {
            int ival;
            LONGLONG lval;
    } null_value;

    const cpl_column_flag **null_mask = NULL;
    const cpl_column_flag   array_null = 1;    /* Only its address is used! */



    /*
     * If the column has any invalid elements, i.e. one of the array pointers
     * stored in the column elements is a null pointer, set up an array
     * container for column null values which is used as a replacement of the
     * missing data array.
     *
     * In the following switch statement it is only used if a pointer in the
     * 'arrays' array is NULL. The following check guarantees that if there
     * are any null pointers in the column 'null_array' can safely be used
     * in the following switch statement.
     */

    if (layout->tnull[icol].flag & CPL_NULL_FLAG_ARRAY) {
        null_array = cpl_array_new(layout->depth[icol],
                                   layout->value_type[icol]);
    }

    switch (layout->value_type[icol]) {

        case CPL_TYPE_STRING:
        {

            register cpl_size _irow;

            cpl_size depth = layout->depth[icol];

            const char **values = cpl_malloc(nrows * depth *
                                             sizeof(const char *));
            void *_null_array = NULL;


            sdata = values;
            tnull = null_string;

            if (null_array) {

                cpl_array_fill_window_string(null_array, 0, depth,
                                             null_string);
                _null_array = cpl_array_get_data_string(null_array);

            }


            for (_irow = 0; _irow < nrows; ++_irow) {

                register cpl_size ival;
                register cpl_size stride = _irow * depth;

                const char **cdata = NULL;


                if (arrays[_irow]) {
                    cdata = cpl_array_get_data_string_const(arrays[_irow]);
                }
                else {
                    cdata = _null_array;
                }

                if ((cdata == _null_array) ||
                        !cpl_array_has_invalid(arrays[_irow])) {
                    memcpy(&values[stride], cdata, depth * sizeof *cdata);
                }
                else {

                    for (ival = 0; ival < depth; ++ival) {
                        values[stride + ival] =
                                cdata[ival] == NULL ? null_string : cdata[ival];
                    }

                }

                tdata[_irow] = &values[stride];

            }

            break;

        }

        case CPL_TYPE_INT:
        {

            if (layout->tnull[icol].flag & CPL_NULL_FLAG_IVALUE) {
                null_value.ival = layout->tnull[icol].value;
                tnull = &null_value.ival;
            }


            if (null_array) {

                register cpl_size _irow;

                void *_null_array = cpl_array_get_data_int(null_array);


                if (tnull) {
                    cpl_array_fill_window_int(null_array, 0,
                                              layout->depth[icol],
                                              null_value.ival);
                }

                for (_irow = 0; _irow < nrows; ++_irow) {

                    if (arrays[_irow]) {
                        tdata[_irow] =
                                cpl_array_get_data_int_const(arrays[_irow]);
                    }
                    else {
                        tdata[_irow] = _null_array;
                    }

                }

            }
            else {

                register cpl_size _irow;

                for (_irow = 0; _irow < nrows; ++_irow) {
                    tdata[_irow] =
                            cpl_array_get_data_int_const(arrays[_irow]);
                }

            }
            break;

        }

        case CPL_TYPE_LONG_LONG:
        {

            if (layout->tnull[icol].flag & CPL_NULL_FLAG_IVALUE) {
                null_value.lval = layout->tnull[icol].value;
                tnull = &null_value.lval;
            }


            if (null_array) {

                register cpl_size _irow;

                void *_null_array = cpl_array_get_data_long_long(null_array);


                if (tnull) {
                    cpl_array_fill_window_long_long(null_array, 0,
                                                    layout->depth[icol],
                                                    null_value.lval);
                }

                for (_irow = 0; _irow < nrows; ++_irow) {

                    if (arrays[_irow]) {
                        tdata[_irow] =
                                cpl_array_get_data_long_long_const(arrays[_irow]);
                    }
                    else {
                        tdata[_irow] = _null_array;
                    }

                }

            }
            else {

                register cpl_size _irow;

                for (_irow = 0; _irow < nrows; ++_irow) {
                    tdata[_irow] =
                            cpl_array_get_data_long_long_const(arrays[_irow]);
                }

            }
            break;

        }

        case CPL_TYPE_FLOAT:
        {

            register cpl_size _irow;

            void *_null_array = NULL;


            null_mask = cpl_calloc(nrows, sizeof *null_mask);

            if (null_array) {
                _null_array = cpl_array_get_data_float(null_array);
            }

            for (_irow = 0; _irow < nrows; ++_irow) {

                if (arrays[_irow]) {

                    const cpl_column *cdata =
                            cpl_array_get_column_const(arrays[_irow]);

                    tdata[_irow] =
                            cpl_column_get_data_float_const(cdata);

                    null_mask[_irow] =
                            cpl_column_get_data_invalid_const(cdata);
                }
                else {
                    tdata[_irow]     = _null_array;
                    null_mask[_irow] = &array_null;

                }

            }
            break;

        }

        case CPL_TYPE_DOUBLE:
        {

            register cpl_size _irow;

            void *_null_array = NULL;


            null_mask = cpl_calloc(nrows, sizeof *null_mask);

            if (null_array) {
                _null_array = cpl_array_get_data_double(null_array);
            }

            for (_irow = 0; _irow < nrows; ++_irow) {

                if (arrays[_irow]) {

                    const cpl_column *cdata =
                            cpl_array_get_column_const(arrays[_irow]);

                    tdata[_irow] =
                            cpl_column_get_data_double_const(cdata);

                    null_mask[_irow] =
                            cpl_column_get_data_invalid_const(cdata);
                }
                else {

                    tdata[_irow]     = _null_array;
                    null_mask[_irow] = &array_null;

                }

            }
            break;

        }

        case CPL_TYPE_FLOAT_COMPLEX:
        {

            register cpl_size _irow;
            register cpl_size depth = layout->depth[icol];

            float *values = cpl_malloc(2 * nrows * depth * sizeof(float));

            void *_null_array = NULL;


            sdata = values;

            null_mask = cpl_calloc(nrows, sizeof *null_mask);

            if (null_array) {
                _null_array = cpl_array_get_data_float_complex(null_array);
            }

            for (_irow = 0; _irow < nrows; ++_irow) {

                register cpl_size ival;
                register cpl_size stride = 2 * _irow * depth;

                const float complex *cdata = NULL;

                if (arrays[_irow]) {

                    const cpl_column *_cdata =
                            cpl_array_get_column_const(arrays[_irow]);

                    cdata = cpl_column_get_data_float_complex_const(_cdata);

                    null_mask[_irow] =
                            cpl_column_get_data_invalid_const(_cdata);

                }
                else {
                    cdata            = _null_array;
                    null_mask[_irow] = &array_null;
                }


                for (ival = 0; ival < depth; ++ival) {

                    register cpl_size i = 2 * ival + stride;

                    values[i]     = crealf(cdata[ival]);
                    values[i + 1] = cimagf(cdata[ival]);

                }

                tdata[_irow] = &values[stride];

            }
            break;

        }

        case CPL_TYPE_DOUBLE_COMPLEX:
        {

            register cpl_size _irow;
            register cpl_size depth = layout->depth[icol];

            double *values = cpl_malloc(2 * nrows * depth * sizeof(double));

            void *_null_array = NULL;


            sdata = values;

            null_mask = cpl_calloc(nrows, sizeof *null_mask);

            if (null_array) {
                _null_array = cpl_array_get_data_double_complex(null_array);
            }

            for (_irow = 0; _irow < nrows; ++_irow) {

                register cpl_size ival;
                register cpl_size stride = 2 * _irow * depth;

                const double complex *cdata = NULL;


                if (arrays[_irow]) {

                    const cpl_column *_cdata =
                            cpl_array_get_column_const(arrays[_irow]);

                    cdata = cpl_column_get_data_double_complex_const(_cdata);

                    null_mask[_irow] =
                            cpl_column_get_data_invalid_const(_cdata);

                }
                else {
                    cdata            = _null_array;
                    null_mask[_irow] = &array_null;
                }


                for (ival = 0; ival < depth; ++ival) {

                    register cpl_size i = 2 * ival + stride;

                    values[i]     = creal(cdata[ival]);
                    values[i + 1] = cimag(cdata[ival]);

                }

                tdata[_irow] = &values[stride];

            }
            break;

        }

        default:
        {
            /* This should never be reached! */

            if (null_array) {
                cpl_array_delete(null_array);
            }
            cpl_free(tdata);

            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            break;
        }

    }


    for (irow = 0; irow < nrows; ++irow) {

        cpl_fits_write_colnull(outfile, layout->native_type[icol], icol + 1,
                               trow + irow + 1, 1, layout->depth[icol],
                               tdata[irow], tnull, &status);

    }


    /*
     * A null mask has been setup only for floating point columns. For
     * these columns overwrite masked entries with the appropriate null value.
     * If the null mask points to the rows table data, all elements are
     * invalid, and the whole row is filled with the null value. Otherwise,
     * if the null mask is set figure out which elements need to be rewritten,
     * and overwrite them with the appropriate null value. If no null mask
     * is set for a row, all elements are valid and nothing needs to be done.
     */

    if ((status == 0) && null_mask) {

        register cpl_size _irow;
        register cpl_size depth = layout->depth[icol];

        for (_irow = 0; _irow < nrows; ++_irow) {

            const cpl_column_flag *_null_mask = null_mask[_irow];

            if (_null_mask == &array_null) {
                fits_write_col_null(outfile, icol + 1, trow + _irow + 1, 1,
                                    depth, &status);
            }
            else if (_null_mask != NULL) {

                register cpl_size ival;
                register cpl_size first = 0;
                register cpl_size count = 0;

                for (ival = 0; ival < depth; ++ival) {

                    if (_null_mask[ival]) {

                        if (count == 0) {
                            first = ival;
                        }

                        ++count;

                    }
                    else {

                        if (count) {
                            fits_write_col_null(outfile, icol + 1,
                                                trow + _irow + 1, first + 1,
                                                count, &status);

                            if (status) {
                                break;
                            }

                        }

                        count = 0;

                    }

                }

                if ((status == 0) && count) {
                    fits_write_col_null(outfile, icol + 1, trow + _irow + 1,
                                        first + 1, count, &status);
                }

            }

        }

    }


    /*
     * Cleanup
     */

    if (null_mask) {
        cpl_free(null_mask);
    }

    if (null_array) {
        cpl_array_delete(null_array);
    }

    if (sdata) {
        cpl_free(sdata);
    }

    cpl_free(tdata);


    if (status) {
        return cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                  status, fits_write_col_null,
                                  "column=%s, icol=%d, srow=%d, nrows=%d",
                                  cpl_column_get_name(column),
                                  (int)icol, (int)srow, (int)nrows);
    }

    return CPL_ERROR_NONE;

}


inline static cpl_error_code
_cpl_table_save_extend(const cpl_table *table, fitsfile *outfile,
                       const cpl_propertylist *header)
{

    cpl_table_layout layout;
    cpl_size cstart, csize;


    cpl_errorstate error_state = cpl_errorstate_get();


    _cpl_table_layout_create(&layout, table);

    if (!cpl_errorstate_is_equal(error_state)) {

        _cpl_table_layout_clear(&layout);
        return cpl_error_set_where_();

    }


    /*
     * Create an empty binary FITS table at the end of the output stream
     * and setup the FITS table structure, i.e. write the header keywords
     * describing the table structure.
     */

    error_state = cpl_errorstate_get();

    _cpl_table_layout_write(&layout, outfile);

    if (!cpl_errorstate_is_equal(error_state)) {

        _cpl_table_layout_clear(&layout);
        return cpl_error_set_where_();

    }


    /*
     * Write table properties to the current extension header
     */

    if (header) {

        const char *ignore_keys = "^(" ERASE_WCS_REGEXP ")$|"
                CPL_FITS_BADKEYS_EXT "|" CPL_FITS_COMPRKEYS;

        if (cpl_fits_add_properties(outfile, header, ignore_keys)) {
            int status = 0;
            (void)cpl_io_fits_delete_file(outfile, &status);
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                          "could not write data properties");
        }
    }


    /*
     * Write table columns to the output stream writing 'csize' rows at a time,
     * where 'csize' is the optimal size of rows as given by the underlying
     * cfitsio infrastructure.
     */

    csize = _cpl_table_save_get_chunksize(outfile, layout.rows);


    error_state = cpl_errorstate_get();

    for (cstart = 0; cstart < layout.rows; cstart += csize) {

        cpl_size icol;


        /*
         * Adjust the chunk size of the last chunk of rows, which may actually
         * contain less than 'csize' rows.
         */

        if (cstart + csize > layout.rows) {
            csize = layout.rows - cstart;
        }

        for (icol = 0; icol < layout.columns; ++icol) {

            const cpl_column *column = table->columns[icol];


            if (layout.depth[icol] > 0) {
                _cpl_table_write_column_array(outfile, cstart, column, icol,
                                              cstart, csize, &layout);
            }
            else {
                _cpl_table_write_column(outfile, cstart, column, icol,
                                        cstart, csize, &layout);
            }

        }

    }

    if (!cpl_errorstate_is_equal(error_state)) {

        _cpl_table_layout_clear(&layout);
        return cpl_error_set_where_();

    }

    _cpl_table_layout_clear(&layout);

    return CPL_ERROR_NONE;

}


inline static cpl_error_code
_cpl_table_save_append(const cpl_table *table, fitsfile *outfile)
{

    cpl_table_layout layout;
    cpl_table_layout _layout;
    cpl_size cstart, csize;

    cpl_errorstate error_state = cpl_errorstate_get();


    _cpl_table_layout_create(&layout, table);

    if (!cpl_errorstate_is_equal(error_state)) {

        _cpl_table_layout_clear(&layout);

        return cpl_error_set_where_();

    }


    /*
     * Get table layout from the output stream to verify its structural
     * equivalence.
     */

    _cpl_table_layout_read(&_layout, outfile);

    if (!cpl_errorstate_is_equal(error_state)) {

        _cpl_table_layout_clear(&_layout);
        _cpl_table_layout_clear(&layout);

        return cpl_error_set_where_();

    }

    if (_cpl_table_layout_compare(&layout, &_layout) != 0) {

        _cpl_table_layout_clear(&_layout);
        _cpl_table_layout_clear(&layout);

        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);

    }

    /*
     * Write table columns to the output stream writing 'csize' rows at
     * a time, where 'csize' is the optimal size of rows as given by
     * the underlying cfitsio infrastructure.
     */

    csize = _cpl_table_save_get_chunksize(outfile, layout.rows);


    error_state = cpl_errorstate_get();

    for (cstart = 0; cstart < layout.rows; cstart += csize) {

        cpl_size icol;


        /*
         * Adjust the chunk size of the last chunk of rows, which
         * may actually contain less than 'csize' rows.
         */

        if (cstart + csize > layout.rows) {
            csize = layout.rows - cstart;
        }

        for (icol = 0; icol < layout.columns; ++icol) {

            const cpl_column *column = table->columns[icol];


            if (layout.depth[icol] > 0) {
                _cpl_table_write_column_array(outfile, _layout.rows + cstart,
                                              column, icol, cstart, csize,
                                              &layout);
            }
            else {
                _cpl_table_write_column(outfile, _layout.rows + cstart,
                                        column, icol, cstart, csize,
                                        &layout);
            }

        }

    }

    if (!cpl_errorstate_is_equal(error_state)) {

        _cpl_table_layout_clear(&_layout);
        _cpl_table_layout_clear(&layout);

        return cpl_error_set_where_();

    }

    _cpl_table_layout_clear(&_layout);
    _cpl_table_layout_clear(&layout);

    return CPL_ERROR_NONE;

}


/*
 * @brief
 *   Save a @em cpl_table to a FITS file.
 *
 * @param table       Input table.
 * @param pheader     Primary header entries.
 * @param header      Table header entries.
 * @param filename    Name of output FITS file.
 * @param mode        Output mode.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>filename</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSUPPORTED_MODE</td>
 *       <td class="ecr">
 *         A saving mode other than <tt>CPL_IO_CREATE</tt> and
 *         <tt>CPL_IO_EXTEND</tt> was specified.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_FOUND</td>
 *       <td class="ecr">
 *         While <i>mode</i> is set to <tt>CPL_IO_EXTEND</tt>,
 *         a file named as specified in <i>filename</i> is not found.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_BAD_FILE_FORMAT</td>
 *       <td class="ecr">
 *         While <i>mode</i> is set to <tt>CPL_IO_EXTEND</tt>, the
 *         specified file is not in FITS format.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_CREATED</td>
 *       <td class="ecr">
 *         The FITS file could not be created.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_IO</td>
 *       <td class="ecr">
 *         The FITS file could only partially be created.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         Generic error condition, that should be reported to the
 *         CPL Team.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function can be used to convert a CPL table into a binary FITS
 * table extension. If the @em mode is set to @c CPL_IO_CREATE, a new
 * FITS file will be created containing an empty primary array, with
 * just one FITS table extension. An existing (and writable) FITS file
 * with the same name would be overwritten. If the @em mode flag is set
 * to @c CPL_IO_EXTEND, a new table extension would be appended to an
 * existing FITS file. If @em mode is set to @c CPL_IO_APPEND it is possible
 * to add rows to the last FITS table extension of the output FITS file.
 *
 * Note that the modes @c CPL_IO_EXTEND and @c CPL_IO_APPEND require that
 * the target file must be writable (and do not take for granted that a file
 * is writable just because it was created by the same application,
 * as this depends from the system @em umask).
 *
 * For using the mode @c CPL_IO_APPEND additional requirements must be
 * fulfilled, which are that the column properties like type, format, units,
 * etc. must match as the properties of the FITS table extension to which the
 * rows should be added exactly. In particular this means that both tables use
 * the same null value representation for integral type columns!
 *
 * Two property lists may be passed to this function, both
 * optionally. The first property list, @em pheader, is just used if
 * the @em mode is set to @c CPL_IO_CREATE, and it is assumed to
 * contain entries for the FITS file primary header. In @em pheader any
 * property name related to the FITS convention, as SIMPLE, BITPIX,
 * NAXIS, EXTEND, BLOCKED, and END, are ignored: such entries would
 * be written anyway to the primary header and set to some standard
 * values.
 *
 * If a @c NULL @em pheader is passed, the primary array would be created
 * with just such entries, that are mandatory in any regular FITS file.
 * The second property list, @em header, is assumed to contain entries
 * for the FITS table extension header. In this property list any
 * property name related to the FITS convention, as XTENSION, BITPIX,
 * NAXIS, PCOUNT, GCOUNT, and END, and to the table structure, as
 * TFIELDS, TTYPEi, TUNITi, TDISPi, TNULLi, TFORMi, would be ignored:
 * such entries are always computed internally, to guarantee their
 * consistency with the actual table structure. A DATE keyword
 * containing the date of table creation in ISO8601 format is also
 * added automatically.
 *
 * @note
 *   Invalid strings in columns of type @c CPL_TYPE_STRING are
 *   written to FITS as blanks. Invalid values in columns of type
 *   @c CPL_TYPE_FLOAT or @c CPL_TYPE_DOUBLE will be written to
 *   the FITS file as a @c NaN. Invalid values in columns of type
 *   @c CPL_TYPE_INT  and @c CPL_TYPE_LONG_LONG are the only ones
 *   that need a specific code to be explicitly assigned to them.
 *   This can be realised by calling the functions cpl_table_fill_invalid_int()
 *   and cpl_table_fill_invalid_long_long(), respectively, for each table
 *   column of type @c CPL_TYPE_INT and @c CPL_TYPE_LONG_LONG
 *   containing invalid values, just before saving the table to FITS. The
 *   numerical values identifying invalid integer column elements
 *   are written to the FITS keywords TNULLn (where n is the column
 *   sequence number). Beware that if valid column elements have
 *   the value identical to the chosen null-code, they will also
 *   be considered invalid in the FITS convention.
 *
 *   When using the mode @c CPL_IO_APPEND the column properties of the table
 *   to be appended are compared to the column properties of the target
 *   FITS extension for each call, which introduces a certain overhead. This
 *   means that appending a single table row at a time may not be efficient
 *   and is not recommended. Rather than writing one row at a time one should
 *   write table chunks containing a suitable number or rows.
 */

static cpl_error_code
_cpl_table_save(const cpl_table *table, const cpl_propertylist *pheader,
                const cpl_propertylist *header, const char *filename,
                unsigned int mode)
{

    fitsfile *outfile = NULL;

    int extend = TRUE;
    int status = 0;

    cpl_errorstate error_state;



    if (table == NULL || filename == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }


    if (mode & CPL_IO_CREATE) {
        char *name;

        /*
         *  If the file exists, it must be (over)writable.
         */

        if (access(filename, F_OK) == 0) {

            if (access(filename, W_OK) != 0) {
                return cpl_error_set_(CPL_ERROR_FILE_NOT_CREATED);
            }

        }


        /*
         * Always recreate the output file, i.e. overwrite an existing file
         * (DFS04866)
         */

        name = cpl_sprintf("!%s", filename);

        cpl_io_fits_create_file(&outfile, name, &status);
        cpl_free(name);

        if (status) {
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                      "filename='%s', mode=%d",
                                      filename, mode);
        }


        /*
         * Create the primary (empty) FITS HDU. If it is given add the
         * property list to the header of the primary HDU. It also adds
         * a dummy DATE record in order to make it appear in the final
         * FITS header before any hierarchical keyword (DFS04595).
         */

        if (pheader) {

            if (fits_create_img(outfile, SHORT_IMG, 0, NULL, &status)) {
                return cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                          status, fits_create_img,
                                          "filename='%s', mode=%d",
                                          filename, mode);
            }

            fits_write_date(outfile, &status);

            if (cpl_fits_add_properties(outfile, pheader, "^("
                                        ERASE_WCS_REGEXP ")$|"
                                        CPL_FITS_BADKEYS_PRIM "|"
                                        CPL_FITS_COMPRKEYS)) {
                (void)cpl_io_fits_delete_file(outfile, &status);
                return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                              "filename='%s', mode=%d",
                                              filename, mode);
            }

        }

    }
    else if (mode & CPL_IO_EXTEND) {

        /*
         *  The file must exist, and must be writable.
         */

        if (access(filename, F_OK) != 0) {
            return cpl_error_set_(CPL_ERROR_FILE_NOT_FOUND);
        }

        if (access(filename, W_OK) != 0) {
            return cpl_error_set_(CPL_ERROR_FILE_NOT_CREATED);
        }

        if (cpl_io_fits_open_diskfile(&outfile, filename, READWRITE, &status)) {
        
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                          "filename='%s', mode=%d",
                                          filename, mode);
        }

    }
    else if (mode & CPL_IO_APPEND) {

        int nhdu = 0;
        int hdu_type;


        /*
         * The file must exist, be writable, and the very last extension
         * must be a table extension.
         */

        if (access(filename, F_OK) != 0) {
            return cpl_error_set_(CPL_ERROR_FILE_NOT_FOUND);
        }

        if (access(filename, W_OK) != 0) {
            return cpl_error_set_(CPL_ERROR_FILE_NOT_CREATED);
        }

        if (cpl_io_fits_open_diskfile(&outfile, filename, READWRITE, &status)) {
        
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                          "filename='%s', mode=%d",
                                          filename, mode);
        }

        fits_get_num_hdus(outfile, &nhdu, &status);
        fits_movabs_hdu(outfile, nhdu, &hdu_type, &status);

        if (status) {
            const cpl_error_code code =
                cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT,
                                   status, fits_movabs_hdu,
                                   "filename='%s', mode=%d",
                                   filename, mode);
            status = 0;
            cpl_io_fits_delete_file(outfile, &status);
            return code;
        }

        extend = FALSE;

    }
    else {

        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);

    }


    /*
     * Write table to disk, either as a new extension, or appending
     * data (only!) to the last extension of the file.
     */

    error_state = cpl_errorstate_get();

    if (extend == TRUE) {
        status = _cpl_table_save_extend(table, outfile, header);
    }
    else {
        status = _cpl_table_save_append(table, outfile);
    }

    if (!cpl_errorstate_is_equal(error_state)) {
        return cpl_error_set_where_();
    }

    if (status) {
        return status;
    }


    /*
     * Update the DATE record in the primary HDU with the current time.
     * This is done last in order to have the DATE entry as the date of
     * its last modification.
     */

    fits_movabs_hdu(outfile, 1, NULL, &status);
    fits_write_date(outfile, &status);

    if (status) {
        const cpl_error_code code =
            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                               status, fits_write_date,
                               "filename='%s', mode=%d",
                               filename, mode);
        status = 0;
        cpl_io_fits_delete_file(outfile, &status);
        return code;
    }


    /*
     * Cleanup
     */

        cpl_io_fits_close_file(outfile, &status);
    
        if (status) {
            status = 0;
            (void)cpl_io_fits_delete_file(outfile, &status);
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                               "filename='%s', mode=%d",
                               filename, mode);
        }

    return CPL_ERROR_NONE;

}


static cpl_error_code
_cpl_table_save_legacy(const cpl_table *table,
                       const cpl_propertylist *pheader,
                       const cpl_propertylist *header,
                       const char *filename,
                       unsigned mode)
{


    cpl_errorstate    prestate = cpl_errorstate_get();
    cpl_size          depth;
    cpl_size          dimensions;

    cpl_array       **arrays;
    cpl_column       *acolumn;

    cpl_column      **column;
    fitsfile         *outfile;

    cpl_size          nc = cpl_table_get_ncol(table);
    cpl_size          nr = cpl_table_get_nrow(table);
    cpl_size          bwidth;
    cpl_size          field_size = 1;
    cpl_size          j, k, z;
    int               i;
    int              *found;
    cpl_size         *nb;
    cpl_size          count;
    cpl_size          chunk, spell;
    long              optimal_nrows;
    cpl_size          fcount = 0;

    long long        *nval;

    int              *idata;
    short int        *sidata;
    long long        *lldata;
    float            *fdata;
    double           *ddata;
    float complex    *cfdata;
    double complex   *cddata;
    const char      **sdata;
    cpl_column_flag  *ndata;
    const char       *nstring;
    char             *sval;
    char             *bdata;
    unsigned char    *ubdata;

    const char      **tunit;
    const char      **ttype;
    const char      **tform;
    char              tnull[FLEN_KEYWORD];

    int               status = 0;


    if (table == NULL || filename == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (mode & CPL_IO_EXTEND) {

        /*
         *  The file must exist, and must be writable.
         */

        if (access(filename, F_OK))
            return cpl_error_set_(CPL_ERROR_FILE_NOT_FOUND);

        if (access(filename, W_OK))
            return cpl_error_set_(CPL_ERROR_FILE_NOT_CREATED);

        if (cpl_io_fits_open_diskfile(&outfile, filename, READWRITE, &status)) {
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                      "filename='%s', mode=%d",
                                      filename, mode);
        }

    }
    else if (mode & CPL_IO_CREATE) {

        /*
         *  If the file exists, it must be (over)writable.
         */

        if (!access(filename, F_OK)) {
            if (access(filename, W_OK)) {
                return cpl_error_set_(CPL_ERROR_FILE_NOT_CREATED);
            }
#ifdef CPL_TABLE_DFS04866
            else {
                cpl_io_fits_open_diskfile(&outfile, filename, READWRITE, &status);
                if (status == 0) {
                    cpl_io_fits_delete_file(outfile, &status);
                    if (status) {
                        return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                                      "filename='%s', mode=%d",
                                                      filename, mode);
                    }
                }
                status = 0;
            }
#endif
        }

        sval = cpl_sprintf("!%s", filename);
        cpl_io_fits_create_file(&outfile, sval, &status);
        cpl_free(sval);
        if (status) {
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                          "filename='%s', mode=%d",
                                          filename, mode);
        }

        if (pheader) {

            /*
             * Write property list to primary FITS header
             */

            if (fits_create_img(outfile, SHORT_IMG, 0, NULL, &status)) {
                return cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                          status, fits_create_img,
                                          "filename='%s', mode=%d",
                                          filename, mode);
            }

            fits_write_date(outfile, &status);

            if (cpl_fits_add_properties(outfile, pheader, "^("
                                        ERASE_WCS_REGEXP ")$|"
                                        CPL_FITS_BADKEYS_PRIM "|"
                                        CPL_FITS_COMPRKEYS)) {
                (void)cpl_io_fits_delete_file(outfile, &status);
                return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                              "filename='%s', mode=%d",
                                              filename, mode);
            }
        }
    }
    else {
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    /*
     *  Defining the FITS table and its columns.
     */

    /*
     *  The table width is the sum of all column widths (in bytes).
     */

    nb = cpl_calloc(nc, sizeof(cpl_size));
    bwidth = 0;
    column = table->columns;

    ttype = cpl_calloc(nc, sizeof(char *));
    tform = cpl_calloc(nc, sizeof(char *));
    tunit = cpl_calloc(nc, sizeof(char *));

    for (i = 0; i < nc; i++, column++) {

        cpl_type column_type = cpl_column_get_type(*column);

        depth = cpl_table_get_column_depth(table, cpl_column_get_name(*column));

        /*
         * Note that strings ttype[i] and tunit[i] do not need to be
         * deallocated (this is done by cpl_table_delete()), while
         * tform[i] do. This is because of the string format (see below),
         * and this is why cpl_strdup() is also used below.
         */

        ttype[i] = cpl_column_get_name(*column);
        tunit[i] = cpl_column_get_unit(*column);

        if (tunit[i] == NULL) {
            tunit[i] = "";
        }

        if (_cpl_columntype_is_pointer(column_type)) {

            switch (_cpl_columntype_get_valuetype(column_type)) {

                case CPL_TYPE_STRING:
                {
                    /*
                     * A new kind of "depth" needs to be defined.
                     * If an element of a table is an array of strings,
                     * this is expressed in FITS like a string as long
                     * as the concatenation of all the strings in the
                     * array. We need therefore to take first of all
                     * the longest string in all the arrays of strings.
                     */

                    arrays = cpl_column_get_data_array(*column);
                    field_size = 0;

                    for (k = 0; k < nr; k++) {
                        if (arrays[k]) {
                            sdata = cpl_array_get_data_string_const(arrays[k]);
                            for (j = 0; j < depth; j++) {
                                if (sdata[j]) {
                                    if ((int)strlen(sdata[j]) > field_size) {
                                        field_size = strlen(sdata[j]);
                                    }
                                }
                            }
                        }
                    }

                    if (field_size == 0)
                        field_size = 1;

                    /*
                     * The new depth is the number of strings for each array,
                     * multiplied by the length of the longest string in all
                     * the arrays.
                     */

                    tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "A",
                                           depth * field_size);

                    /*
                     * In the TDIM keyword will have to be written the total
                     * number of characters for each string field, field_size,
                     * and the number of fields for one table element, depth.
                     */

                    break;
                }

                case CPL_TYPE_INT:
                {
                    if (depth > 0) {

                        switch (cpl_column_get_save_type(*column)) {

                            case CPL_TYPE_BOOL:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "L",
                                                       depth);
                                break;
                            case CPL_TYPE_CHAR:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "S",
                                                       depth);
                                break;
                            case CPL_TYPE_UCHAR:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "B",
                                                       depth);
                                break;
                            case CPL_TYPE_SHORT:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "I",
                                                       depth);
                                break;
                            default:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "J",
                                                       depth);
                        }
                        nb[i] = depth;

                        // tform[i] = cpl_sprintf(cpl_column_get_save_type(*column)
                        //                       == CPL_TYPE_BOOL ? "%dL" : "%dJ",
                        //                       depth);
                        //nb[i] = depth;
                    }
                    else {
                        cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
                    }
                    break;
                }

                case CPL_TYPE_LONG_LONG:
                {
                    if (depth > 0) {

                        switch (cpl_column_get_save_type(*column)) {

                            case CPL_TYPE_BOOL:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "L",
                                                       depth);
                                break;
                            case CPL_TYPE_CHAR:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "S",
                                                       depth);
                                break;
                            case CPL_TYPE_UCHAR:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "B",
                                                       depth);
                                break;
                            case CPL_TYPE_SHORT:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "I",
                                                       depth);
                                break;
                            case CPL_TYPE_INT:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "J",
                                                       depth);
                                break;
                            default:
                                tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "K",
                                                       depth);
                                break;
                        }
                        nb[i] = depth;

                    }
                    else {
                        cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
                    }
                    break;
                }

                case CPL_TYPE_FLOAT:
                {
                    if (depth > 0) {

                        tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "E", depth);
                        nb[i] = depth;
                    }
                    else {
                        cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
                    }
                    break;
                }

                case CPL_TYPE_DOUBLE:
                {
                    if (depth > 0) {
                        tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "D", depth);
                        nb[i] = depth;
                    }
                    else {
                        cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
                    }
                    break;
                }

                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    if (depth > 0) {

                        tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "C", depth);
                        nb[i] = 2 * depth;
                    }
                    else {
                        cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
                    }
                    break;
                }

                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    if (depth > 0) {
                        tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "M", depth);
                        nb[i] = 2 * depth;
                    }
                    else {
                        cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
                    }
                    break;
                }

                default:
                {

                    /* FIXME: No unit test coverage here... */

                    cpl_free(nb);

                    for (i = 0; i < nc; i++) {
                        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
                        cpl_free((void*)tform[i]);
                        CPL_DIAG_PRAGMA_POP;
                    }

                    cpl_free((void*)tform);

                    (void)cpl_io_fits_delete_file(outfile, &status);
                    return cpl_error_set_message_(CPL_ERROR_UNSPECIFIED,
                                                  "filename='%s', mode=%d",
                                                  filename, mode);
                    break;
                }

            }

        }
        else {

            switch (_cpl_columntype_get_valuetype(column_type)) {

                case CPL_TYPE_INT:
                {
                    switch (cpl_column_get_save_type(*column)) {

                        case CPL_TYPE_BOOL:
                            tform[i] = cpl_strdup("1L");
                            break;
                        case CPL_TYPE_CHAR:
                            tform[i] = cpl_strdup("1S");
                            break;
                        case CPL_TYPE_UCHAR:
                            tform[i] = cpl_strdup("1B");
                            break;
                        case CPL_TYPE_SHORT:
                            tform[i] = cpl_strdup("1I");
                            break;
                        default:
                            tform[i] = cpl_strdup("1J");
                            break;
                    }

                    nb[i] = 1;

                    /*
                     * if (cpl_column_get_save_type(*column) == CPL_TYPE_BOOL)
                     *     tform[i] = cpl_strdup("1L");
                     * else
                     *     tform[i] = cpl_strdup("1J");
                     * nb[i] = 1;
                     */

                    break;
                }

                case CPL_TYPE_LONG_LONG:
                {
                    switch (cpl_column_get_save_type(*column)) {

                        case CPL_TYPE_BOOL:
                            tform[i] = cpl_strdup("1L");
                            break;
                        case CPL_TYPE_CHAR:
                            tform[i] = cpl_strdup("1S");
                            break;
                        case CPL_TYPE_UCHAR:
                            tform[i] = cpl_strdup("1B");
                            break;
                        case CPL_TYPE_SHORT:
                            tform[i] = cpl_strdup("1I");
                            break;
                        case CPL_TYPE_INT:
                            tform[i] = cpl_strdup("1J");
                            break;
                        default:
                            tform[i] = cpl_strdup("1K");
                            break;
                    }

                    nb[i] = 1;
                    break;
                }

                case CPL_TYPE_FLOAT:
                {
                    tform[i] = cpl_strdup("1E");
                    nb[i] = 1;
                    break;
                }

                case CPL_TYPE_DOUBLE:
                {
                    tform[i] = cpl_strdup("1D");
                    nb[i] = 1;
                    break;
                }

                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    tform[i] = cpl_strdup("1C");
                    nb[i] = 2;
                    break;
                }

                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    tform[i] = cpl_strdup("1M");
                    nb[i] = 2;
                    break;
                }

                case CPL_TYPE_STRING:
                {
                    /*
                     *  Determine the longest string in column.
                     */

                    sdata = cpl_column_get_data_string_const(*column);
                    field_size = 0;

                    for (j = 0; j < nr; j++)
                        if (sdata[j])
                            if ((int)strlen(sdata[j]) > field_size)
                                field_size = strlen(sdata[j]);

                    if (field_size == 0)
                        field_size = 1;

                    nb[i] = field_size;

                    tform[i] = cpl_sprintf("%" CPL_SIZE_FORMAT "A", field_size);

                    break;
                }

                default:
                {
                    /* FIXME: No unit test coverage here... */
                    cpl_free(nb);

                    for (i = 0; i < nc; i++) {
                        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
                        cpl_free((void*)tform[i]);
                        CPL_DIAG_PRAGMA_POP;
                    }

                    cpl_free((void*)tform);

                    (void)cpl_io_fits_delete_file(outfile, &status);
                    return cpl_error_set_message_(CPL_ERROR_UNSPECIFIED,
                                                  "filename='%s', mode=%d",
                                                  filename, mode);
                    break;
                }

            }

        }

        bwidth += nb[i];

    }

    if (cpl_errorstate_is_equal(prestate)) {
        cpl_fits_create_tbl(outfile, BINARY_TBL, nr, nc, 
                        ttype, tform, tunit, NULL, &status);
    }

    for (i = 0; i < nc; i++) {
        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
        cpl_free((void*)tform[i]);
        CPL_DIAG_PRAGMA_POP;
    }
    cpl_free((void*)tform);
    cpl_free((void*)ttype);
    cpl_free((void*)tunit);

    if (!cpl_errorstate_is_equal(prestate))
        return cpl_error_set_where_();

    if (status) {
        const cpl_error_code code =
            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                               status, fits_create_tbl,
                               "filename='%s', mode=%d",
                               filename, mode);
        status = 0;
        cpl_io_fits_delete_file(outfile, &status);
        return code;
    }

    /*
     *  Determine column properties for each column (null value, dimensions,
     *  etc.).
     *
     *  In order to use a single array for the null values of all types
     *  of integer columns, it is created for the largest supported integer
     *  type and the values are cast to the correct type as needed.
     */

    nval = cpl_calloc(nc, sizeof(long long));

    found = cpl_calloc(nc, sizeof(int));
    column = table->columns;

    for (i = 0; i < nc; i++, column++) {

        cpl_type column_type = cpl_column_get_type(*column);

        depth = cpl_table_get_column_depth(table, cpl_column_get_name(*column));

        /*
         *  Determine the NULL value (if any). The TNULLi keyword
         *  can be used just for integer type columns. The following
         *  code checks that the caller sets the NULL column
         *  elements to a special value. If this was not done, then
         *  no TNULLi keyword will be generated, and the NULL column
         *  elements may contain garbage.
         */

        if (_cpl_columntype_is_pointer(column_type)) {

            switch (_cpl_columntype_get_valuetype(column_type)) {

                case CPL_TYPE_INT:
                {
                    arrays = cpl_column_get_data_array(*column);

                    for (j = 0; j < nr; j++) {
                        if (arrays[j] == NULL) {
                            arrays[j] = cpl_array_new(depth, CPL_TYPE_INT);
                        }
                        acolumn = cpl_array_get_column(arrays[j]);

                        if (cpl_column_has_invalid(acolumn) == 1) {
                            idata = cpl_column_get_data_int(acolumn);
                            ndata = cpl_column_get_data_invalid(acolumn);

                            if (ndata == NULL) {
                                for (z = 0; z < depth; z++) {
                                    if (found[i]) {
                                        if (nval[i] != idata[z]) {
                                            found[i] = 0;
                                            break;
                                        }
                                    }
                                    else {
                                        found[i] = 1;
                                        nval[i] = idata[z];
                                    }
                                }
                            }
                            else {
                                for (z = 0; z < depth; z++) {
                                    if (ndata[z]) {
                                        if (found[i]) {
                                            if (nval[i] != idata[z]) {
                                                found[i] = 0;
                                                break;
                                            }
                                        }
                                        else {
                                            found[i] = 1;
                                            nval[i] = idata[z];
                                        }
                                    }
                                }
                            }
                        }
                    }

                    /*
                     * Write the appropriate TNULLi keyword to header if necessary
                     */

                    if (found[i]) {

                        int inval = (int)nval[i];

                        sprintf(tnull, "TNULL%d", i + 1);
                        fits_write_key(outfile, TINT, tnull, &inval, NULL, &status);

                        if (status) {
                            const cpl_error_code code =
                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                   status, fits_write_key,
                                                   "filename='%s', mode=%d",
                                                   filename, mode);
                            status = 0;
                            cpl_io_fits_delete_file(outfile, &status);
                            cpl_free(found);
                            cpl_free(nval);
                            cpl_free(nb);
                            return code;
                        }
                    }

                    break;
                }

                case CPL_TYPE_LONG_LONG:
                {
                    arrays = cpl_column_get_data_array(*column);

                    for (j = 0; j < nr; j++) {
                        if (arrays[j] == NULL) {
                            arrays[j] = cpl_array_new(depth, CPL_TYPE_LONG_LONG);
                        }
                        acolumn = cpl_array_get_column(arrays[j]);

                        if (cpl_column_has_invalid(acolumn) == 1) {
                            lldata = cpl_column_get_data_long_long(acolumn);
                            ndata = cpl_column_get_data_invalid(acolumn);

                            if (ndata == NULL) {
                                for (z = 0; z < depth; z++) {
                                    if (found[i]) {
                                        if (nval[i] != lldata[z]) {
                                            found[i] = 0;
                                            break;
                                        }
                                    }
                                    else {
                                        found[i] = 1;
                                        nval[i] = lldata[z];
                                    }
                                }
                            }
                            else {
                                for (z = 0; z < depth; z++) {
                                    if (ndata[z]) {
                                        if (found[i]) {
                                            if (nval[i] != lldata[z]) {
                                                found[i] = 0;
                                                break;
                                            }
                                        }
                                        else {
                                            found[i] = 1;
                                            nval[i] = lldata[z];
                                        }
                                    }
                                }
                            }
                        }
                    }

                    /*
                     * Write the appropriate TNULLi keyword to header if necessary
                     */

                    if (found[i]) {

                        long long llnval = nval[i];  /* for consistency only! */

                        sprintf(tnull, "TNULL%d", i + 1);
                        fits_write_key(outfile, TLONGLONG, tnull, &llnval, NULL,
                                       &status);

                        if (status) {
                            const cpl_error_code code =
                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                   status, fits_write_key,
                                                   "filename='%s', mode=%d",
                                                   filename, mode);
                            status = 0;
                            cpl_io_fits_delete_file(outfile, &status);
                            cpl_free(found);
                            cpl_free(nval);
                            cpl_free(nb);
                            return code;
                        }
                    }

                    break;
                }

                default:
                    break;

            }

        }
        else {

            switch (_cpl_columntype_get_valuetype(column_type)) {

                case CPL_TYPE_INT:
                {
                    if (cpl_column_has_invalid(*column)) {

                        idata = cpl_column_get_data_int(*column);

                        ndata = cpl_column_get_data_invalid(*column);
                        if (ndata == NULL) {
                            for (j = 0; j < nr; j++) {
                                if (found[i]) {
                                    if (nval[i] != idata[j]) {
                                        found[i] = 0;
                                        break;
                                    }
                                }
                                else {
                                    found[i] = 1;
                                    nval[i] = idata[j];
                                }
                            }
                        }
                        else {
                            for (j = 0; j < nr; j++) {
                                if (ndata[j]) {
                                    if (found[i]) {
                                        if (nval[i] != idata[j]) {
                                            found[i] = 0;
                                            break;
                                        }
                                    }
                                    else {
                                        found[i] = 1;
                                        nval[i] = idata[j];
                                    }
                                }
                            }
                        }
                    }

                    /*
                     * Write the appropriate TNULLi keyword to header if necessary
                     */

                    if (found[i]) {

                        int inval = (int)nval[i];

                        sprintf(tnull, "TNULL%d", i + 1);
                        fits_write_key(outfile, TINT, tnull, &inval, NULL, &status);

                        if (status) {
                            const cpl_error_code code =
                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                   status, fits_write_key,
                                                   "filename='%s', mode=%d",
                                                   filename, mode);
                            status = 0;
                            cpl_io_fits_delete_file(outfile, &status);
                            cpl_free(found);
                            cpl_free(nval);
                            cpl_free(nb);
                            return code;
                        }
                    }

                    break;
                }

                case CPL_TYPE_LONG_LONG:
                {
                    if (cpl_column_has_invalid(*column)) {

                        lldata = cpl_column_get_data_long_long(*column);

                        ndata = cpl_column_get_data_invalid(*column);

                        if (ndata == NULL) {
                            for (j = 0; j < nr; j++) {
                                if (found[i]) {
                                    if (nval[i] != lldata[j]) {
                                        found[i] = 0;
                                        break;
                                    }
                                }
                                else {
                                    found[i] = 1;
                                    nval[i] = lldata[j];
                                }
                            }
                        }
                        else {
                            for (j = 0; j < nr; j++) {
                                if (ndata[j]) {
                                    if (found[i]) {
                                        if (nval[i] != lldata[j]) {
                                            found[i] = 0;
                                            break;
                                        }
                                    }
                                    else {
                                        found[i] = 1;
                                        nval[i] = lldata[j];
                                    }
                                }
                            }
                        }
                    }

                    /*
                     * Write the appropriate TNULLi keyword to header if necessary
                     */

                    if (found[i]) {

                        long long llnval = nval[i];   /* for consistency only! */

                        sprintf(tnull, "TNULL%d", i + 1);
                        fits_write_key(outfile, TLONGLONG, tnull, &llnval, NULL,
                                       &status);

                        if (status) {
                            const cpl_error_code code =
                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                   status, fits_write_key,
                                                   "filename='%s', mode=%d",
                                                   filename, mode);
                            status = 0;
                            cpl_io_fits_delete_file(outfile, &status);
                            cpl_free(found);
                            cpl_free(nval);
                            cpl_free(nb);
                            return code;
                        }
                    }

                    break;
                }

                default:
                    break;

            }

        }


        /*
         * Determine TDIM if the current column is an array column.
         */

        if (_cpl_columntype_is_pointer(column_type)) {

            switch (_cpl_columntype_get_valuetype(column_type)) {

            case CPL_TYPE_INT:
            case CPL_TYPE_LONG_LONG:
            case CPL_TYPE_FLOAT:
            case CPL_TYPE_DOUBLE:
            case CPL_TYPE_FLOAT_COMPLEX:
            case CPL_TYPE_DOUBLE_COMPLEX:

                dimensions = cpl_column_get_dimensions(*column);

                if (dimensions > 1) {
                    cpl_size *naxes = cpl_malloc(dimensions * sizeof(cpl_size));

                    for (z = 0; z < dimensions; z++)
                        naxes[z] = cpl_column_get_dimension(*column, z);

                    /*
                     * Note the cast to int, to prevent compiler warnings.
                     * Naturally "dimensions" is a size, so it should be
                     * of type cpl_size. However in cfitsio is int.
                     */

                    {
                        /*FIXME: debug purpose only.
                         * Compatibility with cpl_size = int.
                         */

                        LONGLONG *longnaxes = cpl_malloc(dimensions *
                                                         sizeof(LONGLONG));
                        int       t;

                        for (t = 0; t < dimensions; t++) {
                            longnaxes[t] = (LONGLONG)naxes[t];
                        }

                        fits_write_tdimll(outfile, i + 1, (int)dimensions,
                                          longnaxes, &status);

                        cpl_free(longnaxes);

                    }

                    if (status) {
                        const cpl_error_code code =
                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                               status, fits_write_tdimll,
                                               "filename='%s', mode=%d",
                                               filename, mode);
                        status = 0;
                        cpl_io_fits_delete_file(outfile, &status);
                        cpl_free(found);
                        cpl_free(nval);
                        cpl_free(nb);
                        return code;
                    }

                    cpl_free(naxes);
                }

                break;

            case CPL_TYPE_STRING:

                /*
                 * In the TDIM keyword will have to be written the total number
                 * of characters for each string field, field_size, and the
                 * number of fields for one table element, depth.
                 */

                {
                    cpl_size *naxes = cpl_malloc(2 * sizeof(cpl_size));

                    naxes[0] = field_size;
                    naxes[1] = depth;

                    {
                        /*FIXME: debug purpose only.
                         * Compatibility with cpl_size = int.
                         */

                        LONGLONG *longnaxes = cpl_malloc(2 * sizeof(LONGLONG));
                        int       t;

                        for (t = 0; t < 2; t++) {
                            longnaxes[t] = (LONGLONG)naxes[t];
                        }

                        fits_write_tdimll(outfile, i + 1, 2, longnaxes, &status);

                        cpl_free(longnaxes);

                    }

                    cpl_free(naxes);
                }

                break;

            default:
                break;       /* Nothing to do for other types... */
            }

        }

    }

    if (header) {

        /*
         *  Complete secondary header with property list information
         */

        if (cpl_fits_add_properties(outfile, header, "^("
                                    ERASE_WCS_REGEXP ")$|"
                                    CPL_FITS_BADKEYS_EXT "|"
                                    CPL_FITS_COMPRKEYS)) {
            cpl_free(found);
            cpl_free(nval);
            cpl_free(nb);
            (void)cpl_io_fits_delete_file(outfile, &status);
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED,
                                          "filename='%s', mode=%d",
                                          filename, mode);
        }
    }


    /*
     * Before writing data to the output file flush all buffers. This
     * guarantees that the column definitions inside the fitsfile structure
     * are completely valid!
     *
     * This is a workaround for a bug in CFITSIO, which somewhere misses
     * a rescan of the table structure before the data is written. This may
     * cause problems for the columnwise creation of the table used before.
     *
     * It happens that fits_write_tdimll() sets up the column structures
     * filling the tnull member with NULL_UNDEFINED. The actual null value
     * of the following columns is however written to the header only in a
     * later iteration. Since the buffers are not flushed before writing
     * data values it happens that the null value in the fitsfile structure
     * is not updated, although it is properly written to the header already.
     * This causes false 'null not defined' errors in CFITSIO!
     */

    fits_flush_file(outfile, &status);

    if (status) {
        const cpl_error_code code =
            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                               status, fits_flush_file,
                               "filename='%s', mode=%d",
                               filename, mode);
        status = 0;
        cpl_io_fits_delete_file(outfile, &status);
        cpl_free(found);
        cpl_free(nval);
        cpl_free(nb);
        return code;
    }


    /*
     *  Finally prepare the data section.
     */

    /*
     * Get the max ideal number of rows to process at a time
     */

    fits_get_rowsize(outfile, &optimal_nrows, &status);

    if (status) {
        status = 0;
        chunk = nr;
    }
    else {
        chunk = (int)optimal_nrows; /* Note, this cast is really BAD */
        if (chunk > nr)
            chunk = nr;
    }

    spell = chunk;

    for (k = 0; k < nr; k += chunk) {

        if (chunk > nr - k) {
            spell = nr - k;
        }

        column = table->columns;
        for (i = 0; i < nc; i++, column++) {

            cpl_type column_type = cpl_column_get_type(*column);

            depth = cpl_table_get_column_depth(table, 
                                               cpl_column_get_name(*column));
    
            if (_cpl_columntype_is_pointer(column_type)) {

                switch (_cpl_columntype_get_valuetype(column_type)) {

                    case CPL_TYPE_STRING:
                    {
                        int eccolo;
                        int m, n;
                        char *long_string = cpl_malloc(depth * field_size * sizeof(char));
                        arrays = cpl_column_get_data_array(*column);
                        arrays += k;

                        nstring = "";

                        for (z = 0; z < spell; z++) {
                            if (arrays[z]) {

                                sdata = cpl_array_get_data_string_const(arrays[z]);
                                for (j = 0; j < depth; j++) {
                                    if (sdata[j] == NULL) {
                                        sdata[j] = nstring;
                                    }
                                }
                            }
                            else {
                                sdata = cpl_malloc(depth * sizeof(char *));
                                for (j = 0; j < depth; j++) {
                                    sdata[j] = nstring;
                                }
                            }
                            for (m = 0, j = 0; j < depth; j++) {
                                eccolo = 0;
                                for (n = 0; n < field_size; n++, m++) {
                                    if (!eccolo && (sdata[j][n] == '\0')) {
                                        eccolo = 1;
                                    }
                                    if (eccolo) {
                                        long_string[m] = ' ';
                                    }
                                    else {
                                        long_string[m] = sdata[j][n];
                                    }
                                }
                                // strncpy(long_string + j * field_size,
                                //         sdata[j], field_size);
                            }

                            long_string[depth*field_size - 1] = '\0';

                            // "Correct one"
                            // fits_write_colnull(outfile, TSTRING, i + 1,
                            //                    k + z + 1, 1, depth, sdata,
                            //                    nstring, &status);

                            cpl_fits_write_colnull(outfile, TSTRING, i + 1,
                                                   k + z + 1, 1, 1,
                                                   &long_string,
                                                   nstring, &status);

                            if (arrays[z]) {
                                for (j = 0; j < depth; j++) {
                                    if (sdata[j] == nstring) {
                                        sdata[j] = NULL;
                                    }
                                }
                            }
                            else {
                                cpl_free(sdata);
                            }
                        }
                        cpl_free(long_string);
                        break;
                    }

                    case CPL_TYPE_INT:
                    {
                        if(depth == 0)
                            break;

                        arrays = cpl_column_get_data_array(*column);
                        arrays += k;

                        if (cpl_column_get_save_type(*column) == CPL_TYPE_BOOL) {
                            bdata = cpl_malloc(depth * sizeof(char));
                            if (found[i]) {

                                char bnval = (char)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = idata[z];
                                    }
                                    fits_write_colnull(outfile, TLOGICAL,
                                                       i + 1, k + j + 1, 1,
                                                       depth, bdata, &bnval,
                                                       &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = idata[z];
                                    }
                                    fits_write_col(outfile, TLOGICAL, i + 1,
                                                   k + j + 1, 1, depth, bdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_CHAR) {
                            bdata = cpl_malloc(depth * sizeof(char));
                            if (found[i]) {

                                char bnval = (char)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = idata[z];
                                    }
                                    fits_write_colnull(outfile, TSBYTE, i + 1,
                                                       k + j + 1, 1, depth, bdata,
                                                       &bnval, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = idata[z];
                                    }
                                    fits_write_col(outfile, TSBYTE, i + 1,
                                                   k + j + 1, 1, depth, bdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_UCHAR) {
                            ubdata = cpl_malloc(depth * sizeof(unsigned char));
                            if (found[i]) {

                                unsigned char ubnval = (unsigned char)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        ubdata[z] = idata[z];
                                    }
                                    fits_write_colnull(outfile, TBYTE, i + 1,
                                                       k + j + 1, 1,
                                                       depth, ubdata, &ubnval,
                                                       &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        ubdata[z] = idata[z];
                                    }
                                    fits_write_col(outfile, TBYTE, i + 1,
                                                   k + j + 1, 1,
                                                   depth, ubdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(ubdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_SHORT) {
                            sidata = cpl_malloc(depth * sizeof(short));
                            if (found[i]) {

                                short sinval = (short)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        sidata[z] = idata[z];
                                    }
                                    fits_write_colnull(outfile, TSHORT, i + 1,
                                                       k + j + 1, 1,
                                                       depth, sidata, &sinval,
                                                       &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    idata = cpl_column_get_data_int(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        sidata[z] = idata[z];
                                    }
                                    fits_write_col(outfile, TSHORT, i + 1,
                                                   k + j + 1, 1,
                                                   depth, sidata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(sidata);
                        }
                        else {
                            if (found[i]) {

                                int inval = (int)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    fits_write_colnull(outfile, TINT, i + 1,
                                                       k + j + 1, 1, depth,
                                                       cpl_column_get_data_int(acolumn),
                                                       &inval, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    fits_write_col(outfile, TINT,
                                                   i + 1, k + j + 1, 1, depth,
                                                   cpl_column_get_data_int(acolumn),
                                                   &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                        }
                        break;
                    }

                    case CPL_TYPE_LONG_LONG:
                    {
                        if(depth == 0)
                            break;

                        arrays = cpl_column_get_data_array(*column);
                        arrays += k;

                        if (cpl_column_get_save_type(*column) == CPL_TYPE_BOOL) {
                            bdata = cpl_malloc(depth * sizeof(char));
                            if (found[i]) {

                                char bnval = (char)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = lldata[z];
                                    }
                                    fits_write_colnull(outfile, TLOGICAL,
                                                       i + 1, k + j + 1, 1,
                                                       depth, bdata, &bnval,
                                                       &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = lldata[z];
                                    }
                                    fits_write_col(outfile, TLOGICAL, i + 1,
                                                   k + j + 1, 1, depth, bdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_CHAR) {
                            bdata = cpl_malloc(depth * sizeof(char));
                            if (found[i]) {

                                char bnval = (char)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = lldata[z];
                                    }
                                    fits_write_colnull(outfile, TSBYTE, i + 1,
                                                       k + j + 1, 1, depth, bdata,
                                                       &bnval, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        bdata[z] = lldata[z];
                                    }
                                    fits_write_col(outfile, TSBYTE, i + 1,
                                                   k + j + 1, 1, depth, bdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_UCHAR) {
                            ubdata = cpl_malloc(depth * sizeof(unsigned char));
                            if (found[i]) {

                                unsigned char ubnval = (unsigned char)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        ubdata[z] = lldata[z];
                                    }
                                    fits_write_colnull(outfile, TBYTE, i + 1,
                                                       k + j + 1, 1,
                                                       depth, ubdata, &ubnval,
                                                       &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        ubdata[z] = lldata[z];
                                    }
                                    fits_write_col(outfile, TBYTE, i + 1,
                                                   k + j + 1, 1,
                                                   depth, ubdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(ubdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_SHORT) {
                            sidata = cpl_malloc(depth * sizeof(short));
                            if (found[i]) {

                                short sinval = (short)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        sidata[z] = lldata[z];
                                    }
                                    fits_write_colnull(outfile, TSHORT, i + 1,
                                                       k + j + 1, 1,
                                                       depth, sidata, &sinval,
                                                       &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        sidata[z] = lldata[z];
                                    }
                                    fits_write_col(outfile, TSHORT, i + 1,
                                                   k + j + 1, 1,
                                                   depth, sidata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(sidata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_INT) {
                            idata = cpl_malloc(depth * sizeof(int));
                            if (found[i]) {

                                int inval = (int)nval[i];

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        idata[z] = lldata[z];
                                    }
                                    fits_write_colnull(outfile, TINT, i + 1,
                                                       k + j + 1, 1,
                                                       depth, idata, &inval,
                                                       &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    lldata = cpl_column_get_data_long_long(acolumn);
                                    for (z = 0; z < depth; z++) {
                                        idata[z] = lldata[z];
                                    }
                                    fits_write_col(outfile, TINT, i + 1,
                                                   k + j + 1, 1,
                                                   depth, idata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            cpl_free(idata);
                        }
                        else {
                            if (found[i]) {

                                long long llnval = nval[i];  /* for consistency only! */

                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    fits_write_colnull(outfile, TLONGLONG, i + 1,
                                                       k + j + 1, 1, depth,
                                                       cpl_column_get_data_long_long(acolumn),
                                                       &llnval, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_colnull,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {
                                for (j = 0; j < spell; j++) {
                                    acolumn = cpl_array_get_column(arrays[j]);
                                    fits_write_col(outfile, TLONGLONG,
                                                   i + 1, k + j + 1, 1, depth,
                                                   cpl_column_get_data_long_long(acolumn),
                                                   &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                        }
                        break;
                    }

                    case CPL_TYPE_FLOAT:
                    {
                        if (depth == 0)
                            break;

                        arrays = cpl_column_get_data_array(*column);
                        arrays += k;

                        for (j = 0; j < spell; j++) {

                            if (arrays[j]) {

                                /*
                                 * Get the data buffer of the current array.
                                 */

                                acolumn = cpl_array_get_column(arrays[j]);
                                fdata = cpl_column_get_data_float(acolumn);

                                if (cpl_column_has_invalid(acolumn)) {

                                    /*
                                     * If some invalid values are present,
                                     * get also the array with invalid flags.
                                     */

                                    ndata = cpl_column_get_data_invalid(acolumn);
                                    if (ndata) {

                                        /*
                                         * Preliminarily fill this data section
                                         * including also the garbage (i.e., the
                                         * invalid values).
                                         */

                                        fits_write_col(outfile, TFLOAT, i + 1,
                                                       k + j + 1, 1, depth, fdata,
                                                       &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }

                                        /*
                                         * Finally overwrite the garbage with NaN.
                                         * The function fits_write_colnull(), that
                                         * would allow to do this operation in a single
                                         * step, is not used here because
                                         *
                                         *    1) it is based on == comparison between
                                         *       floats, and
                                         *    2) it would introduce an API change,
                                         *       forcing the user to specify a float
                                         *       value before saving a table column.
                                         */

                                        count = 0;
                                        for (z = 0; z < depth; z++) {
                                            if (ndata[z]) {

                                                /*
                                                 * Invalid flag found. If the first of
                                                 * a sequence mark its position, and
                                                 * keep counting.
                                                 */

                                                if (count == 0) {
                                                    fcount = z + 1;
                                                }
                                                count++;
                                            }
                                            else {

                                                /*
                                                 * Valid element found. If it's closing
                                                 * a sequence of invalid elements, dump
                                                 * it to this data section and reset
                                                 * counter.
                                                 */

                                                if (count) {
                                                    fits_write_col_null(outfile, i + 1,
                                                                        k + j + 1,
                                                                        fcount, count,
                                                                        &status);

                                                    if (status) {
                                                        const cpl_error_code code =
                                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                               status, fits_write_col_null,
                                                                               "filename='%s', mode=%d",
                                                                               filename, mode);
                                                        status = 0;
                                                        cpl_io_fits_delete_file(outfile, &status);
                                                        cpl_free(found);
                                                        cpl_free(nval);
                                                        cpl_free(nb);
                                                        return code;
                                                    }
                                                    count = 0;
                                                }
                                            }
                                        }
                                        if (count) {
                                            fits_write_col_null(outfile, i + 1,
                                                                k + j + 1,
                                                                fcount, count,
                                                                &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                        }
                                    }
                                    else {

                                        /*
                                         * All elements are invalid: just pad with NaN
                                         * the current data section.
                                         */

                                        fits_write_col_null(outfile, i + 1, k + j + 1,
                                                            1, depth, &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col_null,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }
                                    }
                                }
                                else {

                                    /*
                                     * No invalid values are present, simply copy
                                     * the whole array buffer to this data section.
                                     */

                                    fits_write_col(outfile, TFLOAT, i + 1,
                                                   k + j + 1, 1,
                                                   depth, fdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + j + 1,
                                                    1, depth, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        break;
                    }

                    case CPL_TYPE_DOUBLE:
                    {
                        if (depth == 0)
                            break;

                        arrays = cpl_column_get_data_array(*column);
                        arrays += k;

                        for (j = 0; j < spell; j++) {

                            if (arrays[j]) {

                                /*
                                 * Get the data buffer of the current array.
                                 */

                                acolumn = cpl_array_get_column(arrays[j]);
                                ddata = cpl_column_get_data_double(acolumn);

                                if (cpl_column_has_invalid(acolumn)) {

                                    /*
                                     * If some invalid values are present,
                                     * get also the array with invalid flags.
                                     */

                                    ndata = cpl_column_get_data_invalid(acolumn);
                                    if (ndata) {

                                        /*
                                         * Preliminarily fill this data section
                                         * including also the garbage (i.e., the
                                         * invalid values).
                                         */

                                        fits_write_col(outfile, TDOUBLE, i + 1,
                                                       k + j + 1, 1, depth, ddata,
                                                       &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }

                                        /*
                                         * Finally overwrite the garbage with NaN.
                                         * The function fits_write_colnull(), that
                                         * would allow to do this operation in a single
                                         * step, is not used here because
                                         *
                                         *    1) it is based on == comparison between
                                         *       floats, and
                                         *    2) it would introduce an API change,
                                         *       forcing the user to specify a float
                                         *       value before saving a table column.
                                         */

                                        count = 0;
                                        for (z = 0; z < depth; z++) {
                                            if (ndata[z]) {

                                                /*
                                                 * Invalid flag found. If the first of
                                                 * a sequence mark its position, and
                                                 * keep counting.
                                                 */

                                                if (count == 0) {
                                                    fcount = z + 1;
                                                }
                                                count++;
                                            }
                                            else {

                                                /*
                                                 * Valid element found. If it's closing
                                                 * a sequence of invalid elements, dump
                                                 * it to this data section and reset
                                                 * counter.
                                                 */

                                                if (count) {
                                                    fits_write_col_null(outfile, i + 1,
                                                                        k + j + 1,
                                                                        fcount, count,
                                                                        &status);

                                                    if (status) {
                                                        const cpl_error_code code =
                                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                               status, fits_write_col_null,
                                                                               "filename='%s', mode=%d",
                                                                               filename, mode);
                                                        status = 0;
                                                        cpl_io_fits_delete_file(outfile, &status);
                                                        cpl_free(found);
                                                        cpl_free(nval);
                                                        cpl_free(nb);
                                                        return code;
                                                    }
                                                    count = 0;
                                                }
                                            }
                                        }
                                        if (count) {
                                            fits_write_col_null(outfile, i + 1,
                                                                k + j + 1, fcount,
                                                                count, &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                        }
                                    }
                                    else {

                                        /*
                                         * All elements are invalid: just pad with NaN
                                         * the current data section.
                                         */

                                        fits_write_col_null(outfile, i + 1, k + j + 1,
                                                            1, depth, &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col_null,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }
                                    }
                                }
                                else {

                                    /*
                                     * No invalid values are present, simply copy
                                     * the whole array buffer to this data section.
                                     */

                                    fits_write_col(outfile, TDOUBLE, i + 1, k + j + 1,
                                                   1, depth, ddata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + j + 1,
                                                    1, depth, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        break;
                    }

                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        if (depth == 0)
                            break;

                        fdata = cpl_malloc(2 * depth * sizeof(float));
                        arrays = cpl_column_get_data_array(*column);
                        arrays += k;

                        for (j = 0; j < spell; j++) {

                            if (arrays[j]) {

                                /*
                                 * Get the data buffer of the current array.
                                 */

                                acolumn = cpl_array_get_column(arrays[j]);
                                cfdata = cpl_column_get_data_float_complex(acolumn);

                                for (z = 0; z < depth; z++) {
                                    fdata[2*z] = crealf(cfdata[z]);
                                    fdata[2*z + 1] = cimagf(cfdata[z]);
                                }

                                if (cpl_column_has_invalid(acolumn)) {

                                    /*
                                     * If some invalid values are present,
                                     * get also the array with invalid flags.
                                     */

                                    ndata = cpl_column_get_data_invalid(acolumn);
                                    if (ndata) {

                                        /*
                                         * Preliminarily fill this data section
                                         * including also the garbage (i.e., the
                                         * invalid values).
                                         */

                                        fits_write_col(outfile, TCOMPLEX, i + 1,
                                                       k + j + 1, 1, depth, fdata,
                                                       &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }

                                        /*
                                         * Finally overwrite the garbage with NaN.
                                         * The function fits_write_colnull(), that
                                         * would allow to do this operation in a single
                                         * step, is not used here because
                                         *
                                         *    1) it is based on == comparison between
                                         *       floats, and
                                         *    2) it would introduce an API change,
                                         *       forcing the user to specify a float
                                         *       value before saving a table column.
                                         */

                                        count = 0;
                                        for (z = 0; z < depth; z++) {
                                            if (ndata[z]) {

                                                /*
                                                 * Invalid flag found. If the first of
                                                 * a sequence mark its position, and
                                                 * keep counting.
                                                 */

                                                if (count == 0) {
                                                    fcount = z + 1;
                                                }
                                                count++;
                                            }
                                            else {

                                                /*
                                                 * Valid element found. If it's closing
                                                 * a sequence of invalid elements, dump
                                                 * it to this data section and reset
                                                 * counter.
                                                 */

                                                if (count) {
                                                    fits_write_col_null(outfile, i + 1,
                                                                        k + j + 1,
                                                                        fcount, count,
                                                                        &status);

                                                    if (status) {
                                                        const cpl_error_code code =
                                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                               status, fits_write_col_null,
                                                                               "filename='%s', mode=%d",
                                                                               filename, mode);
                                                        status = 0;
                                                        cpl_io_fits_delete_file(outfile, &status);
                                                        cpl_free(found);
                                                        cpl_free(nval);
                                                        cpl_free(nb);
                                                        return code;
                                                    }
                                                    count = 0;
                                                }
                                            }
                                        }
                                        if (count) {
                                            fits_write_col_null(outfile, i + 1,
                                                                k + j + 1,
                                                                fcount, count,
                                                                &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                        }
                                    }
                                    else {

                                        /*
                                         * All elements are invalid: just pad with NaN
                                         * the current data section.
                                         */

                                        fits_write_col_null(outfile, i + 1, k + j + 1,
                                                            1, depth, &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col_null,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }
                                    }
                                }
                                else {

                                    /*
                                     * No invalid values are present, simply copy
                                     * the whole array buffer to this data section.
                                     */

                                    fits_write_col(outfile, TCOMPLEX, i + 1,
                                                   k + j + 1, 1,
                                                   depth, fdata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + j + 1,
                                                    1, depth, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        cpl_free(fdata);
                        break;
                    }

                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        if (depth == 0)
                            break;

                        ddata = cpl_malloc(2 * depth * sizeof(double));
                        arrays = cpl_column_get_data_array(*column);
                        arrays += k;

                        for (j = 0; j < spell; j++) {

                            if (arrays[j]) {

                                /*
                                 * Get the data buffer of the current array.
                                 */

                                acolumn = cpl_array_get_column(arrays[j]);
                                cddata = cpl_column_get_data_double_complex(acolumn);

                                for (z = 0; z < depth; z++) {
                                    ddata[2*z] = creal(cddata[z]);
                                    ddata[2*z + 1] = cimag(cddata[z]);
                                }

                                if (cpl_column_has_invalid(acolumn)) {

                                    /*
                                     * If some invalid values are present,
                                     * get also the array with invalid flags.
                                     */

                                    ndata = cpl_column_get_data_invalid(acolumn);
                                    if (ndata) {

                                        /*
                                         * Preliminarily fill this data section
                                         * including also the garbage (i.e., the
                                         * invalid values).
                                         */

                                        fits_write_col(outfile, TDBLCOMPLEX, i + 1,
                                                       k + j + 1, 1, depth, ddata,
                                                       &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }

                                        /*
                                         * Finally overwrite the garbage with NaN.
                                         * The function fits_write_colnull(), that
                                         * would allow to do this operation in a single
                                         * step, is not used here because
                                         *
                                         *    1) it is based on == comparison between
                                         *       floats, and
                                         *    2) it would introduce an API change,
                                         *       forcing the user to specify a float
                                         *       value before saving a table column.
                                         */

                                        count = 0;
                                        for (z = 0; z < depth; z++) {
                                            if (ndata[z]) {

                                                /*
                                                 * Invalid flag found. If the first of
                                                 * a sequence mark its position, and
                                                 * keep counting.
                                                 */

                                                if (count == 0) {
                                                    fcount = z + 1;
                                                }
                                                count++;
                                            }
                                            else {

                                                /*
                                                 * Valid element found. If it's closing
                                                 * a sequence of invalid elements, dump
                                                 * it to this data section and reset
                                                 * counter.
                                                 */

                                                if (count) {
                                                    fits_write_col_null(outfile, i + 1,
                                                                        k + j + 1,
                                                                        fcount, count,
                                                                        &status);

                                                    if (status) {
                                                        const cpl_error_code code =
                                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                               status, fits_write_col_null,
                                                                               "filename='%s', mode=%d",
                                                                               filename, mode);
                                                        status = 0;
                                                        cpl_io_fits_delete_file(outfile, &status);
                                                        cpl_free(found);
                                                        cpl_free(nval);
                                                        cpl_free(nb);
                                                        return code;
                                                    }
                                                    count = 0;
                                                }
                                            }
                                        }
                                        if (count) {
                                            fits_write_col_null(outfile, i + 1,
                                                                k + j + 1, fcount,
                                                                count, &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                        }
                                    }
                                    else {

                                        /*
                                         * All elements are invalid: just pad with NaN
                                         * the current data section.
                                         */

                                        fits_write_col_null(outfile, i + 1, k + j + 1,
                                                            1, depth, &status);

                                        if (status) {
                                            const cpl_error_code code =
                                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                   status, fits_write_col_null,
                                                                   "filename='%s', mode=%d",
                                                                   filename, mode);
                                            status = 0;
                                            cpl_io_fits_delete_file(outfile, &status);
                                            cpl_free(found);
                                            cpl_free(nval);
                                            cpl_free(nb);
                                            return code;
                                        }
                                    }
                                }
                                else {

                                    /*
                                     * No invalid values are present, simply copy
                                     * the whole array buffer to this data section.
                                     */

                                    fits_write_col(outfile, TDBLCOMPLEX, i + 1,
                                                   k + j + 1, 1, depth, ddata, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + j + 1,
                                                    1, depth, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        cpl_free(ddata);
                        break;
                    }

                    default:
                        break;       /* Should never get here... */

                }

            }
            else {

                switch (_cpl_columntype_get_valuetype(column_type)) {

                    case CPL_TYPE_STRING:
                    {
                        sdata = cpl_column_get_data_string_const(*column);
                        sdata += k;

                        /*
                         * If there are NULL strings, assign a zero-length
                         * string or a call to strcmp() in fits_write_colnull()
                         * would segfault.
                         */

                        nstring = "";

                        for (j = 0; j < spell; j++)
                            if (sdata[j] == NULL)
                                sdata[j] = nstring;

                        cpl_fits_write_colnull(outfile, TSTRING, i + 1, k + 1,
                                               1, spell, sdata,
                                               nstring, &status);
                        if (status) {
                            const cpl_error_code code =
                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                   status, fits_write_colnull,
                                                   "filename='%s', mode=%d",
                                                   filename, mode);
                            status = 0;
                            cpl_io_fits_delete_file(outfile, &status);
                            cpl_free(found);
                            cpl_free(nval);
                            cpl_free(nb);
                            return code;
                        }

                        /*
                         * Restore the fake zero-length strings to a NULL
                         * pointer. Note that this is done only if the
                         * zero-length string is the one pointed by nstring:
                         * in this way the zero-length strings that were
                         * already present in the table are not leaked.
                         */

                        for (j = 0; j < spell; j++)
                            if (sdata[j] == nstring)
                                sdata[j] = NULL;

                        break;
                    }

                    case CPL_TYPE_INT:
                    {
                        /*
                         * Get the data buffer of the current column.
                         */

                        idata = cpl_column_get_data_int(*column);
                        idata += k;

                        if (cpl_column_get_save_type(*column) == CPL_TYPE_BOOL) {
                            bdata = cpl_malloc(spell * sizeof(char));
                            for (j = 0; j < spell; j++) {
                                bdata[j] = idata[j];
                            }
                            if (found[i]) {
                                char bnval = (char)nval[i];

                                fits_write_colnull(outfile, TLOGICAL, i + 1, k + 1, 1,
                                                   spell, bdata, &bnval, &status);
                            }
                            else {
                                fits_write_col(outfile, TLOGICAL, i + 1, k + 1, 1,
                                               spell, bdata, &status);
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_CHAR) {
                            bdata = cpl_malloc(spell * sizeof(char));
                            for (j = 0; j < spell; j++) {
                                bdata[j] = idata[j];
                            }
                            if (found[i]) {
                                char bnval = (char)nval[i];

                                fits_write_colnull(outfile, TSBYTE, i + 1, k + 1, 1,
                                                   spell, bdata, &bnval, &status);
                            }
                            else {
                                fits_write_col(outfile, TSBYTE, i + 1, k + 1, 1,
                                               spell, bdata, &status);
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_UCHAR) {
                            ubdata = cpl_malloc(spell * sizeof(unsigned char));
                            for (j = 0; j < spell; j++) {
                                ubdata[j] = idata[j];
                            }
                            if (found[i]) {
                                unsigned char ubnval = (unsigned char)nval[i];

                                fits_write_colnull(outfile, TBYTE, i + 1, k + 1, 1,
                                                   spell, ubdata, &ubnval, &status);
                            }
                            else {
                                fits_write_col(outfile, TBYTE, i + 1, k + 1, 1,
                                               spell, ubdata, &status);
                            }
                            cpl_free(ubdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_SHORT) {
                            sidata = cpl_malloc(spell * sizeof(short));
                            for (j = 0; j < spell; j++) {
                                sidata[j] = idata[j];
                            }
                            if (found[i]) {
                                short sinval = (short)nval[i];

                                fits_write_colnull(outfile, TSHORT, i + 1, k + 1, 1,
                                                   spell, sidata, &sinval, &status);
                            }
                            else {
                                fits_write_col(outfile, TSHORT, i + 1, k + 1, 1,
                                               spell, sidata, &status);
                            }
                            cpl_free(sidata);
                        }
                        else {
                            if (found[i]) {
                                int inval = (int)nval[i];
                                fits_write_colnull(outfile, TINT, i + 1, k + 1, 1,
                                                   spell, idata, &inval, &status);
                            }
                            else {
                                fits_write_col(outfile, TINT, i + 1, k + 1, 1, spell,
                                               idata, &status);
                            }
                        }

                        if (status) {
                            const cpl_error_code code =
                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                   status, fits_write_col,
                                                   "filename='%s', mode=%d",
                                                   filename, mode);
                            status = 0;
                            cpl_io_fits_delete_file(outfile, &status);
                            cpl_free(found);
                            cpl_free(nval);
                            cpl_free(nb);
                            return code;
                        }
                        break;
                    }

                    case CPL_TYPE_LONG_LONG:
                    {
                        /*
                         * Get the data buffer of the current column.
                         */

                        lldata = cpl_column_get_data_long_long(*column);
                        lldata += k;

                        if (cpl_column_get_save_type(*column) == CPL_TYPE_BOOL) {
                            bdata = cpl_malloc(spell * sizeof(char));
                            for (j = 0; j < spell; j++) {
                                bdata[j] = lldata[j];
                            }
                            if (found[i]) {
                                char bnval = (char)nval[i];

                                fits_write_colnull(outfile, TLOGICAL, i + 1, k + 1, 1,
                                                   spell, bdata, &bnval, &status);
                            }
                            else {
                                fits_write_col(outfile, TLOGICAL, i + 1, k + 1, 1,
                                               spell, bdata, &status);
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_CHAR) {
                            bdata = cpl_malloc(spell * sizeof(char));
                            for (j = 0; j < spell; j++) {
                                bdata[j] = lldata[j];
                            }
                            if (found[i]) {
                                char bnval = (char)nval[i];

                                fits_write_colnull(outfile, TSBYTE, i + 1, k + 1, 1,
                                                   spell, bdata, &bnval, &status);
                            }
                            else {
                                fits_write_col(outfile, TSBYTE, i + 1, k + 1, 1,
                                               spell, bdata, &status);
                            }
                            cpl_free(bdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_UCHAR) {
                            ubdata = cpl_malloc(spell * sizeof(unsigned char));
                            for (j = 0; j < spell; j++) {
                                ubdata[j] = lldata[j];
                            }
                            if (found[i]) {
                                unsigned char ubnval = (unsigned char)nval[i];

                                fits_write_colnull(outfile, TBYTE, i + 1, k + 1, 1,
                                                   spell, ubdata, &ubnval, &status);
                            }
                            else {
                                fits_write_col(outfile, TBYTE, i + 1, k + 1, 1,
                                               spell, ubdata, &status);
                            }
                            cpl_free(ubdata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_SHORT) {
                            sidata = cpl_malloc(spell * sizeof(short));
                            for (j = 0; j < spell; j++) {
                                sidata[j] = lldata[j];
                            }
                            if (found[i]) {
                                short sinval = (short)nval[i];

                                fits_write_colnull(outfile, TSHORT, i + 1, k + 1, 1,
                                                   spell, sidata, &sinval, &status);
                            }
                            else {
                                fits_write_col(outfile, TSHORT, i + 1, k + 1, 1,
                                               spell, sidata, &status);
                            }
                            cpl_free(sidata);
                        }
                        else if (cpl_column_get_save_type(*column) == CPL_TYPE_INT) {
                            idata = cpl_malloc(spell * sizeof(int));
                            for (j = 0; j < spell; j++) {
                                idata[j] = lldata[j];
                            }
                            if (found[i]) {
                                int inval = (int)nval[i];

                                fits_write_colnull(outfile, TINT, i + 1, k + 1, 1,
                                                   spell, idata, &inval, &status);
                            }
                            else {
                                fits_write_col(outfile, TINT, i + 1, k + 1, 1,
                                               spell, idata, &status);
                            }
                            cpl_free(idata);
                        }
                        else {
                            if (found[i]) {

                                long long llnval = nval[i];  /* for consistency only! */

                                fits_write_colnull(outfile, TLONGLONG, i + 1, k + 1, 1,
                                                   spell, lldata, &llnval, &status);
                            }
                            else {
                                fits_write_col(outfile, TLONGLONG, i + 1, k + 1, 1,
                                               spell, lldata, &status);
                            }
                        }

                        if (status) {
                            const cpl_error_code code =
                                cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                   status, fits_write_col,
                                                   "filename='%s', mode=%d",
                                                   filename, mode);
                            status = 0;
                            cpl_io_fits_delete_file(outfile, &status);
                            cpl_free(found);
                            cpl_free(nval);
                            cpl_free(nb);
                            return code;
                        }
                        break;
                    }

                    case CPL_TYPE_FLOAT:
                    {
                        /*
                         * Get the data buffer of the current column.
                         */

                        fdata = cpl_column_get_data_float(*column);
                        fdata += k;

                        if (cpl_column_has_invalid(*column)) {

                            /*
                             * If some invalid values are present,
                             * get also the array with invalid flags.
                             */

                            ndata = cpl_column_get_data_invalid(*column);
                            if (ndata) {

                                ndata += k;

                                /*
                                 * Preliminarily fill this data section
                                 * including also the garbage (i.e., the
                                 * invalid values).
                                 */

                                fits_write_col(outfile, TFLOAT, i + 1, k + 1, 1, spell,
                                               fdata, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }

                                /*
                                 * Finally overwrite the garbage with NaN.
                                 * The function fits_write_colnull(), that
                                 * would allow to do this operation in a single
                                 * step, is not used here because
                                 *
                                 *    1) it is based on == comparison between
                                 *       floats, and
                                 *    2) it would introduce an API change,
                                 *       forcing the user to specify a float
                                 *       value before saving a table column.
                                 */

                                count = 0;
                                for (j = 0; j < spell; j++) {
                                    if (ndata[j]) {

                                        /*
                                         * Invalid flag found. If the first of
                                         * a sequence mark its position, and
                                         * keep counting.
                                         */

                                        if (count == 0) {
                                            fcount = k + j + 1;
                                        }
                                        count++;
                                    }
                                    else {

                                        /*
                                         * Valid element found. If it's closing
                                         * a sequence of invalid elements, dump
                                         * it to this data section and reset
                                         * counter.
                                         */

                                        if (count) {
                                            fits_write_col_null(outfile, i + 1, fcount,
                                                                1, count, &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                            count = 0;
                                        }
                                    }
                                }
                                if (count) {
                                    fits_write_col_null(outfile, i + 1, fcount,
                                                        1, count, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col_null,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + 1, 1,
                                                    spell, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        else {

                            /*
                             * No invalid values are present, simply copy
                             * the whole array buffer to this data section.
                             */

                            fits_write_col(outfile, TFLOAT, i + 1, k + 1, 1, spell,
                                           fdata, &status);

                            if (status) {
                                const cpl_error_code code =
                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                       status, fits_write_col,
                                                       "filename='%s', mode=%d",
                                                       filename, mode);
                                status = 0;
                                cpl_io_fits_delete_file(outfile, &status);
                                cpl_free(found);
                                cpl_free(nval);
                                cpl_free(nb);
                                return code;
                            }
                        }
                        break;
                    }

                    case CPL_TYPE_DOUBLE:
                    {
                        /*
                         * Get the data buffer of the current column.
                         */

                        ddata = cpl_column_get_data_double(*column);
                        ddata += k;

                        if (cpl_column_has_invalid(*column)) {

                            /*
                             * If some invalid values are present,
                             * get also the array with invalid flags.
                             */

                            ndata = cpl_column_get_data_invalid(*column);
                            if (ndata) {

                                ndata += k;

                                /*
                                 * Preliminarily fill this data section
                                 * including also the garbage (i.e., the
                                 * invalid values).
                                 */

                                fits_write_col(outfile, TDOUBLE, i + 1, k + 1, 1,
                                               spell, ddata, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }

                                /*
                                 * Finally overwrite the garbage with NaN.
                                 * The function fits_write_colnull(), that
                                 * would allow to do this operation in a single
                                 * step, is not used here because
                                 *
                                 *    1) it is based on == comparison between
                                 *       floats, and
                                 *    2) it would introduce an API change,
                                 *       forcing the user to specify a float
                                 *       value before saving a table column.
                                 */

                                count = 0;
                                for (j = 0; j < spell; j++) {
                                    if (ndata[j]) {

                                        /*
                                         * Invalid flag found. If the first of
                                         * a sequence mark its position, and
                                         * keep counting.
                                         */

                                        if (count == 0) {
                                            fcount = k + j + 1;
                                        }
                                        count++;
                                    }
                                    else {

                                        /*
                                         * Valid element found. If it's closing
                                         * a sequence of invalid elements, dump
                                         * it to this data section and reset
                                         * counter.
                                         */

                                        if (count) {
                                            fits_write_col_null(outfile, i + 1, fcount,
                                                                1, count, &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                            count = 0;
                                        }
                                    }
                                }
                                if (count) {
                                    fits_write_col_null(outfile, i + 1, fcount,
                                                        1, count, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col_null,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + 1, 1,
                                                    spell, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        else {

                            /*
                             * No invalid values are present, simply copy
                             * the whole array buffer to this data section.
                             */

                            fits_write_col(outfile, TDOUBLE, i + 1, k + 1, 1,
                                           spell, ddata, &status);

                            if (status) {
                                const cpl_error_code code =
                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                       status, fits_write_col,
                                                       "filename='%s', mode=%d",
                                                       filename, mode);
                                status = 0;
                                cpl_io_fits_delete_file(outfile, &status);
                                cpl_free(found);
                                cpl_free(nval);
                                cpl_free(nb);
                                return code;
                            }
                        }
                        break;
                    }

                    case CPL_TYPE_FLOAT_COMPLEX:
                    {
                        /*
                         * Get the data buffer of the current column.
                         */

                        fdata = cpl_malloc(2 * spell * sizeof(float));
                        cfdata = cpl_column_get_data_float_complex(*column);
                        cfdata += k;
                        for (j = 0; j < spell; j++) {
                            fdata[2*j] = crealf(cfdata[j]);
                            fdata[2*j + 1] = cimagf(cfdata[j]);
                        }

                        if (cpl_column_has_invalid(*column)) {

                            /*
                             * If some invalid values are present,
                             * get also the array with invalid flags.
                             */

                            ndata = cpl_column_get_data_invalid(*column);
                            if (ndata) {

                                ndata += k;

                                /*
                                 * Preliminarily fill this data section
                                 * including also the garbage (i.e., the
                                 * invalid values).
                                 */

                                fits_write_col(outfile, TCOMPLEX, i + 1, k + 1, 1,
                                               spell, fdata, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }

                                /*
                                 * Finally overwrite the garbage with NaN.
                                 * The function fits_write_colnull(), that
                                 * would allow to do this operation in a single
                                 * step, is not used here because
                                 *
                                 *    1) it is based on == comparison between
                                 *       floats, and
                                 *    2) it would introduce an API change,
                                 *       forcing the user to specify a float
                                 *       value before saving a table column.
                                 */

                                count = 0;
                                for (j = 0; j < spell; j++) {
                                    if (ndata[j]) {

                                        /*
                                         * Invalid flag found. If the first of
                                         * a sequence, mark its position and
                                         * keep counting.
                                         */

                                        if (count == 0) {
                                            fcount = k + j + 1;
                                        }
                                        count++;
                                    }
                                    else {

                                        /*
                                         * Valid element found. If it's closing
                                         * a sequence of invalid elements, dump
                                         * it to this data section and reset
                                         * counter.
                                         */

                                        if (count) {
                                            fits_write_col_null(outfile, i + 1, fcount,
                                                                1, count, &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                            count = 0;
                                        }
                                    }
                                }
                                if (count) {
                                    fits_write_col_null(outfile, i + 1, fcount,
                                                        1, count, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col_null,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + 1, 1,
                                                    spell, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        else {

                            /*
                             * No invalid values are present, simply copy
                             * the whole array buffer to this data section.
                             */

                            fits_write_col(outfile, TCOMPLEX,
                                           i + 1, k + 1, 1, spell, fdata, &status);

                            if (status) {
                                const cpl_error_code code =
                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                       status, fits_write_col,
                                                       "filename='%s', mode=%d",
                                                       filename, mode);
                                status = 0;
                                cpl_io_fits_delete_file(outfile, &status);
                                cpl_free(found);
                                cpl_free(nval);
                                cpl_free(nb);
                                return code;
                            }
                        }
                        cpl_free(fdata);
                        break;
                    }

                    case CPL_TYPE_DOUBLE_COMPLEX:
                    {
                        /*
                         * Get the data buffer of the current column.
                         */

                        ddata = cpl_malloc(2 * spell * sizeof(double));

                        cddata = cpl_column_get_data_double_complex(*column);
                        cddata += k;

                        for (j = 0; j < spell; j++) {
                            ddata[2*j] = creal(cddata[j]);
                            ddata[2*j + 1] = cimag(cddata[j]);
                        }

                        if (cpl_column_has_invalid(*column)) {

                            /*
                             * If some invalid values are present,
                             * get also the array with invalid flags.
                             */

                            ndata = cpl_column_get_data_invalid(*column);
                            if (ndata) {

                                ndata += k;

                                /*
                                 * Preliminarily fill this data section
                                 * including also the garbage (i.e., the
                                 * invalid values).
                                 */

                                fits_write_col(outfile, TDBLCOMPLEX, i + 1, k + 1, 1,
                                               spell, ddata, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }

                                /*
                                 * Finally overwrite the garbage with NaN.
                                 * The function fits_write_colnull(), that
                                 * would allow to do this operation in a single
                                 * step, is not used here because
                                 *
                                 *    1) it is based on == comparison between
                                 *       floats, and
                                 *    2) it would introduce an API change,
                                 *       forcing the user to specify a float
                                 *       value before saving a table column.
                                 */

                                count = 0;
                                for (j = 0; j < spell; j++) {
                                    if (ndata[j]) {

                                        /*
                                         * Invalid flag found. If the first of
                                         * a sequence mark its position, and
                                         * keep counting.
                                         */

                                        if (count == 0) {
                                            fcount = k + j + 1;
                                        }
                                        count++;
                                    }
                                    else {

                                        /*
                                         * Valid element found. If it's closing
                                         * a sequence of invalid elements, dump
                                         * it to this data section and reset
                                         * counter.
                                         */

                                        if (count) {
                                            fits_write_col_null(outfile, i + 1, fcount,
                                                                1, count, &status);

                                            if (status) {
                                                const cpl_error_code code =
                                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                                       status, fits_write_col_null,
                                                                       "filename='%s', mode=%d",
                                                                       filename, mode);
                                                status = 0;
                                                cpl_io_fits_delete_file(outfile, &status);
                                                cpl_free(found);
                                                cpl_free(nval);
                                                cpl_free(nb);
                                                return code;
                                            }
                                            count = 0;
                                        }
                                    }
                                }
                                if (count) {
                                    fits_write_col_null(outfile, i + 1, fcount,
                                                        1, count, &status);

                                    if (status) {
                                        const cpl_error_code code =
                                            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                               status, fits_write_col_null,
                                                               "filename='%s', mode=%d",
                                                               filename, mode);
                                        status = 0;
                                        cpl_io_fits_delete_file(outfile, &status);
                                        cpl_free(found);
                                        cpl_free(nval);
                                        cpl_free(nb);
                                        return code;
                                    }
                                }
                            }
                            else {

                                /*
                                 * All elements are invalid: just pad with NaN
                                 * the current data section.
                                 */

                                fits_write_col_null(outfile, i + 1, k + 1, 1,
                                                    spell, &status);

                                if (status) {
                                    const cpl_error_code code =
                                        cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                           status, fits_write_col_null,
                                                           "filename='%s', mode=%d",
                                                           filename, mode);
                                    status = 0;
                                    cpl_io_fits_delete_file(outfile, &status);
                                    cpl_free(found);
                                    cpl_free(nval);
                                    cpl_free(nb);
                                    return code;
                                }
                            }
                        }
                        else {

                            /*
                             * No invalid values are present, simply copy
                             * the whole array buffer to this data section.
                             */

                            fits_write_col(outfile, TDBLCOMPLEX, i + 1, k + 1, 1,
                                           spell, ddata, &status);

                            if (status) {
                                const cpl_error_code code =
                                    cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                                                       status, fits_write_col,
                                                       "filename='%s', mode=%d",
                                                       filename, mode);
                                status = 0;
                                cpl_io_fits_delete_file(outfile, &status);
                                cpl_free(found);
                                cpl_free(nval);
                                cpl_free(nb);
                                return code;
                            }
                        }
                        cpl_free(ddata);
                        break;
                    }

                    default:
                        break;       /* Should never get here... */

                }

            }

        }

    }

    cpl_free(nval);
    cpl_free(found);
    cpl_free(nb);

    /* 
     * Overwrite the date of creation to the primary array.
     * The "DATE" record was already written, for reserving
     * that position in header and not having "DATE" as the
     * last record in header (DFS04595), but that wouldn't 
     * be exactly the creation date.
     */

    fits_movabs_hdu(outfile, 1, NULL, &status);
    fits_write_date(outfile, &status);
    if (status) {
        const cpl_error_code code =
            cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED,
                               status, fits_write_date,
                               "filename='%s', mode=%d",
                               filename, mode);
        status = 0;
        cpl_io_fits_delete_file(outfile, &status);
        return code;
    }

    cpl_io_fits_close_file(outfile, &status);

    if (status) {
        status = 0;
        cpl_io_fits_delete_file(outfile, &status);
        return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED, 
                                      "filename='%s', mode=%d",
                                      filename, mode);
    }

    return CPL_ERROR_NONE;

}


/*
 * Temporary driver function to dispatch only CPL_IO_APPEND mode requests
 * to the new _cpl_table_save(). Once _cpl_table_save() has been fully
 * validated, this workaround will be removed and a complete switch over
 * will take place!
 */

/**
 * @brief
 *   Save a @em cpl_table to a FITS file.
 *
 * @param table       Input table.
 * @param pheader     Primary header entries.
 * @param header      Table header entries.
 * @param filename    Name of output FITS file.
 * @param mode        Output mode.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>table</i> or <i>filename</i> are <tt>NULL</tt> pointers.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSUPPORTED_MODE</td>
 *       <td class="ecr">
 *         A saving mode other than <tt>CPL_IO_CREATE</tt> and
 *         <tt>CPL_IO_EXTEND</tt> was specified.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_FOUND</td>
 *       <td class="ecr">
 *         While <i>mode</i> is set to <tt>CPL_IO_EXTEND</tt>,
 *         a file named as specified in <i>filename</i> is not found.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_BAD_FILE_FORMAT</td>
 *       <td class="ecr">
 *         While <i>mode</i> is set to <tt>CPL_IO_EXTEND</tt>, the
 *         specified file is not in FITS format.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_CREATED</td>
 *       <td class="ecr">
 *         The FITS file could not be created.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_IO</td>
 *       <td class="ecr">
 *         The FITS file could only partially be created.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         Generic error condition, that should be reported to the
 *         CPL Team.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function can be used to convert a CPL table into a binary FITS
 * table extension. If the @em mode is set to @c CPL_IO_CREATE, a new
 * FITS file will be created containing an empty primary array, with
 * just one FITS table extension. An existing (and writable) FITS file
 * with the same name would be overwritten. If the @em mode flag is set
 * to @c CPL_IO_EXTEND, a new table extension would be appended to an
 * existing FITS file. If @em mode is set to @c CPL_IO_APPEND it is possible
 * to add rows to the last FITS table extension of the output FITS file.
 *
 * Note that the modes @c CPL_IO_EXTEND and @c CPL_IO_APPEND require that
 * the target file must be writable (and do not take for granted that a file
 * is writable just because it was created by the same application,
 * as this depends on the system @em umask).
 *
 * When using the mode @c CPL_IO_APPEND additional requirements must be
 * fulfilled, which are that the column properties like type, format, units,
 * etc. must match as the properties of the FITS table extension to which the
 * rows should be added exactly. In particular this means that both tables use
 * the same null value representation for integral type columns!
 *
 * Two property lists may be passed to this function, both
 * optionally. The first property list, @em pheader, is just used if
 * the @em mode is set to @c CPL_IO_CREATE, and it is assumed to
 * contain entries for the FITS file primary header. In @em pheader any
 * property name related to the FITS convention, as SIMPLE, BITPIX,
 * NAXIS, EXTEND, BLOCKED, and END, are ignored: such entries would
 * be written anyway to the primary header and set to some standard
 * values.
 *
 * If a @c NULL @em pheader is passed, the primary array would be created
 * with just such entries, that are mandatory in any regular FITS file.
 * The second property list, @em header, is assumed to contain entries
 * for the FITS table extension header. In this property list any
 * property name related to the FITS convention, as XTENSION, BITPIX,
 * NAXIS, PCOUNT, GCOUNT, and END, and to the table structure, as
 * TFIELDS, TTYPEi, TUNITi, TDISPi, TNULLi, TFORMi, would be ignored:
 * such entries are always computed internally, to guarantee their
 * consistency with the actual table structure. A DATE keyword
 * containing the date of table creation in ISO8601 format is also
 * added automatically.
 *
 * @note
 * - Invalid strings in columns of type @c CPL_TYPE_STRING are
 *   written to FITS as blanks.
 *
 * - Invalid values in columns of type @c CPL_TYPE_FLOAT or
 *   @c CPL_TYPE_DOUBLE will be written to the FITS file as a @c NaN.
 *
 * - Invalid values in columns of type @c CPL_TYPE_INT  and
 *   @c CPL_TYPE_LONG_LONG are the only ones that need a specific code to
 *   be explicitly assigned to them. This can be realised by calling the
 *   functions cpl_table_fill_invalid_int() and
 *   cpl_table_fill_invalid_long_long(), respectively, for each table
 *   column of type @c CPL_TYPE_INT and @c CPL_TYPE_LONG_LONG
 *   containing invalid values, just before saving the table to FITS. The
 *   numerical values identifying invalid integer column elements
 *   are written to the FITS keywords TNULLn (where n is the column
 *   sequence number). Beware that if valid column elements have
 *   the value identical to the chosen null-code, they will also
 *   be considered invalid in the FITS convention.
 *
 * - Using the mode @c CPL_IO_APPEND requires that the column properties of
 *   the table to be appended are compared to the column properties of the
 *   target FITS extension for each call, which introduces a certain overhead.
 *   This means that appending a single table row at a time may not be
 *   efficient and is not recommended. Rather than writing one row at a
 *   time one should write table chunks containing a suitable number or rows.
 */

cpl_error_code
cpl_table_save(const cpl_table *table, const cpl_propertylist *pheader,
               const cpl_propertylist *header, const char *filename,
               unsigned mode)
{
    const cpl_error_code code = mode & CPL_IO_APPEND
        ? _cpl_table_save(table, pheader, header, filename, mode)
        : _cpl_table_save_legacy(table, pheader, header, filename, mode);

    return code ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/**@}*/
