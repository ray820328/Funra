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

#include "cpl_array.h"
#include "cpl_math_const.h"

#include "cpl_test.h"

#include <complex.h>

/*----------------------------------------------------------------------------
                 Function prototypes
 ----------------------------------------------------------------------------*/

static void cpl_array_test_set_string(void);
static void cpl_array_test_power(void);
static void cpl_array_test_power_complex(void);
static void cpl_array_test_power_valid(cpl_array *, double, double, double)
    CPL_ATTR_NONNULL;
static void cpl_array_test_power_complex_one(cpl_array *, double,
                                         double complex,
                                         double complex) CPL_ATTR_NONNULL;
static void cpl_array_test_power_invalid(cpl_array *, double, double)
    CPL_ATTR_NONNULL;

static void cpl_array_test_new_complex(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/

int main(void)
{

    FILE        * stream;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    /* Testing begins here */

    cpl_test_nonnull( stream );

    cpl_array_dump_structure(NULL, stream);
    cpl_array_dump(NULL, 1, 1, stream);

    cpl_array_test_power();
    cpl_array_test_power_complex();

    cpl_array_test_set_string();

    cpl_array_test_new_complex();

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    /* End of tests */
    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the cpl_array function
  @see cpl_array_power

 */
/*----------------------------------------------------------------------------*/
static void cpl_array_test_power_complex(void)
{

    const cpl_type num_type[] = {CPL_TYPE_FLOAT_COMPLEX,
                                 CPL_TYPE_DOUBLE_COMPLEX};
    const size_t ntype = sizeof(num_type)/sizeof(num_type[0]);

    cpl_array *myarray;

    cpl_error_code code;

    code = cpl_array_power_complex(NULL, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    myarray = cpl_array_new(1, CPL_TYPE_STRING);

    code = cpl_array_power_complex(myarray, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

    code = cpl_array_set_string(myarray, 0, "I am a string");

    code = cpl_array_power_complex(myarray, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

    cpl_array_delete(myarray);

    for (size_t i = 0; i < ntype; i++) {
        const cpl_type mytype = num_type[i];

        cpl_msg_info(cpl_func, "Type: %s", cpl_type_get_name(mytype));

        myarray = cpl_array_new(1, mytype);
        cpl_test_nonnull(myarray);

        /* e^(i * pi) is -1, see man cpow() */
        cpl_array_test_power_complex_one(myarray, CPL_MATH_E,
                                         CPL_MATH_PI * I, -1.0);
        /* e^(i * pi/2) is i, see man cpow() */
        cpl_array_test_power_complex_one(myarray, CPL_MATH_E,
                                         CPL_MATH_PI_2 * I, I);
        cpl_array_delete(myarray);
    }

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the cpl_array function
  @see cpl_array_power

 */
/*----------------------------------------------------------------------------*/
static void cpl_array_test_power(void)
{

    const cpl_type num_type[] = {CPL_TYPE_INT,
                                 CPL_TYPE_LONG,
                                 CPL_TYPE_LONG_LONG,
                                 CPL_TYPE_SIZE,
                                 CPL_TYPE_FLOAT,
                                 CPL_TYPE_FLOAT_COMPLEX,
                                 CPL_TYPE_DOUBLE,
                                 CPL_TYPE_DOUBLE_COMPLEX};
    const size_t ntype = sizeof(num_type)/sizeof(num_type[0]);

    cpl_array *myarray;

    cpl_error_code code;

    code = cpl_array_power(NULL, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    myarray = cpl_array_new(1, CPL_TYPE_STRING);

    code = cpl_array_power(myarray, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

    code = cpl_array_set_string(myarray, 0, "I am a string");

    code = cpl_array_power(myarray, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

    cpl_array_delete(myarray);

    for (size_t i = 0; i < ntype; i++) {
        const cpl_type mytype = num_type[i];

        cpl_msg_info(cpl_func, "Type: %s", cpl_type_get_name(mytype));

        myarray = cpl_array_new(1, mytype);
        cpl_test_nonnull(myarray);

        /* 2^2 is 4, see man pow() */
        cpl_array_test_power_valid(myarray, 2.0, 2.0, 4.0);

        /* 0.5^2 is 0.25, see man pow() (rounded to zero for int types) */
        if (mytype == CPL_TYPE_FLOAT || mytype == CPL_TYPE_DOUBLE)
            cpl_array_test_power_valid(myarray, 0.5, 2.0, 0.25);
        else
            cpl_array_test_power_valid(myarray, 0.5, 2.0, 0.0);

        /* 8^(1/3) is 2, see man pow() */
        cpl_array_test_power_valid(myarray, 8.0, 1.0/3.0, 2.0);

        /* 16^(1/4) is 2, see man pow() */
        cpl_array_test_power_valid(myarray, 16.0, 0.25, 2.0);

        /* 0^0 is 1, see man pow() */
        cpl_array_test_power_valid(myarray, 0.0, 0.0, 1.0);

        /* 0^1 is 0, see man pow() */
        cpl_array_test_power_valid(myarray, 0.0, 1.0, 0.0);

        /* 0^-1 is invalid, see man pow() */
        cpl_array_test_power_invalid(myarray, 0.0, -1.0);

        /* 0^-0.5 is invalid, see man pow() */
        cpl_array_test_power_invalid(myarray, 0.0, -0.5);

        /* -2^0 is 1, see man pow() */
        cpl_array_test_power_valid(myarray, -2.0, 0.0, 1.0);

        /* 2^0 is 1, see man pow() */
        cpl_array_test_power_valid(myarray, 2.0, 0.0, 1.0);

        /* -2^1 is -2, see man pow() */
        cpl_array_test_power_valid(myarray, -2.0, 1.0, -2.0);

        /* -2^2 is 4, see man pow() */
        cpl_array_test_power_valid(myarray, -2.0, 2.0, 4.0);

        /* -0.5^2 is 0.25, see man pow() (rounded to zero for int types) */
        if (mytype == CPL_TYPE_FLOAT || mytype == CPL_TYPE_DOUBLE)
            cpl_array_test_power_valid(myarray, -0.5, 2.0, 0.25);
        else
            cpl_array_test_power_valid(myarray, -0.5, 2.0, 0.0);

        /* 256^-(1/4) is 1/4, see man pow()
           - (rounded to 1 for int types) */
        if (mytype & (CPL_TYPE_FLOAT | CPL_TYPE_DOUBLE)) {
            cpl_array_test_power_valid(myarray, 256.0, -0.25, 0.25);
        } else {
            cpl_array_test_power_valid(myarray, 256.0, -0.25, 0.0);
        }

        /* -1^2 is 1, see man pow() */
        cpl_array_test_power_valid(myarray, -1.0, 2.0, 1.0);

        /* -1^3 is -1, see man pow() */
        cpl_array_test_power_valid(myarray, -1.0, 3.0, -1.0);

        /* -1^-1 is -1, see man pow() */
        cpl_array_test_power_valid(myarray, -1.0, -1.0, -1.0);

        /* -1^0.5 is i or invalid (for non-complex), see man pow() */
        cpl_array_test_power_complex_one(myarray, -1.0, 0.5, 1.0 * I);

        /* -4^-0.5 is -0.5i or invalid (for non-complex), see man pow() */
        cpl_array_test_power_complex_one(myarray, -4.0, -0.5, -0.5 * I);

        /* -4^0.5 is 2i or invalid (for non-complex), see man pow() */
        cpl_array_test_power_complex_one(myarray, -4.0, 0.5, 2.0 * I);

        /* -4^0.25 is 1+i or invalid (for non-complex), see man pow() */
        cpl_array_test_power_complex_one(myarray, -4.0, 0.25, 1.0 + 1.0 * I);

        /* -4^-0.25 is 0.5-0.5i or invalid (for non-complex), see man pow() */
        cpl_array_test_power_complex_one(myarray, -4.0, -0.25, 0.5 - 0.5 * I);

        /* 1^-1 is 1, see man pow() */
        cpl_array_test_power_valid(myarray, 1.0, -1.0, 1.0);

        /* 1^-2 is 1, see man pow() */
        cpl_array_test_power_valid(myarray, 1.0, -2.0, 1.0);

        /* 1^-0.5 is 1, see man pow() */
        cpl_array_test_power_valid(myarray, 1.0, -0.5, 1.0);

        /* 2^-1 is 0.5, see man pow() */
        if (mytype == CPL_TYPE_FLOAT || mytype == CPL_TYPE_DOUBLE)
            cpl_array_test_power_valid(myarray, 2.0, -1.0, 0.5);
        else
            cpl_array_test_power_valid(myarray, 2.0, -1.0, 1.0);

        /* 4^-0.5 is 0.5, see man pow() */
        if (mytype == CPL_TYPE_FLOAT || mytype == CPL_TYPE_DOUBLE)
            cpl_array_test_power_valid(myarray, 4.0, -0.5, 0.5);
        else
            cpl_array_test_power_valid(myarray, 4.0, -0.5, 1.0);

        cpl_array_delete(myarray);
    }

    return;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the cpl_array_power for a single valid value, x^y
  @param    self   The array (of a numerical type) to work with
  @param    x      Base
  @param    y      Power
  @param    result Expected result
  @see cpl_array_power

 */
/*----------------------------------------------------------------------------*/
static void cpl_array_test_power_valid(cpl_array *self, double x, double y,
                                       double result)
{

    const cpl_type mytype = cpl_array_get_type(self);
    int            isvalid;
    cpl_error_code code;

    code = cpl_array_set(self, 0, x);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_array_power(self, y);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    isvalid = cpl_array_is_valid(self, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(isvalid, 1);

    if ((mytype & CPL_TYPE_COMPLEX) == 0) {
        int isnull;
        const double value = cpl_array_get(self, 0, &isnull);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_zero(isnull);
        cpl_test_abs(value, result, 0.0);
    }

}

/*----------------------------------------------------------------------------*/
static void cpl_array_test_power_complex_one(cpl_array *self, double x,
                                             double complex y,
                                             double complex result)
{

    const cpl_type mytype = cpl_array_get_type(self);
    int            isvalid;
    cpl_array*     acopy;
    cpl_error_code code;

    code = cpl_array_set(self, 0, x);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    acopy = cpl_array_duplicate(self);

    code = cpl_array_power(self, (double)y);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    isvalid = cpl_array_is_valid(self, 0);
    cpl_test_error(CPL_ERROR_NONE);

    code = cpl_array_power_complex(acopy, y);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    isvalid = cpl_array_is_valid(acopy, 0);
    cpl_test_error(CPL_ERROR_NONE);

    if ((mytype & CPL_TYPE_COMPLEX) == 0) {
        int isnull;
        /* cpl_array_get() returns zero, when the value is invalid,
           we would need to use cpl_array_get_data_float_const() to
           verify that the function did not actually modify the value */
        const double value = cpl_array_get(self, 0, &isnull);

        cpl_test_zero(isvalid);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(isnull, 1);
        cpl_test_abs(value, 0.0, 0.0);
        if (mytype == CPL_TYPE_FLOAT) {
            const float * fptr = cpl_array_get_data_float_const(self);
            cpl_test_abs(fptr[0], x, 0.0);
        }

        cpl_test_array_abs(self, acopy, 0.0);
    } else {
        int isnull, isnull2;
        const double complex value  = cpl_array_get_complex(self, 0, &isnull);
        const double complex value2 = cpl_array_get_complex(acopy, 0, &isnull2);

        cpl_test_eq(isvalid, 1);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_zero(isnull);
        cpl_test_zero(isnull2);
        if (!(y != (double)y))
            cpl_test_abs_complex(value, result, mytype & CPL_TYPE_FLOAT
                                 ? FLT_EPSILON : DBL_EPSILON);
        cpl_test_abs_complex(value2, result, mytype & CPL_TYPE_FLOAT
                             ? 2.0 * FLT_EPSILON : DBL_EPSILON);
    }
    cpl_array_delete(acopy);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the cpl_array_power for a single invalid value
  @param    self   The array (of a numerical type) to work with
  @param    x      Base
  @param    y      Power
  @see cpl_array_power

 */
/*----------------------------------------------------------------------------*/
static void cpl_array_test_power_invalid(cpl_array *self, double x, double y)
{
    const cpl_type mytype = cpl_array_get_type(self);
    int            isvalid;
    cpl_error_code code;

    code = cpl_array_set(self, 0, x);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_array_power(self, y);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    isvalid = cpl_array_is_valid(self, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(isvalid);

    code = cpl_array_power_complex(self, y);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    isvalid = cpl_array_is_valid(self, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(isvalid);

    if ((mytype & CPL_TYPE_COMPLEX) == 0) {
        int isnull;
        /* cpl_array_get() returns zero, when the value is invalid,
           we would need to use cpl_array_get_data_float_const() to
           verify that the function did not actually modify the value */
        const double value = cpl_array_get(self, 0, &isnull);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(isnull, 1);
        cpl_test_abs(value, 0.0, 0.0);
        if (mytype == CPL_TYPE_FLOAT) {
            const float * fptr = cpl_array_get_data_float_const(self);
            cpl_test_abs(fptr[0], x, 0.0);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the cpl_array_set_string() for out-of-bounds input
  @param    self   The array (of a numerical type) to work with
  @param    x      Base
  @param    y      Power
  @see cpl_array_power

 */
/*----------------------------------------------------------------------------*/
static void cpl_array_test_set_string(void)
{
    cpl_array    * self = cpl_array_new(1, CPL_TYPE_STRING);
    cpl_error_code code;

    cpl_test_nonnull(self);

    code = cpl_array_set_string(self, 2, "test");
    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    code = cpl_array_fill_window_string(self, 1, 2, "test");
    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    cpl_array_delete(self);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test cpl_array_new_complex_from_arrays()
  @see cpl_array_new_complex_from_arrays()

 */
/*----------------------------------------------------------------------------*/
static void cpl_array_test_new_complex(void)
{

    double darray[] = {1.0, 2.0};
    float  farray[] = {1.0, 2.0};
    cpl_array *null;
    cpl_array *aint    = cpl_array_new(1, CPL_TYPE_INT);
    cpl_array *adouble = cpl_array_new(1, CPL_TYPE_DOUBLE);
    cpl_array *afloat  = cpl_array_new(2, CPL_TYPE_FLOAT);
    cpl_array *acomp;

    cpl_test_nonnull(aint);
    cpl_test_nonnull(adouble);
    cpl_test_nonnull(afloat);

    null = cpl_array_new_complex_from_arrays(NULL, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(null);

    null = cpl_array_new_complex_from_arrays(NULL, adouble);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(null);

    null = cpl_array_new_complex_from_arrays(adouble, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(null);

    null = cpl_array_new_complex_from_arrays(aint, aint);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_null(null);

    null = cpl_array_new_complex_from_arrays(adouble, aint);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_null(null);

    null = cpl_array_new_complex_from_arrays(adouble, afloat);
    cpl_test_error(CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_test_null(null);

    acomp = cpl_array_new_complex_from_arrays(adouble, adouble);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(acomp);
    cpl_test_eq(cpl_array_get_size(acomp), cpl_array_get_size(adouble));
    cpl_test_eq(cpl_array_get_type(acomp),
                cpl_array_get_type(adouble) | CPL_TYPE_COMPLEX);
    cpl_test_eq(cpl_array_count_invalid(acomp),
                cpl_array_count_invalid(adouble));
    cpl_array_delete(acomp);

    acomp = cpl_array_new_complex_from_arrays(afloat, afloat);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(acomp);
    cpl_test_eq(cpl_array_get_size(acomp), cpl_array_get_size(afloat));
    cpl_test_eq(cpl_array_get_type(acomp),
                cpl_array_get_type(afloat) | CPL_TYPE_COMPLEX);
    cpl_test_eq(cpl_array_count_invalid(acomp),
                cpl_array_count_invalid(afloat));
    cpl_array_delete(acomp);

    cpl_array_delete(adouble);
    cpl_array_delete(afloat);
    cpl_array_delete(aint);

    adouble = cpl_array_wrap_double(darray, 2);
    afloat  = cpl_array_wrap_float(farray, 2);

    acomp = cpl_array_new_complex_from_arrays(adouble, adouble);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(acomp);
    cpl_test_eq(cpl_array_get_size(acomp), cpl_array_get_size(adouble));
    cpl_test_eq(cpl_array_get_type(acomp),
                cpl_array_get_type(adouble) | CPL_TYPE_COMPLEX);
    cpl_test_zero(cpl_array_count_invalid(acomp));
    cpl_array_delete(acomp);

    acomp = cpl_array_new_complex_from_arrays(afloat, afloat);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(acomp);
    cpl_test_eq(cpl_array_get_size(acomp), cpl_array_get_size(afloat));
    cpl_test_eq(cpl_array_get_type(acomp),
                cpl_array_get_type(afloat) | CPL_TYPE_COMPLEX);
    cpl_test_zero(cpl_array_count_invalid(acomp));
    cpl_array_delete(acomp);

    cpl_test_eq_ptr(cpl_array_unwrap(adouble), darray);
    cpl_test_eq_ptr(cpl_array_unwrap(afloat), farray);
}
