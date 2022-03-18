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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cpl_array_impl.h>

#include "cpl_memory.h"
#include <cpl_error_impl.h>
#include "cpl_tools.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>



/**
 * @defgroup cpl_array Arrays
 *
 * This module provides functions to create, destroy and use a @em cpl_array.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_array.h>
 * @endcode
 */

/**@{*/


/*
 *  The array type (private);
 */

struct _cpl_array_ {
    cpl_column *column;
};


/**
 * @brief
 *   Create a new array of given type.
 *
 * @param length    Number of elements in array.
 * @param type      Type of array
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * This function allocates memory for an array, its type is assigned,
 * and its number of elements is allocated. Only arrays of types
 * @c CPL_TYPE_INT, @c CPL_TYPE_FLOAT, @c CPL_TYPE_DOUBLE, 
 * @c CPL_TYPE_FLOAT_COMPLEX, @c CPL_TYPE_DOUBLE_COMPLEX and
 * @c CPL_TYPE_STRING, are supported. An error @c CPL_ERROR_INVALID_TYPE
 * is set in case other types are specified. All array elements are
 * initially flagged as invalid. If a negative length is specified,
 * an error @c CPL_ERROR_ILLEGAL_INPUT is set. Zero length arrays are
 * allowed.
 */

cpl_array *cpl_array_new(cpl_size length, cpl_type type)
{
    cpl_column *column;
    cpl_array  *array;


    switch(type) {
    case CPL_TYPE_INT:
        column = cpl_column_new_int(length);
        break;
    case CPL_TYPE_LONG:
        column = cpl_column_new_long(length);
        break;
    case CPL_TYPE_LONG_LONG:
        column = cpl_column_new_long_long(length);
        break;
    case CPL_TYPE_SIZE:
        column = cpl_column_new_cplsize(length);
        break;
    case CPL_TYPE_FLOAT:
        column = cpl_column_new_float(length);
        break;
    case CPL_TYPE_DOUBLE:
        column = cpl_column_new_double(length);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        column = cpl_column_new_float_complex(length);
        break;
    case CPL_TYPE_DOUBLE_COMPLEX:
        column = cpl_column_new_double_complex(length);
        break;
    case CPL_TYPE_STRING:
        column = cpl_column_new_string(length);
        break;
    default:
        (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "%s",
                                     cpl_type_get_name(type));
        return NULL;
    }

    if (column == NULL) {
        (void)cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;
}


/*
 * @brief
 *   Create a new complex array from a pair of non-complex arrays
 *
 * @param areal     Array of real values, double or float
 * @param aimag     Array of imaginary values, of matching type and length
 *
 * @return Pointer to the new column, or @c NULL in case of error.
 *
 * The function allocates memory for an array, and fills it with values
 * from the two input arrays, which must be of matching type and length.
 * For each row any invalid values are or'ed together to form the output value.
 */
cpl_array *cpl_array_new_complex_from_arrays(const cpl_array * areal,
                                             const cpl_array * aimag)
{
    cpl_array  *self  = NULL;
    cpl_column *cself;


    if (areal == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    } else if (aimag == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cself = cpl_column_new_complex_from_arrays(areal->column, aimag->column);

    if (cself == NULL) {
        (void)cpl_error_set_where_();
    } else {
        self = cpl_calloc(1, sizeof(cpl_array));
        self->column = cself;
    }

    return self;
}

/**
 * @brief
 *   Create a new @em integer array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * This function creates a new @em integer array that will encapsulate
 * the given data. Note that the size of the data buffer is not checked in
 * any way, and that the data values are all considered valid: invalid
 * values should be marked using the functions @c cpl_array_set_invalid()
 * The data array is not copied, so it should never be deallocated: to
 * deallocate it, the function @c cpl_array_delete() should be called
 * instead. Alternatively, the function @c cpl_array_unwrap() might be
 * used, and the data array deallocated afterwards. A zero or negative
 * length is illegal, and would cause an error @c CPL_ERROR_ILLEGAL_INPUT
 * to be set. An input @c NULL pointer would set an error
 * @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle arrays assume that an array data buffer
 *   is dynamically allocated: with a statically allocated data buffer
 *   any function implying memory handling (@c cpl_array_set_size(),
 *   @c cpl_array_delete(), etc.) would crash the program. This means
 *   that a static data buffer should never be passed to this function
 *   if memory handling is planned. In case of a static data buffer,
 *   only the @c cpl_array_unwrap() destructor can be used.
 */

cpl_array *cpl_array_wrap_int(int *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_int(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new @em long @em integer array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * This function creates a new @em long @em integer array that will encapsulate
 * the given data. Note that the size of the data buffer is not checked in
 * any way, and that the data values are all considered valid: invalid
 * values should be marked using the functions @c cpl_array_set_invalid()
 * The data array is not copied, so it should never be deallocated: to
 * deallocate it, the function @c cpl_array_delete() should be called
 * instead. Alternatively, the function @c cpl_array_unwrap() might be
 * used, and the data array deallocated afterwards. A zero or negative
 * length is illegal, and would cause an error @c CPL_ERROR_ILLEGAL_INPUT
 * to be set. An input @c NULL pointer would set an error
 * @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle arrays assume that an array data buffer
 *   is dynamically allocated: with a statically allocated data buffer
 *   any function implying memory handling (@c cpl_array_set_size(),
 *   @c cpl_array_delete(), etc.) would crash the program. This means
 *   that a static data buffer should never be passed to this function
 *   if memory handling is planned. In case of a static data buffer,
 *   only the @c cpl_array_unwrap() destructor can be used.
 */

cpl_array *cpl_array_wrap_long(long *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_long(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new @em long @em long @em integer array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * This function creates a new @em long @em long @em integer array that
 * will encapsulate the given data. Note that the size of the data buffer
 * is not checked in any way, and that the data values are all considered
 * valid: invalid values should be marked using the functions
 * @c cpl_array_set_invalid() The data array is not copied, so it should never
 * be deallocated: to deallocate it, the function @c cpl_array_delete()
 * should be called instead. Alternatively, the function @c cpl_array_unwrap()
 * might be used, and the data array deallocated afterwards. A zero or negative
 * length is illegal, and would cause an error @c CPL_ERROR_ILLEGAL_INPUT
 * to be set. An input @c NULL pointer would set an error
 * @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle arrays assume that an array data buffer
 *   is dynamically allocated: with a statically allocated data buffer
 *   any function implying memory handling (@c cpl_array_set_size(),
 *   @c cpl_array_delete(), etc.) would crash the program. This means
 *   that a static data buffer should never be passed to this function
 *   if memory handling is planned. In case of a static data buffer,
 *   only the @c cpl_array_unwrap() destructor can be used.
 */

cpl_array *cpl_array_wrap_long_long(long long *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_long_long(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new @em cpl_size array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * This function creates a new @em cpl_size array that will encapsulate
 * the given data. Note that the size of the data buffer is not checked
 * in any way, and that the data values are all considered valid: invalid
 * values should be marked using the functions @c cpl_array_set_invalid().
 * The data array is not copied, so it should never be deallocated: to
 * deallocate it, the function @c cpl_array_delete() should be called
 * instead. Alternatively, the function @c cpl_array_unwrap() might
 * be used, and the data array deallocated afterwards. A zero or negative
 * length is illegal, and would cause an error @c CPL_ERROR_ILLEGAL_INPUT
 * to be set. An input @c NULL pointer would set an error
 * @c CPL_ERROR_NULL_INPUT.
 *
 * @note
 *   Functions that handle arrays assume that an array data buffer
 *   is dynamically allocated: with a statically allocated data buffer
 *   any function implying memory handling (@c cpl_array_set_size(),
 *   @c cpl_array_delete(), etc.) would crash the program. This means
 *   that a static data buffer should never be passed to this function
 *   if memory handling is planned. In case of a static data buffer,
 *   only the @c cpl_array_unwrap() destructor can be used.
 */

cpl_array *cpl_array_wrap_cplsize(cpl_size *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_cplsize(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new @em float array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_array_wrap_int().
 */

cpl_array *cpl_array_wrap_float(float *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_float(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new @em float complex array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_array_wrap_int().
 */

cpl_array *cpl_array_wrap_float_complex(float complex *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_float_complex(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new @em double array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_array_wrap_int().
 */

cpl_array *cpl_array_wrap_double(double *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_double(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new @em double complex array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_array_wrap_int().
 */

cpl_array *cpl_array_wrap_double_complex(double complex *data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_double_complex(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Create a new character string array from existing data.
 *
 * @param data      Existing data buffer.
 * @param length    Number of elements in array.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * See documentation of function @c cpl_array_wrap_int().
 */

cpl_array *cpl_array_wrap_string(char **data, cpl_size length)
{

    cpl_column *column;
    cpl_array  *array;


    column = cpl_column_wrap_string(data, length);

    if (column == NULL) {
        cpl_error_set_where_();
        return NULL;
    }

    array = cpl_calloc(1, sizeof(cpl_array));
    array->column = column;

    return array;

}


/**
 * @brief
 *   Copy buffer of numerical data to a numerical array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the array is not numerical,
 *   a @c CPL_ERROR_INVALID_TYPE is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified array. If the type of the
 * accessed array is not @em CPL_TYPE_DOUBLE, the data values will be
 * truncated according to C casting rules. The size of the input data
 * buffer is not checked in any way, and the values are all considered
 * valid: invalid values should be marked using the functions
 * @c cpl_array_set_invalid(). If @em N is the length of the array,
 * the first @em N values of the input data buffer would be copied to
 * the column buffer. If the array had length zero, no values would
 * be copied.
 */

cpl_error_code cpl_array_copy_data(cpl_array *array, const double *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy buffer of complex data to a complex array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the array is not complex,
 *   a @c CPL_ERROR_INVALID_TYPE is returned. At any @c NULL input
 *   pointer a @c CPL_ERROR_NULL_INPUT would be returned.
 *
 * The input data are copied into the specified array. If the type of the
 * accessed array is not @em CPL_TYPE_DOUBLE, the data values will be
 * truncated according to C casting rules. The size of the input data
 * buffer is not checked in any way, and the values are all considered
 * valid: invalid values should be marked using the functions
 * @c cpl_array_set_invalid(). If @em N is the length of the array,
 * the first @em N values of the input data buffer would be copied to
 * the column buffer. If the array had length zero, no values would
 * be copied.
 */

cpl_error_code cpl_array_copy_data_complex(cpl_array *array, 
                                           const double complex *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_complex(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to an @em integer array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not
 *   of type @c CPL_TYPE_INT, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT would be
 *   returned.
 *
 * The input data are copied into the specified array. The size of the
 * input data buffer is not checked in any way, and the data values are all
 * considered valid: invalid values should be marked using the functions
 * @c cpl_array_set_invalid(). If @em N is the length of the array, the
 * first @em N values of the input data buffer would be copied to the
 * array buffer. If the array had length zero, no values would be copied.
 */

cpl_error_code cpl_array_copy_data_int(cpl_array *array, const int *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_int(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em long @em integer array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not
 *   of type @c CPL_TYPE_LONG, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT would be
 *   returned.
 *
 * The input data are copied into the specified array. The size of the
 * input data buffer is not checked in any way, and the data values are all
 * considered valid: invalid values should be marked using the functions
 * @c cpl_array_set_invalid(). If @em N is the length of the array, the
 * first @em N values of the input data buffer would be copied to the
 * array buffer. If the array had length zero, no values would be copied.
 */

cpl_error_code cpl_array_copy_data_long(cpl_array *array, const long *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_long(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em long @em long @em integer array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not
 *   of type @c CPL_TYPE_LONG_LONG, a @c CPL_ERROR_TYPE_MISMATCH is
 *   returned. At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT
 *   would be returned.
 *
 * The input data are copied into the specified array. The size of the
 * input data buffer is not checked in any way, and the data values are all
 * considered valid: invalid values should be marked using the functions
 * @c cpl_array_set_invalid(). If @em N is the length of the array, the
 * first @em N values of the input data buffer would be copied to the
 * array buffer. If the array had length zero, no values would be copied.
 */

cpl_error_code cpl_array_copy_data_long_long(cpl_array *array,
                                             const long long *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_long_long(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em cpl_size array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not
 *   of type @c CPL_TYPE_SIZE, a @c CPL_ERROR_TYPE_MISMATCH is
 *   returned. At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT
 *   would be returned.
 *
 * The input data are copied into the specified array. The size of the
 * input data buffer is not checked in any way, and the data values are all
 * considered valid: invalid values should be marked using the functions
 * @c cpl_array_set_invalid(). If @em N is the length of the array, the
 * first @em N values of the input data buffer would be copied to the
 * array buffer. If the array had length zero, no values would be copied.
 */

cpl_error_code cpl_array_copy_data_cplsize(cpl_array *array,
                                             const cpl_size *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_cplsize(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em float array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not of
 *   type @c CPL_TYPE_FLOAT, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT would be
 *   returned.
 *
 * See documentation of function @c cpl_array_copy_data_int().
 */

cpl_error_code cpl_array_copy_data_float(cpl_array *array, const float *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_float(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em float complex array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not of
 *   type @c CPL_TYPE_FLOAT_COMPLEX, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT would be
 *   returned.
 *
 * See documentation of function @c cpl_array_copy_data_int().
 */

cpl_error_code cpl_array_copy_data_float_complex(cpl_array *array,
                                                 const float complex *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_float_complex(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em double array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not of
 *   type @c CPL_TYPE_DOUBLE, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT would be
 *   returned.
 *
 * See documentation of function @c cpl_array_copy_data_int().
 */

cpl_error_code cpl_array_copy_data_double(cpl_array *array, const double *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_double(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em double complex array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not of
 *   type @c CPL_TYPE_DOUBLE_COMPLEX, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT would be
 *   returned.
 *
 * See documentation of function @c cpl_array_copy_data_int().
 */

cpl_error_code cpl_array_copy_data_double_complex(cpl_array *array,
                                                  const double complex *data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_double_complex(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Copy existing data to a @em string array.
 *
 * @param array    Existing array.
 * @param data     Existing data buffer.
 *
 * @return @c CPL_ERROR_NONE on success. If the input array is not of
 *   type @c CPL_TYPE_STRING, a @c CPL_ERROR_TYPE_MISMATCH is returned.
 *   At any @c NULL input pointer a @c CPL_ERROR_NULL_INPUT would be
 *   returned.
 *
 * See documentation of function @c cpl_array_copy_data_int().
 *
 * The input data are copied into the specified array. The size of the
 * input buffer is not checked in any way. The strings pointed by the input
 * buffer are all duplicated, while the strings contained in the array
 * are released before being overwritten.
 */

cpl_error_code cpl_array_copy_data_string(cpl_array *array, const char **data)
{

    if (data == NULL || array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_copy_data_string(array->column, data))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Delete an array.
 *
 * @param array  Array to be deleted.
 *
 * @return Nothing.
 *
 * This function deletes an array. If the input array is @c NULL,
 * nothing is done, and no error is set.
 */

void cpl_array_delete(cpl_array *array)
{

    if (array != NULL) {
        cpl_column_delete(array->column);
        cpl_free(array);
    }

}


/**
 * @brief
 *   Delete an array, without losing the data buffer.
 *
 * @param array  Array to be deleted.
 *
 * @return Pointer to the internal data buffer.
 *
 * This function deletes an array, but its data buffer is not destroyed.
 * Supposedly, the developer knows that the data are static, or the
 * developer holds the pointer to the data obtained with the functions
 * @c cpl_array_get_data_int(), @c cpl_array_get_data_float(), etc.
 * If the input array is @c NULL, nothing is done, and no error is set.
 */

void *cpl_array_unwrap(cpl_array *array)
{

    void       *d = NULL;


    if (array != NULL) {
        d = cpl_column_unwrap(array->column);
        cpl_free(array);
    }

    return d;

}


/**
 * @brief
 *   Get the length of an array.
 *
 * @param array  Input array.
 *
 * @return Length of array, or zero. The latter case can occur either
 * with an array having zero length, or if a @c NULL array is passed to
 * the function, but in the latter case a @c CPL_ERROR_NULL_INPUT is set.
 *
 * If the array is @em NULL, zero is returned.
 */

cpl_size cpl_array_get_size(const cpl_array *array)
{

    if (array)
        return cpl_column_get_size(array->column);

    cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return 0;

}


/**
 * @brief
 *   Resize an array.
 *
 * @param array       Input array.
 * @param new_length  New number of elements in array.
 *
 * @return @c CPL_ERROR_NONE on success. The new array size must not be
 *   negative, or a @c CPL_ERROR_ILLEGAL_INPUT is returned. The input
 *   array pointer should not be @c NULL, or a @c CPL_ERROR_NULL_INPUT
 *   is returned.
 *
 * Reallocate an array to a new number of elements. The contents of the
 * array data buffer will be unchanged up to the lesser of the new and
 * old sizes. If the array size is increased, the new array elements
 * are flagged as invalid. The pointer to data may change, therefore
 * pointers previously retrieved by calling @c cpl_array_get_data_int(),
 * @c cpl_array_get_data_string(), etc. should be discarded). Resizing
 * to zero is allowed, and would produce a zero-length array. In case
 * of failure, the old data buffer is left intact.
 *
 * If the array is @em NULL, zero is returned.
 */

cpl_error_code cpl_array_set_size(cpl_array *array, cpl_size new_length)
{
    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_size(array->column, new_length))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Get the type of an array.
 *
 * @param array  Input array
 *
 * @return Type of array, or @c CPL_TYPE_INVALID if a @c NULL array is
 *   passed to the function.
 *
 * If the array is @c NULL, @c CPL_ERROR_NULL_INPUT is set.
 */

cpl_type cpl_array_get_type(const cpl_array *array)
{

    if (array)
        return cpl_column_get_type(array->column);

    cpl_error_set("cpl_array_get_type", CPL_ERROR_NULL_INPUT);

    return CPL_TYPE_INVALID;

}


/**
 * @brief
 *   Check if an array contains at least one invalid element.
 *
 * @param array  Array to inquire.
 *
 * @return 1 if the array contains at least one invalid element, 0 if not,
 *   -1 in case of error.
 *
 * Check if there are invalid elements in an array. If the input array is a
 * @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is set.
 */

int cpl_array_has_invalid(const cpl_array *array)
{

    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    return cpl_column_has_invalid(array->column);

}


/**
 * @brief
 *   Check if an array contains at least one valid value.
 *
 * @param array  Array to inquire.
 *
 * @return 1 if the array contains at least one valid value, 0 if not
 *   -1 in case of error.
 *
 * Check if there are valid values in an array. If the input array is a
 * @c NULL pointer, a @c CPL_ERROR_NULL_INPUT is set.
 */

int cpl_array_has_valid(const cpl_array *array)
{

    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    return cpl_column_has_valid(array->column);

}


/**
 * @brief
 *   Count number of invalid elements in an array.
 *
 * @param array  Array to inquire.
 *
 * @return Number of invalid elements in an array. -1 is always returned
 *   in case of error.
 *
 * Count number of invalid elements in an array. If the array itself is
 * a @c NULL pointer, an error @c CPL_ERROR_NULL_INPUT is set.
 */

cpl_size cpl_array_count_invalid(const cpl_array *array)
{

    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    return cpl_column_count_invalid(array->column);

}


/**
 * @brief
 *   Check if an array element is valid.
 *
 * @param array   Pointer to array.
 * @param indx    Array element to examine.
 *
 * @return 1 if the array element is valid, 0 if invalid, -1 in case of
 *    error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>array</i> has zero length, or <i>indx</i> is
 *         outside the array boundaries.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Check if an array element is valid.
 */

int cpl_array_is_valid(const cpl_array *array, cpl_size indx)
{
    int          validity;
    cpl_errorstate prevstate;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    prevstate = cpl_errorstate_get();

    validity = cpl_column_is_invalid(array->column, indx) ? 0 : 1;

    if (!cpl_errorstate_is_equal(prevstate)) {
        cpl_error_set_where_();
        return -1;
    }

    return validity;
}


/**
 * @brief
 *   Get a pointer to @c integer array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c integer array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_INT, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of array data rules
 *   out any check performed by the array object interface, and may
 *   introduce inconsistencies between the array information maintained
 *   internally and the actual array data.
 */

int *cpl_array_get_data_int(cpl_array *array)
{
    /* Function body modified from below */
    int *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_int(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;
}


/**
 * @brief
 *   Get a pointer to constant @c integer array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @c integer array data. If @em array contains
 *   no data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_INT, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const int *cpl_array_get_data_int_const(const cpl_array *array)
{

    const int *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_int_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to @c long @c integer array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c long @c integer array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of array data rules
 *   out any check performed by the array object interface, and may
 *   introduce inconsistencies between the array information maintained
 *   internally and the actual array data.
 */

long *cpl_array_get_data_long(cpl_array *array)
{
    /* Function body modified from below */
    long *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_long(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;
}


/**
 * @brief
 *   Get a pointer to constant @c long @c integer array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @c long @c integer array data. If @em array
 *   contains no data (zero length), a @c NULL is returned. If @em array is
 *   a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const long *cpl_array_get_data_long_const(const cpl_array *array)
{

    const long *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_long_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to @c long @c long @c integer array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c long @c long @c integer array data. If @em array
 *   contains no data (zero length), a @c NULL is returned. If @em array is
 *   a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_LONG_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of array data rules
 *   out any check performed by the array object interface, and may
 *   introduce inconsistencies between the array information maintained
 *   internally and the actual array data.
 */

long long *cpl_array_get_data_long_long(cpl_array *array)
{
    /* Function body modified from below */
    long long *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_long_long(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;
}


/**
 * @brief
 *   Get a pointer to constant @c long @c long @c integer array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @c long @c long @c integer array data. If
 *  @em array contains no data (zero length), a @c NULL is returned. If
 *   @em array is a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_LONG_LONG, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const long long *cpl_array_get_data_long_long_const(const cpl_array *array)
{

    const long long *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_long_long_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to @c cpl_size array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c cpl_size array data. If @em array contains
 *   no data (zero length), a @c NULL is returned. If @em array is
 *   a @c NULL, a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_SIZE, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * @note
 *   Use at your own risk: direct manipulation of array data rules
 *   out any check performed by the array object interface, and may
 *   introduce inconsistencies between the array information maintained
 *   internally and the actual array data.
 */

cpl_size *cpl_array_get_data_cplsize(cpl_array *array)
{
    /* Function body modified from below */
    cpl_size *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_cplsize(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;
}


/**
 * @brief
 *   Get a pointer to constant @c cpl_size array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @c cpl_size array data. If @em array contains
 *  no data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *  a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_SIZE, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const cpl_size *cpl_array_get_data_cplsize_const(const cpl_array *array)
{

    const cpl_size *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_cplsize_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to @c float array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c float array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_FLOAT, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int().
 */

float *cpl_array_get_data_float(cpl_array *array)
{
    /* Function body modified from below */
    float *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_float(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;
}


/**
 * @brief
 *   Get a pointer to constant @em float array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @em float array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_FLOAT, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int_const().
 */

const float *cpl_array_get_data_float_const(const cpl_array *array)
{

    const float *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_float_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to @c float complex array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c float complex array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_FLOAT_COMPLEX, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int().
 */

float complex *cpl_array_get_data_float_complex(cpl_array *array)
{
    /* Function body modified from below */
    float complex *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }
    
    data = cpl_column_get_data_float_complex(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;
}


/**
 * @brief
 *   Get a pointer to constant @em float complex array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @em float complex array data. If @em array 
 *   contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_FLOAT_COMPLEX, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int_const().
 */

const float complex *
cpl_array_get_data_float_complex_const(const cpl_array *array)
{

    const float complex *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }
    
    data = cpl_column_get_data_float_complex_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to @c double array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c double array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_DOUBLE, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int().
 */

double *cpl_array_get_data_double(cpl_array *array)
{
    /* Function body modified from below */
    double *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_double(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to constant @em double array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @em double array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_DOUBLE, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int_const().
 */

const double *cpl_array_get_data_double_const(const cpl_array *array)
{

    const double *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_double_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to @c double complex array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @c double complex array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_DOUBLE_COMPLEX, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int().
 */

double complex *cpl_array_get_data_double_complex(cpl_array *array)
{
    /* Function body modified from below */
    double complex *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_double_complex(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Get a pointer to constant @em double complex array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @em double complex array data. If @em array 
 *   contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_DOUBLE_COMPLEX, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int_const().
 */

const double complex *
cpl_array_get_data_double_complex_const(const cpl_array *array)
{

    const double complex *data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_double_complex_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/*
 * This is a private function, to be used only by other CPL functions,
 * and listed in cpl_array_impl.h
 */

const cpl_column *cpl_array_get_column_const(const cpl_array *array)
{

    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return array->column;

}

cpl_column *cpl_array_get_column(cpl_array *array)
{

    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return array->column;

}



/**
 * @internal
 * @brief Insert an existing cpl_column into the array, replacing any original
 * @param array  Array to be modified
 * @param column Column to insert
 *
 * @return @c CPL_ERROR_NONE on success. If @em array  is a @c NULL
 *   pointer a @c CPL_ERROR_NULL_INPUT is returned.
 * @note This is a private function, to be used only by other CPL functions,
 * and listed in cpl_array_impl.h
 *
 * FIXME: May column be NULL ?
 */

cpl_error_code cpl_array_set_column(cpl_array *array, cpl_column *column)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (array->column)
        cpl_column_delete(array->column);

    array->column = column;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get a pointer to @em string array data.
 *
 * @param array  Array to get the data from.
 *
 * @return Pointer to @em string array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_STRING, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int().
 */

char **cpl_array_get_data_string(cpl_array *array)
{
    /* Function body modified from below */
    char **data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_string(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;
}


/**
 * @brief
 *   Get a pointer to constant @em string array data.
 *
 * @param array  Constant array to get the data from.
 *
 * @return Pointer to constant @em string array data. If @em array contains no
 *   data (zero length), a @c NULL is returned. If @em array is a @c NULL,
 *   a @c NULL is returned, and an error is set.
 *
 * If the array is not of type @c CPL_TYPE_STRING, a
 * @c CPL_ERROR_TYPE_MISMATCH is set.
 *
 * See documentation of function cpl_array_get_data_int().
 */

const char **cpl_array_get_data_string_const(const cpl_array *array)
{

    const char **data = NULL;


    if (array == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_data_string_const(array->column);

    if (data == NULL)
        (void)cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a numerical array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Value read. In case of an invalid array element, or in
 *   case of error, 0.0 is returned.
 *
 * Read a value from a numerical array. A @c CPL_ERROR_NULL_INPUT is set in
 * case @em array is a @c NULL pointer. A @c CPL_ERROR_INVALID_TYPE is set
 * in case a non-numerical array is accessed. @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 * is set if the @em indx is outside the array range. Indexes are
 * counted starting from 0. If the input array has length zero,
 * @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. The @em null
 * flag is used to indicate whether the accessed array element is
 * valid (0) or invalid (1). The null flag also signals an error
 * condition (-1). The @em null argument can be left to @c NULL.
 */

double cpl_array_get(const cpl_array *array, cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    double data;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return 0.0;
    }

    data = cpl_column_get(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a complex array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Value read. In case of an invalid array element, or in
 *   case of error, 0.0 is returned.
 *
 * Read a value from a complex array. A @c CPL_ERROR_NULL_INPUT is set in
 * case @em array is a @c NULL pointer. A @c CPL_ERROR_INVALID_TYPE is set
 * in case a non-complex array is accessed. @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 * is set if the @em indx is outside the array range. Indexes are
 * counted starting from 0. If the input array has length zero,
 * @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. The @em null
 * flag is used to indicate whether the accessed array element is
 * valid (0) or invalid (1). The null flag also signals an error
 * condition (-1). The @em null argument can be left to @c NULL.
 */

double complex cpl_array_get_complex(const cpl_array *array, 
                                     cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    double complex data;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return 0.0;
    }

    data = cpl_column_get_complex(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from an @em integer array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Integer value read. In case of an invalid array element, or in
 *   case of error, 0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_INT. If @em array is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the array is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input array has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * array element is valid (0) or invalid (1). The null flag also signals
 * an error condition (-1).
 */

int cpl_array_get_int(const cpl_array *array, cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    int data = 0;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return data;
    }

    data = cpl_column_get_int(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a @em long @em integer array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Long integer value read. In case of an invalid array element, or in
 *   case of error, 0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_LONG. If @em array is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the array is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input array has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * array element is valid (0) or invalid (1). The null flag also signals
 * an error condition (-1).
 */

long cpl_array_get_long(const cpl_array *array, cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    long data = 0;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return data;
    }

    data = cpl_column_get_long(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a @em long @em long @em integer array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Long long integer value read. In case of an invalid array element,
 *   or in case of error, 0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_LONG_LONG. If @em array is
 * a @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the array is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input array has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * array element is valid (0) or invalid (1). The null flag also signals
 * an error condition (-1).
 */

long long cpl_array_get_long_long(const cpl_array *array, cpl_size indx,
                                  int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    long long data = 0;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return data;
    }

    data = cpl_column_get_long_long(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a @em cpl_size array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return The cpl_size value read. In case of an invalid array element,
 *   or in case of error, 0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_SIZE. If @em array is
 * a @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the array is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input array has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set. If the @em null
 * flag is a valid pointer, it is used to indicate whether the accessed
 * array element is valid (0) or invalid (1). The null flag also signals
 * an error condition (-1).
 */

cpl_size cpl_array_get_cplsize(const cpl_array *array, cpl_size indx,
                               int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_size data = 0;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return data;
    }

    data = cpl_column_get_cplsize(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a @em float array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Array value read. In case of an invalid array element, or in
 *   case of error, 0.0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_FLOAT. See the
 * documentation of the function cpl_array_get_int().
 */

float cpl_array_get_float(const cpl_array *array, cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    float data;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return 0.0;
    }

    data = cpl_column_get_float(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a @em float complex array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Array value read. In case of an invalid array element, or in
 *   case of error, 0.0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_FLOAT_COMPLEX. See the
 * documentation of the function cpl_array_get_int().
 */

float complex cpl_array_get_float_complex(const cpl_array *array, 
                                          cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    float complex data;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return 0.0;
    }

    data = cpl_column_get_float_complex(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a @em double array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Array value read. In case of an invalid array element, or in
 *   case of error, 0.0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_DOUBLE. See the
 * documentation of the function cpl_array_get_int().
 */

double cpl_array_get_double(const cpl_array *array, cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    double data;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return 0.0;
    }

    data = cpl_column_get_double(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a @em double complex array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 * @param null   Flag indicating null values, or error condition.
 *
 * @return Array value read. In case of an invalid array element, or in
 *   case of error, 0.0 is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_DOUBLE_COMPLEX. See the
 * documentation of the function cpl_array_get_int().
 */

double complex cpl_array_get_double_complex(const cpl_array *array,
                                            cpl_size indx, int *null)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    double complex data;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        if (null)
            *null = -1;
        return 0.0;
    }

    data = cpl_column_get_double_complex(array->column, indx, null);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Read a value from a string array.
 *
 * @param array  Array to be accessed.
 * @param indx   Position of element to be read.
 *
 * @return Character string read. In case of an invalid array element,
 *   or in case of error, a @c NULL pointer is returned.
 *
 * Read a value from an array of type @c CPL_TYPE_STRING. If @em array is a
 * @c NULL pointer a @c CPL_ERROR_NULL_INPUT is set. If the array is not
 * of the expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx
 * is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set.
 * Indexes are counted starting from 0. If the input array has length zero,
 * the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set.
 *
 * @note
 *   The returned string is a pointer to an array element, not its
 *   copy. Its manipulation will directly affect that array element,
 *   while changing that array element using @c cpl_array_set_string()
 *   will turn it into garbage. Therefore, if a real copy of a string
 *   array element is required, this function should be called as an
 *   argument of the function @c strdup().
 */

const char *cpl_array_get_string(const cpl_array *array, cpl_size indx)
{

    const char *data;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    data = cpl_column_get_string_const(array->column, indx);

    if (data == NULL)
        cpl_error_set_where_();

    return data;

}


/**
 * @brief
 *   Write a value to a numerical array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of numerical
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em indx is outside
 *   the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input array has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a numerical array element. The value is cast to the
 * accessed array type. The written value is automatically flagged as
 * valid. To invalidate an array value use @c cpl_array_set_invalid().
 * Array elements are counted starting from 0.
 */

cpl_error_code cpl_array_set(cpl_array *array, cpl_size indx, double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a complex array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of numerical
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em indx is outside
 *   the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input array has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a numerical array element. The value is cast to the
 * accessed array type. The written value is automatically flagged as
 * valid. To invalidate an array value use @c cpl_array_set_invalid().
 * Array elements are counted starting from 0.
 */

cpl_error_code cpl_array_set_complex(cpl_array *array, 
                                     cpl_size indx, double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_complex(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to an @em integer array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to an @em integer array element. The written value
 * is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting
 * from 0.
 */

cpl_error_code cpl_array_set_int(cpl_array *array, cpl_size indx, int value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_int(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em long @em integer array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em long @em integer array element. The written value
 * is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting
 * from 0.
 */

cpl_error_code cpl_array_set_long(cpl_array *array, cpl_size indx, long value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_long(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em long @em long @em integer array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em long @em long @em integer array element. The written
 * value is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting from 0.
 */

cpl_error_code cpl_array_set_long_long(cpl_array *array, cpl_size indx,
                                       long long value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_long_long(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em cpl_size array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em cpl_size array element. The written value
 * is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting from 0.
 */

cpl_error_code cpl_array_set_cplsize(cpl_array *array, cpl_size indx,
                                     cpl_size value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_cplsize(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em float array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em float array element. The written value
 * is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting
 * from 0.
 */

cpl_error_code cpl_array_set_float(cpl_array *array, cpl_size indx, float value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_float(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em float complex array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em float complex array element. The written value
 * is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting
 * from 0.
 */

cpl_error_code cpl_array_set_float_complex(cpl_array *array, 
                                           cpl_size indx, float complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_float_complex(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em double array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em double array element. The written value
 * is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting
 * from 0.
 */

cpl_error_code cpl_array_set_double(cpl_array *array, 
                                    cpl_size indx, double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_double(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a value to a @em double complex array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write value.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is set. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write a value to a @em double array element. The written value
 * is automatically flagged as valid. To invalidate an array value use
 * @c cpl_array_set_invalid(). Array elements are counted starting
 * from 0.
 */

cpl_error_code cpl_array_set_double_complex(cpl_array *array, cpl_size indx, 
                                            double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_double_complex(array->column, indx, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a character string to a string array element.
 *
 * @param array  Array to be accessed.
 * @param indx   Position where to write character string.
 * @param string Character string to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em indx is
 *   outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned.
 *   If the input array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Copy a character string to a @em string array element. The written
 * value can also be a @c NULL pointer. Note that the input character
 * string is copied, therefore the original can be modified without
 * affecting the column content. To "plug" a character string directly
 * into an array element, use the function @c cpl_array_get_data_string().
 * Array elements are counted starting from zero.
 */

cpl_error_code cpl_array_set_string(cpl_array *array,
                                    cpl_size indx, const char *string)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_string(array->column, indx, string))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Invalidate an array element
 *
 * @param array  Array to be accessed
 * @param indx   Position of element to invalidate
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If @em indx is outside the
 *   array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is set. If the input
 *   array has length 0, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always set.
 *
 * In the case of a string array, the string is set free and its
 * pointer is set to @c NULL; for other data types, the corresponding
 * element of the null flags buffer is flagged. Array elements are
 * counted starting from zero.
 */

cpl_error_code cpl_array_set_invalid(cpl_array *array, cpl_size indx)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_set_invalid(array->column, indx))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a numerical array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of numerical
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em start is outside
 *   the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input array has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write the same value to a numerical array segment. The value is cast to
 * the accessed array type. The written values are automatically flagged as
 * valid. To invalidate an array interval use @c cpl_array_fill_window_invalid().
 */

cpl_error_code cpl_array_fill_window(cpl_array *array, cpl_size start, 
                                     cpl_size count, double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a complex array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of numerical
 *   type, a @c CPL_ERROR_INVALID_TYPE is returned. If @em start is outside
 *   the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If
 *   the input array has length zero, the @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned.
 *
 * Write the same value to a complex array segment. The value is cast to
 * the accessed array type. The written values are automatically flagged as
 * valid. To invalidate an array interval use @c cpl_array_fill_window_invalid().
 */

cpl_error_code cpl_array_fill_window_complex(cpl_array *array, 
                                             cpl_size start, cpl_size count, 
                                             double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_complex(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within an @em integer array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to an @em integer array segment. The written
 * values are automatically flagged as valid. To invalidate an array
 * interval use @c cpl_array_fill_window_invalid(). The @em count argument
 * can go beyond the array end, and in that case the specified @em value
 * will be written just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_int(cpl_array *array,
                                         cpl_size start, cpl_size count, 
                                         int value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_int(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a @em long @em integer array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em long @em integer array segment. The written
 * values are automatically flagged as valid. To invalidate an array
 * interval use @c cpl_array_fill_window_invalid(). The @em count argument
 * can go beyond the array end, and in that case the specified @em value
 * will be written just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_long(cpl_array *array,
                                          cpl_size start, cpl_size count,
                                          long value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_long(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a @em long @em long @em integer array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em long @em long @em integer array segment. The
 * written values are automatically flagged as valid. To invalidate an array
 * interval use @c cpl_array_fill_window_invalid(). The @em count argument
 * can go beyond the array end, and in that case the specified @em value
 * will be written just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_long_long(cpl_array *array,
                                               cpl_size start, cpl_size count,
                                               long long value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_long_long(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a @em cpl_size array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em cpl_size array segment. The written values
 * are automatically flagged as valid. To invalidate an array interval use
 * @c cpl_array_fill_window_invalid(). The @em count argument can go beyond
 * the array end, and in that case the specified @em value will be written
 * just up to the end of the array. If @em count is zero, the array is not
 * modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_cplsize(cpl_array *array,
                                             cpl_size start, cpl_size count,
                                             cpl_size value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_cplsize(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a @em float array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em float array segment. The written
 * values are automatically flagged as valid. To invalidate an array
 * interval use @c cpl_array_fill_window_invalid(). The @em count argument
 * can go beyond the array end, and in that case the specified @em value
 * will be written just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_float(cpl_array *array,
                                           cpl_size start, cpl_size count, 
                                           float value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_float(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a @em float complex array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em float complex array segment. The written
 * values are automatically flagged as valid. To invalidate an array
 * interval use @c cpl_array_fill_window_invalid(). The @em count argument
 * can go beyond the array end, and in that case the specified @em value
 * will be written just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_float_complex(cpl_array *array, 
                                                   cpl_size start, 
                                                   cpl_size count, 
                                                   float complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_float_complex(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a @em double array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em double array segment. The written
 * values are automatically flagged as valid. To invalidate an array
 * interval use @c cpl_array_fill_window_invalid(). The @em count argument
 * can go beyond the array end, and in that case the specified @em value
 * will be written just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_double(cpl_array *array,
                                            cpl_size start, cpl_size count, 
                                            double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_double(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write the same value within a @em double complex array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Write the same value to a @em double complex array segment. The written
 * values are automatically flagged as valid. To invalidate an array
 * interval use @c cpl_array_fill_window_invalid(). The @em count argument
 * can go beyond the array end, and in that case the specified @em value
 * will be written just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_double_complex(cpl_array *array,
                                                    cpl_size start, 
                                                    cpl_size count,
                                                    double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_double_complex(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Write a string to a string array segment.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to begin write value.
 * @param count  Number of values to write.
 * @param value  Value to write.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If the array is not of the
 *   expected type, a @c CPL_ERROR_TYPE_MISMATCH is returned. If @em start
 *   is outside the array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is returned. If the input array has length zero, the error
 *   @c CPL_ERROR_ACCESS_OUT_OF_RANGE is always returned. If @em count
 *   is negative, a @c CPL_ERROR_ILLEGAL_INPUT is returned.
 *
 * Copy the same string to a string array segment. If the input string
 * is not a @c NULL pointer, it is duplicated for each accessed array
 * element. If the input string is @c NULL, this call is equivalent to
 * @c cpl_array_fill_window_invalid(). The @em count argument can go beyond
 * the array end, and in that case the specified @em value will be
 * copied just up to the end of the array. If @em count is zero,
 * the array is not modified and no error is set.
 */

cpl_error_code cpl_array_fill_window_string(cpl_array *array, 
                                            cpl_size start,
                                            cpl_size count, 
                                            const char *value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_string(array->column, start, count, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Set an array segment to NULL.
 *
 * @param array  Array to be accessed.
 * @param start  Position where to start writing NULLs.
 * @param count  Number of column elements to set to NULL.
 *
 * @return @c CPL_ERROR_NONE on success. If @em array is a @c NULL pointer
 *   a @c CPL_ERROR_NULL_INPUT is returned. If @em start is outside the
 *   array range, a @c CPL_ERROR_ACCESS_OUT_OF_RANGE is returned. If the
 *   input array has length zero, the error @c CPL_ERROR_ACCESS_OUT_OF_RANGE
 *   is always returned. If @em count is negative, a @c CPL_ERROR_ILLEGAL_INPUT
 *   is returned.
 *
 * Invalidate values contained in an array segment. The @em count argument
 * can go beyond the array end, and in that case the values will be
 * invalidated up to the end of the array. If @em count is zero, the
 * array is not modified and no error is set. In the case of a @em string
 * array, the invalidated strings are set free and their pointers are set
 * to @c NULL; for other data types, the corresponding elements are flagged
 * as invalid.
 */

cpl_error_code cpl_array_fill_window_invalid(cpl_array *array,
                                             cpl_size start, cpl_size count)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_fill_invalid(array->column, start, count))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Make a copy of an array.
 *
 * @param array  Array to be duplicated.
 *
 * @return Pointer to the new array, or @c NULL in case of error.
 *
 * If the input @em array is a @c NULL pointer, a @c CPL_ERROR_NULL_INPUT
 * is returned. Copy is "in depth": in the case of a @em string array,
 * also the string elements are duplicated.
 */

cpl_array *cpl_array_duplicate(const cpl_array *array)
{

    cpl_column  *column = NULL;
    cpl_array   *new_array;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (array->column)
        column = cpl_column_duplicate(array->column);

    new_array = cpl_array_new(cpl_array_get_size(array),
                              cpl_array_get_type(array));

    cpl_column_delete(new_array->column);
    new_array->column = column;

    return new_array;

}


/**
 * @brief
 *   Create an array from a section of another array.
 *
 * @param array  Input array
 * @param start  First element to be copied to new array.
 * @param count  Number of elements to be copied.
 *
 * @return Pointer to the new array, or @c NULL in case or error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input <i>array</i> has zero length, or <i>start</i> is
 *         outside the array boundaries.
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
 * A number of consecutive elements are copied from an input array to a
 * newly created array. If the sum of @em start and @em count goes beyond 
 * the end of the input array, elements are copied up to the end. 
 */

cpl_array *cpl_array_extract(const cpl_array *array, 
                             cpl_size start, cpl_size count)
{

    cpl_size     length    = cpl_array_get_size(array);
    cpl_array   *new_array = NULL;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (count > length - start)
        count = length - start;

    new_array = cpl_array_new(count, cpl_array_get_type(array));

    cpl_column_delete(new_array->column);

    new_array->column = cpl_column_extract(array->column, start, count);

    if (new_array->column == NULL) {
        cpl_error_set_where_();
        cpl_array_delete(new_array);
        new_array = NULL;
    }

    return new_array;
}


/**
 * @brief
 *   Cast a numeric array to a new numeric type array.
 *
 * @param array     Pointer to array.
 * @param type      Type of new array.
 *
 * @return New array.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified column is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified <i>type</i> is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A new array of the specified type is created, and the content of the
 * input numeric array is cast to the new type. If the input array type
 * is identical to the specified type the array is duplicated as is done
 * by the function @c cpl_array_duplicate(). 
 */

cpl_array *cpl_array_cast(const cpl_array *array, cpl_type type)
{

    cpl_array *new_array = NULL;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    new_array = cpl_calloc(1, sizeof(cpl_array));
    new_array->column = NULL;  // Paranoid...

    switch (type) {
        case CPL_TYPE_INT:
            new_array->column = cpl_column_cast_to_int(array->column);
            break;
        case CPL_TYPE_LONG:
            new_array->column = cpl_column_cast_to_long(array->column);
            break;
        case CPL_TYPE_LONG_LONG:
            new_array->column = cpl_column_cast_to_long_long(array->column);
            break;
        case CPL_TYPE_SIZE:
            new_array->column = cpl_column_cast_to_cplsize(array->column);
            break;
        case CPL_TYPE_FLOAT:
            new_array->column = cpl_column_cast_to_float(array->column);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            new_array->column = cpl_column_cast_to_float_complex(array->column);
            break;
        case CPL_TYPE_DOUBLE:
            new_array->column = cpl_column_cast_to_double(array->column);
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            new_array->column = cpl_column_cast_to_double_complex(array->column);
            break;
        default:
          cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
          break;
    }

    if (!(new_array->column)) {
        cpl_error_set_where_();
        cpl_free(new_array);
        new_array = NULL;
    }

    return new_array;
}


/**
 * @brief
 *   Insert a segment of new elements into array.
 *
 * @param array   Input array
 * @param start   Element where to insert the segment.
 * @param count   Length of the segment.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         <i>array</i> is a <tt>NULL</tt> pointer.
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
 * Insert a segment of empty (invalid) elements.
 * Setting @em start to a number greater than the array length is legal,
 * and has the effect of appending extra elements at the end of the array:
 * this is equivalent to expanding the array using @c cpl_array_set_size().
 * The input @em array may also have zero length. The pointers to array
 * data values may change, therefore pointers previously retrieved by
 * calling @c cpl_array_get_data_int(), @c cpl_array_get_data_string(),
 * etc., should be discarded. 
 */

cpl_error_code cpl_array_insert_window(cpl_array *array, 
                                       cpl_size start, cpl_size count)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_insert_segment(array->column, start, count))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Delete a segment of an array.
 *
 * @param array   Input array
 * @param start   First element to delete.
 * @param count   Number of elements to delete.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The input array has length zero, or <i>start</i> is
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
 * A portion of the array data is physically removed. The pointers to 
 * data may change, therefore pointers previously retrieved by calling
 * @c cpl_array_get_data_int(), @c cpl_array_get_data_string(), etc.,
 * should be discarded. The specified segment can extend beyond the end 
 * of the array, and in that case elements will be removed up to the end 
 * of the array.
 */

cpl_error_code cpl_array_erase_window(cpl_array *array, 
                                      cpl_size start, cpl_size count)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (count > cpl_array_get_size(array) - start)
        count = cpl_array_get_size(array) - start;

    if (cpl_column_erase_segment(array->column, start, count))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Merge two arrays.
 *
 * @param target_array Target array.
 * @param insert_array Array to be inserted in the target array.
 * @param start        Element where to insert the insert array.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input array is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         <i>start</i> is negative.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The input arrays do not have the same type.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The input arrays must have the same type. Data from the @em insert_array 
 * are duplicated and inserted at the specified position of the 
 * @em target_array. If the specified @em start is not less than the 
 * target array length, the second array will be appended to the target 
 * array. The pointers to array data in the target array may change, 
 * therefore pointers previously retrieved by calling 
 * @c cpl_array_get_data_int(), @c cpl_array_get_data_string(), etc., 
 * should be discarded.
 */

cpl_error_code cpl_array_insert(cpl_array *target_array,
                                const cpl_array *insert_array, cpl_size start)
{

    if (target_array == NULL || insert_array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_merge(target_array->column, insert_array->column, start))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Add the values of two numeric or complex arrays.
 *
 * @param to_array   Target array.
 * @param from_array Source array.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The input arrays have different sizes.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified array is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The arrays are summed element by element, and the result of the sum is
 * stored in the target array. The arrays' types may differ, and in that
 * case the operation would be performed using the standard C upcasting
 * rules, with a final cast of the result to the target array type.
 * Invalid elements are propagated consistently: if either or both members
 * of the sum are invalid, the result will be invalid too. Underflows and
 * overflows are ignored.
 */

cpl_error_code cpl_array_add(cpl_array *to_array, 
                             const cpl_array *from_array)
{

    if (to_array == NULL || from_array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_add(to_array->column, from_array->column))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Subtract the values of two numeric or complex arrays.
 *
 * @param to_array   Target array.
 * @param from_array Source array.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The input arrays have different sizes.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified array is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The arrays are subtracted element by element, and the result is
 * stored in the target array. See the documentation of the function
 * @c cpl_array_add() for further details.
 */

cpl_error_code cpl_array_subtract(cpl_array *to_array, 
                                  const cpl_array *from_array)
{

    if (to_array == NULL || from_array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_subtract(to_array->column, from_array->column))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Multiply the values of two numeric or complex arrays.
 *
 * @param to_array   Target array.
 * @param from_array Source array.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The input arrays have different sizes.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified array is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The arrays are multiplied element by element, and the result is
 * stored in the target array. See the documentation of the function
 * @c cpl_array_add() for further details.
 */

cpl_error_code cpl_array_multiply(cpl_array *to_array, 
                                  const cpl_array *from_array)
{

    if (to_array == NULL || from_array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_multiply(to_array->column, from_array->column))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Divide the values of two numeric or complex arrays.
 *
 * @param to_array   Target array.
 * @param from_array Source array.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Any input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The input arrays have different sizes.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         Any specified array is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The arrays are divided element by element, and the result is
 * stored in the target array. See the documentation of the function
 * @c cpl_array_add() for further details.
 */

cpl_error_code cpl_array_divide(cpl_array *to_array, 
                                const cpl_array *from_array)
{

    if (to_array == NULL || from_array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_divide(to_array->column, from_array->column))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Add a constant value to a numerical array.
 *
 * @param array   Target array
 * @param value   Value to add.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input array is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_array_add_scalar(cpl_array *array, double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_add_scalar(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Add a constant complex value to a complex array.
 *
 * @param array   Target array
 * @param value   Value to add.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input array is not complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_array_add_scalar_complex(cpl_array *array, 
                                            double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_add_scalar_complex(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Subtract a constant value from a numerical array.
 *
 * @param array   Target array
 * @param value   Value to subtract.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input array is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_array_subtract_scalar(cpl_array *array, double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_subtract_scalar(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Subtract a constant complex value from a complex array.
 *
 * @param array   Target array
 * @param value   Value to subtract.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input array is not complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_array_subtract_scalar_complex(cpl_array *array, 
                                                 double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_subtract_scalar_complex(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Multiply a numerical array by a constant value.
 *
 * @param array   Target array
 * @param value   Factor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input array is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_array_multiply_scalar(cpl_array *array, double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_multiply_scalar(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Multiply a complex array by a constant complex value.
 *
 * @param array   Target array
 * @param value   Factor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input array is not complex.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * are not modified by this operation.
 */

cpl_error_code cpl_array_multiply_scalar_complex(cpl_array *array, 
                                                 double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_multiply_scalar_complex(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Divide a numerical array by a constant value.
 *
 * @param array   Target array
 * @param value   Divisor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DIVISION_BY_ZERO</td>
 *       <td class="ecr">
 *         The input <i>value</i> is zero.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * not modified by this operation.
 */

cpl_error_code cpl_array_divide_scalar(cpl_array *array, double value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_divide_scalar(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Divide a complex array by a constant complex value.
 *
 * @param array   Target array
 * @param value   Divisor.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The input <i>array</i> is not complex.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DIVISION_BY_ZERO</td>
 *       <td class="ecr">
 *         The input <i>value</i> is zero.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The operation is always performed in double precision, with a final
 * cast of the result to the target array type. Invalid elements are
 * not modified by this operation.
 */

cpl_error_code cpl_array_divide_scalar_complex(cpl_array *array, 
                                               double complex value)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_divide_scalar_complex(array->column, value))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the power of numerical array elements.
 *
 * @param array    Pointer to numerical array.
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
 *         Input <i>array</i> is <tt>NULL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each array element is replaced by its power to the specified exponent.
 * Each column element is replaced by its power to the specified exponent.
 * For float and float complex the operation is performed in single precision,
 * otherwise it is performed in double precision and then rounded if the column
 * is of an integer type. Results that would or do cause domain errors or
 * overflow are marked as invalid.
 */

cpl_error_code cpl_array_power(cpl_array *array, double exponent)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_power(array->column, exponent)
        ? cpl_error_set_where_()
        : CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the complex power of complex, numerical array elements.
 *
 * @param array    Pointer to numerical array.
 * @param exponent Constant exponent.
 *
 * @return @c CPL_ERROR_NONE on success.
 * @see cpl_array_power()
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is <tt>NULL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each array element is replaced by its power to the specified exponent.
 * Each column element is replaced by its power to the specified exponent.
 * For float and float complex the operation is performed in single precision,
 * otherwise it is performed in double precision and then rounded if the column
 * is of an integer type. Results that would or do cause domain errors or
 * overflow are marked as invalid.
 */

cpl_error_code cpl_array_power_complex(cpl_array *array,
                                       double complex exponent)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    return cpl_column_power_complex(array->column, exponent)
        ? cpl_error_set_where_()
        : CPL_ERROR_NONE;
}


/**
 * @brief
 *   Compute the logarithm of array elements.
 *
 * @param array   Pointer to array.
 * @param base    Logarithm base.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical or complex.
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
 * Each array element is replaced by its logarithm in the specified base.
 * The operation is always performed in double precision, with a final
 * cast of the result to the array type. Invalid elements are not
 * modified by this operation, but zero or negative elements are
 * invalidated by this operation. In case of complex numbers, values
 * very close to the origin may cause an overflow.
 */

cpl_error_code cpl_array_logarithm(cpl_array *array, double base)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_logarithm(array->column, base))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the exponential of array elements.
 *
 * @param array   Pointer to array.
 * @param base    Exponential base.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical or complex.
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
 * cast of the result to the array type. Invalid elements are not 
 * modified by this operation.
 */

cpl_error_code cpl_array_exponential(cpl_array *array, double base)
{

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_exponential(array->column, base))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the absolute value of array elements.
 *
 * @param array   Pointer to array.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each array element is replaced by its absolute value.
 * Invalid elements are not modified by this operation.
 * If the array is complex, its type will be turned to 
 * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
 * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
 * and any pointer retrieved by calling @c cpl_array_get_data_float(), 
 * @c cpl_array_get_data_double_complex(), etc., should be discarded.
 */

cpl_error_code cpl_array_abs(cpl_array *array)
{

    cpl_type    type;
    cpl_column *column;

    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    type = cpl_array_get_type(array);

    if (type & CPL_TYPE_COMPLEX) {
        column = cpl_column_absolute_complex(array->column);
        if (column)
            cpl_array_set_column(array, column);
        else
            return cpl_error_set_where_();
    }
    else {
        if (cpl_column_absolute(array->column))
            return cpl_error_set_where_();
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Compute the phase angle value of array elements.
 *
 * @param array   Pointer to array.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Each array element is replaced by its phase angle value.
 * The phase angle will be in the range of [-pi,pi].
 * Invalid elements are not modified by this operation.
 * If the array is complex, its type will be turned to 
 * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
 * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
 * and any pointer retrieved by calling @c cpl_array_get_data_float(), 
 * @c cpl_array_get_data_double_complex(), etc., should be discarded.
 */

cpl_error_code cpl_array_arg(cpl_array *array)
{

    cpl_type    type;
    cpl_column *column;
    
    if (array == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    type = cpl_array_get_type(array);

    if (type & CPL_TYPE_COMPLEX) {
        column = cpl_column_phase_complex(array->column);
        if (column)
            cpl_array_set_column(array, column);
        else
            return cpl_error_set_where_();
    }
    else {
        int     length = cpl_array_get_size(array);

        if (length) {
            switch (type) {
                case CPL_TYPE_FLOAT:
                {
                    float *fdata = cpl_array_get_data_float(array);

                    memset(fdata, 0.0, length * sizeof(float)); // keeps the NULLs
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double *ddata = cpl_array_get_data_double(array);

                    memset(ddata, 0.0, length * sizeof(double));
                    break;
                }
                default:
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                    break;
            }
        }
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Extract the real value of array elements.
 *
 * @param array   Pointer to array.
 *
 * @return New array with real part of input array elements.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A new array is created with the real part of all input array
 * elements. If the input array is complex, the output type will be 
 * CPL_TYPE_FLOAT if input is CPL_TYPE_FLOAT_COMPLEX, and CPL_TYPE_DOUBLE
 * if input is CPL_TYPE_DOUBLE_COMPLEX).
 */

cpl_array *cpl_array_extract_real(const cpl_array *array)
{

    cpl_type    type;
    cpl_column *column;
    cpl_array  *new_array = NULL;

    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    type = cpl_array_get_type(array);

    if (type & CPL_TYPE_COMPLEX) {
        column = cpl_column_extract_real(array->column);
        if (column) {
            new_array = cpl_array_new(0, CPL_TYPE_FLOAT); // Irrelevant type
            cpl_array_set_column(new_array, column);
        }
        else {
            cpl_error_set_where_();
        }
    }
    else {
        new_array = cpl_array_duplicate(array);
        if (new_array == NULL)
            cpl_error_set_where_();
    }

    return new_array;

}


/**
 * @brief
 *   Extract the imaginary value of array elements.
 *
 * @param array   Pointer to array.
 *
 * @return New array with imaginary part of input array elements.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * A new array is created with the imaginary part of all input array
 * elements. If the input array is complex, the output type will be 
 * CPL_TYPE_FLOAT if input is CPL_TYPE_FLOAT_COMPLEX, and CPL_TYPE_DOUBLE
 * if input is CPL_TYPE_DOUBLE_COMPLEX).
 */

cpl_array *cpl_array_extract_imag(const cpl_array *array)
{

    cpl_type    type;
    cpl_column *column;
    cpl_array  *new_array = NULL;

    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    type = cpl_array_get_type(array);

    if (type & CPL_TYPE_COMPLEX) {
        column = cpl_column_extract_imag(array->column);
        if (column) {
            new_array = cpl_array_new(0, CPL_TYPE_FLOAT); // Irrelevant type
            cpl_array_set_column(new_array, column);
        }
        else {
            cpl_error_set_where_();
        }
    }
    else {
        new_array = cpl_array_duplicate(array);
        if (new_array == NULL)
            cpl_error_set_where_();
    }

    return new_array;

}


/**
 * @brief
 *   Compute the mean value of a numeric array.
 *
 * @param array   Input array.
 *
 * @return Mean value. In case of error, this is set to 0.0.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero, 
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array elements marked as invalid are excluded from the computation.
 * The array must contain at least one valid value. Arrays of strings
 * or complex are not allowed.
 */

double cpl_array_get_mean(const cpl_array *array)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    double mean;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    }

    mean = cpl_column_get_mean(array->column);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return mean;
}


/**
 * @brief
 *   Compute the mean value of a complex array.
 *
 * @param array   Input array.
 *
 * @return Mean value. In case of error, this is set to 0.0.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not complex.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero, 
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array elements marked as invalid are excluded from the computation.
 * The array must contain at least one valid value. Arrays of strings
 * or numerical are not allowed.
 */

double complex cpl_array_get_mean_complex(const cpl_array *array)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    double complex mean;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    }

    mean = cpl_column_get_mean_complex(array->column);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return mean;
}


/**
 * @brief
 *   Compute the standard deviation of a numeric array.
 *
 * @param array   Input array.
 *
 * @return Standard deviation. In case of error, this is set to 0.0.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero,
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array elements marked as invalid are excluded from the computation.
 * The array must contain at least one valid value. Arrays of strings
 * or complex are not allowed.
 */

double cpl_array_get_stdev(const cpl_array *array)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    double sigma;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    }

    sigma = cpl_column_get_stdev(array->column);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return sigma;
}


/**
 * @brief
 *   Compute the median of a numeric array.
 *
 * @param array   Input array.
 *
 * @return Median. In case of error, this is set to 0.0.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero,
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array elements marked as invalid are excluded from the computation.
 * The array must contain at least one valid value. Arrays of strings
 * or complex are not allowed.
 */

double cpl_array_get_median(const cpl_array *array)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    double median;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    }

    median = cpl_column_get_median(array->column);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return median;
}


/**
 * @brief
 *   Get maximum value in a numerical array.
 *
 * @param array   Input array.
 *
 * @return Maximum value. In case of error, this is set to 0.0.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero,
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array elements marked as invalid are excluded from the search.
 * The array must contain at least one valid value. Arrays of strings 
 * or complex are not allowed.
 */

double cpl_array_get_max(const cpl_array *array)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    double max;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    }

    max = cpl_column_get_max(array->column);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return max;
}


/**
 * @brief
 *   Get minimum value in a numerical array.
 *
 * @param array   Input array.
 *
 * @return Minimum value. In case of error, this is set to 0.0.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero,
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array elements marked as invalid are excluded from the search.
 * The array must contain at least one valid value. Arrays of strings
 * or complex are not allowed.
 */

double cpl_array_get_min(const cpl_array *array)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    double min;


    if (array == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0.0;
    }

    min = cpl_column_get_min(array->column);

    if (!cpl_errorstate_is_equal(prestate))
        cpl_error_set_where_();

    return min;
}


/**
 * @brief
 *   Get position of maximum in a numerical array.
 *
 * @param array   Pointer to array.
 * @param indx    Returned position of maximum value.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> or <i>indx</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero,
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array values marked as invalid are excluded from the search.
 * The @em indx argument will be assigned the position of the maximum
 * value. Indexes are counted starting from 0. If more than one array
 * element correspond to the max value, the position with the lowest
 * indx is returned. In case of error, @em indx is set to zero.
 * Arrays of strings or complex are not allowed.
 */

cpl_error_code cpl_array_get_maxpos(const cpl_array *array, cpl_size *indx)
{

    if (array == NULL || indx == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_get_maxpos(array->column, indx))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get position of minimum in a numerical array.
 *
 * @param array   Pointer to array.
 * @param indx    Returned position of minimum value.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         Input <i>array</i> or <i>indx</i> is <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         The specified <i>array</i> is not numerical.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The specified <i>array</i> has either size zero,
 *         or all its elements are invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * Array values marked as invalid are excluded from the search.
 * The @em indx argument will be assigned the position of the minimum
 * value. Indexes are counted starting from 0. If more than one array
 * element correspond to the min value, the position with the lowest
 * indx is returned. In case of error, @em indx is set to zero.
 * Arrays of strings or complex are not allowed.
 */

cpl_error_code cpl_array_get_minpos(const cpl_array *array, cpl_size *indx)
{

    if (array == NULL || indx == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_column_get_minpos(array->column, indx))
        return cpl_error_set_where_();

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Describe the structure and the contents of an array.
 *
 * @param array    Pointer to array.
 * @param stream   The output stream
 *
 * @return Nothing.
 *
 * This function is mainly intended for debug purposes. Some information
 * about the structure of an array and its contents is printed to terminal:
 *
 * - Data type of the array
 * - Number of elements
 * - Number of invalid elements
 *
 * If the specified stream is @c NULL, it is set to @em stdout. The function
 * used for printing is the standard C @c fprintf().
 */

void cpl_array_dump_structure(const cpl_array *array, FILE *stream)
{

    if (stream == NULL)
        stream = stdout;

    if (array == NULL) {
        fprintf(stream, "NULL array\n\n");
        return;
    }

    fprintf(stream, "Array with %" CPL_SIZE_FORMAT, 
            cpl_column_get_size(array->column));

    switch (cpl_column_get_type(array->column)) {
        case CPL_TYPE_INT:
            fprintf(stream, "integer ");
            break;
        case CPL_TYPE_LONG:
            fprintf(stream, "long ");
            break;
        case CPL_TYPE_LONG_LONG:
            fprintf(stream, "long long");
            break;
        case CPL_TYPE_SIZE:
            fprintf(stream, "size_type ");
            break;
        case CPL_TYPE_FLOAT:
            fprintf(stream, "float ");
            break;
        case CPL_TYPE_DOUBLE:
            fprintf(stream, "double ");
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            fprintf(stream, "float complex ");
            break;
        case CPL_TYPE_DOUBLE_COMPLEX:
            fprintf(stream, "double complex ");
            break;
        case CPL_TYPE_STRING:
            fprintf(stream, "string ");
            break;
        default:
            fprintf(stream, "UNDEFINED ");
            break;
    }

    fprintf(stream, "elements, of which %" CPL_SIZE_FORMAT 
            " are flagged invalid.\n",
            cpl_column_count_invalid(array->column));
}


/**
 * @brief
 *   Print an array
 *
 * @param array    Pointer to array
 * @param start    First element to print
 * @param count    Number of elements to print
 * @param stream   The output stream
 *
 * @return Nothing.
 *
 * This function is mainly intended for debug purposes.
 * Array elements are counted from 0, and their sequence number is printed
 * at the left of each element. Invalid elements are represented
 * as a sequence of "-" as wide as the field occupied by the array.
 * Specifying a @em start beyond the array boundaries,
 * or a non-positive @em count, would generate a warning message, but no
 * error would be set. The specified number of elements to print may exceed
 * the array end, and in that case the array would be printed up to its
 * last element. If the specified stream is @c NULL, it is set to @em stdout.
 * The function used for printing is the standard C @c fprintf().
 */

void cpl_array_dump(const cpl_array *array, 
                    cpl_size start, cpl_size count, FILE *stream)
{

    cpl_size size;
    char **fields;
    char *row_field;
    int *field_size;
    int *label_len;
    int null;
    cpl_size nc = 1; /* recicling cpl_table_dump() */
    int found;
    cpl_size offset;
    int row;
    cpl_size i, j, k;
    cpl_size end;


    if (stream == NULL)
        stream = stdout;

    if (array == NULL) {
        fprintf(stream, "NULL array\n\n");
        return;
    }

    size = cpl_column_get_size(array->column);

    if (size == 0) {
        fprintf(stream, "Zero length array\n\n");
        return;
    }

    if (start < 0 || start >= size || count < 1) {
        fprintf(stream, "Illegal cpl_array_dump() arguments!\n");
        return;
    }

    if (count > size - start)
        count = size - start;

    end = start + count;

    row = cx_snprintf(NULL, 0, "% -" CPL_SIZE_FORMAT, end);
    row_field = cpl_malloc((row + 1) * sizeof(char));
    memset(row_field, ' ', row + 1);
    row_field[row] = '\0';

    label_len = cpl_calloc(nc, sizeof(int));
    field_size = cpl_calloc(nc, sizeof(int));
    fields = cpl_calloc(nc, sizeof(char *));

    for (j = 0; j < nc; j++) {
        label_len[j] = field_size[j] = strlen("Array");
        switch (cpl_column_get_type(array->column)) {
            case CPL_TYPE_INT:
            {
                for (i = start; i < end; i++) {
                    int inum = cpl_column_get_int(array->column, i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(array->column),
                                        inum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_LONG:
            {
                for (i = start; i < end; i++) {
                    long lnum = cpl_column_get_long(array->column, i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(array->column),
                                        lnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_LONG_LONG:
            {
                for (i = start; i < end; i++) {
                    long long lnum =
                            cpl_column_get_long_long(array->column, i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(array->column),
                                        lnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_SIZE:
            {
                for (i = start; i < end; i++) {
                    cpl_size snum = cpl_column_get_cplsize(array->column,
                                                           i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(array->column),
                                        snum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_FLOAT:
            {
                for (i = start; i < end; i++) {
                    float fnum = cpl_column_get_float(array->column, i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(array->column),
                                        fnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_DOUBLE:
            {
                for (i = start; i < end; i++) {
                    double dnum = cpl_column_get_double(array->column, i, &null);

                    if (null)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(array->column),
                                        dnum);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_FLOAT_COMPLEX:
            {
                for (i = start; i < end; i++) {
                    float complex cfnum =
                            cpl_column_get_float_complex(array->column,
                                                         i, &null);

                    if (null)
                        size = 4;
                    else
                        size = 3 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(array->column),
                                   crealf(cfnum))
                                 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(array->column),
                                   cimagf(cfnum));

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX:
            {
                for (i = start; i < end; i++) {
                    double complex cdnum =
                            cpl_column_get_double_complex(array->column,
                                                          i, &null);

                    if (null)
                        size = 4;
                    else
                        size = 3 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(array->column),
                                   creal(cdnum))
                                 + cx_snprintf(NULL, 0,
                                   cpl_column_get_format(array->column),
                                   cimag(cdnum));

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            case CPL_TYPE_STRING:
            {
                for (i = start; i < end; i++) {
                    char *string =
                            (char *)cpl_column_get_string(array->column, i);

                    if (string == NULL)
                        size = 4;
                    else
                        size = cx_snprintf(NULL, 0,
                                        cpl_column_get_format(array->column),
                                        string);

                    if (size > field_size[j])
                        field_size[j] = size;
                }
                break;
            }
            default:
                field_size[j] = 4;
                break;
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
        cx_snprintf(fields[j] + offset, label_len[j], "Array");
        for (i = label_len[j] + offset - 1; i < field_size[j]; i++)
            fields[j][i] = ' ';
        fields[j][field_size[j] - 1] = '\0';
        fprintf(stream, "%-*s ", label_len[j], fields[j]);
    }

    fprintf(stream, "\n\n");


    for (i = start; i < end; i++) {
        fprintf(stream, "%*" CPL_SIZE_FORMAT " ", row, i);
        for (j = 0; j < nc; j++) {
            switch (cpl_column_get_type(array->column)) {
                case CPL_TYPE_INT:
                {
                    int inum = cpl_column_get_int(array->column, i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else
                        cx_snprintf(fields[j], field_size[j],
                                 cpl_column_get_format(array->column), inum);
                    break;
                }
                case CPL_TYPE_LONG:
                {
                    long lnum = cpl_column_get_long(array->column, i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else
                        cx_snprintf(fields[j], field_size[j],
                                 cpl_column_get_format(array->column), lnum);
                    break;
                }
                case CPL_TYPE_LONG_LONG:
                {
                    long long lnum = cpl_column_get_long_long(array->column,
                                                              i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else
                        cx_snprintf(fields[j], field_size[j],
                                 cpl_column_get_format(array->column), lnum);
                    break;
                }
                case CPL_TYPE_SIZE:
                {
                    cpl_size snum = cpl_column_get_cplsize(array->column,
                                                           i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else
                        cx_snprintf(fields[j], field_size[j],
                                 cpl_column_get_format(array->column), snum);
                    break;
                }
                case CPL_TYPE_FLOAT:
                {
                    float fnum = cpl_column_get_float(array->column, i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else
                        cx_snprintf(fields[j], field_size[j],
                                 cpl_column_get_format(array->column), fnum);
                    break;
                }
                case CPL_TYPE_DOUBLE:
                {
                    double dnum = cpl_column_get_double(array->column, i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else
                        cx_snprintf(fields[j], field_size[j],
                                 cpl_column_get_format(array->column), dnum);
                    break;
                }
                case CPL_TYPE_FLOAT_COMPLEX:
                {
                    float complex cfnum =
                            cpl_column_get_float_complex(array->column,
                                                         i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else {
                        char *s = cpl_sprintf("(%s,%s)",
                                  cpl_column_get_format(array->column),
                                  cpl_column_get_format(array->column));

                        cx_snprintf(fields[j], field_size[j], s,
                                    crealf(cfnum), cimagf(cfnum));
                        cpl_free(s);
                    }
                    break;
                }
                case CPL_TYPE_DOUBLE_COMPLEX:
                {
                    double complex cdnum =
                            cpl_column_get_double_complex(array->column,
                                                          i, &null);

                    if (null) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else {
                        char *s = cpl_sprintf("(%s,%s)",
                                  cpl_column_get_format(array->column),
                                  cpl_column_get_format(array->column));

                        cx_snprintf(fields[j], field_size[j], s,
                                    creal(cdnum), cimag(cdnum));
                        cpl_free(s);
                    }
                    break;
                }
                case CPL_TYPE_STRING:
                {
                    char *string =
                            (char *)cpl_column_get_string(array->column, i);

                    if (!string) {
                        memset(fields[j], '-', field_size[j]);
                        fields[j][field_size[j] - 1] = '\0';
                    }
                    else
                        cx_snprintf(fields[j], field_size[j],
                                 cpl_column_get_format(array->column), string);
                    break;
                }
                default:
                {
                    cpl_array *_array = cpl_column_get_array(array->column, i);

                    if (!_array) {
                        memset(fields[j], '-', field_size[j]);
                    }
                    else
                        memset(fields[j], '+', field_size[j]);
                    fields[j][field_size[j] - 1] = '\0';
                    break;
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

}

/*
 * @internal
 * @brief Initialize the permutation array for the LU-decomposition
 * @param self    The integer array to initialize
 * @return Nothing.
 * @see cpl_matrix_decomp_lu()
 */
void cpl_array_init_perm(cpl_array* self)
{
    cpl_column  * cself = cpl_array_get_column(self);
    int         * iself = cpl_column_get_data_int(cself);
    const size_t  n     = (size_t)cpl_column_get_size(cself);

    assert(iself != NULL);

    for (size_t i = 0; i < n; i++) {
        iself[i] = (int)i;
    }

    cpl_column_unset_null_all(cself);
}

/**@}*/
