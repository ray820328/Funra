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

/* Ensure declaration of the complex functions */
#include <complex.h>

#include "cpl_table.h"

#include "cpl_math_const.h"
#include "cpl_test.h"
#include "cpl_memory.h"

#include <math.h>

/*-----------------------------------------------------------------------------
                        Defines
 -----------------------------------------------------------------------------*/

#define VERBOSE /* The actual verbosity is controlled with CPLs messaging */

#ifdef CPL_TEST_LARGE
#define NROWS_LARGE 25000000
#elif !defined NROWS_LARGE
#define NROWS_LARGE 2
#endif

#ifndef NROWS
#define NROWS 10
#endif

#define BASE "cpl_table-test"


/* 
 * Test for functions returning a generic pointer to data.
 *
 * r = variable to store returned pointer to data - expected non-NULL
 * f = function call
 * m = message
 */

#define test_data(r,f,m)                        \
    do {                                        \
        cpl_msg_info("test_data", "%s", m);     \
        r = f;                                  \
        cpl_assert(r != NULL);                  \
        cpl_test_error(CPL_ERROR_NONE);         \
    } while (0)                                 \


/*
 * Test for functions returning 0 on success.
 *
 * f = function call
 * m = message
 */

#define test(f,m)                               \
     do {                                       \
         cpl_msg_info("test", "%s", m);         \
         cpl_test_zero(f);                      \
         cpl_test_error(CPL_ERROR_NONE);        \
     } while (0)

/*
 * Test for expected failure in functions returning 0 on success.
 *
 * e = expected error code
 * f = function call
 * m = message
 */

#define test_failure(e,f,m)                     \
     do {                                       \
         cpl_msg_info("test_failure", "%s", m); \
         cpl_test_eq_error(f, e);               \
     } while (0)

/*
 * Test for functions returning an expected integer value.
 *
 * e = expected value
 * f = function call
 * m = message
 */

#define test_ivalue(e,f,m)                      \
     do {                                       \
         cpl_msg_info("test_ivalue", "%s", m);  \
         cpl_test_eq(f, e);                     \
         cpl_test_error(CPL_ERROR_NONE);        \
     } while (0)

/*
 * Test for functions returning an expected pointer value.
 *
 * e = expected value
 * f = function call
 * m = message
 */

#define test_pvalue(e,f,m)                      \
     do {                                       \
         cpl_msg_info("test_pvalue", "%s", m);  \
         cpl_test_eq_ptr(f, e);                 \
         cpl_test_error(CPL_ERROR_NONE);        \
     } while (0)

/*
 * Test for functions returning an expected floating point value.
 *
 * e = expected value
 * t = tolerance on expected value
 * f = function call
 * m = message
 */

#define test_fvalue(e,t,f,m)                    \
     do {                                       \
         cpl_msg_info("test_fvalue", "%s", m);  \
         cpl_test_abs(f, e, t);                 \
         cpl_test_error(CPL_ERROR_NONE);        \
     } while (0)

/*
 * Test for functions returning an expected complex value.
 *
 * e = expected value
 * t = tolerance on expected value
 * f = function call
 * m = message
 */

#define test_cvalue(e,t,f,m)                    \
     do {                                       \
         cpl_msg_info("test_cvalue", "%s", m);  \
         cpl_test_abs_complex(f, e, t);         \
         cpl_test_error(CPL_ERROR_NONE);        \
     } while (0)
  
/*
 * Test for functions returning an expected character string.
 *
 * e = expected value
 * f = function call
 * m = message
 */

#define test_svalue(e,f,m)                      \
     do {                                       \
         cpl_msg_info("test_svalue", "%s", m);  \
         cpl_test_eq_string(f, e);              \
         cpl_test_error(CPL_ERROR_NONE);        \
     } while (0)

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_table_test_zero_one(void);
static void cpl_table_test_large(cpl_size);
static int cpl_table_test_main(void);
static int cpl_table_test_rest(void);

static void cpl_table_test_20(cpl_table *);
static void cpl_table_test_21(cpl_table *);
static void cpl_table_test_22(cpl_table *);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/

int main(void)
{

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /*
     *  Testing begins here
     */

    cpl_table_test_zero_one();

    (void)cpl_table_test_main();
    (void)cpl_table_test_rest();

    cpl_table_test_large(NROWS_LARGE);

    return cpl_test_end(0);



}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the main part of the CPL table functions
  @return   Zero iff successful
 */
/*----------------------------------------------------------------------------*/
static int cpl_table_test_main(void)
{

    int                nrows = NROWS;
    int                i, j, k, null;
    char               message[80];

    int               *iArray;
    long long         *llArray;
    float             *fArray;
    double            *dArray;
    double            *ddArray;
#ifdef _Complex_I
    float complex     *cfArray;
    double complex    *cdArray;
    double complex    *cddArray;
#endif
    char             **sArray;

    int                icheck[25];
    float              fcheck[25];
    double             dcheck[25];
#ifdef _Complex_I
    float complex      cfcheck[25];
    double complex     cdcheck[25];
#endif
    const char        *scheck[25];

    const char        *names[2];

    cpl_table         *table;
    cpl_table         *copia;
    cpl_array         *array;
    cpl_array         *new_array;
    cpl_array         *colnames;

    cpl_propertylist  *reflist;
    cpl_error_code     error;


    iArray = cpl_malloc(nrows * sizeof(int));
    llArray = cpl_malloc(nrows * sizeof(long long));
    fArray = cpl_malloc(nrows * sizeof(float));
    dArray = cpl_malloc(nrows * sizeof(double));
    ddArray = cpl_malloc(nrows * sizeof(double));
#ifdef _Complex_I
    cfArray = cpl_malloc(nrows * sizeof(float complex));
    cdArray = cpl_malloc(nrows * sizeof(double complex));
    cddArray = cpl_malloc(nrows * sizeof(double complex));
#endif
    sArray = cpl_malloc(nrows * sizeof(char *));

    iArray[0] = 5;
    iArray[1] = 0;
    iArray[2] = 2;
    iArray[3] = 8;
    iArray[4] = 9;
    iArray[5] = 3;
    iArray[6] = 7;
    iArray[7] = 1;
    iArray[8] = 4;
    iArray[9] = 6;

    llArray[0] = 5LL;
    llArray[1] = 0LL;
    llArray[2] = 2LL;
    llArray[3] = 8LL;
    llArray[4] = 9LL;
    llArray[5] = 3LL;
    llArray[6] = 7LL;
    llArray[7] = 1LL;
    llArray[8] = 4LL;
    llArray[9] = 6LL;

    fArray[0] = 5.1;
    fArray[1] = 0.1;
    fArray[2] = 2.1;
    fArray[3] = 8.1;
    fArray[4] = 9.1;
    fArray[5] = 3.1;
    fArray[6] = 7.1;
    fArray[7] = 1.1;
    fArray[8] = 4.1;
    fArray[9] = 6.1;

    ddArray[0] = dArray[0] = 5.11;
    ddArray[1] = dArray[1] = 0.11;
    ddArray[2] = dArray[2] = 2.11;
    ddArray[3] = dArray[3] = 8.11;
    ddArray[4] = dArray[4] = 9.11;
    ddArray[5] = dArray[5] = 3.11;
    ddArray[6] = dArray[6] = 7.11;
    ddArray[7] = dArray[7] = 1.11;
    ddArray[8] = dArray[8] = 4.11;
    ddArray[9] = dArray[9] = 6.11;

#ifdef _Complex_I
    cfArray[0] = 5.1;
    cfArray[1] = 0.1 * I;
    cfArray[2] = 2.1;
    cfArray[3] = 8.1 * I;
    cfArray[4] = 9.1 + 9.1 * I;
    cfArray[5] = 3.1 - 3.1 * I;
    cfArray[6] = 7.1 + 1.1 * I;
    cfArray[7] = 1.1 - 7.1 * I;
    cfArray[8] = -4.1 + 4.1 * I;
    cfArray[9] = -6.1 - 6.1 * I;

    cddArray[0] = cdArray[0] = 5.11;
    cddArray[1] = cdArray[1] = 0.11 * I;
    cddArray[2] = cdArray[2] = 2.11;
    cddArray[3] = cdArray[3] = 8.11 * I;
    cddArray[4] = cdArray[4] = 9.11 + 9.11 * I;
    cddArray[5] = cdArray[5] = 3.11 - 3.11 * I;
    cddArray[6] = cdArray[6] = 7.11 + 1.11 * I;
    cddArray[7] = cdArray[7] = 1.11 - 7.11 * I;
    cddArray[8] = cdArray[8] = -4.11 + 4.11 * I;
    cddArray[9] = cdArray[9] = -6.11 - 6.11 * I;
#endif

    sArray[0] = cpl_strdup("caaaa");
    sArray[1] = cpl_strdup("abcd");
    sArray[2] = cpl_strdup("aaaa");
    sArray[3] = cpl_strdup("daaa");
    sArray[4] = cpl_strdup("acde");
    sArray[5] = cpl_strdup("baaa");
    sArray[6] = cpl_strdup("aaaa");
    sArray[7] = cpl_strdup("acde");
    sArray[8] = cpl_strdup(" sss");
    sArray[9] = cpl_strdup("daaa");


  /*
   *  Testing tables with more rows
   */

  test_data(table, cpl_table_new(nrows), "Creating the test table... ");

  test(cpl_table_wrap_int(table, iArray, "Integer"), 
                                         "Wrapping the Integer column... ");

  test_pvalue(iArray, cpl_table_unwrap(table, "Integer"),
                                         "Unwrap the Integer column data... ");

  test(cpl_table_wrap_int(table, iArray, "Integer"), 
                                         "Creating the Integer column... ");

  test(cpl_table_wrap_long_long(table, llArray, "LongLong"),
                                "Creating the LongLong column... ");

  test(cpl_table_wrap_double(table, dArray, "Double"), 
                                         "Creating the Double column... ");

  test(cpl_table_wrap_double(table, ddArray, "DoubleDouble"), 
                                  "Creating the DoubleDouble column... ");

#ifdef _Complex_I
  test(cpl_table_wrap_double_complex(table, cdArray, "CDouble"), 
                                         "Creating the CDouble column... ");

  test(cpl_table_wrap_double_complex(table, cddArray, "CDoubleDouble"), 
                                  "Creating the CDoubleDouble column... ");
#endif

  test(cpl_table_wrap_string(table, sArray, "String"),
                                         "Creating the String column... ");

  test(cpl_table_new_column(table, "Float", CPL_TYPE_FLOAT),
                                         "Creating the Float column... ");

  for (i = 0; i < nrows; i++) {
    sprintf(message, "Writing to row %d of the Float column... ", i);
    test(cpl_table_set_float(table, "Float", i, fArray[i]), message);
  }

  test(cpl_table_new_column(table, "CFloat", CPL_TYPE_FLOAT_COMPLEX),
                                         "Creating the CFloat column... ");

#ifdef _Complex_I
  for (i = 0; i < nrows; i++) {
    sprintf(message, "Writing to row %d of the CFloat column... ", i);
    test(cpl_table_set_float_complex(table, "CFloat", i, cfArray[i]), message);
  }
#endif

  test(cpl_table_new_column_array(table, "AInt", 
                                  CPL_TYPE_INT | CPL_TYPE_POINTER, 20),
                                  "Creating the ArrayInt column... ");

  test(cpl_table_new_column_array(table, "ALongLong",
                                  CPL_TYPE_LONG_LONG | CPL_TYPE_POINTER, 20),
                                  "Creating the ArrayLongLong column... ");

  test(cpl_table_new_column_array(table, "AFloat", CPL_TYPE_FLOAT, 20),
                                  "Creating the ArrayFloat column... ");

  test(cpl_table_new_column_array(table, "ADouble", 
                                  CPL_TYPE_DOUBLE | CPL_TYPE_POINTER, 20),
                                  "Creating the ArrayDouble column... ");

  test(cpl_table_new_column_array(table, "CAFloat", CPL_TYPE_FLOAT_COMPLEX, 20),
                                  "Creating the CArrayFloat column... ");

  test(cpl_table_new_column_array(table, "CADouble", 
                  CPL_TYPE_DOUBLE_COMPLEX | CPL_TYPE_POINTER, 20),
                  "Creating the CArrayDouble column... ");

  test(cpl_table_new_column_array(table, "AString", CPL_TYPE_STRING, 10),
                                  "Creating the ArrayString column... ");

  test_ivalue(20, cpl_table_get_column_depth(table, "AInt"), 
              "Check \"AInt\" depth (2)... ");

  k = 0;
  array = cpl_array_new(20, CPL_TYPE_INT);
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < 20; j++) {
      sprintf(message, 
              "Writing element %d of array %d of the AInt column... ", j, i);
      k++;
      test(cpl_array_set_int(array, j, k), message);
    }
    sprintf(message, "Setting array at position %d of the AInt column... ", i);
    test(cpl_table_set_array(table, "AInt", i, array), message);
  }
  new_array = cpl_array_cast(array, CPL_TYPE_DOUBLE);
  cpl_array_delete(new_array);
  cpl_array_delete(array);

  k = 0;
  for (i = 0; i < nrows; i++) {
    sprintf(message, "Getting array %d of the AInt column... ", i);
    test_data(array, (cpl_array *)cpl_table_get_array(table, "AInt", i), 
              message);
    for (j = 0; j < 20; j++) {
      sprintf(message,
              "Reading element %d of array %d of the AInt column... ", j, i);
      k++;
      test_ivalue(k, cpl_array_get_int(array, j, &null), message);
      cpl_test_zero(null);
    }
  }

  k = 0;
  array = cpl_array_new(20, CPL_TYPE_LONG_LONG);
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < 20; j++) {
      sprintf(message,
              "Writing element %d of array %d of the ALongLong column... ", j, i);
      k++;
      test(cpl_array_set_long_long(array, j, k), message);
    }
    sprintf(message, "Setting array at position %d of the ALongLong column... ", i);
    test(cpl_table_set_array(table, "ALongLong", i, array), message);
  }
  new_array = cpl_array_cast(array, CPL_TYPE_DOUBLE);
  cpl_array_delete(new_array);
  cpl_array_delete(array);

  k = 0;
  for (i = 0; i < nrows; i++) {
    sprintf(message, "Getting array %d of the ALongLong column... ", i);
    test_data(array, (cpl_array *)cpl_table_get_array(table, "ALongLong", i),
              message);
    for (j = 0; j < 20; j++) {
      sprintf(message,
              "Reading element %d of array %d of the ALongLong column... ", j, i);
      k++;
      test_ivalue((long long)k, cpl_array_get_long_long(array, j, &null), message);
      cpl_test_zero(null);
    }
  }

  k = 0;
  array = cpl_array_new(20, CPL_TYPE_FLOAT);
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < 20; j++) {
      sprintf(message, 
              "Writing element %d of array %d of the AFloat column... ", j, i);
      k++;
      test(cpl_array_set_float(array, j, k), message);
    }
    sprintf(message, 
            "Setting array at position %d of the AFloat column... ", i);
    test(cpl_table_set_array(table, "AFloat", i, array), message);
  }
  new_array = cpl_array_cast(array, CPL_TYPE_INT);
  cpl_array_delete(new_array);
  cpl_array_delete(array);
  
  k = 0;
  for (i = 0; i < nrows; i++) {
    sprintf(message, "Getting array %d of the AFloat column... ", i);
    test_data(array, (cpl_array *)cpl_table_get_array(table, "AFloat", i),  
              message);
    for (j = 0; j < 20; j++) {
      sprintf(message,
              "Reading element %d of array %d of the AFloat column... ", j, i);
      k++;
      test_fvalue((float)k, 0.0001, 
            cpl_array_get_float(array, j, &null), message);
            cpl_test_zero(null);
    }
  }

#ifdef _Complex_I
  k = 0;
  array = cpl_array_new(20, CPL_TYPE_FLOAT_COMPLEX);
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < 20; j++) {
      sprintf(message,
              "Writing element %d of array %d of the CAFloat column... ", j, i);
      k++;
      test(cpl_array_set_float_complex(array, j, k + k * I), message);
    }
    sprintf(message,
            "Setting array at position %d of the CAFloat column... ", i);
    test(cpl_table_set_array(table, "CAFloat", i, array), message);
  }

  cpl_array_delete(array);


  k = 0;
  for (i = 0; i < nrows; i++) {
    sprintf(message, "Getting array %d of the CAFloat column... ", i);
    test_data(array, (cpl_array *)cpl_table_get_array(table, "CAFloat", i),
              message);
    for (j = 0; j < 20; j++) {
      sprintf(message,
              "Reading element %d of array %d of the CAFloat column... ", j, i);
      k++;
      test_cvalue(k + k * I, 0.0001,
            cpl_array_get_float_complex(array, j, &null), message);
            cpl_test_zero(null);
    }
  }
#endif

  k = 0;
  array = cpl_array_new(20, CPL_TYPE_DOUBLE);
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < 20; j++) {
      sprintf(message, 
              "Writing element %d of array %d of the ADouble column... ", j, i);
      k++;
      test(cpl_array_set_double(array, j, k), message);
    }
    sprintf(message, 
            "Setting array at position %d of the ADouble column... ", i);
    test(cpl_table_set_array(table, "ADouble", i, array), message);
  }
  cpl_array_delete(array);
  
  k = 0;
  for (i = 0; i < nrows; i++) {
    sprintf(message, "Getting array %d of the ADouble column... ", i);
    test_data(array, (cpl_array *)cpl_table_get_array(table, "ADouble", i),
              message);
    for (j = 0; j < 20; j++) {
      sprintf(message,
              "Reading element %d of array %d of the ADouble column... ", j, i);
      k++;
      test_fvalue((float)k, 0.0001, 
            cpl_array_get_double(array, j, &null), message);
            cpl_test_zero(null);
    }
  }


#ifdef _Complex_I
  k = 0;
  array = cpl_array_new(20, CPL_TYPE_DOUBLE_COMPLEX);
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < 20; j++) {
      sprintf(message,
         "Writing element %d of array %d of the CADouble column... ", j, i);
      k++;
      test(cpl_array_set_double_complex(array, j, k + k * I), message);
    }
    sprintf(message, 
            "Setting array at position %d of the CADouble column... ", i);
    test(cpl_table_set_array(table, "CADouble", i, array), message);
  }

  cpl_array_delete(array);
    
  k = 0;
  for (i = 0; i < nrows; i++) {
    sprintf(message, "Getting array %d of the CADouble column... ", i);
    test_data(array, (cpl_array *)cpl_table_get_array(table, "CADouble", i),
              message);
    for (j = 0; j < 20; j++) {
      sprintf(message,
          "Reading element %d of array %d of the CADouble column... ", j, i);
      k++;
      test_cvalue(k + k * I, 0.0001,
            cpl_array_get_double_complex(array, j, &null), message);
            cpl_test_zero(null);
    }
  }
#endif

  test_ivalue(20, cpl_table_get_column_depth(table, "AInt"), 
              "Check \"AInt\" depth (3)... ");

  test_data(array, (cpl_array *)cpl_table_get_array(table, "AInt", 0), 
            "Get AInt array ");
  test_ivalue(CPL_TYPE_INT, cpl_array_get_type(array),
              "Array AInt must be int... ");

  test_ivalue(10, cpl_table_get_nrow(table), "Check table length (1)... ");
  test_ivalue(16, cpl_table_get_ncol(table), "Check table width... ");

  test_failure(CPL_ERROR_DATA_NOT_FOUND, 
               cpl_table_erase_column(table, "Diable"), 
               "Trying to delete a not existing column... ");

  test(cpl_table_erase_column(table, "DoubleDouble"), 
                                "Delete column \"DoubleDouble\"... ");

  test_ivalue(15, cpl_table_get_ncol(table), "Check again table width... ");

  test_ivalue(CPL_TYPE_INT, cpl_table_get_column_type(table, "Integer"),
                                "Column Integer must be int... ");
  test_ivalue(CPL_TYPE_DOUBLE, cpl_table_get_column_type(table, "Double"),
                                "Column Double must be double... ");
  test_ivalue(CPL_TYPE_STRING, cpl_table_get_column_type(table, "String"),
                                "Column String must be char*... ");
  test_ivalue(CPL_TYPE_FLOAT, cpl_table_get_column_type(table, "Float"),
                                "Column Float must be float... ");
  test_ivalue(CPL_TYPE_FLOAT_COMPLEX, 
              cpl_table_get_column_type(table, "CFloat"),
              "Column CFloat must be float complex... ");
  test_ivalue(CPL_TYPE_DOUBLE_COMPLEX, 
              cpl_table_get_column_type(table, "CDouble"),
              "Column CDouble must be double complex... ");
  test_ivalue((CPL_TYPE_INT | CPL_TYPE_POINTER), 
              cpl_table_get_column_type(table, "AInt"),
              "Column AInt must be arrays of int... ");
  test_ivalue((CPL_TYPE_DOUBLE | CPL_TYPE_POINTER), 
              cpl_table_get_column_type(table, "ADouble"),
              "Column Double must be arrays of double... ");
  test_ivalue((CPL_TYPE_FLOAT | CPL_TYPE_POINTER), 
              cpl_table_get_column_type(table, "AFloat"),
              "Column Float must be arrays of float... ");
  test_ivalue((CPL_TYPE_FLOAT_COMPLEX | CPL_TYPE_POINTER),
              cpl_table_get_column_type(table, "CAFloat"),
              "Column CAFloat must be arrays of float complex... ");
  test_ivalue((CPL_TYPE_DOUBLE_COMPLEX | CPL_TYPE_POINTER),
              cpl_table_get_column_type(table, "CADouble"),
              "Column CADouble must be arrays of double complex... ");
  test_pvalue(iArray, cpl_table_get_data_int(table, "Integer"),
                                "Check pointer to column Integer data... ");

  test_pvalue(dArray, cpl_table_get_data_double(table, "Double"),
                                "Check pointer to column Double data... ");
  test_pvalue(sArray, cpl_table_get_data_string(table, "String"),
                                "Check pointer to column String data... ");

  copia = cpl_table_new(5);

  error = cpl_table_set_column_unit(table, "Integer", "Counts");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_column_unit(table, "Double", "erg/sec");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_column_unit(table, "String", "Name");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_column_unit(table, "AFloat", "erg/sec");
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  test(cpl_table_copy_structure(copia, table),
       "Creating a new cpl_table modeled on an existing cpl_table... ");

  test_ivalue(5, cpl_table_get_nrow(copia), "Check table length (2)... ");
  test_ivalue(15, cpl_table_get_ncol(copia), "Check table width... ");

  test(cpl_table_compare_structure(table, copia), 
                                 "Tables must have the same structure... ");
  cpl_table_erase_column(copia, "Double");
  test_ivalue(1, cpl_table_compare_structure(table, copia), 
    "Deleting column Double - now tables must have different structure... ");
  test(cpl_table_new_column(copia, "Double", CPL_TYPE_DOUBLE),
                                 "Creating again the Double column... ");
  error = cpl_table_set_column_unit(copia, "Double", "erg/sec");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  test(cpl_table_compare_structure(table, copia), 
                         "Tables must have the same structure again... ");

  test(cpl_table_fill_column_window_int(copia, "Integer", 0, 5, -1),
                                 "Fill column Integer of new table... ");
  test(cpl_table_fill_column_window_long_long(copia, "LongLong", 0, 5, -1),
                                 "Fill column LongLong of new table... ");
  test(cpl_table_fill_column_window_double(copia, "Double", 0, 5, -1.11),
                                 "Fill column Double of new table... ");
  test(cpl_table_fill_column_window_float(copia, "Float", 0, 5, -1.1),
                                 "Fill column Float of new table... ");

#ifdef _Complex_I
  test(cpl_table_fill_column_window_double_complex(copia, "CDouble", 
                                                   0, 5, -1.11 + 4.11 * I),
                                 "Fill column CDouble of new table... ");

  test(cpl_table_fill_column_window_float_complex(copia, "CFloat", 
                                                  0, 5, -1.1 + 4.1 * I),
                                 "Fill column CFloat of new table... ");
#endif
  test(cpl_table_fill_column_window_string(copia, "String", 0, 5, "extra"),
                                 "Fill column String of new table... ");

  array = cpl_array_new(20, CPL_TYPE_INT);
  for (j = 0; j < 20; j++)
    cpl_array_set_int(array, j, j);
  test(cpl_table_fill_column_window_array(copia, "AInt", 0, 5, array),
                                 "Fill column AInt of new table... ");
  cpl_array_delete(array);

  array = cpl_array_new(20, CPL_TYPE_LONG_LONG);
  for (j = 0; j < 20; j++)
    cpl_array_set_long_long(array, j, j);
  test(cpl_table_fill_column_window_array(copia, "ALongLong", 0, 5, array),
                                 "Fill column ALongLong of new table... ");
  cpl_array_delete(array);

  array = cpl_array_new(20, CPL_TYPE_FLOAT);
  for (j = 0; j < 20; j++) 
    cpl_array_set_float(array, j, j);
  test(cpl_table_fill_column_window_array(copia, "AFloat", 0, 5, array),
                                 "Fill column AFloat of new table... ");
  cpl_array_delete(array);

  array = cpl_array_new(20, CPL_TYPE_DOUBLE);
  for (j = 0; j < 20; j++) 
    cpl_array_set_double(array, j, j);
  test(cpl_table_fill_column_window_array(copia, "ADouble", 0, 5, array),
                                 "Fill column ADouble of new table... ");
  cpl_array_delete(array);

#ifdef _Complex_I
  array = cpl_array_new(20, CPL_TYPE_FLOAT_COMPLEX);
  for (j = 0; j < 20; j++)
    cpl_array_set_float_complex(array, j, j + 0.5*I);
  test(cpl_table_fill_column_window_array(copia, "CAFloat", 0, 5, array),
                                 "Fill column CAFloat of new table... ");
  cpl_array_delete(array);

  array = cpl_array_new(20, CPL_TYPE_DOUBLE_COMPLEX);
  for (j = 0; j < 20; j++)
    cpl_array_set_double_complex(array, j, j + 0.5*I);
  test(cpl_table_fill_column_window_array(copia, "CADouble", 0, 5, array),
                                 "Fill column CADouble of new table... ");
  cpl_array_delete(array);
#endif

  array = cpl_array_wrap_string(sArray, 10);
  test(cpl_table_fill_column_window_array(copia, "AString", 0, 5, array),
                                 "Fill column AString of new table... ");
  cpl_array_unwrap(array);


  test(cpl_table_insert(table, copia, 15), 
                                 "Appending new table to old table... ");
  test(cpl_table_insert(table, copia, 5), 
                                 "Inserting new table in old table... ");
  test(cpl_table_insert(table, copia, 0), 
                                 "Prepending new table to old table... ");

  cpl_table_delete(copia);

/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "2.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "2.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "2.fits", 1, 1);
  cpl_test_nonnull(table);
  // FIXME: IO_APPEND initial test only!
  cpl_table_save(table, NULL, NULL, BASE "2a.fits", CPL_IO_CREATE);
  cpl_table_save(table, NULL, NULL, BASE "2a.fits", CPL_IO_APPEND);
  cpl_test_fits(BASE "2a.fits");
/**/
  
  test_ivalue(25, cpl_table_get_nrow(table), "Check table length (3)... ");

  icheck[0] = -1;
  icheck[1] = -1;
  icheck[2] = -1;
  icheck[3] = -1;
  icheck[4] = -1;
  icheck[5] = 5;
  icheck[6] = 0;
  icheck[7] = 2;
  icheck[8] = 8;
  icheck[9] = 9;
  icheck[10] = -1;
  icheck[11] = -1;
  icheck[12] = -1;
  icheck[13] = -1;
  icheck[14] = -1;
  icheck[15] = 3;
  icheck[16] = 7;
  icheck[17] = 1;
  icheck[18] = 4;
  icheck[19] = 6;
  icheck[20] = -1;
  icheck[21] = -1;
  icheck[22] = -1;
  icheck[23] = -1;
  icheck[24] = -1;

  for (i = 0; i < 25; i++) {
      test_ivalue(icheck[i], cpl_table_get_int(table, "Integer", i, &null),
                  "Check Integer column... ");
      cpl_test_zero(null);
  }

  for (i = 0; i < 25; i++) {
      test_ivalue(icheck[i], cpl_table_get_long_long(table, "LongLong",
                                                     i, &null),
                  "Check LongLong column... ");
      cpl_test_zero(null);
  }

  dcheck[0] = -1.1100;
  dcheck[1] = -1.1100;
  dcheck[2] = -1.1100;
  dcheck[3] = -1.1100;
  dcheck[4] = -1.1100;
  dcheck[5] = 5.1100;
  dcheck[6] = 0.1100;
  dcheck[7] = 2.1100;
  dcheck[8] = 8.1100;
  dcheck[9] = 9.1100;
  dcheck[10] = -1.1100;
  dcheck[11] = -1.1100;
  dcheck[12] = -1.1100;
  dcheck[13] = -1.1100;
  dcheck[14] = -1.1100;
  dcheck[15] = 3.1100;
  dcheck[16] = 7.1100;
  dcheck[17] = 1.1100;
  dcheck[18] = 4.1100;
  dcheck[19] = 6.1100;
  dcheck[20] = -1.1100;
  dcheck[21] = -1.1100;
  dcheck[22] = -1.1100;
  dcheck[23] = -1.1100;
  dcheck[24] = -1.1100;

  for (i = 0; i < 25; i++) {
      test_fvalue(dcheck[i], 0.00001,
                  cpl_table_get_double(table, "Double", i, &null),
                  "Check Double column... ");
                  cpl_test_zero(null);
  }

  scheck[0] = "extra";
  scheck[1] = "extra";
  scheck[2] = "extra";
  scheck[3] = "extra";
  scheck[4] = "extra";
  scheck[5] = "caaaa";
  scheck[6] = "abcd";
  scheck[7] = "aaaa";
  scheck[8] = "daaa";
  scheck[9] = "acde";
  scheck[10] = "extra";
  scheck[11] = "extra";
  scheck[12] = "extra";
  scheck[13] = "extra";
  scheck[14] = "extra";
  scheck[15] = "baaa";
  scheck[16] = "aaaa";
  scheck[17] = "acde";
  scheck[18] = " sss";
  scheck[19] = "daaa";
  scheck[20] = "extra";
  scheck[21] = "extra";
  scheck[22] = "extra";
  scheck[23] = "extra";
  scheck[24] = "extra";

  for (i = 0; i < 25; i++) {
      test_svalue(scheck[i], cpl_table_get_string(table, "String", i),
                  "Check String column... ");
  }

  fcheck[0] = -1.10;
  fcheck[1] = -1.10;
  fcheck[2] = -1.10;
  fcheck[3] = -1.10;
  fcheck[4] = -1.10;
  fcheck[5] = 5.10;
  fcheck[6] = 0.10;
  fcheck[7] = 2.10;
  fcheck[8] = 8.10;
  fcheck[9] = 9.10;
  fcheck[10] = -1.10;
  fcheck[11] = -1.10;
  fcheck[12] = -1.10;
  fcheck[13] = -1.10;
  fcheck[14] = -1.10;
  fcheck[15] = 3.10;
  fcheck[16] = 7.10;
  fcheck[17] = 1.10;
  fcheck[18] = 4.10;
  fcheck[19] = 6.10;
  fcheck[20] = -1.10;
  fcheck[21] = -1.10;
  fcheck[22] = -1.10;
  fcheck[23] = -1.10;
  fcheck[24] = -1.10;

  for (i = 0; i < 25; i++) {
      test_fvalue(fcheck[i], 0.00001,
                  cpl_table_get_float(table, "Float", i, &null),
                  "Check Float column... ");
                  cpl_test_zero(null);
  }

#ifdef _Complex_I
  cfcheck[0] = -1.1 + 4.1 * I;
  cfcheck[1] = -1.1 + 4.1 * I;
  cfcheck[2] = -1.1 + 4.1 * I;
  cfcheck[3] = -1.1 + 4.1 * I;
  cfcheck[4] = -1.1 + 4.1 * I;
  cfcheck[5] = 5.1;
  cfcheck[6] = 0.1 * I;
  cfcheck[7] = 2.1;
  cfcheck[8] = 8.1 * I;
  cfcheck[9] = 9.1 + 9.1 * I;
  cfcheck[10] = -1.1 + 4.1 * I;
  cfcheck[11] = -1.1 + 4.1 * I;
  cfcheck[12] = -1.1 + 4.1 * I;
  cfcheck[13] = -1.1 + 4.1 * I;
  cfcheck[14] = -1.1 + 4.1 * I;
  cfcheck[15] = 3.1 - 3.1 * I;
  cfcheck[16] = 7.1 + 1.1 * I;
  cfcheck[17] = 1.1 - 7.1 * I;
  cfcheck[18] = -4.1 + 4.1 * I;
  cfcheck[19] = -6.1 - 6.1 * I;
  cfcheck[20] = -1.1 + 4.1 * I;
  cfcheck[21] = -1.1 + 4.1 * I;
  cfcheck[22] = -1.1 + 4.1 * I;
  cfcheck[23] = -1.1 + 4.1 * I;
  cfcheck[24] = -1.1 + 4.1 * I;

  for (i = 0; i < 25; i++) {
      test_cvalue(cfcheck[i], 0.00001,
                  cpl_table_get_float_complex(table, "CFloat", i, &null),
                  "Check CFloat column... ");
                  cpl_test_zero(null);
  }

  cdcheck[0] = -1.11 + 4.11 * I;
  cdcheck[1] = -1.11 + 4.11 * I;
  cdcheck[2] = -1.11 + 4.11 * I;
  cdcheck[3] = -1.11 + 4.11 * I;
  cdcheck[4] = -1.11 + 4.11 * I;
  cdcheck[5] = 5.11;
  cdcheck[6] = 0.11 * I;
  cdcheck[7] = 2.11;
  cdcheck[8] = 8.11 * I;
  cdcheck[9] = 9.11 + 9.11 * I;
  cdcheck[10] = -1.11 + 4.11 * I;
  cdcheck[11] = -1.11 + 4.11 * I;
  cdcheck[12] = -1.11 + 4.11 * I;
  cdcheck[13] = -1.11 + 4.11 * I;
  cdcheck[14] = -1.11 + 4.11 * I;
  cdcheck[15] = 3.11 - 3.11 * I;
  cdcheck[16] = 7.11 + 1.11 * I;
  cdcheck[17] = 1.11 - 7.11 * I;
  cdcheck[18] = -4.11 + 4.11 * I;
  cdcheck[19] = -6.11 - 6.11 * I;
  cdcheck[20] = -1.11 + 4.11 * I;
  cdcheck[21] = -1.11 + 4.11 * I;
  cdcheck[22] = -1.11 + 4.11 * I;
  cdcheck[23] = -1.11 + 4.11 * I;
  cdcheck[24] = -1.11 + 4.11 * I;

  for (i = 0; i < 25; i++) {
      test_cvalue(cdcheck[i], 0.00001,
                  cpl_table_get_double_complex(table, "CDouble", i, &null),
                  "Check CDouble column... ");
                  cpl_test_zero(null);
  }
#endif

  test(cpl_table_set_invalid(table, "Integer", 0), 
                             "Set Integer 0 to NULL... ");
  test(cpl_table_set_invalid(table, "Integer", 5), 
                             "Set Integer 5 to NULL... ");
  test(cpl_table_set_invalid(table, "Integer", 24), 
                             "Set Integer 24 to NULL... ");

  test(cpl_table_set_invalid(table, "LongLong", 0),
                             "Set LongLong 0 to NULL... ");
  test(cpl_table_set_invalid(table, "LongLong", 5),
                             "Set LongLong 5 to NULL... ");
  test(cpl_table_set_invalid(table, "LongLong", 24),
                             "Set LongLong 24 to NULL... ");

  test(cpl_table_set_invalid(table, "AInt", 0),
                             "Set AInt 0 to NULL... ");
  test(cpl_table_set_invalid(table, "ALongLong", 0),
                             "Set ALongLong 0 to NULL... ");
  test(cpl_table_set_invalid(table, "AFloat", 5),
                             "Set AFloat 5 to NULL... ");
  test(cpl_table_set_invalid(table, "ADouble", 24),
                             "Set ADouble 24 to NULL... ");
  test(cpl_table_set_invalid(table, "CAFloat", 5),
                             "Set CAFloat 5 to NULL... ");
  test(cpl_table_set_invalid(table, "CADouble", 24),
                             "Set CADouble 24 to NULL... ");


  test_ivalue(3, cpl_table_count_invalid(table, "Integer"), 
              "Count Integer written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "AInt"),
              "Count AInt written NULLs... ");
  test_ivalue(3, cpl_table_count_invalid(table, "LongLong"),
              "Count LongLong written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "ALongLong"),
              "Count ALongLong written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "AFloat"),
              "Count AFloat written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "ADouble"),
              "Count ADouble written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "CAFloat"),
              "Count CAFloat written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "CADouble"),
              "Count CADouble written NULLs... ");


/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_float(table, "AFloat", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_double(table, "ADouble", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_float_complex(table, "CAFloat", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_double_complex(table, "CADouble", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  test_ivalue(3, cpl_table_count_invalid(table, "Integer"), 
              "Count Integer written NULLs... ");
  test_ivalue(3, cpl_table_count_invalid(table, "LongLong"),
              "Count LongLong written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "AFloat"),
              "Count AFloat written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "ADouble"),
              "Count ADouble written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "CAFloat"),
              "Count CAFloat written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "CADouble"),
              "Count CADouble written NULLs... ");
  /* Array elements are flagged as valid! */
  test_ivalue(0, cpl_table_count_invalid(table, "AInt"),
              "Count AInt written NULLs... ");
  test_ivalue(0, cpl_table_count_invalid(table, "ALongLong"),
              "Count ALongLong written NULLs... ");

  error = cpl_table_save(table, NULL, NULL, BASE "3.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "3.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "3.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(3, cpl_table_count_invalid(table, "Integer"), 
              "Count Integer written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "AInt"),
              "Count AInt written NULLs... ");
  test_ivalue(3, cpl_table_count_invalid(table, "LongLong"),
              "Count LongLong written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "ALongLong"),
              "Count ALongLong written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "AFloat"),
              "Count AFloat written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "ADouble"),
              "Count ADouble written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "CAFloat"),
              "Count CAFloat written NULLs... ");
  test_ivalue(1, cpl_table_count_invalid(table, "CADouble"),
              "Count CADouble written NULLs... ");

  for (i = 0; i < 25; i++) {
    const int ival = cpl_table_get_int(table, "Integer", i, &null);
    if (!null) {
        test_ivalue(icheck[i], ival, "Check Integer column... ");
    } else {
        cpl_test(i == 0 || i == 5 || i == 24);
    }
  }

  for (i = 0; i < 25; i++) {
    const long long ival = cpl_table_get_long_long(table, "LongLong", i, &null);
    if (!null) {
        test_ivalue(icheck[i], ival, "Check LongLong column... ");
    } else {
        cpl_test(i == 0 || i == 5 || i == 24);
    }
  }

  test(cpl_table_set_int(table, "Integer", 0, -1), 
                                              "Set Integer 0 to -1... ");
  test(cpl_table_set_int(table, "Integer", 5, 5), 
                                              "Set Integer 5 to 5... ");
  test(cpl_table_set_int(table, "Integer", 24, -1), 
                                              "Set Integer 24 to -1... ");

  array = cpl_array_new(20, CPL_TYPE_INT);
  for (j = 0; j < 20; j++)
    cpl_array_set_int(array, j, j);
  test(cpl_table_set_array(table, "AInt", 0, array),
                           "Set a valid array to AInt 0... ");
  cpl_array_delete(array);
  test_ivalue(0, cpl_table_count_invalid(table, "AInt"),
              "No invalid elements in AInt... ");

  test(cpl_table_set_long_long(table, "LongLong", 0, -1),
                                              "Set LongLong 0 to -1... ");
  test(cpl_table_set_long_long(table, "LongLong", 5, 5),
                                              "Set LongLong 5 to 5... ");
  test(cpl_table_set_long_long(table, "LongLong", 24, -1),
                                              "Set LongLong 24 to -1... ");

  array = cpl_array_new(20, CPL_TYPE_LONG_LONG);
  for (j = 0; j < 20; j++)
    cpl_array_set_long_long(array, j, j);
  test(cpl_table_set_array(table, "ALongLong", 0, array),
                           "Set a valid array to ALongLong 0... ");
  cpl_array_delete(array);
  test_ivalue(0, cpl_table_count_invalid(table, "ALongLong"),
              "No invalid elements in ALongLong... ");

  array = cpl_array_new(20, CPL_TYPE_FLOAT);
  for (j = 0; j < 20; j++)
    cpl_array_set_float(array, j, j);
  test(cpl_table_set_array(table, "AFloat", 5, array),
                           "Set a valid array to AFloat 5... ");
  cpl_array_delete(array);
  test_ivalue(0, cpl_table_count_invalid(table, "AFloat"),
              "No invalid elements in AFloat... ");

  array = cpl_array_new(20, CPL_TYPE_DOUBLE);
  for (j = 0; j < 20; j++)
    cpl_array_set_double(array, j, j);
  test(cpl_table_set_array(table, "ADouble", 24, array),
                           "Set a valid array to ADouble 24... ");
  cpl_array_delete(array);
  test_ivalue(0, cpl_table_count_invalid(table, "ADouble"),
              "No invalid elements in ADouble... ");

#ifdef _Complex_I
  array = cpl_array_new(20, CPL_TYPE_FLOAT_COMPLEX);
  for (j = 0; j < 20; j++)
    cpl_array_set_float_complex(array, j, j + 0.5*I);
  test(cpl_table_set_array(table, "CAFloat", 5, array),
                           "Set a valid array to CAFloat 5... ");
  cpl_array_delete(array);
  test_ivalue(0, cpl_table_count_invalid(table, "CAFloat"),
              "No invalid elements in CAFloat... ");

  array = cpl_array_new(20, CPL_TYPE_DOUBLE_COMPLEX);
  for (j = 0; j < 20; j++)
    cpl_array_set_double_complex(array, j, j + 0.5*I);
  test(cpl_table_set_array(table, "CADouble", 24, array),
                           "Set a valid array to CADouble 24... ");
  cpl_array_delete(array);
  test_ivalue(0, cpl_table_count_invalid(table, "CADouble"),
              "No invalid elements in CADouble... ");
#endif

/**+ FIXME: RESTORE!!! %%%%% */ 
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "4.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "4.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "4.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(0, cpl_table_count_invalid(table, "Integer"), 
                                              "Count NULLs... ");

  for (i = 0; i < 25; i++) {
    const int ival = cpl_table_get_int(table, "Integer", i, &null);

    cpl_test_zero(null);

    if (!null) {
        test_ivalue(icheck[i], ival, "Check Integer column... ");
    }
  }

  test_ivalue(0, cpl_table_count_invalid(table, "LongLong"),
                                              "Count NULLs... ");

  for (i = 0; i < 25; i++) {
    const int ival = cpl_table_get_long_long(table, "LongLong", i, &null);

    cpl_test_zero(null);

    if (!null) {
        test_ivalue(icheck[i], ival, "Check LongLong column... ");
    }
  }

  test(cpl_table_set_invalid(table, "Double", 0), "Set Double 0 to NULL... ");
  test(cpl_table_set_invalid(table, "Double", 5), "Set Double 5 to NULL... ");
  test(cpl_table_set_invalid(table, "Double", 24), "Set Double 24 to NULL... ");

/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "5.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "5.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "5.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(3, cpl_table_count_invalid(table, "Double"), 
                                "Count written Double NULLs... ");

  for (i = 0; i < 25; i++) {
    const double dval = cpl_table_get_double(table, "Double", i, &null);
    if (!null) {
        test_fvalue(dcheck[i], 0.0, dval, "Check Double column... ");
    }
    else {
        cpl_test(i == 0 || i == 5 || i == 24);
    }
  }

  test(cpl_table_set_double(table, "Double", 0, -1.11), 
                                              "Set Double 0 to -1.11... ");
  test(cpl_table_set_double(table, "Double", 5, 5.11), 
                                              "Set Double 5 to 5.11... ");
  test(cpl_table_set_double(table, "Double", 24, -1.11), 
                                              "Set Double 24 to -1.11... ");

/**+ FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "6.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "6.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "6.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(0, cpl_table_count_invalid(table, "Double"), 
                                                  "Count NULLs... ");

  for (i = 0; i < 25; i++) {
    const double dval = cpl_table_get_double(table, "Double", i, &null);

    cpl_test_zero(null);
    if (!null) {
        test_fvalue(dcheck[i], 0.00001, dval, "Check Double column... ");
    }
  }

  test(cpl_table_set_invalid(table, "String", 0), "Set String 0 to NULL... ");
  test(cpl_table_set_invalid(table, "String", 5), "Set String 5 to NULL... ");
  test(cpl_table_set_invalid(table, "String", 24), "Set String 24 to NULL... ");

/**+ FIXME: RESTORE!!! %%% */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "7.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "7.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "7.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(3, cpl_table_count_invalid(table, "String"), 
                              "Count written String NULLs... ");

  for (i = 0; i < 25; i++) {
      const char * sval = cpl_table_get_string(table, "String", i);

      if (sval != NULL) {
          test_svalue(scheck[i], sval, "Check String column... ");
      } else {
          cpl_test(i == 0 || i == 5 || i == 24);
      }
  }

  test(cpl_table_set_string(table, "String", 0, "extra"),
                                              "Set String 0 to \"extra\"... ");
  test(cpl_table_set_string(table, "String", 5, "caaaa"), 
                                              "Set String 5 to \"caaaa\"... ");
  test(cpl_table_set_string(table, "String", 24, "extra"), 
                                              "Set String 24 to \"extra\"... ");

/**+ FIXME: RESTORE!!! %%% */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "8.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "8.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "8.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(0, cpl_table_count_invalid(table, "String"), 
                                              "Count NULLs... ");

  for (i = 0; i < 25; i++) {

      test_svalue(scheck[i], cpl_table_get_string(table, "String", i),
                  "Check String column... ");
  }

  test(cpl_table_set_invalid(table, "Float", 0), "Set Float 0 to NULL... ");
  test(cpl_table_set_invalid(table, "Float", 5), "Set Float 5 to NULL... ");
  test(cpl_table_set_invalid(table, "Float", 24), "Set Float 24 to NULL... ");

/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "9.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "9.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "9.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(3, cpl_table_count_invalid(table, "Float"), 
                              "Count written Float NULLs... ");

  for (i = 0; i < 25; i++) {
    const float fval = cpl_table_get_float(table, "Float", i, &null);

    if (!null) {
        test_fvalue(fcheck[i], 0.0, fval, "Check Float column... ");
    } else {
        cpl_test(i == 0 || i == 5 || i == 24);
    }
  }

  test(cpl_table_set_float(table, "Float", 0, -1.1), 
                                              "Set Float 0 to -1.1... ");
  test(cpl_table_set_float(table, "Float", 5, 5.1), 
                                              "Set Float 5 to 5.1... ");
  test(cpl_table_set_float(table, "Float", 24, -1.1), 
                                              "Set Float 24 to -1.1... ");

/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "10.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "10.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "10.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(0, cpl_table_count_invalid(table, "Float"), 
                                              "Count NULLs... ");

  for (i = 0; i < 25; i++) {
    const float fval = cpl_table_get_float(table, "Float", i, &null);

    cpl_test_zero(null);
    if (!null) {
        test_fvalue(fcheck[i], 0.00001, fval, "Check Float column... ");

    }
  }

 /* %%% */

  test(cpl_table_set_invalid(table, "CFloat", 0), "Set CFloat 0 to NULL... ");
  test(cpl_table_set_invalid(table, "CFloat", 5), "Set CFloat 5 to NULL... ");
  test(cpl_table_set_invalid(table, "CFloat", 24), "Set CFloat 24 to NULL... ");

/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "9a.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "9a.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "9a.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

#ifdef _Complex_I
  test_ivalue(3, cpl_table_count_invalid(table, "CFloat"),
                              "Count written CFloat NULLs... ");
  
  for (i = 0; i < 25; i++) {
    const float complex fval = cpl_table_get_float_complex(table, 
                                                      "CFloat", i, &null);

    if (!null) {
        test_cvalue(cfcheck[i], 0.0, fval, "Check CFloat column... ");
    } else {
        cpl_test(i == 0 || i == 5 || i == 24);
    }
  }
  
  test(cpl_table_set_float_complex(table, "CFloat", 0, -1.1 + 4.1*I),
                                       "Set CFloat 0 to -1.1 + i4.1... ");
  test(cpl_table_set_float_complex(table, "CFloat", 5, 5.1), 
                                       "Set CFloat 5 to 5.1... ");
  test(cpl_table_set_float_complex(table, "CFloat", 24, -1.1 + 4.1*I), 
                                       "Set CFloat 24 to -1.1 + i4.1... ");
#endif

/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "10.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "10.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "10.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

#ifdef _Complex_I
  test_ivalue(0, cpl_table_count_invalid(table, "CFloat"), "Count NULLs... ");

  for (i = 0; i < 25; i++) {
    const float complex fval = cpl_table_get_float_complex(table, 
                                                  "CFloat", i, &null);

    cpl_test_zero(null);
    if (!null) {
        test_cvalue(cfcheck[i], 0.00001, fval, "Check CFloat column... ");

    }
  }
#endif

 /* %%% */

  test(cpl_table_set_column_invalid(table, "Integer", 0, 3), 
                                  "Set Integer 0-2 to NULL... ");
  test(cpl_table_set_column_invalid(table, "Integer", 5, 3), 
                                  "Set Integer 5-7 to NULL... ");
  test(cpl_table_set_column_invalid(table, "Integer", 20, 20), 
                                  "Set Integer 20 till end to NULL... ");

  test(cpl_table_set_column_invalid(table, "AInt", 0, 3),
                                  "Set AInt 0-2 to NULL... ");
  test(cpl_table_set_column_invalid(table, "AInt", 5, 3),
                                  "Set AInt 5-7 to NULL... ");
  test(cpl_table_set_column_invalid(table, "AInt", 20, 20),
                                  "Set AInt 20 till end to NULL... ");

  /********/

  array = (cpl_array *)cpl_table_get_array(table, "AInt", 3);
  test_ivalue(0, cpl_array_get_int(array, 0, &null), 
                                   "Read array element 0...");
  test_ivalue(0, null, "Check array element 0 is valid...");
  test_ivalue(5, cpl_array_get_int(array, 5, &null), 
                                   "Read array element 5...");
  test_ivalue(0, null, "Check array element 5 is valid...");
  test_ivalue(19, cpl_array_get_int(array, 19, &null), 
                                   "Read array element 19...");
  test_ivalue(0, null, "Check array element 19 is valid...");

  cpl_array_set_invalid(array, 0);
  cpl_array_set_invalid(array, 5);
  cpl_array_set_invalid(array, 19);

  test_ivalue(0, cpl_array_get_int(array, 0, &null), 
                                   "Read again array element 0...");
  test_ivalue(1, null, "Check array element 0 is invalid...");
  test_ivalue(0, cpl_array_get_int(array, 5, &null), 
                                   "Read again array element 5...");
  test_ivalue(1, null, "Check array element 5 is invalid...");
  test_ivalue(0, cpl_array_get_int(array, 19, &null), 
                                   "Read again array element 19...");
  test_ivalue(1, null, "Check array element 19 is invalid...");


  test(cpl_table_set_column_invalid(table, "LongLong", 0, 3),
                                  "Set LongLong 0-2 to NULL... ");
  test(cpl_table_set_column_invalid(table, "LongLong", 5, 3),
                                  "Set LongLong 5-7 to NULL... ");
  test(cpl_table_set_column_invalid(table, "LongLong", 20, 20),
                                  "Set LongLong 20 till end to NULL... ");

  test(cpl_table_set_column_invalid(table, "ALongLong", 0, 3),
                                  "Set ALongLong 0-2 to NULL... ");
  test(cpl_table_set_column_invalid(table, "ALongLong", 5, 3),
                                  "Set ALongLong 5-7 to NULL... ");
  test(cpl_table_set_column_invalid(table, "ALongLong", 20, 20),
                                  "Set ALongLong 20 till end to NULL... ");

  array = (cpl_array *)cpl_table_get_array(table, "ALongLong", 3);
  test_ivalue(0, cpl_array_get_long_long(array, 0, &null),
                                   "Read array element 0...");
  test_ivalue(0, null, "Check array element 0 is valid...");
  test_ivalue(5, cpl_array_get_long_long(array, 5, &null),
                                   "Read array element 5...");
  test_ivalue(0, null, "Check array element 5 is valid...");
  test_ivalue(19, cpl_array_get_long_long(array, 19, &null),
                                   "Read array element 19...");
  test_ivalue(0, null, "Check array element 19 is valid...");

  cpl_array_set_invalid(array, 0);
  cpl_array_set_invalid(array, 5);
  cpl_array_set_invalid(array, 19);

  test_ivalue(0, cpl_array_get_long_long(array, 0, &null),
                                   "Read again array element 0...");
  test_ivalue(1, null, "Check array element 0 is invalid...");
  test_ivalue(0, cpl_array_get_long_long(array, 5, &null),
                                   "Read again array element 5...");
  test_ivalue(1, null, "Check array element 5 is invalid...");
  test_ivalue(0, cpl_array_get_long_long(array, 19, &null),
                                   "Read again array element 19...");
  test_ivalue(1, null, "Check array element 19 is invalid...");

/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "11.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "11.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "11.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(11, cpl_table_count_invalid(table, "Integer"), 
              "Count Integer NULLs... ");

  test_ivalue(11, cpl_table_count_invalid(table, "AInt"), 
              "Count AInt NULLs... ");

  for (i = 0; i < 25; i++) {
    const int ival = cpl_table_get_int(table, "Integer", i, &null);
    if (!null) {
        test_ivalue(icheck[i], ival, "Check Integer column... ");
    } else {
        cpl_test(i <= 2 || i >= 5);
        cpl_test(i <= 7 || i >= 20);
    }
  }

  test_ivalue(11, cpl_table_count_invalid(table, "LongLong"),
              "Count LongLong NULLs... ");

  test_ivalue(11, cpl_table_count_invalid(table, "ALongLong"),
              "Count ALongLong NULLs... ");

  for (i = 0; i < 25; i++) {
    const long long llval = cpl_table_get_long_long(table, "LongLong", i, &null);
    if (!null) {
        test_ivalue(icheck[i], llval, "Check LongLong column... ");
    } else {
        cpl_test(i <= 2 || i >= 5);
        cpl_test(i <= 7 || i >= 20);
    }
  }

  /*
   * Test valid and invalid elements of an array of AInt (after loading).
   */

  array = (cpl_array *)cpl_table_get_array(table, "AInt", 3);

  test_ivalue(0, cpl_array_get_int(array, 0, &null),
                                   "Load again array element 0...");
  test_ivalue(1, null, "Reloaded array element 0 is invalid...");
  test_ivalue(0, cpl_array_get_int(array, 5, &null),
                                   "Load again array element 5...");
  test_ivalue(1, null, "Reloaded array element 5 is invalid...");
  test_ivalue(0, cpl_array_get_int(array, 19, &null),
                                   "Load again array element 19...");
  test_ivalue(1, null, "Reloaded array element 19 is invalid...");

  test_ivalue(6, cpl_array_get_int(array, 6, &null),
                                   "Load array element 6...");
  test_ivalue(0, null, "Reloaded array element 6 is valid...");


  /*
   * Test valid and invalid elements of an array of ALongLong (after loading).
   */

  array = (cpl_array *)cpl_table_get_array(table, "ALongLong", 3);

  test_ivalue(0, cpl_array_get_long_long(array, 0, &null),
                                   "Load again array element 0...");
  test_ivalue(1, null, "Reloaded array element 0 is invalid...");
  test_ivalue(0, cpl_array_get_long_long(array, 5, &null),
                                   "Load again array element 5...");
  test_ivalue(1, null, "Reloaded array element 5 is invalid...");
  test_ivalue(0, cpl_array_get_long_long(array, 19, &null),
                                   "Load again array element 19...");
  test_ivalue(1, null, "Reloaded array element 19 is invalid...");

  test_ivalue(6, cpl_array_get_long_long(array, 6, &null),
                                   "Load array element 6...");
  test_ivalue(0, null, "Reloaded array element 6 is valid...");


  /****/

  test(cpl_table_set_column_invalid(table, "Double", 0, 3),
                                  "Set Double 0-2 to NULL... ");
  test(cpl_table_set_column_invalid(table, "Double", 5, 3),
                                  "Set Double 5-7 to NULL... ");
  test(cpl_table_set_column_invalid(table, "Double", 20, 20),
                                  "Set Double 20 till end to NULL... ");

  array = (cpl_array *)cpl_table_get_array(table, "ADouble", 3);
  test_fvalue(0.0, 0.0001, cpl_array_get_double(array, 0, &null),
                                   "Read double array element 0...");
  test_ivalue(0, null, "Check double array element 0 is valid...");
  test_fvalue(5.0, 0.0001, cpl_array_get_double(array, 5, &null),
                                   "Read double array element 5...");
  test_ivalue(0, null, "Check double array element 5 is valid...");
  test_fvalue(19.0, 0.0001, cpl_array_get_double(array, 19, &null),
                                   "Read double array element 19...");
  test_ivalue(0, null, "Check array element 19 is valid...");

  cpl_array_set_invalid(array, 0);
  cpl_array_set_invalid(array, 5);
  cpl_array_set_invalid(array, 19);

  test_fvalue(0.0, 0.0001, cpl_array_get_double(array, 0, &null),
                                   "Read again double array element 0...");
  test_ivalue(1, null, "Check double array element 0 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_double(array, 5, &null),
                                   "Read again double array element 5...");
  test_ivalue(1, null, "Check double array element 5 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_double(array, 19, &null),
                                   "Read again double array element 19...");
  test_ivalue(1, null, "Check double array element 19 is invalid...");


/**+ FIXME: RESTORE!!! */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "12.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "12.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "12.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(11, cpl_table_count_invalid(table, "Double"),
                                "Re-count written Double NULLs... ");

  for (i = 0; i < 25; i++) {
    const double dval = cpl_table_get_double(table, "Double", i, &null);
    if (!null) {
        test_fvalue(dcheck[i], 0.000001, dval, "Check Double column... ");
    } else {
        cpl_test(i <= 2 || i >= 5);
        cpl_test(i <= 7 || i >= 20);
    }
  }

  /*
   * Check double array after reload
   */

  array = (cpl_array *)cpl_table_get_array(table, "ADouble", 3);

  test_fvalue(0.0, 0.0001, cpl_array_get_double(array, 0, &null),
                                   "Read loaded double array element 0...");
  test_ivalue(1, null, "Loaded double array element 0 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_double(array, 5, &null),
                                   "Read loaded double array element 5...");
  test_ivalue(1, null, "Loaded double array element 5 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_double(array, 19, &null),
                                   "Read loaded double array element 19...");
  test_ivalue(1, null, "Loaded double array element 19 is invalid...");

  /****/

  test(cpl_table_set_column_invalid(table, "Float", 0, 3),
                                  "Set Float 0-2 to NULL... ");
  test(cpl_table_set_column_invalid(table, "Float", 5, 3),
                                  "Set Float 5-7 to NULL... ");
  test(cpl_table_set_column_invalid(table, "Float", 20, 20),
                                  "Set Float 20 till end to NULL... ");

  array = (cpl_array *)cpl_table_get_array(table, "AFloat", 3);
  test_fvalue(0.0, 0.0001, cpl_array_get_float(array, 0, &null),
                                   "Read float array element 0...");
  test_ivalue(0, null, "Check float array element 0 is valid...");
  test_fvalue(5.0, 0.0001, cpl_array_get_float(array, 5, &null),
                                   "Read float array element 5...");
  test_ivalue(0, null, "Check double array element 5 is valid...");
  test_fvalue(19.0, 0.0001, cpl_array_get_float(array, 19, &null),
                                   "Read float array element 19...");
  test_ivalue(0, null, "Check float array element 19 is valid...");

  cpl_array_set_invalid(array, 0);
  cpl_array_set_invalid(array, 5);
  cpl_array_set_invalid(array, 19);

  test_fvalue(0.0, 0.0001, cpl_array_get_float(array, 0, &null),
                                   "Read again float array element 0...");
  test_ivalue(1, null, "Check float array element 0 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_float(array, 5, &null),
                                   "Read again float array element 5...");
  test_ivalue(1, null, "Check float array element 5 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_float(array, 19, &null),
                                   "Read again float array element 19...");
  test_ivalue(1, null, "Check float array element 19 is invalid...");

/**+ FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "13.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "13.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "13.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(11, cpl_table_count_invalid(table, "Float"),
                               "Re-count written Float NULLs... ");

  for (i = 0; i < 25; i++) {
    const float fval = cpl_table_get_float(table, "Float", i, &null);
    if (!null) {
        test_fvalue(fcheck[i], 0.000001, fval, "Check Float column... ");
    } else {
        cpl_test(i <= 2 || i >= 5);
        cpl_test(i <= 7 || i >= 20);
    }
  }

  /*
   * Check float array after reload
   */

  array = (cpl_array *)cpl_table_get_array(table, "AFloat", 3);

  test_fvalue(0.0, 0.0001, cpl_array_get_float(array, 0, &null),
                                   "Read loaded float array element 0...");
  test_ivalue(1, null, "Loaded float array element 0 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_float(array, 5, &null),
                                   "Read loaded float array element 5...");
  test_ivalue(1, null, "Loaded float array element 5 is invalid...");
  test_fvalue(0.0, 0.0001, cpl_array_get_float(array, 19, &null),
                                   "Read loaded float array element 19...");
  test_ivalue(1, null, "Loaded float array element 19 is invalid...");

  /****/

  test(cpl_table_set_column_invalid(table, "String", 0, 3),
                                  "Set String 0-2 to NULL... ");
  test(cpl_table_set_column_invalid(table, "String", 5, 3),
                                  "Set String 5-7 to NULL... ");
  test(cpl_table_set_column_invalid(table, "String", 20, 20),
                                  "Set String 20 till end to NULL... ");

/**+ FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "14.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "14.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "14.fits", 1, 1);
  cpl_test_nonnull(table);
  cpl_test_eq(cpl_table_get_nrow(table), 25);
/**/

  test_ivalue(11, cpl_table_count_invalid(table, "String"),
                            "Re-count written String NULLs... ");

  for (i = 0; i < 25; i++) {
      const char * sval = cpl_table_get_string(table, "String", i);

    if (sval != NULL) {
        test_svalue(scheck[i], sval, "Check String column... ");
    } else {
        cpl_test(i <= 2 || i >= 5);
        cpl_test(i <= 7 || i >= 20);
    }
  }

  test(cpl_table_erase_window(table, 21, 4), "Delete last 4 table rows... ");

  test(cpl_table_erase_window(table, 7, 4), 
                               "Delete table rows from 7 to 10... ");

  test(cpl_table_erase_window(table, 3, 3), 
                               "Delete table rows from 3 to 5... ");

  test(cpl_table_erase_window(table, 0, 2), "Delete first two table rows... ");

  test_ivalue(12, cpl_table_get_nrow(table), "Check table length (4)... ");

  test_ivalue(3, cpl_table_count_invalid(table, "Integer"), 
                                       "Count Integer NULLs... ");

  test_ivalue(3, cpl_table_count_invalid(table, "LongLong"),
                                       "Count LongLong NULLs... ");

  test_ivalue(3, cpl_table_count_invalid(table, "Double"), 
                                       "Count Double NULLs... ");

  test_ivalue(3, cpl_table_count_invalid(table, "String"), 
                                       "Count String NULLs... ");

  test_ivalue(3, cpl_table_count_invalid(table, "Float"), 
                                       "Count Float NULLs... ");

  test_ivalue(3, cpl_table_count_invalid(table, "AInt"),
                                       "Count AInt NULLs... ");

  test_ivalue(0, cpl_table_count_invalid(table, "ADouble"),
                                       "Count ADouble NULLs... ");

  test_ivalue(0, cpl_table_count_invalid(table, "AFloat"),
                                       "Count AFloat NULLs... ");

  test(cpl_table_insert_window(table, 20, 5),
                                "Append 5 NULLs at table end... ");

  test(cpl_table_insert_window(table, 6, 4),
                                "Insert segment of 4 NULLs at row 6... ");

  test(cpl_table_insert_window(table, 1, 2),
                                "Insert segment of 2 NULLs at row 1... ");

  test_ivalue(23, cpl_table_get_nrow(table), "Check table length (5)... ");

  test_ivalue(14, cpl_table_count_invalid(table, "Integer"), 
                                       "Count Integer NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "LongLong"),
                                       "Count LongLong NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "Double"), 
                                       "Count Double NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "String"), 
                                       "Count String NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "Float"), 
                                       "Count Float NULLs... ");

  test(cpl_table_fill_column_window_int(table, "Integer", 0, 2, 999),
       "Write 999 in \"Integer\" column from 0 to 1... ");

  test(cpl_table_fill_column_window_int(table, "Integer", 3, 3, 999),
       "Write 999 in \"Integer\" column from 3 to 5... ");

  test(cpl_table_fill_column_window_int(table, "Integer", 7, 4, 999),
       "Write 999 in \"Integer\" column from 7 to 10... ");

  test(cpl_table_fill_column_window_int(table, "Integer", 20, 7, 999),
       "Write 999 in \"Integer\" column from 20 to end... ");

  test(cpl_table_fill_column_window_long_long(table, "LongLong", 0, 2, 999),
       "Write 999 in \"LongLong\" column from 0 to 1... ");

  test(cpl_table_fill_column_window_long_long(table, "LongLong", 3, 3, 999),
       "Write 999 in \"LongLong\" column from 3 to 5... ");

  test(cpl_table_fill_column_window_long_long(table, "LongLong", 7, 4, 999),
       "Write 999 in \"LongLong\" column from 7 to 10... ");

  test(cpl_table_fill_column_window_long_long(table, "LongLong", 20, 7, 999),
       "Write 999 in \"LongLong\" column from 20 to end... ");

  test(cpl_table_fill_column_window_float(table, "Float", 0, 2, 999.99),
       "Write 999.99 in \"Float\" column from 0 to 1... ");

  test(cpl_table_fill_column_window_float(table, "Float", 3, 3, 999.99),
       "Write 999.99 in \"Float\" column from 3 to 5... ");

  test(cpl_table_fill_column_window_float(table, "Float", 7, 4, 999.99),
       "Write 999.99 in \"Float\" column from 7 to 10... ");

  test(cpl_table_fill_column_window_float(table, "Float", 20, 7, 999.99),
       "Write 999.99 in \"Float\" column from 20 to end... ");

  test(cpl_table_fill_column_window_double(table, "Double", 0, 2, 999.88),
       "Write 999.88 in \"Double\" column from 0 to 1... ");

  test(cpl_table_fill_column_window_double(table, "Double", 3, 3, 999.88),
       "Write 999.88 in \"Double\" column from 3 to 5... ");

  test(cpl_table_fill_column_window_double(table, "Double", 7, 4, 999.88),
       "Write 999.88 in \"Double\" column from 7 to 10... ");

  test(cpl_table_fill_column_window_double(table, "Double", 20, 7, 999.88),
       "Write 999.88 in \"Double\" column from 20 to end... ");

  test(cpl_table_fill_column_window_string(table, "String", 0, 2, "999"),
       "Write \"999\" in \"String\" column from 0 to 1... ");

  test(cpl_table_fill_column_window_string(table, "String", 3, 3, "999"),
       "Write \"999\" in \"String\" column from 3 to 5... ");

  test(cpl_table_fill_column_window_string(table, "String", 7, 4, "999"),
       "Write \"999\" in \"String\" column from 7 to 10... ");

  test(cpl_table_fill_column_window_string(table, "String", 20, 7, "999"),
       "Write \"999\" in \"String\" column from 20 to end... ");

/**+ FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "15.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "15.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "15.fits", 1, 1);
/**/

  test_ivalue(23, cpl_table_get_nrow(table), "Check table length (6)... ");

  test_ivalue(5, cpl_table_count_invalid(table, "Integer"), 
                                       "Count Integer NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "LongLong"),
                                       "Count LongLong NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "Float"), 
                                       "Count Float NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "Double"), 
                                       "Count Double NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "String"), 
                                       "Count String NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "AInt"),
                                       "Count AInt NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "ALongLong"),
                                       "Count AInt NULLs... ");

  test_ivalue(11, cpl_table_count_invalid(table, "AFloat"),
                                       "Count AFloat NULLs... ");

  test_ivalue(11, cpl_table_count_invalid(table, "ADouble"),
                                       "Count ADouble NULLs... ");

  test_ivalue(0, cpl_table_is_valid(table, "Integer", 2), 
                    "Check that third element of \"Integer\" is NULL... ");

  test_ivalue(0, cpl_table_is_valid(table, "LongLong", 2),
                    "Check that third element of \"LongLong\" is NULL... ");

  test_ivalue(1, cpl_table_is_valid(table, "Double", 0), 
                    "Check that first element of \"Double\" is not NULL... ");

  test_ivalue(1, cpl_table_is_valid(table, "String", 0), 
                    "Check that first element of \"String\" is not NULL... ");

  test_ivalue(0, cpl_table_is_valid(table, "String", 2), 
                    "Check that third element of \"String\" is NULL... ");

  test_ivalue(0, cpl_table_is_valid(table, "AInt", 17),
                    "Check that third element of \"AInt\" is NULL... ");

  test_ivalue(0, cpl_table_is_valid(table, "ALongLong", 17),
                    "Check that third element of \"ALongLong\" is NULL... ");

  test_ivalue(1, cpl_table_is_valid(table, "ADouble", 17),
                    "Check that first element of \"ADouble\" is not NULL... ");

  test_ivalue(1, cpl_table_is_valid(table, "AFloat", 17),
                    "Check that third element of \"AFloat\" is NULL... ");

  test_data(copia, cpl_table_duplicate(table), "Duplicate table... ");

  test(cpl_table_duplicate_column(table, "New Integer", table, "Integer"),
                        "Duplicate \"Integer\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New LongLong", table, "LongLong"),
                        "Duplicate \"LongLong\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New Float", table, "Float"),
                        "Duplicate \"Float\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New Double", table, "Double"),
                        "Duplicate \"Double\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New String", table, "String"),
                        "Duplicate \"String\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New AInt", table, "AInt"),
                        "Duplicate \"AInt\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New ALongLong", table, "ALongLong"),
                        "Duplicate \"ALongLong\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New AFloat", table, "AFloat"),
                        "Duplicate \"AFloat\" column within same table... ");

  test(cpl_table_duplicate_column(table, "New ADouble", table, "ADouble"),
                        "Duplicate \"ADouble\" column within same table... ");

  test_ivalue(5, cpl_table_count_invalid(table, "New Integer"), 
                                       "Count New Integer NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "New LongLong"),
                                       "Count New LongLong NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "New Float"), 
                                       "Count New Float NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "New Double"), 
                                       "Count New Double NULLs... ");

  test_ivalue(5, cpl_table_count_invalid(table, "New String"), 
                                       "Count New String NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "New AInt"),
                                       "Count New AInt NULLs... ");

  test_ivalue(14, cpl_table_count_invalid(table, "New ALongLong"),
                                       "Count New ALongLong NULLs... ");

  test_ivalue(11, cpl_table_count_invalid(table, "New AFloat"),
                                       "Count New AFloat NULLs... ");

  test_ivalue(11, cpl_table_count_invalid(table, "New ADouble"),
                                       "Count New ADouble NULLs... ");

  test(cpl_table_move_column(copia, "New Integer", table), 
           "Moving column \"New Integer\" to another table... ");

  test(cpl_table_move_column(copia, "New LongLong", table),
           "Moving column \"New LongLong\" to another table... ");

  test(cpl_table_move_column(copia, "New Float", table), 
           "Moving column \"New Float\" to another table... ");

  test(cpl_table_move_column(copia, "New Double", table), 
           "Moving column \"New Double\" to another table... ");

  test(cpl_table_move_column(copia, "New String", table), 
           "Moving column \"New String\" to another table... ");

  test_failure(CPL_ERROR_ILLEGAL_OUTPUT, 
               cpl_table_name_column(copia, "New String", "String"),
               "Try illegal column renaming... ");

  test(cpl_table_name_column(copia, "New Integer", "Old Integer"),
           "Try legal column renaming... ");

/**+ FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "New ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "16.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "16.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "16.fits", 1, 1);
/**/

  test_ivalue(!0, cpl_table_has_column(copia, "Old Integer"),
           "Check if column \"Old Integer\" exists... ");

#ifdef CPL_TABLE_TEST_DEPRECATED
  test_svalue("Integer", cpl_table_get_column_name(copia),
                                            "Check name column 1... ");

  test_svalue("LongLong", cpl_table_get_column_name(NULL),
                                            "Check name column 2... ");

  test_svalue("Double", cpl_table_get_column_name(NULL),
                                            "Check name column 3... ");

  test_svalue("CDouble", cpl_table_get_column_name(NULL),
                                            "Check name column 3a... ");

  test_svalue("CDoubleDouble", cpl_table_get_column_name(NULL),
                                            "Check name column 3b... ");

  test_svalue("String", cpl_table_get_column_name(NULL),
                                            "Check name column 4... ");

  test_svalue("Float", cpl_table_get_column_name(NULL),
                                            "Check name column 5... ");

  test_svalue("CFloat", cpl_table_get_column_name(NULL),
                                            "Check name column 5a... ");

  test_svalue("AInt", cpl_table_get_column_name(NULL),
                                            "Check name column 6... ");

  test_svalue("ALongLong", cpl_table_get_column_name(NULL),
                                            "Check name column 7... ");

  test_svalue("AFloat", cpl_table_get_column_name(NULL),
                                            "Check name column 8... ");
  
  test_svalue("ADouble", cpl_table_get_column_name(NULL),
                                            "Check name column 9... ");

  test_svalue("CAFloat", cpl_table_get_column_name(NULL),
                                            "Check name column 8a... ");
  
  test_svalue("CADouble", cpl_table_get_column_name(NULL),
                                            "Check name column 9a... ");
  
  test_svalue("AString", cpl_table_get_column_name(NULL),
                                            "Check name column 10... ");

  test_svalue("Old Integer", cpl_table_get_column_name(NULL),
                                            "Check name column 11... ");

  test_svalue("New LongLong", cpl_table_get_column_name(NULL),
                                            "Check name column 12... ");

  test_svalue("New Float", cpl_table_get_column_name(NULL),
                                            "Check name column 13... ");

  test_svalue("New Double", cpl_table_get_column_name(NULL),
                                            "Check name column 14... ");

  test_svalue("New String", cpl_table_get_column_name(NULL),
                                            "Check name column 15... ");

  test_pvalue(NULL, (void *)cpl_table_get_column_name(NULL),
                                            "Check if no more colums... ");
#endif

  /*
   * Test new function cpl_table_get_column_names()
   */

  colnames = cpl_table_get_column_names(copia);
  test_ivalue(20, cpl_array_get_size(colnames), "Count table colnames... ");

  test_svalue("Integer", cpl_array_get_string(colnames, 0),
                                            "Check name col 1... ");

  test_svalue("LongLong", cpl_array_get_string(colnames, 1),
                                            "Check name col 2... ");

  test_svalue("Double", cpl_array_get_string(colnames, 2),
                                            "Check name col 3... ");

  test_svalue("CDouble", cpl_array_get_string(colnames, 3),
                                            "Check name col 4... ");

  test_svalue("CDoubleDouble", cpl_array_get_string(colnames, 4),
                                            "Check name column 5... ");

  test_svalue("String", cpl_array_get_string(colnames, 5),
                                            "Check name col 6... ");

  test_svalue("Float", cpl_array_get_string(colnames, 6),
                                            "Check name col 7... ");

  test_svalue("CFloat", cpl_array_get_string(colnames, 7),
                                            "Check name col 8... ");

  test_svalue("AInt", cpl_array_get_string(colnames, 8),
                                            "Check name col 9... ");

  test_svalue("ALongLong", cpl_array_get_string(colnames, 9),
                                            "Check name col 10... ");

  test_svalue("AFloat", cpl_array_get_string(colnames, 10),
                                            "Check name col 11... ");

  test_svalue("ADouble", cpl_array_get_string(colnames, 11),
                                            "Check name col 12... ");

  test_svalue("CAFloat", cpl_array_get_string(colnames, 12),
                                            "Check name col 13... ");

  test_svalue("CADouble", cpl_array_get_string(colnames, 13),
                                            "Check name col 14... ");

  test_svalue("AString", cpl_array_get_string(colnames, 14),
                                            "Check name col 15... ");

  test_svalue("Old Integer", cpl_array_get_string(colnames, 15),
                                            "Check name col 16... ");

  test_svalue("New LongLong", cpl_array_get_string(colnames, 16),
                                            "Check name col 17... ");

  test_svalue("New Float", cpl_array_get_string(colnames, 17),
                                            "Check name col 18... ");

  test_svalue("New Double", cpl_array_get_string(colnames, 18),
                                            "Check name col 19... ");

  test_svalue("New String", cpl_array_get_string(colnames, 19),
                                            "Check name col 20... ");
  cpl_array_delete(colnames);
  cpl_table_delete(copia);


  test(cpl_table_set_size(table, 30), "Expanding table to 30 rows... ");

/*
 * The following would do the same as cpl_table_set_size(table, 30), in
 * case cpl_table_set_size() would be crossed out...

  test(cpl_table_insert_window(table, 24, 7), "Expanding table to 30 rows... ");
*/

  test_ivalue(12, cpl_table_count_invalid(table, "Integer"),
                                       "Count \"Integer\" NULLs... ");

  test_ivalue(12, cpl_table_count_invalid(table, "String"),
                                       "Count \"String\" NULLs... ");

  test(cpl_table_set_size(table, 22), "Truncating table to 22 rows... ");

/*
 * The following would do the same as cpl_table_set_size(table, 30), in
 * case cpl_table_set_size() would be crossed out...

  test(cpl_table_erase_window(table, 22, 1000), 
                               "Truncating table to 22 rows... ");
*/

/**+ FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "LongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_long_long(table, "New ALongLong", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "17.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "17.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "17.fits", 1, 1);
/**/

  cpl_table_erase_column(table, "CDouble");
  cpl_table_erase_column(table, "CDoubleDouble");
  cpl_table_erase_column(table, "CFloat");
  cpl_table_erase_column(table, "CAFloat");
  cpl_table_erase_column(table, "CADouble");

  // FIXME: To be continued.
  cpl_table_erase_column(table, "LongLong");
  cpl_table_erase_column(table, "ALongLong");
  cpl_table_erase_column(table, "New ALongLong");

  // FIXME: Remove the string array column here because for some of
  //        the following tests it would mess up the expected row/column
  //        numbers. They need to be fixed before this line is removed!
  cpl_table_erase_column(table, "AString");


  test_ivalue(5, cpl_table_count_invalid(table, "Integer"),
                                       "Count \"Integer\" NULLs (2)... ");

  test_ivalue(5, cpl_table_count_invalid(table, "String"),
                                       "Count \"String\" NULLs (2)... ");

  test_data(copia, cpl_table_extract(table, 0, 5), 
                       "Creating subtable from rows 0-5 of original... ");

  test_ivalue(1, cpl_table_count_invalid(copia, "Integer"),
                                       "Count \"Integer\" NULLs... ");

  test_ivalue(1, cpl_table_count_invalid(copia, "String"),
                                       "Count \"String\" NULLs... ");

  cpl_table_delete(copia);

  test_data(copia, cpl_table_extract(table, 8, 5), 
                       "Creating subtable from rows 8-5 of original... ");

  test_ivalue(1, cpl_table_count_invalid(copia, "Float"),
                                       "Count \"Float\" NULLs... ");

  test_ivalue(1, cpl_table_count_invalid(copia, "String"),
                                       "Count \"String\" NULLs... ");

  cpl_table_delete(copia);

  test_data(copia, cpl_table_extract(table, 15, 30), 
              "Creating subtable from rows 15 till end of original... ");

  test_ivalue(3, cpl_table_count_invalid(copia, "Double"),
                                       "Count \"Double\" NULLs... ");

  test_ivalue(3, cpl_table_count_invalid(copia, "String"),
                                       "Count \"String\" NULLs... ");

  cpl_table_delete(copia);

  test(cpl_table_cast_column(table, "Float", "FloatToInt", CPL_TYPE_INT),
                      "Casting float column to integer colum... ");

/**+ FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "FloatToInt", -2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "18.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "18.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "18.fits", 1, 1);
/**/

  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 0, &null),
                       "Check element  1 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 1, &null),
                       "Check element  2 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "FloatToInt", 2),
                       "Check element  3 of casted column... ");
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 3, &null),
                       "Check element  4 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 4, &null),
                       "Check element  5 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 5, &null),
                       "Check element  6 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(-1, cpl_table_get_int(table, "FloatToInt", 6, &null),
                       "Check element  7 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 7, &null),
                       "Check element  8 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 8, &null),
                       "Check element  9 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 9, &null),
                       "Check element 10 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 10, &null),
                       "Check element 11 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "FloatToInt", 11),
                       "Check element 12 of casted column... ");
  test_ivalue(3, cpl_table_get_int(table, "FloatToInt", 12, &null),
                       "Check element 13 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(7, cpl_table_get_int(table, "FloatToInt", 13, &null),
                       "Check element 14 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(1, cpl_table_get_int(table, "FloatToInt", 14, &null),
                       "Check element 15 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(4, cpl_table_get_int(table, "FloatToInt", 15, &null),
                       "Check element 16 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(6, cpl_table_get_int(table, "FloatToInt", 16, &null),
                       "Check element 17 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "FloatToInt", 17),
                       "Check element 18 of casted column... ");
  test_ivalue(0, cpl_table_is_valid(table, "FloatToInt", 18),
                       "Check element 19 of casted column... ");
  test_ivalue(0, cpl_table_is_valid(table, "FloatToInt", 19),
                       "Check element 20 of casted column... ");
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 20, &null),
                       "Check element 21 of casted column... ");
  cpl_test_zero(null);
  test_ivalue(999, cpl_table_get_int(table, "FloatToInt", 21, &null),
                       "Check element 22 of casted column... ");
  cpl_test_zero(null);

  test(cpl_table_erase_column(table, "FloatToInt"),
                                      "Delete casted column... ");

  test(cpl_table_cast_column(table, "Integer", "IntToFloat", CPL_TYPE_FLOAT),
                      "Casting integer column to float colum... ");

/**- FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "19.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "19.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "19.fits", 1, 1);
/**/

  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 0, &null),
                     "Check element  1 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 1, &null),
                     "Check element  2 of casted column (2)... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 2),
                     "Check element  3 of casted column (2)... ");
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 3, &null),
                     "Check element  4 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 4, &null),
                     "Check element  5 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 5, &null),
                     "Check element  6 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(-1.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 6, &null),
                     "Check element  7 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 7, &null),
                     "Check element  8 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 8, &null),
                     "Check element  9 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 9, &null),
                     "Check element 10 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 10, &null),
                     "Check element 11 of casted column (2)... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 11),
                     "Check element 12 of casted column (2)... ");
  test_fvalue(3.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 12, &null),
                     "Check element 13 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(7.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 13, &null),
                     "Check element 14 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(1.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 14, &null),
                     "Check element 15 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(4.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 15, &null),
                     "Check element 16 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(6.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 16, &null),
                     "Check element 17 of casted column (2)... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 17),
                     "Check element 18 of casted column (2)... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 18),
                     "Check element 19 of casted column (2)... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 19),
                     "Check element 20 of casted column (2)... ");
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 20, &null),
                     "Check element 21 of casted column (2)... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 21, &null),
                     "Check element 22 of casted column (2)... ");
                     cpl_test_zero(null);

  test(cpl_table_shift_column(table, "IntToFloat", 1), 
                              "Shift new column one position down... ");

  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 0),
                     "Check element  1 of shifted column... ");
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 1, &null),
                     "Check element  2 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 2, &null),
                     "Check element  3 of shifted column... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 3),
                     "Check element  4 of shifted column... ");
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 4, &null),
                     "Check element  5 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 5, &null),
                     "Check element  6 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 6, &null),
                     "Check element  7 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(-1.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 7, &null),
                     "Check element  8 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 8, &null),
                     "Check element  9 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 9, &null),
                     "Check element 10 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 10, &null),
                     "Check element 11 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 11, &null),
                     "Check element 12 of shifted column... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 12),
                     "Check element 13 of shifted column... ");
  test_fvalue(3.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 13, &null),
                     "Check element 14 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(7.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 14, &null),
                     "Check element 15 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(1.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 15, &null),
                     "Check element 16 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(4.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 16, &null),
                     "Check element 17 of shifted column... ");
                     cpl_test_zero(null);
  test_fvalue(6.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 17, &null),
                     "Check element 18 of shifted column... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 18),
                     "Check element 19 of shifted column... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 19),
                     "Check element 20 of shifted column... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 20),
                     "Check element 21 of shifted column... ");
  test_fvalue(999.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 21, &null),
                     "Check element 22 of shifted column... ");
                     cpl_test_zero(null);

  test(cpl_table_add_columns(table, "Integer", "IntToFloat"), 
                            "Sum \"IntToFloat\" to \"Integer\"... ");

/**- FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "20.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "20.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "20.fits", 1, 1);
/**/

  cpl_table_test_20(table);

  test(cpl_table_name_column(table, "IntToFloat", "Float"),
           "Renaming \"IntToFloat\" to \"Float\"... ");

  test(cpl_table_set_column_invalid(table, "Integer", 7, 2), 
                              "Set NULLs in \"Integer\" column... ");

  test(cpl_table_set_invalid(table, "Float", 7), 
                              "Set NULL in \"Float\" column... ");

  test(cpl_table_set_invalid(table, "Float", 9), 
                              "Set another NULL in \"Float\" column... ");

  test(cpl_table_set_invalid(table, "Double", 7), 
                              "Set NULL in \"Double\" column... ");

  test(cpl_table_set_invalid(table, "String", 7), 
                              "Set NULL in \"String\" column... ");

  test(cpl_table_new_column(table, "Sequence", CPL_TYPE_INT),
                                "Creating the \"Sequence\" column... ");

  for (i = 0; i < 18; i++) {
    sprintf(message, "Writing to row %d of the \"Sequence\" column... ", i);
    test(cpl_table_set_int(table, "Sequence", i, i), message);
  }

  names[0] = "Integer";

  reflist = cpl_propertylist_new();
  cpl_propertylist_append_bool(reflist, names[0], 0);
/* %$% */
/* *+
error = cpl_table_set_column_unit(table, "Integer", "Pixel");
cpl_test_eq_error(error, CPL_ERROR_NONE);
cpl_table_dump_structure(table, NULL);
cpl_table_dump(table, 0, cpl_table_get_nrow(table), NULL);
exit;
+* */

  {
    int *hold = cpl_table_get_data_int(table, "Integer");
  
    test(cpl_table_sort(table, reflist), 
         "Sorting by increasing values of the \"Integer\" column... ");
  
    test_pvalue(hold, cpl_table_get_data_int(table, "Integer"), 
                                           "Check pointer... ");
  
    cpl_propertylist_delete(reflist);
  
    error = cpl_table_set_column_unit(table, "AInt", "Pixel");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
  
    test_svalue("Pixel", cpl_table_get_column_unit(table, "AInt"),
                                              "Check column unit... ");
  }

/*
  if (strcmp(unit = (char *)cpl_table_get_column_unit(table, "AInt"),
      "Pixel")) {
    printf("Check column unit... "); 
    printf("Expected \"Pixel\", obtained \"%s\"\n", unit);
    cpl_end();
    return 1;
  }
*/

/**- FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "21.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "21.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "21.fits", 1, 1);
/**/

  cpl_table_test_21(table);

  cpl_table_delete(table);
  table = cpl_table_load(BASE "22.fits", 1, 1);
/**/

  cpl_table_test_22(table);

  /*
   * Here partial loading of table is tested
   */

  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "24.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "24.fits");
  cpl_table_delete(table);
  colnames = cpl_array_new(7, CPL_TYPE_STRING);
  cpl_array_set_string(colnames, 0, "Integer");
  cpl_array_set_string(colnames, 1, "Double");
  cpl_array_set_string(colnames, 2, "String");
  cpl_array_set_string(colnames, 3, "AInt");
  cpl_array_set_string(colnames, 4, "AFloat");
  cpl_array_set_string(colnames, 5, "ADouble");
  cpl_array_set_string(colnames, 6, "Float");
  table = cpl_table_load_window(BASE "24.fits", 1, 1, colnames, 7, 6);
  cpl_array_delete(colnames);

  /*  Test some values here... */

  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "25.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "25.fits");

  cpl_table_delete(table);
  cpl_free(fArray);
#ifdef _Complex_I
  cpl_free(cfArray);
#endif

  return 0;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the another part of the CPL table functions
  @return   Zero iff successful
 */
/*----------------------------------------------------------------------------*/
static int cpl_table_test_rest(void)
{

    int                nrows = NROWS;
    int                i, j, k, null;
    int                pp;
    char               message[80];

    const char        *unit;

    cpl_table         *table;
    cpl_table         *copia;
    cpl_array         *array;
    cpl_array         *dimensions;
    cpl_array        **arrays;

    FILE              *stream;

    cpl_error_code     error;

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull( stream );


  /*
   * Powers
   */

  nrows = 7;

  table = cpl_table_new(nrows);
  error = cpl_table_new_column(table, "Int", CPL_TYPE_INT);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_new_column(table, "Float", CPL_TYPE_FLOAT);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_new_column(table, "Double", CPL_TYPE_DOUBLE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_new_column(table, "CFloat", CPL_TYPE_FLOAT_COMPLEX);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_new_column(table, "CDouble", CPL_TYPE_DOUBLE_COMPLEX);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  for (i = 0; i < nrows; i++) {
      error = cpl_table_set_int(table, "Int", i, i);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
      error = cpl_table_set_float(table, "Float", i, i);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
      error = cpl_table_set_double(table, "Double", i, i);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
#ifdef _Complex_I
      error = cpl_table_set_float_complex(table, "CFloat", i, i + i * I);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
      error = cpl_table_set_double_complex(table, "CDouble", i, i + I);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
#endif
  }

  error = cpl_table_exponential_column(table, "Int", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_exponential_column(table, "Float", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_exponential_column(table, "Double", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_exponential_column(table, "CFloat", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_exponential_column(table, "CDouble", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  pp = 1;
  for (i = 0; i < nrows; i++) {
#ifdef _Complex_I
      const double ln2 = CPL_MATH_LN2;

      test_cvalue(exp(i*ln2)*(cos(i*ln2) + I*sin(i*ln2)), 0.00001, 
                  cpl_table_get_float_complex(table, "CFloat", i, &null), 
                  "Check expo CFloat... ");
      cpl_test_zero(null);

      test_cvalue(exp(i*ln2)*(cos(ln2) + I*sin(ln2)), 0.00001, 
                  cpl_table_get_double_complex(table, "CDouble", i, &null), 
                  "Check expo CDouble... ");
      cpl_test_zero(null);
#endif

      test_ivalue(pp, cpl_table_get_int(table, 
                      "Int", i, &null), "Check expo Int... ");
      cpl_test_zero(null);
      test_fvalue((float)pp, 0.00001, cpl_table_get_float(table, 
                      "Float", i, &null), "Check expo Float... ");
      cpl_test_zero(null);
      test_fvalue((float)pp, 0.00001, cpl_table_get_double(table, 
                      "Double", i, &null), "Check expo Double... ");
      cpl_test_zero(null);
      pp *= 2;

  }

  error = cpl_table_logarithm_column(table, "Int", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_logarithm_column(table, "Float", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_logarithm_column(table, "Double", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_logarithm_column(table, "CFloat", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_logarithm_column(table, "CDouble", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  for (i = 0; i < nrows; i++) {
#ifdef _Complex_I
      const double ln2 = CPL_MATH_LN2;

      test_cvalue(clog(exp(i*ln2)*(cos(i*ln2) + I*sin(i*ln2)))/ln2, 0.00001,
                  cpl_table_get_float_complex(table,
                     "CFloat", i, &null), "Check log CFloat... ");
      cpl_test_zero(null);
      test_cvalue(clog(exp(i*ln2)*(cos(ln2) + I*sin(ln2)))/ln2, 0.00001,
                  cpl_table_get_double_complex(table,
                     "CDouble", i, &null), "Check log CDouble... ");
      cpl_test_zero(null);
#endif
      test_ivalue(i, cpl_table_get_int(table, 
                     "Int", i, &null), "Check log Int... ");
      cpl_test_zero(null);
      test_fvalue((float)i, 0.00001, cpl_table_get_float(table, 
                     "Float", i, &null), "Check log Float... ");
      cpl_test_zero(null);
      test_fvalue((float)i, 0.00001, cpl_table_get_double(table, 
                     "Double", i, &null), "Check log Double... ");
      cpl_test_zero(null);
  }

#ifdef _Complex_I
  for (i = 0; i < nrows; i++) {
      error = cpl_table_set_float_complex(table, "CFloat", i, i + i * I);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
      test_cvalue(i + i*I, 0.0, cpl_table_get_float_complex(table, 
                  "CFloat", i, &null), " ");
      cpl_test_zero(null);
      error = cpl_table_set_double_complex(table, "CDouble", i, i + I);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
      test_cvalue(i + I, 0.0, cpl_table_get_double_complex(table, 
                  "CDouble", i, &null), " ");
      cpl_test_zero(null);
  }
#endif

  cpl_test_zero(cpl_table_count_invalid(table, "CFloat"));
  cpl_test_zero(cpl_table_count_invalid(table, "CDouble"));

  error = cpl_table_power_column(table, "Int", 2.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "Float", 2.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "Double", 2.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CFloat", 2.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CDouble", 2.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_test_zero(cpl_table_count_invalid(table, "CFloat"));
  cpl_test_zero(cpl_table_count_invalid(table, "CDouble"));

  for (i = 0; i < nrows; i++) {
      test_ivalue(i*i, cpl_table_get_int(table,
                     "Int", i, &null), "Check pow Int... ");
      cpl_test_zero(null);
      test_fvalue((float)i*i, 0.00001, cpl_table_get_float(table,
                     "Float", i, &null), "Check pow Float... ");
      cpl_test_zero(null);
      test_fvalue((float)i*i, 0.00001, cpl_table_get_double(table,
                     "Double", i, &null), "Check pow Double... ");
      cpl_test_zero(null);
#ifdef _Complex_I
      test_cvalue(2*I*i*i, 0.00001, cpl_table_get_float_complex(table,
                     "CFloat", i, &null), "Check pow CFloat... ");
      cpl_test_zero(null);
      test_cvalue(i*i - 1 + 2*i*I, 0.00001, cpl_table_get_double_complex(table,
                     "CDouble", i, &null), "Check pow CDouble... ");
      cpl_test_zero(null);
#endif
  }

#ifdef _Complex_I
  /* On a ppc-Mac with gcc 4.0 cpowf(0.0, 2.0) returns NaN */
  /* FIXME: For a proper unit test the table should be re-set for each test */
  error = cpl_table_set_float_complex(table, "CFloat", 0, 0.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
#endif

  error = cpl_table_power_column(table, "Int", 0.5);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "Float", 0.5); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "Double", 0.5); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CFloat", 0.5); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CDouble", 0.5); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);


  for (i = 0; i < nrows; i++) {
      test_ivalue(i, cpl_table_get_int(table,
                     "Int", i, &null), "Check sqrt Int... ");
      cpl_test_zero(null);
      test_fvalue((float)i, 0.00001, cpl_table_get_float(table,
                     "Float", i, &null), "Check sqrt Float... ");
      cpl_test_zero(null);
      test_fvalue((float)i, 0.00001, cpl_table_get_double(table,
                     "Double", i, &null), "Check sqrt Double... ");
      cpl_test_zero(null);
#ifdef _Complex_I
      test_cvalue(i + i*I, 0.00001, cpl_table_get_float_complex(table,
                     "CFloat", i, &null), "Check sqrt CFloat... ");
      cpl_test_zero(null);
      test_cvalue(i + I, 0.00001, cpl_table_get_double_complex(table,
                     "CDouble", i, &null), "Check sqrt CDouble... ");
      cpl_test_zero(null);
#endif
  }

#ifdef _Complex_I
  /* On a ppc-Mac with gcc 4.0 cpowf(0.0, 0.5) returns NaN */
  /* FIXME: For a proper unit test the table should be re-set for each test */
  error = cpl_table_set_float_complex(table, "CFloat", 0, 0.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
#endif

  error = cpl_table_power_column(table, "Int", -1.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "Float", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "Double", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CFloat", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CDouble", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  /* Zero valued elements are rejected */
  cpl_test_zero(cpl_table_is_valid(table, "Int", 0));
  cpl_test_zero(cpl_table_is_valid(table, "Float", 0));
  cpl_test_zero(cpl_table_is_valid(table, "Double", 0));
  cpl_test_zero(cpl_table_is_valid(table, "CFloat", 0));

  cpl_test_eq(cpl_table_count_invalid(table, "Int"), 1);
  cpl_test_eq(cpl_table_count_invalid(table, "Float"), 1);
  cpl_test_eq(cpl_table_count_invalid(table, "Double"), 1);
  cpl_test_eq(cpl_table_count_invalid(table, "CFloat"), 1);

  for (i = 1; i < nrows; i++) {
      test_ivalue(i == 1 || i == 2 ? 1.0 : 0.0, cpl_table_get_int(table,
                     "Int", i, &null), "Check pow(-1) Int... ");
      cpl_test_zero(null);
      test_fvalue(1.0/(double)i, 0.00001, cpl_table_get_float(table,
                     "Float", i, &null), "Check pow(-1) Float... ");
      cpl_test_zero(null);
      test_fvalue(1.0/(double)i, 0.00001, cpl_table_get_double(table,
                     "Double", i, &null), "Check pow(-1) Double... ");
      cpl_test_zero(null);
#ifdef _Complex_I
      test_cvalue(1.0/((double)i + (double)i*I), 0.00001,
                  cpl_table_get_float_complex(table,
                     "CFloat", i, &null), "Check pow(-1) CFloat... ");
      cpl_test_zero(null);
      test_cvalue(1.0/((double)i + I), 0.00001,
                  cpl_table_get_double_complex(table,
                     "CDouble", i, &null), "Check pow(-1) CDouble... ");
      cpl_test_zero(null);
#endif
  }


  /* Set the values back */
  for (i = 0; i < nrows; i++) {
      error = cpl_table_set_int(table, "Int", i, i);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
  }
  error = cpl_table_power_column(table, "Float", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "Double", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CFloat", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_power_column(table, "CDouble", -1.0); 
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  error = cpl_table_set_float(table, "Float", 0, 0.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_double(table, "Double", 0, 0.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
#ifdef _Complex_I
  error = cpl_table_set_float_complex(table, "CFloat", 0, 0.0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
#endif

  cpl_test_zero(cpl_table_has_invalid(table, "Int"));
  cpl_test_zero(cpl_table_has_invalid(table, "Float"));
  cpl_test_zero(cpl_table_has_invalid(table, "Double"));
  cpl_test_zero(cpl_table_has_invalid(table, "CFloat"));
  cpl_test_zero(cpl_table_has_invalid(table, "CDouble"));

  /*
   * Test cpl_table_abs_column()
   */

  error = cpl_table_set_int(table, "Int", 2, -2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_float(table, "Float", 2, -2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_double(table, "Double", 2, -2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_table_abs_column(table, "Int");
  cpl_table_abs_column(table, "Float");
  cpl_table_abs_column(table, "Double");
  cpl_table_abs_column(table, "CFloat");
  cpl_table_abs_column(table, "CDouble");

  for (i = 0; i < nrows; i++) {
      double s2 = sqrt(2.0);

      test_ivalue(i, cpl_table_get_int(table,
                     "Int", i, &null), "Check abs Int... ");
      cpl_test_zero(null);
      test_fvalue((float)i, 0.00001, cpl_table_get_float(table,
                     "Float", i, &null), "Check abs Float... ");
      cpl_test_zero(null);
      test_fvalue((float)i, 0.00001, cpl_table_get_double(table,
                     "Double", i, &null), "Check abs Double... ");
      cpl_test_zero(null);
      test_fvalue(i*s2, 0.00001, cpl_table_get_float(table,
                     "CFloat", i, &null), "Check abs CFloat... ");
      cpl_test_zero(null);
      test_fvalue(sqrt(i*i + 1), 0.00001, cpl_table_get_double(table,
                     "CDouble", i, &null), "Check abs CDouble... ");
      cpl_test_zero(null);
  }

  cpl_table_delete(table);


  /*
   * Testing the selection functions
   */

  nrows = 7;

  table = cpl_table_new(nrows);
  cpl_table_new_column(table, "Int", CPL_TYPE_INT);
  cpl_table_new_column(table, "String", CPL_TYPE_STRING);

  unit = "abcd\0efgh\0ijkl\0mnop\0qrst\0uvwx\0yz";

  for (i = 0; i < nrows; i++) {
       error = cpl_table_set_int(table, "Int", i, i);
       cpl_test_eq_error(error, CPL_ERROR_NONE);
       error = cpl_table_set_string(table, "String", i, unit + i*5);
       cpl_test_eq_error(error, CPL_ERROR_NONE);
  }

  cpl_table_duplicate_column(table, "Int2", table, "Int");
  cpl_table_multiply_columns(table, "Int2", "Int2");
  cpl_table_cast_column(table, "Int", "Float", CPL_TYPE_FLOAT);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now erase all selected:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  test_ivalue(7, cpl_table_count_selected(copia), "Check all selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(0, cpl_table_get_nrow(copia), 
              "Check length erase all selected... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete all Int >= Int2:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected(copia, "Int", CPL_NOT_LESS_THAN, "Int2");
  test_ivalue(2, cpl_table_count_selected(copia), 
              "Check Int >= Int2 selected... ");
  array = cpl_table_where_selected(copia);
  test_ivalue(CPL_TYPE_SIZE, cpl_array_get_type(array),
                                            "Check type of where array...");
  test_ivalue(2, cpl_array_get_size(array), "Check size of where array...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
                                   "Check first element of where array...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
                                   "Check second element of where array...");
  cpl_test_zero(null);
  cpl_array_delete(array);
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(5, cpl_table_get_nrow(copia), 
              "Check length erase all Int >= Int2... ");
  array = cpl_table_where_selected(copia);
  test_ivalue(CPL_TYPE_SIZE, cpl_array_get_type(array),
                                    "Check type of everywhere array...");
  test_ivalue(5, cpl_array_get_size(array), 
                                    "Check size of everywhere array...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
                                "Check first element of everywhere array...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
                                "Check second element of everywhere array...");
  cpl_test_zero(null);
  test_ivalue(2, cpl_array_get_cplsize(array, 2, &null),
                                "Check third element of everywhere array...");
  cpl_test_zero(null);
  test_ivalue(3, cpl_array_get_cplsize(array, 3, &null),
                                "Check fourth element of everywhere array...");
  cpl_test_zero(null);
  test_ivalue(4, cpl_array_get_cplsize(array, 4, &null),
                                "Check fifth element of everywhere array...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  cpl_table_unselect_all(copia);
  array = cpl_table_where_selected(copia);
  test_ivalue(CPL_TYPE_SIZE, cpl_array_get_type(array),
                                    "Check type of nowhere array...");
  test_ivalue(0, cpl_array_get_size(array),
                                    "Check size of nowhere array...");
  cpl_array_delete(array);

  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete all Int > 3:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_int(copia, "Int", CPL_GREATER_THAN, 3);
  test_ivalue(3, cpl_table_count_selected(copia), 
              "Check Int > 3 selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(4, cpl_table_get_nrow(copia), 
              "Check length erase all Int > 3... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete all Int2 > Float:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected(copia, "Int2", CPL_GREATER_THAN, "Float");
  test_ivalue(5, cpl_table_count_selected(copia), 
              "Check Int2 > Float selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(2, cpl_table_get_nrow(copia), 
              "Check length erase all Int2 > Float... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete all String == \"^[a-l].*\":\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_string(copia, "String", CPL_EQUAL_TO, "^[a-l].*");
  test_ivalue(3, cpl_table_count_selected(copia), 
              "Check String == \"^[a-l].*\" selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(4, cpl_table_get_nrow(copia), 
              "Check length erase all String == \"^[a-l].*\"... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete all String > \"carlo\":\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_string(copia, "String", CPL_GREATER_THAN, "carlo");
  test_ivalue(6, cpl_table_count_selected(copia), 
              "Check String > \"carlo\" selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(1, cpl_table_get_nrow(copia), 
              "Check length erase all String > \"carlo\"... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete all String > \"tattoo\" and Int == 3:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_string(copia, "String", CPL_GREATER_THAN, "tattoo");
  cpl_table_or_selected_int(copia, "Int", CPL_EQUAL_TO, 3);
  test_ivalue(3, cpl_table_count_selected(copia), 
              "Check String > \"tattoo\" and Int == 3 selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(4, cpl_table_get_nrow(copia),
              "Check length erase all String > \"tattoo\" and Int == 3... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "Now keep all String > \"tattoo\" and Int == 3:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_string(copia, "String", CPL_GREATER_THAN, "tattoo");
  cpl_table_or_selected_int(copia, "Int", CPL_EQUAL_TO, 3);
  cpl_table_not_selected(copia);
  test_ivalue(4, cpl_table_count_selected(copia), 
              "Check String > \"tattoo\" and Int == 3 rejected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(3, cpl_table_get_nrow(copia),
              "Check length keep all String > \"tattoo\" and Int == 3... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete rows 0, 2, and 6:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_unselect_all(copia);
  cpl_table_select_row(copia, 0);
  cpl_table_select_row(copia, 2);
  cpl_table_select_row(copia, 6);
  test_ivalue(3, cpl_table_count_selected(copia), 
              "Check rows 0, 2, and 6 selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(4, cpl_table_get_nrow(copia),
              "Check length erase rows 0, 2, and 6... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "Now keep rows 0, 2, and 6:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_unselect_row(copia, 0);
  cpl_table_unselect_row(copia, 2);
  cpl_table_unselect_row(copia, 6);
  test_ivalue(4, cpl_table_count_selected(copia), 
              "Check rows 0, 2, and 6 rejected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(3, cpl_table_get_nrow(copia),
              "Check length erase rows 0, 2, and 6... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "This is the test table:\n\n");
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);

  cpl_msg_info(cpl_func, "Now delete first 3 rows:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_window(copia, 0, 3);
  test_ivalue(3, cpl_table_count_selected(copia), 
              "Check first 3 rows selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(4, cpl_table_get_nrow(copia),
              "Check length erase first 3 rows... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "Now delete last 2 rows:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_window(copia, 5, 2);
  test_ivalue(2, cpl_table_count_selected(copia), 
              "Check last 2 rows selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(5, cpl_table_get_nrow(copia),
              "Check length erase last 2 rows... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "Now delete rows from 2 to 3:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_window(copia, 2, 2);
  test_ivalue(2, cpl_table_count_selected(copia), 
              "Check middle 2 rows selected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(5, cpl_table_get_nrow(copia),
              "Check length erase rows from 2 to 3... ");
  cpl_table_delete(copia);

#ifdef VERBOSE

  cpl_msg_info(cpl_func, "Now delete rows from 1 to 3 and row 6:\n\n");

#endif

  copia = cpl_table_duplicate(table);
  cpl_table_and_selected_window(copia, 1, 3);
  cpl_table_or_selected_window(copia, 6, 1);
  test_ivalue(4, cpl_table_count_selected(copia), 
              "Check rows 1 to 3 and row 6 rejected... ");
  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(3, cpl_table_get_nrow(copia),
              "Check length erase rows from 1 to 3 and row 6... ");
  cpl_table_delete(copia);

  /* Erase only invalid elements */
  copia = cpl_table_duplicate(table);
  for (i = 0; i < nrows; i++) {
      error = cpl_table_set_invalid(copia, "Int", i);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
  }

  cpl_table_unselect_row(copia, nrows-1);

  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(1, cpl_table_get_nrow(copia),
              "Check length erase last row, only invalid values... ");

  cpl_table_delete(copia);

  /* Erase array column with valid/invalid values */
  copia = cpl_table_duplicate(table);

  cpl_table_cast_column(copia, "Int", "Double", CPL_TYPE_DOUBLE);

  test(cpl_table_new_column_array(copia, "ADouble", 
                                  CPL_TYPE_DOUBLE | CPL_TYPE_POINTER, 2),
                                  "Creating the ArrayDouble column... ");

  array = cpl_array_new(2, CPL_TYPE_DOUBLE);
  test(cpl_table_set_array(copia, "ADouble", 1, array),
       "Set a valid array to ADouble 1... ");
  test(cpl_table_set_array(copia, "ADouble", 2, array),
       "Set a valid array to ADouble 2... ");
  cpl_array_delete(array);

  cpl_table_unselect_row(copia, 0);
  cpl_table_unselect_row(copia, 2);
  error = cpl_table_set_invalid(copia, "Int", 6);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_invalid(copia, "Int2", 0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_invalid(copia, "Int2", 1);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_invalid(copia, "Double", 0);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_invalid(copia, "Double", 1);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_table_erase_selected(copia);
#ifdef VERBOSE
  cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), stream);
#endif
  test_ivalue(2, cpl_table_get_nrow(copia),
              "Check length erase valid/invalid values... ");
  test_ivalue(0, cpl_table_is_valid(copia, "Int2", 0), 
                    "Check that first element of \"Int2\" is still NULL... ");
  test_ivalue(1, cpl_table_is_valid(copia, "Int2", 1), 
                    "Check that first element of \"Int2\" is now valid... ");

  cpl_table_unselect_row(copia, 0);
  cpl_table_unselect_row(copia, 1);
  cpl_table_erase_selected(copia);
  test_ivalue(2, cpl_table_count_selected(copia),
              "Check that rows are selected... ");
      
  cpl_table_delete(copia);

  cpl_table_delete(table);

  /*
   *  Select invalid rows
   */
  nrows = 3;

  table = cpl_table_new(nrows);
  cpl_table_new_column(table, "Int", CPL_TYPE_INT);
  cpl_table_new_column(table, "String", CPL_TYPE_STRING);
  
  error = cpl_table_set_int(table, "Int", 0, 18);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_int(table, "Int", 1, 18);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_invalid(table, "Int", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  error = cpl_table_set_string(table, "String", 0, "hello");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "String", 1, "hello");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_invalid(table, "String", 2);
  cpl_test_eq_error(error, CPL_ERROR_NONE);

#ifdef VERBOSE
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);
#endif

  cpl_table_select_all(table);

/* 
 * This test (next 4 calls) is related to ticket DFS03619 
 */

  test_ivalue(1, cpl_table_and_selected_invalid(table, "String"),
              "Check number of selected rows (string)... ");
  test_ivalue(0, cpl_table_is_selected(table, 0), 
              "Check that row is not selected...");
  test_ivalue(0, cpl_table_is_selected(table, 1),
              "Check that row is not selected...");
  test_ivalue(1, cpl_table_is_selected(table, 2),
              "Check that row is selected...");

  cpl_table_select_all(table);
  test_ivalue(1, cpl_table_and_selected_invalid(table, "Int"),
              "Check number of selected rows (integer)... ");
  test_ivalue(0, cpl_table_is_selected(table, 0), 
              "Check that row is not selected...");
  test_ivalue(0, cpl_table_is_selected(table, 1),
              "Check that row is not selected...");
  test_ivalue(1, cpl_table_is_selected(table, 2),
              "Check that row is selected...");

  cpl_table_delete(table);

  /*
   * Test case: dividing a double column by integer
   */

  nrows = 100;

  table = cpl_table_new(nrows);
  cpl_table_new_column(table, "Int", CPL_TYPE_INT);

  for (i = 0; i < nrows; i++)
       error = cpl_table_set_int(table, "Int", i, i + 1);
       cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_table_cast_column(table, "Int", "Double", CPL_TYPE_DOUBLE);

  test(cpl_table_divide_columns(table, "Double", "Int"), 
                     "Divide double column with integer column... ");

  for (i = 0; i < nrows; i++) {
    sprintf(message, "Check element %d of result column... ", i);
    test_fvalue(1.0, 0.00001, cpl_table_get_double(table, "Double", i, &null), 
                message);
    cpl_test_zero(null);
  }

#ifdef VERBOSE
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), stream);
#endif

  cpl_table_delete(table);

  /*
   * Test table of images
   */

  table = cpl_table_new(5);
  test(cpl_table_new_column_array(table, "ADouble", CPL_TYPE_DOUBLE, 30),
                                  "Creating the ADouble column... ");

  k = 0;
  arrays = cpl_table_get_data_array(table, "ADouble");
  for (i = 0; i < 5; i++) {
    arrays[i] = cpl_array_new(30, CPL_TYPE_DOUBLE);
    for (j = 0; j < 30; j++) {
      cpl_array_set_double(arrays[i], j, k * .123456);
      k++;
    }
  }

  dimensions = cpl_array_new(2, CPL_TYPE_INT);
  cpl_array_set_int(dimensions, 0, 5);
  cpl_array_set_int(dimensions, 1, 5);
  
  test_failure(CPL_ERROR_INCOMPATIBLE_INPUT,
               cpl_table_set_column_dimensions(table, "ADouble", dimensions),
               "Try impossible column dimensions... ");

  cpl_array_set_int(dimensions, 1, 6);

  test_ivalue(1, cpl_table_get_column_dimensions(table, "ADouble"), 
              "Check no dimensions assigned to column ADouble... ");

  test(cpl_table_set_column_dimensions(table, "ADouble", dimensions),
               "Set appropriate dimensions for column of images... ");

  cpl_array_delete(dimensions);

  error = cpl_table_save(table, NULL, NULL, BASE "23.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "23.fits");
  cpl_table_delete(table);
  table = cpl_table_load(BASE "23.fits", 1, 1);

  test_ivalue(2, cpl_table_get_column_dimensions(table, "ADouble"),
              "Check column ADouble has dimension 2... ");

  test_ivalue(5, cpl_table_get_column_dimension(table, "ADouble", 0),
              "Check column ADouble dimension 1 is 5... ");
  test_ivalue(6, cpl_table_get_column_dimension(table, "ADouble", 1),
              "Check column ADouble dimension 2 is 6... ");

  // FIXME: CPL_IO_APPEND is supported now. Adjust test case accordingly
  //test_failure(cpl_table_save(table, NULL, NULL, BASE "23.fits",
  //                            CPL_IO_APPEND), CPL_ERROR_UNSUPPORTED_MODE,
  //             "Verify failure with CPL_IO_APPEND");
  //cpl_test_fits(BASE "23.fits");

  cpl_table_delete(table);


  /***********************************************
   * Testing all types of casting
   ***********************************************/

  table = cpl_table_new(10);

  cpl_table_new_column(table, "double" , CPL_TYPE_DOUBLE);
  for (i = 0; i < 10; i++)
      if (i != 5) /* Leave intentionally an invalid value */
          error = cpl_table_set_double(table, "double", i, i/3.);
          cpl_test_eq_error(error, CPL_ERROR_NONE);

  /*
   * Case 7:
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_YYY
   * to_name   type = CPL_TYPE_YYY
   */

  cpl_table_cast_column(table, 
            "double",            "float_from_double", CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, 
            "double",            "int_from_double",   CPL_TYPE_INT);
  cpl_table_cast_column(table, 
            "int_from_double",   "double_from_int",   CPL_TYPE_DOUBLE);
  cpl_table_cast_column(table, 
            "int_from_double",   "float_from_int",    CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, 
            "float_from_double", "double_from_float", CPL_TYPE_DOUBLE);
  cpl_table_cast_column(table, 
            "float_from_double", "int_from_float",    CPL_TYPE_INT);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(i/3., 0.00001, 
                      cpl_table_get_float(table, "float_from_double", i, &null),
                      "Invalid cast from double to float");
                      cpl_test_zero(null);
          test_ivalue(i/3, cpl_table_get_int(table, "int_from_double", i, &null),
                      "Invalid cast from double to int");
          cpl_test_zero(null);
          test_fvalue(i/3, 0.00001, 
                      cpl_table_get_double(table, "double_from_int", i, &null),
                      "Invalid cast from int to double");
                      cpl_test_zero(null);
          test_fvalue(i/3, 0.00001, 
                      cpl_table_get_float(table, "float_from_int", i, &null),
                      "Invalid cast from int to float");
                      cpl_test_zero(null);
          test_fvalue(i/3., 0.00001, 
                      cpl_table_get_double(table, "double_from_float", i, &null),
                      "Invalid cast from float to double");
                      cpl_test_zero(null);
          test_ivalue(i/3, cpl_table_get_int(table, "int_from_float", i, &null),
                      "Invalid cast from float to int");
          cpl_test_zero(null);
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "float_from_double", i),
                      "Invalid cast from double to float");
          test_ivalue(0, cpl_table_is_valid(table, "int_from_double", i),
                      "Invalid cast from double to int");
          test_ivalue(0, cpl_table_is_valid(table, "double_from_int", i),
                      "Invalid cast from int to double");
          test_ivalue(0, cpl_table_is_valid(table, "float_from_int", i),
                      "Invalid cast from int to float");
          test_ivalue(0, cpl_table_is_valid(table, "double_from_float", i),
                      "Invalid cast from float to double");
          test_ivalue(0, cpl_table_is_valid(table, "int_from_float", i),
                      "Invalid cast from float to int");
      }
  }

  /* In place: */

  cpl_table_cast_column(table, "double_from_int", NULL, CPL_TYPE_INT);
  cpl_table_cast_column(table, "float_from_int", NULL, CPL_TYPE_INT);
  cpl_table_cast_column(table, "double_from_float", NULL, CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, "int_from_float", NULL, CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, "float_from_double", NULL, CPL_TYPE_DOUBLE);
  cpl_table_cast_column(table, "int_from_double", NULL, CPL_TYPE_DOUBLE);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_ivalue(i/3,
                      cpl_table_get_int(table, "double_from_int", i, &null),
                      "Invalid in-place cast from double to int");
                      cpl_test_zero(null);
          test_ivalue(i/3, cpl_table_get_int(table, "float_from_int", i, &null),
                      "Invalid in-place cast from float to int");
          cpl_test_zero(null);
          test_fvalue(i/3., 0.00001,
                      cpl_table_get_float(table, "double_from_float", i, &null),
                      "Invalid in-place cast from double to float");
                      cpl_test_zero(null);
          test_fvalue(i/3, 0.00001,
                      cpl_table_get_float(table, "int_from_float", i, &null),
                      "Invalid in-place cast from int to float");
                      cpl_test_zero(null);
          test_fvalue(i/3., 0.00001,
                      cpl_table_get_double(table, "float_from_double", i, &null),
                      "Invalid in-place cast from float to double");
                      cpl_test_zero(null);
          test_fvalue(i/3, 0.00001,
                      cpl_table_get_double(table, "int_from_double", i, &null),
                      "Invalid in-place cast from int to double");
                      cpl_test_zero(null);
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "double_from_int", i),
                      "Invalid in-place cast from double to int");
          test_ivalue(0, cpl_table_is_valid(table, "float_from_int", i),
                      "Invalid in-place cast from float to int");
          test_ivalue(0, cpl_table_is_valid(table, "double_from_float", i),
                      "Invalid in-place cast from double to float");
          test_ivalue(0, cpl_table_is_valid(table, "int_from_float", i),
                      "Invalid in-place cast from int to float");
          test_ivalue(0, cpl_table_is_valid(table, "float_from_double", i),
                      "Invalid in-place cast from float to double");
          test_ivalue(0, cpl_table_is_valid(table, "int_from_double", i),
                      "Invalid in-place cast from int to double");
      }
  }


  /*
   * Case 1:
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_XXX
   * to_name   type = CPL_TYPE_XXX
   *
   * (Note that now double_from_float is float, and double_from_int is int)
   */

  cpl_table_cast_column(table, 
                 "double",            "double_from_double", CPL_TYPE_DOUBLE);
  cpl_table_cast_column(table, 
                 "double_from_float", "float_from_float",   CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, 
                 "double_from_int",   "int_from_int",       CPL_TYPE_INT);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_ivalue(i/3,
                      cpl_table_get_int(table, "int_from_int", i, &null),
                      "Invalid cast from int to int");
                      cpl_test_zero(null);
          test_fvalue(i/3., 0.00001,
                      cpl_table_get_float(table, "float_from_float", i, &null),
                      "Invalid cast from float to float");
                      cpl_test_zero(null);
          test_fvalue(i/3., 0.00001,
                   cpl_table_get_double(table, "double_from_double", i, &null),
                      "Invalid cast from double to double");
                   cpl_test_zero(null);
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "int_from_int", i),
                      "Invalid cast from int to int");
          test_ivalue(0, cpl_table_is_valid(table, "float_from_float", i),
                      "Invalid cast from float to float");
          test_ivalue(0, cpl_table_is_valid(table, "double_from_double", i),
                      "Invalid cast from double to double");
      }
  }

  /* In place: */

  cpl_table_cast_column(table, "double_from_double", NULL, CPL_TYPE_DOUBLE);
  cpl_table_cast_column(table, "float_from_float",   NULL, CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, "int_from_int",       NULL, CPL_TYPE_INT);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_ivalue(i/3,
                      cpl_table_get_int(table, "int_from_int", i, &null),
                      "Invalid cast from int to int");
                      cpl_test_zero(null);
          test_fvalue(i/3., 0.00001,
                      cpl_table_get_float(table, "float_from_float", i, &null),
                      "Invalid cast from float to float");
                      cpl_test_zero(null);
          test_fvalue(i/3., 0.00001,
                   cpl_table_get_double(table, "double_from_double", i, &null),
                      "Invalid cast from double to double");
                   cpl_test_zero(null);
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "int_from_int", i),
                      "Invalid cast from int to int");
          test_ivalue(0, cpl_table_is_valid(table, "float_from_float", i),
                      "Invalid cast from float to float");
          test_ivalue(0, cpl_table_is_valid(table, "double_from_double", i),
                      "Invalid cast from double to double");
      }
  }


  /*
   * RESET: start from a new table
   */

  cpl_table_delete(table);

  table = cpl_table_new(10);

  cpl_table_new_column(table, "double" , CPL_TYPE_DOUBLE);
  for (i = 0; i < 10; i++)
      if (i != 5) /* Leave intentionally an invalid value */
          error = cpl_table_set_double(table, "double", i, i/3.);
          cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_table_cast_column(table, "double", "float", CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, "double", "int",   CPL_TYPE_INT);


  /*
   * Case 11:
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER (depth = 1)
   */

  cpl_table_cast_column(table, "double", "float1_from_double", 
                        CPL_TYPE_FLOAT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "double", "int1_from_double",
                        CPL_TYPE_INT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "int", "double1_from_int",
                        CPL_TYPE_DOUBLE | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "int", "float1_from_int",
                        CPL_TYPE_FLOAT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "float", "double1_from_float",
                        CPL_TYPE_DOUBLE | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "float", "int1_from_float",
                        CPL_TYPE_INT | CPL_TYPE_POINTER);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table, 
                                          "float1_from_double", i), 0, NULL),
                      "Invalid cast from double to float1");
          test_ivalue(i/3, 
                      cpl_array_get_int(cpl_table_get_array(table,
                                        "int1_from_double", i), 0, NULL),
                      "Invalid cast from double to int1");
          test_fvalue(i/3, 0.00001,
                      cpl_array_get_double(cpl_table_get_array(table,
                                           "double1_from_int", i), 0, NULL),
                      "Invalid cast from int to double1");
          test_fvalue(i/3, 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "float1_from_int", i), 0, NULL),
                      "Invalid cast from int to float1");
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_double(cpl_table_get_array(table,
                                           "double1_from_float", i), 0, NULL),
                      "Invalid cast from float to double1");
          test_ivalue(i/3, 
                      cpl_array_get_int(cpl_table_get_array(table,
                                        "int1_from_float", i), 0, NULL),
                      "Invalid cast from float to int1");
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "float1_from_double", i),
                      "Invalid cast from double to float1");
          test_ivalue(0, cpl_table_is_valid(table, "int1_from_double", i),
                      "Invalid cast from double to int1");
          test_ivalue(0, cpl_table_is_valid(table, "double1_from_int", i),
                      "Invalid cast from int to double1");
          test_ivalue(0, cpl_table_is_valid(table, "float1_from_int", i),
                      "Invalid cast from int to float1");
          test_ivalue(0, cpl_table_is_valid(table, "double1_from_float", i),
                      "Invalid cast from float to double1");
          test_ivalue(0, cpl_table_is_valid(table, "int1_from_float", i),
                      "Invalid cast from float to int1");
      }
  }


  /*
   * Case 10 in-place:
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   * specified type = CPL_TYPE_YYY
   * to_name   type = CPL_TYPE_YYY
   */

  cpl_table_cast_column(table, "float1_from_double", NULL, CPL_TYPE_DOUBLE);
  cpl_table_cast_column(table, "int1_from_double", NULL, CPL_TYPE_DOUBLE);
  cpl_table_cast_column(table, "double1_from_int", NULL, CPL_TYPE_INT);
  cpl_table_cast_column(table, "float1_from_int", NULL, CPL_TYPE_INT);
  cpl_table_cast_column(table, "double1_from_float", NULL, CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, "int1_from_float", NULL, CPL_TYPE_FLOAT);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(i/3., 0.00001,
                    cpl_table_get_double(table, "float1_from_double", i, &null),
                      "Invalid cast from float1 to double");
                    cpl_test_zero(null);
          test_fvalue(i/3, 0.00001,
                      cpl_table_get_double(table, "int1_from_double", i, &null),
                      "Invalid cast from int1 to double");
                      cpl_test_zero(null);
          test_ivalue(i/3,
                      cpl_table_get_int(table, "double1_from_int", i, &null),
                      "Invalid cast from double1 to int");
                      cpl_test_zero(null);
          test_fvalue(i/3, 0.00001, 
                      cpl_table_get_int(table, "float1_from_int", i, &null),
                      "Invalid cast from float1 to int");
                      cpl_test_zero(null);
          test_fvalue(i/3., 0.00001,
                    cpl_table_get_float(table, "double1_from_float", i, &null),
                      "Invalid cast from double1 to float");
                    cpl_test_zero(null);
          test_ivalue(i/3,
                      cpl_table_get_float(table, "int1_from_float", i, &null),
                      "Invalid cast from int1 to float");
                      cpl_test_zero(null);
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "float1_from_double", i),
                      "Invalid cast from float1 to double");
          test_ivalue(0, cpl_table_is_valid(table, "int1_from_double", i),
                      "Invalid cast from int1 to double");
          test_ivalue(0, cpl_table_is_valid(table, "double1_from_int", i),
                      "Invalid cast from double1 to int");
          test_ivalue(0, cpl_table_is_valid(table, "float1_from_int", i),
                      "Invalid cast from float1 to int");
          test_ivalue(0, cpl_table_is_valid(table, "double1_from_float", i),
                      "Invalid cast from double1 to float");
          test_ivalue(0, cpl_table_is_valid(table, "int1_from_float", i),
                      "Invalid cast from int1 to float");
      }
  }


  /*
   * RESET: start from a new table
   */

  cpl_table_delete(table);

  table = cpl_table_new(10);

  cpl_table_new_column(table, "double" , CPL_TYPE_DOUBLE);
  for (i = 0; i < 10; i++)
      if (i != 5) /* Leave intentionally an invalid value */
          error = cpl_table_set_double(table, "double", i, i/3.);
          cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_table_duplicate_column(table, "double_to_float1", table, "double");
  cpl_table_duplicate_column(table, "double_to_int1", table, "double");
  cpl_table_cast_column(table, "double", "float", CPL_TYPE_FLOAT);
  cpl_table_duplicate_column(table, "float_to_double1", table, "float");
  cpl_table_duplicate_column(table, "float_to_int1", table, "float");
  cpl_table_cast_column(table, "double", "int",   CPL_TYPE_INT);
  cpl_table_duplicate_column(table, "int_to_double1", table, "int");
  cpl_table_duplicate_column(table, "int_to_float1", table, "int");

  /*
   * Case 11, in-place:
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER (depth = 1)
   */

  cpl_table_cast_column(table, "double_to_float1", NULL,
                        CPL_TYPE_FLOAT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "double_to_int1", NULL,
                        CPL_TYPE_INT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "int_to_double1", NULL,
                        CPL_TYPE_DOUBLE | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "int_to_float1", NULL,
                        CPL_TYPE_FLOAT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "float_to_double1", NULL,
                        CPL_TYPE_DOUBLE | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "float_to_int1", NULL,
                        CPL_TYPE_INT | CPL_TYPE_POINTER);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "double_to_float1", i), 0, NULL),
                      "Invalid cast from double to float1");
          test_ivalue(i/3,
                      cpl_array_get_int(cpl_table_get_array(table,
                                        "double_to_int1", i), 0, NULL),
                      "Invalid cast from double to int1");
          test_fvalue(i/3, 0.00001,
                      cpl_array_get_double(cpl_table_get_array(table,
                                           "int_to_double1", i), 0, NULL),
                      "Invalid cast from int to double1");
          test_fvalue(i/3, 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "int_to_float1", i), 0, NULL),
                      "Invalid cast from int to float1");
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_double(cpl_table_get_array(table,
                                           "float_to_double1", i), 0, NULL),
                      "Invalid cast from float to double1");
          test_ivalue(i/3,
                      cpl_array_get_int(cpl_table_get_array(table,
                                        "float_to_int1", i), 0, NULL),
                      "Invalid cast from float to int1");
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "double_to_float1", i),
                      "Invalid cast from double to float1");
          test_ivalue(0, cpl_table_is_valid(table, "double_to_int1", i),
                      "Invalid cast from double to int1");
          test_ivalue(0, cpl_table_is_valid(table, "int_to_double1", i),
                      "Invalid cast from int to double1");
          test_ivalue(0, cpl_table_is_valid(table, "int_to_float1", i),
                      "Invalid cast from int to float1");
          test_ivalue(0, cpl_table_is_valid(table, "float_to_double1", i),
                      "Invalid cast from float to double1");
          test_ivalue(0, cpl_table_is_valid(table, "float_to_int1", i),
                      "Invalid cast from float to int1");
      }
  }

  /*
   * RESET: start from a new table
   */

  cpl_table_delete(table);

  table = cpl_table_new(10);

  cpl_table_new_column(table, "double" , CPL_TYPE_DOUBLE);
  for (i = 0; i < 10; i++)
      if (i != 5) /* Leave intentionally an invalid value */
          error = cpl_table_set_double(table, "double", i, i/3.);
          cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_table_cast_column(table, "double", "float", CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, "double", "int",   CPL_TYPE_INT);

  /*
   * Case 5 & 6:
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   *
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   */

  cpl_table_cast_column(table, "double", "double1", CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "float", "float1", CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "int", "int1", CPL_TYPE_POINTER);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "float1", i), 0, NULL),
                      "Invalid cast from float to float1");
          test_ivalue(i/3,
                      cpl_array_get_int(cpl_table_get_array(table,
                                        "int1", i), 0, NULL),
                      "Invalid cast from int to int1");
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_double(cpl_table_get_array(table,
                                         "double1", i), 0, NULL),
                      "Invalid cast from double to double1");
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "float1", i),
                      "Invalid cast from float to float1");
          test_ivalue(0, cpl_table_is_valid(table, "int1", i),
                      "Invalid cast from int to int1");
          test_ivalue(0, cpl_table_is_valid(table, "double1", i),
                      "Invalid cast from double to double1");
      }
  }


  /*
   * RESET: start from a new table
   */

  cpl_table_delete(table);

  table = cpl_table_new(10);

  cpl_table_new_column(table, "double" , CPL_TYPE_DOUBLE);
  for (i = 0; i < 10; i++)
      if (i != 5) /* Leave intentionally an invalid value */
          error = cpl_table_set_double(table, "double", i, i/3.);
          cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_table_cast_column(table, "double", "float", CPL_TYPE_FLOAT);
  cpl_table_cast_column(table, "double", "int",   CPL_TYPE_INT);

  /*
   * Case 5 in-place:
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   */

  cpl_table_cast_column(table, "double", NULL, CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "float", NULL, CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "int", NULL, CPL_TYPE_POINTER);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "float", i), 0, NULL),
                      "Invalid in-place cast from float to float1");
          test_ivalue(i/3,
                      cpl_array_get_int(cpl_table_get_array(table,
                                        "int", i), 0, NULL),
                      "Invalid in-place cast from int to int1");
          test_fvalue(i/3., 0.00001,
                      cpl_array_get_double(cpl_table_get_array(table,
                                         "double", i), 0, NULL),
                      "Invalid in-place cast from double to double1");
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "float", i),
                      "Invalid in-place cast from float to float1");
          test_ivalue(0, cpl_table_is_valid(table, "int", i),
                      "Invalid in-place cast from int to int1");
          test_ivalue(0, cpl_table_is_valid(table, "double", i),
                      "Invalid in-place cast from double to double1");
      }
  }

  /*
   * RESET: start from a new table
   */

  cpl_table_delete(table);

  table = cpl_table_new(10);

  cpl_table_new_column_array(table, "double",
                             CPL_TYPE_DOUBLE | CPL_TYPE_POINTER, 2);

  array = cpl_array_new(2, CPL_TYPE_DOUBLE);

  for (i = 0; i < 2; i++)
      cpl_array_set_double(array, i, (i+5)/3.);

  for (i = 0; i < 10; i++)
      if (i != 5) /* Leave intentionally an invalid value */
          error = cpl_table_set_array(table, "double", i, array);
          cpl_test_eq_error(error, CPL_ERROR_NONE);

  cpl_array_delete(array);

  cpl_table_duplicate_column(table, "double1", table, "double");

  /*
   * Case 8 & 9:
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   * specified type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   */

  cpl_table_cast_column(table, "double", "float", 
                        CPL_TYPE_FLOAT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "double", "float1", CPL_TYPE_FLOAT);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(5/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "float", i), 0, NULL),
                      "Invalid cast from double2 to float2");
          test_fvalue(6/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "float", i), 1, NULL),
                      "Invalid cast from double2 to float2");
          test_fvalue(5/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "float1", i), 0, NULL),
                      "Invalid cast from double2 to float2");
          test_fvalue(6/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "float1", i), 1, NULL),
                      "Invalid cast from double2 to float2");
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "float1", i),
                      "Invalid cast from double2 to float2");
      }
  }


  /* In place: */

  cpl_table_cast_column(table, "double", NULL, 
                        CPL_TYPE_FLOAT | CPL_TYPE_POINTER);
  cpl_table_cast_column(table, "double1", NULL, CPL_TYPE_FLOAT);

  for (i = 0; i < 10; i++) {
      if (i != 5) {
          test_fvalue(5/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "double", i), 0, NULL),
                      "Invalid in-place cast from double2 to float2");
          test_fvalue(6/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "double", i), 1, NULL),
                      "Invalid in-place cast from double2 to float2");
          test_fvalue(5/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "double1", i), 0, NULL),
                      "Invalid in-place cast from double2 to float2");
          test_fvalue(6/3., 0.00001,
                      cpl_array_get_float(cpl_table_get_array(table,
                                          "double1", i), 1, NULL),
                      "Invalid in-place cast from double2 to float2");
      }
      else {
          test_ivalue(0, cpl_table_is_valid(table, "float1", i),
                      "Invalid in-place cast from doueble1 to float1");
      }
  }

  cpl_table_delete(table);

  /*
   * Testing comparison between string columns.
   */

  table = cpl_table_new(5);
  cpl_table_new_column(table, "stringa", CPL_TYPE_STRING);
  cpl_table_new_column(table, "stringb", CPL_TYPE_STRING);

  error = cpl_table_set_string(table, "stringa", 0, "bb");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringa", 1, "bb");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringa", 2, "bb");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringa", 3, "bb");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringa", 4, "bb");
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  error = cpl_table_set_string(table, "stringb", 0, "a");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringb", 1, "ba");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringb", 2, "bb");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringb", 3, "bbb");
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_set_string(table, "stringb", 4, "bc");
  cpl_test_eq_error(error, CPL_ERROR_NONE);

  /*
   * And equal to
   */

  test_ivalue(1, cpl_table_and_selected(table, "stringa", CPL_EQUAL_TO, 
              "stringb"), "Invalid string comparison (and equal to)");
  array = cpl_table_where_selected(table);
  test_ivalue(1, cpl_array_get_size(array), "Check how many equal to...");
  test_ivalue(2, cpl_array_get_cplsize(array, 0, &null),
              "Check where equal to...");
  cpl_test_zero(null);
  cpl_array_delete(array);
  cpl_table_select_all(table);

  /*
   * And not equal to
   */

  cpl_table_select_all(table);
  test_ivalue(4, cpl_table_and_selected(table, "stringa", CPL_NOT_EQUAL_TO,
              "stringb"), "Invalid string comparison (and not equal to)");
  array = cpl_table_where_selected(table);
  test_ivalue(4, cpl_array_get_size(array), 
              "Check how many not equal to...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  test_ivalue(3, cpl_array_get_cplsize(array, 2, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  test_ivalue(4, cpl_array_get_cplsize(array, 3, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * And greater than
   */

  cpl_table_select_all(table);
  test_ivalue(2, cpl_table_and_selected(table, "stringa", CPL_GREATER_THAN,
              "stringb"), "Invalid string comparison (and greater than)");
  array = cpl_table_where_selected(table);
  test_ivalue(2, cpl_array_get_size(array), 
              "Check how many greater than...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
              "Check where greater than...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
              "Check where greater than...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * And not greater than
   */

  cpl_table_select_all(table);
  test_ivalue(3, cpl_table_and_selected(table, "stringa", CPL_NOT_GREATER_THAN,
              "stringb"), "Invalid string comparison (and not greater than)");
  array = cpl_table_where_selected(table);
  test_ivalue(3, cpl_array_get_size(array),
              "Check how many not greater than...");
  test_ivalue(2, cpl_array_get_cplsize(array, 0, &null),
              "Check where not greater than...");
  cpl_test_zero(null);
  test_ivalue(3, cpl_array_get_cplsize(array, 1, &null),
              "Check where not greater than...");
  cpl_test_zero(null);
  test_ivalue(4, cpl_array_get_cplsize(array, 2, &null),
              "Check where not greater than...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * And less than
   */

  cpl_table_select_all(table);
  test_ivalue(2, cpl_table_and_selected(table, "stringa", CPL_LESS_THAN,
              "stringb"), "Invalid string comparison (and less than)");
  array = cpl_table_where_selected(table);
  test_ivalue(2, cpl_array_get_size(array),
              "Check how many less than...");
  test_ivalue(3, cpl_array_get_cplsize(array, 0, &null),
              "Check where less than...");
  cpl_test_zero(null);
  test_ivalue(4, cpl_array_get_cplsize(array, 1, &null),
              "Check where less than...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * And not less than
   */

  cpl_table_select_all(table);
  test_ivalue(3, cpl_table_and_selected(table, "stringa", CPL_NOT_LESS_THAN,
              "stringb"), "Invalid string comparison (and not less than)");
  array = cpl_table_where_selected(table);
  test_ivalue(3, cpl_array_get_size(array),
              "Check how many not less than...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
              "Check where not less than...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
              "Check where not less than...");
  cpl_test_zero(null);
  test_ivalue(2, cpl_array_get_cplsize(array, 2, &null),
              "Check where not less than...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * Or equal to
   */

  cpl_table_unselect_all(table);
  test_ivalue(1, cpl_table_or_selected(table, "stringa", CPL_EQUAL_TO,
              "stringb"), "Invalid string comparison (or equal to)");
  array = cpl_table_where_selected(table);
  test_ivalue(1, cpl_array_get_size(array), "Check how many equal to...");
  test_ivalue(2, cpl_array_get_cplsize(array, 0, &null),
              "Check where equal to...");
  cpl_test_zero(null);
  cpl_array_delete(array);
  cpl_table_select_all(table);

  /*
   * Or not equal to
   */

  cpl_table_unselect_all(table);
  test_ivalue(4, cpl_table_or_selected(table, "stringa", CPL_NOT_EQUAL_TO,
              "stringb"), "Invalid string comparison (or not equal to)");
  array = cpl_table_where_selected(table);
  test_ivalue(4, cpl_array_get_size(array),
              "Check how many not equal to...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  test_ivalue(3, cpl_array_get_cplsize(array, 2, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  test_ivalue(4, cpl_array_get_cplsize(array, 3, &null),
              "Check where not equal to...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * Or greater than
   */

  cpl_table_unselect_all(table);
  test_ivalue(2, cpl_table_or_selected(table, "stringa", CPL_GREATER_THAN,
              "stringb"), "Invalid string comparison (or greater than)");
  array = cpl_table_where_selected(table);
  test_ivalue(2, cpl_array_get_size(array),
              "Check how many greater than...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
              "Check where greater than...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
              "Check where greater than...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * Or not greater than
   */

  cpl_table_unselect_all(table);
  test_ivalue(3, cpl_table_or_selected(table, "stringa", CPL_NOT_GREATER_THAN,
              "stringb"), "Invalid string comparison (or not greater than)");
  array = cpl_table_where_selected(table);
  test_ivalue(3, cpl_array_get_size(array),
              "Check how many not greater than...");
  test_ivalue(2, cpl_array_get_cplsize(array, 0, &null),
              "Check where not greater than...");
  cpl_test_zero(null);
  test_ivalue(3, cpl_array_get_cplsize(array, 1, &null),
              "Check where not greater than...");
  cpl_test_zero(null);
  test_ivalue(4, cpl_array_get_cplsize(array, 2, &null),
              "Check where not greater than...");
  cpl_test_zero(null);

  cpl_array_delete(array);

  /*
   * Or less than
   */

  cpl_table_unselect_all(table);
  test_ivalue(2, cpl_table_or_selected(table, "stringa", CPL_LESS_THAN,
              "stringb"), "Invalid string comparison (or less than)");
  array = cpl_table_where_selected(table);
  test_ivalue(2, cpl_array_get_size(array),
              "Check how many less than...");
  test_ivalue(3, cpl_array_get_cplsize(array, 0, &null),
              "Check where less than...");
  cpl_test_zero(null);
  test_ivalue(4, cpl_array_get_cplsize(array, 1, &null),
              "Check where less than...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  /*
   * Or not less than
   */

  cpl_table_unselect_all(table);
  test_ivalue(3, cpl_table_or_selected(table, "stringa", CPL_NOT_LESS_THAN,
              "stringb"), "Invalid string comparison (or not less than)");
  array = cpl_table_where_selected(table);
  test_ivalue(3, cpl_array_get_size(array),
              "Check how many not less than...");
  test_ivalue(0, cpl_array_get_cplsize(array, 0, &null),
              "Check where not less than...");
  cpl_test_zero(null);
  test_ivalue(1, cpl_array_get_cplsize(array, 1, &null),
              "Check where not less than...");
  cpl_test_zero(null);
  test_ivalue(2, cpl_array_get_cplsize(array, 2, &null),
              "Check where not less than...");
  cpl_test_zero(null);
  cpl_array_delete(array);

  cpl_table_delete(table);

  if (stream != stdout) cpl_test_zero( fclose(stream) );

  return 0;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test a potentially large table
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_table_test_large(cpl_size nrows)
{

    cpl_table *table;
    cpl_error_code error;
    double time0, time1;


    cpl_msg_info("test", "Testing table with %" CPL_SIZE_FORMAT " rows", nrows);

    table = cpl_table_new(nrows);
    cpl_test_nonnull(table);
    error = cpl_table_new_column(table, "Int", CPL_TYPE_INT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Float", CPL_TYPE_FLOAT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Double", CPL_TYPE_DOUBLE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Int2", CPL_TYPE_INT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Float2", CPL_TYPE_FLOAT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Double2", CPL_TYPE_DOUBLE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Int3", CPL_TYPE_INT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Float3", CPL_TYPE_FLOAT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Double3", CPL_TYPE_DOUBLE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Int4", CPL_TYPE_INT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Float4", CPL_TYPE_FLOAT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_new_column(table, "Double4", CPL_TYPE_DOUBLE);
#if 1
    error = cpl_table_new_column(table, "String", CPL_TYPE_STRING);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
#endif
    error = cpl_table_fill_column_window_int(table, "Int", 0, nrows, 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_float(table, "Float", 0, nrows, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_double(table, "Double", 0, nrows, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_int(table, "Int2", 0, nrows, 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_float(table, "Float2", 0, nrows, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_double(table, "Double2", 0, nrows,
                                                0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_int(table, "Int3", 0, nrows, 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_float(table, "Float3", 0, nrows, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_double(table, "Double3", 0, nrows,
                                                0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_int(table, "Int4", 0, nrows, 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_float(table, "Float4", 0, nrows, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_table_fill_column_window_double(table, "Double4", 0, nrows,
                                                0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
#if 1
    error = cpl_table_fill_column_window_string(table, "String", 0, nrows,
                                                "dummy");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
#endif
    cpl_msg_info("test", "Saving table with %" CPL_SIZE_FORMAT " rows", nrows);
    time0 = cpl_test_get_walltime();
    error = cpl_table_save(table, NULL, NULL, BASE "26.fits", CPL_IO_CREATE);
    cpl_test_fits(BASE "26.fits");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    time1 = cpl_test_get_walltime();
    cpl_table_delete(table);
    cpl_msg_info("test", "%f sec", time1 - time0);
    cpl_msg_info("test", "Loading table with %" CPL_SIZE_FORMAT " rows", nrows);
    time0 = time1;
    table = cpl_table_load(BASE "26.fits", 1, 1);
    time1 = cpl_test_get_walltime();
    cpl_test_nonnull(table);
    cpl_test_eq(cpl_table_get_nrow(table), NROWS_LARGE);
    cpl_msg_info("test", "%f sec", time1 - time0);
    cpl_table_delete(table);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test tables with 0 and 1 rows, saving with empty propertylist
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_table_test_zero_one(void)
{


    /* Test 1: Test saving a table with empty propertylists */
   
    cpl_table        *table = cpl_table_new(1);
    cpl_propertylist *tlist   = cpl_propertylist_new();
    cpl_propertylist *plist   = cpl_propertylist_new();
    cpl_array        *adc     = cpl_array_new(232, CPL_TYPE_DOUBLE_COMPLEX);
    cpl_array        *adf     = cpl_array_new(232, CPL_TYPE_FLOAT_COMPLEX);
    cpl_array        *add     = cpl_array_new(232, CPL_TYPE_DOUBLE);
    cpl_error_code    code;
   
    (void)remove(BASE ".fits");
    test(cpl_table_save(table, plist, tlist, BASE ".fits", CPL_IO_CREATE),
         "Saving 1-length table to disk using empty propertylists...");
    cpl_test_fits(BASE ".fits");
   
    cpl_propertylist_delete(plist);
    cpl_propertylist_delete(tlist);
    cpl_table_delete(table);
    cpl_test_zero(remove(BASE ".fits"));
  
    /*
     *  Testing tables with zero rows.
     */

    cpl_msg_info("test", "Creating a table without rows... ");
    table = cpl_table_new(0);
    cpl_test_nonnull(table);
    cpl_test_zero(cpl_table_get_nrow(table));

    test(cpl_table_new_column(table, "Int", CPL_TYPE_INT), 
         "Creating empty Integer column... ");
    test(cpl_table_new_column(table, "LongLong", CPL_TYPE_LONG_LONG),
         "Creating empty Integer column... ");
    test(cpl_table_new_column(table, "Float", CPL_TYPE_FLOAT), 
         "Creating empty Float column... ");
    test(cpl_table_new_column(table, "Double", CPL_TYPE_DOUBLE), 
         "Creating empty Double column... ");
    test(cpl_table_new_column(table, "CFloat", CPL_TYPE_FLOAT_COMPLEX), 
         "Creating empty Float Complex column... ");
    test(cpl_table_new_column(table, "CDouble", CPL_TYPE_DOUBLE_COMPLEX), 
         "Creating empty Double Complex column... ");
    test(cpl_table_new_column(table, "String", CPL_TYPE_STRING), 
         "Creating empty String column... ");
    test(cpl_table_new_column_array(table, "AInt", 
                                    CPL_TYPE_INT | CPL_TYPE_POINTER, 232),
         "Creating empty IntegerArray column... ");
    test(cpl_table_new_column_array(table, "ALongLong",
                                    CPL_TYPE_LONG_LONG | CPL_TYPE_POINTER, 232),
         "Creating empty LongLongArray column... ");
    test(cpl_table_new_column_array(table, "AFloat", 
                                    CPL_TYPE_FLOAT | CPL_TYPE_POINTER, 232),
         "Creating empty FloatArray column... ");
    test(cpl_table_new_column_array(table, "ADouble", 
                                    CPL_TYPE_DOUBLE | CPL_TYPE_POINTER, 232),
         "Creating empty DoubleArray column... ");
    test(cpl_table_new_column_array(table, "CAFloat", 
                                    CPL_TYPE_FLOAT_COMPLEX | CPL_TYPE_POINTER,
                                    232),
         "Creating empty FloatComplexArray column... ");
    test(cpl_table_new_column_array(table, "CADouble", 
                                    CPL_TYPE_DOUBLE_COMPLEX | CPL_TYPE_POINTER,
                                    232),
         "Creating empty DoubleComplexArray column... ");

    test_ivalue(0, cpl_table_get_nrow(table), "Check zero table length... ");
    test_ivalue(13, cpl_table_get_ncol(table), "Check zero table width... ");

    test_ivalue(0, cpl_table_get_column_depth(table, "Double"), 
                "Check \"Double\" depth... ");

    test_ivalue(0, cpl_table_get_column_depth(table, "CDouble"), 
                "Check \"CDouble\" depth... ");

    test_ivalue(232, cpl_table_get_column_depth(table, "AInt"), 
                "Check \"AInt\" depth... ");

    test_ivalue(232, cpl_table_get_column_depth(table, "CAFloat"), 
                "Check \"CAFloat\" depth... ");

    test_ivalue(232, cpl_table_get_column_depth(table, "CADouble"), 
                "Check \"CADouble\" depth... ");

    test(cpl_table_set_size(table, 1), "Expanding table to one row... ");

    test_ivalue(1, cpl_table_get_nrow(table), "Check table with one row... ");

    code = cpl_table_set_array(table, "ADouble", 0, add);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
#if 1
    code = cpl_table_set_array(table, "CAFloat", 0, adf);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_table_set_array(table, "CADouble", 0, adc);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
#endif

    test(cpl_table_set_size(table, 0), "Deleting all rows from table... ");

    test_ivalue(0, cpl_table_get_nrow(table), 
                "Check again zero table length... ");

    test(cpl_table_erase_column(table, "Double"),
         "Delete zero-column \"Double\"... ");

    test_ivalue(12, cpl_table_get_ncol(table), "Check zero-column removal... ");

    test(cpl_table_erase_column(table, "AInt"),
         "Delete zero-column \"AInt\"... ");

    test_ivalue(11, cpl_table_get_ncol(table),
                "Check zero-column array removal... ");

    test(cpl_table_erase_column(table, "CFloat"),
         "Delete zero-column \"CFloat\"... ");

    test_ivalue(10, cpl_table_get_ncol(table),
                "Check zero-column \"CFloat\" removal... ");

    test_pvalue(NULL, cpl_table_get_data_float(table, "Float"),
                "Check NULL pointer to column Float... ");

    test_failure(CPL_ERROR_NULL_INPUT, 
                 cpl_table_erase_selected(NULL),
                 "Erase selected on NULL table... ");

    test(cpl_table_erase_selected(table),
         "Erase selected on empty table... ");

    test_failure(CPL_ERROR_NULL_INPUT, 
                 cpl_table_set_column_unit(NULL, "Float", "arcsec"),
                 "Try to assign unit to NULL table... ");

    test_failure(CPL_ERROR_NULL_INPUT, 
                 cpl_table_set_column_unit(table, NULL, "arcsec"),
                 "Try to assign unit to NULL column... ");

    test_failure(CPL_ERROR_DATA_NOT_FOUND, 
                 cpl_table_set_column_unit(table, "Double", "arcsec"),
                 "Try to assign unit to non existing column... ");

    test(cpl_table_set_column_unit(table, "Float", "arcsec"),
         "Assign unit 'arcsec' to column Float... ");

  
    test_svalue("arcsec", cpl_table_get_column_unit(table, "Float"),
                "Check column unit... ");

    /*
      if (strcmp(unit = (char *)cpl_table_get_column_unit(table, "Float"), 
      "arcsec")) {
      printf("Check column unit... ");
      printf("Expected \"arcsec\", obtained \"%s\"\n", unit);
      cpl_end();
      return 1;
      }
    */

    test(cpl_table_set_column_unit(table, "Float", NULL),
         "Assign unit NULL to column Float... ");

    test_pvalue(NULL, (char *)cpl_table_get_column_unit(table, "Float"),
                "Get unit NULL from column Float... ");

    test(cpl_table_save(table, NULL, NULL, BASE "1.fits", CPL_IO_CREATE),
         "Saving 0-length table to disk...");
    cpl_test_fits(BASE "1.fits");

    cpl_table_delete(table);
    cpl_msg_info("test", "Loading 0-length table from disk... ");
    table = cpl_table_load(BASE "1.fits", 1, 1);
    cpl_test_nonnull(table);

    test(cpl_table_set_size(table, 1), "Expanding again table to one row... ");

    test(cpl_table_erase_invalid_rows(table), "Pruning table to zero... ");

    /*
     * The returned value must be 1, because columns are deleted first,
     * and what is left is a columnless table with 1 row - a perfectly
     * legal CPL table. For instance, when we create a new table having
     * n rows,
     * 
     *     table = cpl_table_new(n);
     *
     * a table is created. No column is yet defined, but still the number 
     * of rows is assigned.
     */

    test_ivalue(1, cpl_table_get_nrow(table),
                "Checking zero-table length after pruning... ");

    test_ivalue(0, cpl_table_get_ncol(table),
                "Checking zero-table width after pruning... ");

    cpl_array_delete(adc);
    cpl_array_delete(adf);
    cpl_array_delete(add);
    cpl_table_delete(table);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test table 20
  @param  table The table to test
  @return   void
  @see cpl_table_test_main()
  @note A ppc-based Mac with gcc 4.0 cannot compile this code and that for
        table 21 + 22 if it is pasted into cpl_table_test_main()
 */
/*----------------------------------------------------------------------------*/
static void cpl_table_test_20(cpl_table * table)
{
    int                null;
    cpl_table         *copia;


  test_ivalue(0, cpl_table_is_valid(table, "Integer", 0),
                       "Check element  1 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(1998, cpl_table_get_int(table, "Integer", 1, &null),
                       "Check element  2 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 2),
                       "Check element  3 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 3),
                       "Check element  4 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(1998, cpl_table_get_int(table, "Integer", 4, &null),
                       "Check element  5 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(1998, cpl_table_get_int(table, "Integer", 5, &null),
                       "Check element  6 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(998, cpl_table_get_int(table, "Integer", 6, &null),
                       "Check element  7 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(998, cpl_table_get_int(table, "Integer", 7, &null),
                       "Check element  8 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(1998, cpl_table_get_int(table, "Integer", 8, &null),
                       "Check element  9 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(1998, cpl_table_get_int(table, "Integer", 9, &null),
                       "Check element 10 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(1998, cpl_table_get_int(table, "Integer", 10, &null),
                       "Check element 11 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 11),
                       "Check element 12 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 12),
                       "Check element 13 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(10, cpl_table_get_int(table, "Integer", 13, &null),
                       "Check element 14 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(8, cpl_table_get_int(table, "Integer", 14, &null),
                       "Check element 15 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(5, cpl_table_get_int(table, "Integer", 15, &null),
                       "Check element 16 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(10, cpl_table_get_int(table, "Integer", 16, &null),
                       "Check element 17 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 17),
                       "Check element 18 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 18),
                       "Check element 19 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 19),
                       "Check element 20 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Integer", 20),
                       "Check element 21 of \"Integer\" += \"IntToFloat\"... ");
  test_ivalue(1998, cpl_table_get_int(table, "Integer", 21, &null),
                       "Check element 22 of \"Integer\" += \"IntToFloat\"... ");
  cpl_test_zero(null);

  test(cpl_table_subtract_columns(table, "Integer", "IntToFloat"), 
                            "Subtract \"IntToFloat\" from \"Integer\"... ");

  test(cpl_table_subtract_columns(table, "IntToFloat", "Integer"), 
                            "Subtract \"Integer\" from \"IntToFloat\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 0),
                     "Check element  1 of \"IntToFloat\" -= \"Integer\"... ");
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 1, &null),
                     "Check element  2 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 2),
                     "Check element  3 of \"IntToFloat\" -= \"Integer\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 3),
                     "Check element  4 of \"IntToFloat\" -= \"Integer\"... ");
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 4, &null),
                     "Check element  5 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 5, &null),
                     "Check element  6 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(1000.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 6, &null),
                     "Check element  7 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(-1000.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 7, &null),
                     "Check element  8 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 8, &null),
                     "Check element  9 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 9, &null),
                     "Check element 10 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 10, &null),
                     "Check element 11 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 11),
                     "Check element 12 of \"IntToFloat\" -= \"Integer\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 12),
                     "Check element 13 of \"IntToFloat\" -= \"Integer\"... ");
  test_fvalue(-4.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 13, &null),
                     "Check element 14 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(6.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 14, &null),
                     "Check element 15 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(-3.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 15, &null),
                     "Check element 16 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_fvalue(-2.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 16, &null),
                     "Check element 17 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 17),
                     "Check element 18 of \"IntToFloat\" -= \"Integer\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 18),
                     "Check element 19 of \"IntToFloat\" -= \"Integer\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 19),
                     "Check element 20 of \"IntToFloat\" -= \"Integer\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 20),
                     "Check element 21 of \"IntToFloat\" -= \"Integer\"... ");
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 21, &null),
                     "Check element 22 of \"IntToFloat\" -= \"Integer\"... ");
                     cpl_test_zero(null);

  test(cpl_table_multiply_columns(table, "IntToFloat", "Double"), 
                     "Multiply double column with float column... ");

  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 0),
                     "Check element  1 of \"IntToFloat\" *= \"Double\"... ");
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 1, &null),
                     "Check element  2 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 2),
                     "Check element  3 of \"IntToFloat\" *= \"Double\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 3),
                     "Check element  4 of \"IntToFloat\" *= \"Double\"... ");
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 4, &null),
                     "Check element  5 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 5, &null),
                     "Check element  6 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(-1110.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 6, &null),
                     "Check element  7 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(-999880.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 7, &null),
                     "Check element  8 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 8, &null),
                     "Check element  9 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 9, &null),
                     "Check element 10 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 10, &null),
                     "Check element 11 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 11),
                     "Check element 12 of \"IntToFloat\" *= \"Double\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 12),
                     "Check element 13 of \"IntToFloat\" *= \"Double\"... ");
  test_fvalue(-28.44, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 13, &null),
                     "Check element 14 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(6.66, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 14, &null),
                     "Check element 15 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(-12.33, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 15, &null),
                     "Check element 16 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_fvalue(-12.22, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 16, &null),
                     "Check element 17 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 17),
                     "Check element 18 of \"IntToFloat\" *= \"Double\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 18),
                     "Check element 19 of \"IntToFloat\" *= \"Double\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 19),
                     "Check element 20 of \"IntToFloat\" *= \"Double\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "IntToFloat", 20),
                     "Check element 21 of \"IntToFloat\" *= \"Double\"... ");
  test_fvalue(0.0, 0.00001,
                     cpl_table_get_float(table, "IntToFloat", 21, &null),
                     "Check element 22 of \"IntToFloat\" *= \"Double\"... ");
                     cpl_test_zero(null);

  test(cpl_table_divide_columns(table, "Float", "IntToFloat"), 
                     "Divide float column with float column... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 0),
                     "Check element  1 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 1),
                     "Check element  2 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 2),
                     "Check element  3 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 3),
                     "Check element  4 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 4),
                     "Check element  5 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 5),
                     "Check element  6 of \"Float\" /= \"IntToFloat\"... ");
  test_fvalue(0.000991, 0.0000001,
                     cpl_table_get_float(table, "Float", 6, &null),
                     "Check element  7 of \"Float\" /= \"IntToFloat\"... ");
                     cpl_test_zero(null);
  test_fvalue(-0.0010001, 0.0000001,
                     cpl_table_get_float(table, "Float", 7, &null),
                     "Check element  8 of \"Float\" /= \"IntToFloat\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "Float", 8),
                     "Check element  9 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 9),
                     "Check element 10 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 10),
                     "Check element 11 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 11),
                     "Check element 12 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 12),
                     "Check element 13 of \"Float\" /= \"IntToFloat\"... ");
  test_fvalue(-0.2496484, 0.0000001,
                     cpl_table_get_float(table, "Float", 13, &null),
                     "Check element 14 of \"Float\" /= \"IntToFloat\"... ");
                     cpl_test_zero(null);
  test_fvalue(0.1651652, 0.0000001,
                     cpl_table_get_float(table, "Float", 14, &null),
                     "Check element 15 of \"Float\" /= \"IntToFloat\"... ");
                     cpl_test_zero(null);
  test_fvalue(-0.3325223, 0.0000001,
                     cpl_table_get_float(table, "Float", 15, &null),
                     "Check element 16 of \"Float\" /= \"IntToFloat\"... ");
                     cpl_test_zero(null);
  test_fvalue(-0.4991817, 0.0000001,
                     cpl_table_get_float(table, "Float", 16, &null),
                     "Check element 17 of \"Float\" /= \"IntToFloat\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "Float", 17),
                     "Check element 18 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 18),
                     "Check element 19 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 19),
                     "Check element 20 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 20),
                     "Check element 21 of \"Float\" /= \"IntToFloat\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 21),
                     "Check element 22 of \"Float\" /= \"IntToFloat\"... ");

  test(cpl_table_add_scalar(table, "Float", 1), 
                     "Add integer constant to \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 0),
                     "Check element  1 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 1),
                     "Check element  2 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 2),
                     "Check element  3 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 3),
                     "Check element  4 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 4),
                     "Check element  5 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 5),
                     "Check element  6 of adding 1 to \"Float\"... ");
  test_fvalue(1.000991, 0.0000001,
                     cpl_table_get_float(table, "Float", 6, &null),
                     "Check element  7 of adding 1 to \"Float\"... ");
                     cpl_test_zero(null);
  test_fvalue(1-0.0010001, 0.0000001,
                     cpl_table_get_float(table, "Float", 7, &null),
                     "Check element  8 of adding 1 to \"Float\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "Float", 8),
                     "Check element  9 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 9),
                     "Check element 10 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 10),
                     "Check element 11 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 11),
                     "Check element 12 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 12),
                     "Check element 13 of adding 1 to \"Float\"... ");
  test_fvalue(1-0.2496484, 0.0000001,
                     cpl_table_get_float(table, "Float", 13, &null),
                     "Check element 14 of adding 1 to \"Float\"... ");
                     cpl_test_zero(null);
  test_fvalue(1.1651652, 0.0000001,
                     cpl_table_get_float(table, "Float", 14, &null),
                     "Check element 15 of adding 1 to \"Float\"... ");
                     cpl_test_zero(null);
  test_fvalue(1-0.3325223, 0.0000001,
                     cpl_table_get_float(table, "Float", 15, &null),
                     "Check element 16 of adding 1 to \"Float\"... ");
                     cpl_test_zero(null);
  test_fvalue(1-0.4991817, 0.0000001,
                     cpl_table_get_float(table, "Float", 16, &null),
                     "Check element 17 of adding 1 to \"Float\"... ");
                     cpl_test_zero(null);
  test_ivalue(0, cpl_table_is_valid(table, "Float", 17),
                     "Check element 18 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 18),
                     "Check element 19 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 19),
                     "Check element 20 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 20),
                     "Check element 21 of adding 1 to \"Float\"... ");
  test_ivalue(0, cpl_table_is_valid(table, "Float", 21),
                     "Check element 22 of adding 1 to \"Float\"... ");

  test(cpl_table_set_column_invalid(table, "Float", 0, 
                     cpl_table_get_nrow(table)), 
                     "Set \"Float\" column to NULL... ");

  copia = cpl_table_duplicate(table);
  cpl_test_nonnull(copia);

  test(cpl_table_erase_invalid_rows(table), "Pruning table... ");

  test_ivalue(18, cpl_table_get_nrow(table), 
                       "Checking table length after pruning... ");

  test_ivalue(10, cpl_table_get_ncol(table), 
                       "Checking table width after pruning... ");

  test(cpl_table_erase_invalid(copia), "Cleaning table... ");

  test_ivalue(8, cpl_table_get_nrow(copia), 
                       "Checking table length after cleaning... ");

  test_ivalue(10, cpl_table_get_ncol(copia), 
                       "Checking table width after cleaning... ");

  cpl_table_delete(copia);



}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test table 21
  @param  table The table to test
  @return   void
  @see cpl_table_test_main()
  @note A ppc-based Mac with gcc 4.0 cannot compile this code and that for
        table 20 + 22 if it is pasted into cpl_table_test_main()
 */
/*----------------------------------------------------------------------------*/
static void cpl_table_test_21(cpl_table * table)
{
    cpl_size           i;
    int                null;
    char               message[80];
    const char        *names[2];
    cpl_table         *copia;
    cpl_propertylist  *reflist;
    cpl_error_code     error;


  test_svalue("Pixel", cpl_table_get_column_unit(table, "AInt"),
                                            "Check column unit... ");

/*
  if (strcmp(unit = (char *)cpl_table_get_column_unit(table, "AInt"),
      "Pixel")) {
    printf("Check column unit... "); 
    printf("Expected \"Pixel\", obtained \"%s\"\n", unit);
    cpl_end();
    return 1;
  }
*/

  test_ivalue(18, cpl_table_get_nrow(table),
                       "Checking table length after sorting... ");

  test_ivalue(11, cpl_table_get_ncol(table),
                       "Checking table width after sorting... ");

  test_ivalue(7, cpl_table_count_invalid(table, "Integer"),
                     "Count \"Integer\" NULLs after sorting... ");

  test_ivalue(7, cpl_table_count_invalid(table, "Float"),
                     "Count \"Float\" NULLs after sorting... ");

  test_ivalue(2, cpl_table_count_invalid(table, "Double"),
                     "Count \"Double\" NULLs after sorting... ");

  test_ivalue(2, cpl_table_count_invalid(table, "String"),
                     "Count \"String\" NULLs after sorting... ");

  for (i = 0; i < 7; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"Integer\"... ", i + 1);
    test_ivalue(0, cpl_table_is_valid(table, "Integer", i), message);
  }

  test_ivalue(-1, cpl_table_get_int(table, "Integer", 7, &null),
                     "Check element  7 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(1, cpl_table_get_int(table, "Integer", 8, &null),
                     "Check element  8 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(4, cpl_table_get_int(table, "Integer", 9, &null),
                     "Check element  9 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(6, cpl_table_get_int(table, "Integer", 10, &null),
                     "Check element 10 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(7, cpl_table_get_int(table, "Integer", 11, &null),
                     "Check element 11 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  for (i = 12; i < 18; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"Integer\"... ", i + 1);
    test_ivalue(999, cpl_table_get_int(table, "Integer", i, &null),
                       message);
    cpl_test_zero(null);
  }

  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 0, &null),
                     "Check element  1 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 1, &null),
                     "Check element  2 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Double", 2), 
                     "Check element  3 of sorted \"Double\"... ");

  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 3, &null),
                     "Check element  4 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_fvalue(3.11, 0.00001,
                     cpl_table_get_double(table, "Double", 4, &null),
                     "Check element  5 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Double", 5), 
                     "Check element  6 of sorted \"Double\"... ");

  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 6, &null),
                     "Check element  7 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_fvalue(-1.11, 0.00001,
                     cpl_table_get_double(table, "Double", 7, &null),
                     "Check element  8 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_fvalue(1.11, 0.00001,
                    cpl_table_get_double(table, "Double", 8, &null),
                    "Check element  9 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  test_fvalue(4.11, 0.00001,
                    cpl_table_get_double(table, "Double", 9, &null),
                    "Check element  10 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  test_fvalue(6.11, 0.00001,
                    cpl_table_get_double(table, "Double", 10, &null),
                    "Check element 11 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  test_fvalue(7.11, 0.00001,
                    cpl_table_get_double(table, "Double", 11, &null),
                    "Check element 12 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  for (i = 12; i < 18; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"Double\"... ", i + 1);
    test_fvalue(999.88, 0.00001, 
                cpl_table_get_double(table, "Double", i, &null), message);
                cpl_test_zero(null);
  }

  test_svalue("999", cpl_table_get_string(table, "String", 0),
              "Check element  1 of sorted \"String\"... ");

  test_svalue("999", cpl_table_get_string(table, "String", 1),
              "Check element  2 of sorted \"String\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "String", 2),
              "Check element  3 of sorted \"String\"... ");

  test_svalue("999", cpl_table_get_string(table, "String", 3),
              "Check element  4 of sorted \"String\"... ");

  test_svalue("baaa", cpl_table_get_string(table, "String", 4),
              "Check element  5 of sorted \"String\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "String", 5),
              "Check element  6 of sorted \"String\"... ");

  test_svalue("999", cpl_table_get_string(table, "String", 6),
              "Check element  7 of sorted \"String\"... ");

  test_svalue("extra", cpl_table_get_string(table, "String", 7),
              "Check element  8 of sorted \"String\"... ");

  test_svalue("acde", cpl_table_get_string(table, "String", 8),
              "Check element  9 of sorted \"String\"... ");

  test_svalue(" sss", cpl_table_get_string(table, "String", 9),
              "Check element 10 of sorted \"String\"... ");

  test_svalue("daaa", cpl_table_get_string(table, "String", 10),
              "Check element 11 of sorted \"String\"... ");

  test_svalue("aaaa", cpl_table_get_string(table, "String", 11),
              "Check element 11 of sorted \"String\"... ");

  for (i = 12; i < 18; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"String\"... ", i + 1);
    test_svalue("999", cpl_table_get_string(table, "String", i), message);
  }

  test_ivalue(0, cpl_table_is_valid(table, "Float", 0),
              "Check element  1 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 1),
              "Check element  2 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 2),
              "Check element  3 of sorted \"Float\"... ");

  test_fvalue(0.0, 0.00001, cpl_table_get_float(table, "Float", 3, &null),
              "Check element  4 of sorted \"Float\"... ");
  cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Float", 4),
              "Check element  5 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 5),
              "Check element  6 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 6),
              "Check element  7 of sorted \"Float\"... ");

  test_fvalue(-1110.0, 0.00001, 
              cpl_table_get_float(table, "Float", 7, &null),
              "Check element  8 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(6.66, 0.00001, 
              cpl_table_get_float(table, "Float", 8, &null),
              "Check element  9 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-12.33, 0.00001, 
              cpl_table_get_float(table, "Float", 9, &null),
              "Check element 10 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-12.22, 0.00001, 
              cpl_table_get_float(table, "Float", 10, &null),
              "Check element 11 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-28.44, 0.00001, 
              cpl_table_get_float(table, "Float", 11, &null),
              "Check element 12 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(0.0, 0.00001, 
              cpl_table_get_float(table, "Float", 12, &null),
              "Check element 13 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(0.0, 0.00001, 
              cpl_table_get_float(table, "Float", 13, &null),
              "Check element 14 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(0.0, 0.00001, 
              cpl_table_get_float(table, "Float", 14, &null),
              "Check element 15 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-999880.0, 0.00001, 
              cpl_table_get_float(table, "Float", 15, &null),
              "Check element 16 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Float", 16),
              "Check element 17 of sorted \"Float\"... ");

  test_fvalue(0.0, 0.00001, 
              cpl_table_get_float(table, "Float", 17, &null),
              "Check element 18 of sorted \"Float\"... ");
              cpl_test_zero(null);

  names[0] = "Sequence";

  reflist = cpl_propertylist_new();
  cpl_propertylist_append_bool(reflist, names[0], 0);

  test(cpl_table_sort(table, reflist), "Undo table sorting... ");

  cpl_propertylist_delete(reflist);

  names[0] = "Integer";

  reflist = cpl_propertylist_new();
  cpl_propertylist_append_bool(reflist, names[0], 1);

  test(cpl_table_sort(table, reflist), 
       "Sorting by decreasing values of the \"Integer\" column... ");

/* %$% */
/*
cpl_table_dump_structure(table, NULL);
cpl_table_dump(table, 0, cpl_table_get_nrow(table), NULL);
*/

/*
  cpl_table_dump_structure(table, NULL);
  cpl_table_dump(table, 0, cpl_table_get_nrow(table), NULL);

  printf("Median of Integer: %d\n", cpl_table_median_int(table, "Integer"));
  printf("Median of Float: %f\n", cpl_table_median_float(table, "Float"));
  printf("Median of Double: %f\n", cpl_table_median_double(table, "Double"));
  printf("Median of Sequence: %d\n", cpl_table_median_int(table, "Sequence"));
*/

  copia = cpl_table_extract(table, 12, 6);
/* 

cpl_table_dump_structure(table, NULL);
cpl_table_dump(table, 0, cpl_table_get_nrow(table), NULL);
cpl_table_dump_structure(copia, NULL);
cpl_table_dump(copia, 0, cpl_table_get_nrow(copia), NULL);

 */

  test_fvalue(999.000000, 0.001, cpl_table_get_column_median(table, "Integer"),
                   "Median of Integer...");
  test_fvalue(0.000000, 0.001, cpl_table_get_column_median(table, "Float"),
                   "Median of Float...");
  test_fvalue(999.880000, 0.001, cpl_table_get_column_median(table, "Double"),
                   "Median of Double...");
  test_fvalue(8.000000, 0.001, cpl_table_get_column_median(table, "Sequence"),
                   "Median of Sequence...");
  test_fvalue(546.454545, 0.001, cpl_table_get_column_mean(table, "Integer"),
                   "Mean of Integer...");
  test_fvalue(-91003.302727, 0.001, cpl_table_get_column_mean(table, "Float"),
                   "Mean of Float...");
  test_fvalue(626.202500, 0.001, cpl_table_get_column_mean(table, "Double"),
                   "Mean of Double...");
  test_fvalue(8.500000, 0.001, cpl_table_get_column_mean(table, "Sequence"),
                   "Mean of Sequence...");
  test_fvalue(519.939489, 0.001, cpl_table_get_column_stdev(table, "Integer"),
                   "Stdev of Integer...");
  test_fvalue(301440.480937, 0.001, cpl_table_get_column_stdev(table, "Float"),
                   "Stdev of Float...");
  test_fvalue(498.239830, 0.001, cpl_table_get_column_stdev(table, "Double"),
                   "Stdev of Double...");
  test_fvalue(5.338539, 0.001, cpl_table_get_column_stdev(table, "Sequence"),
                   "Stdev of Sequence...");

  /* Test on columns without invalid elements */

  test_fvalue(406.463118, 0.001, cpl_table_get_column_stdev(copia, "Integer"),
                   "Stdev of Integer from 6 last elements...");
  test_fvalue(449.534137, 0.001, cpl_table_get_column_stdev(copia, "Float"),
                   "Stdev of Float from 6 last elements...");
  test_fvalue(406.795909, 0.001, cpl_table_get_column_stdev(copia, "Double"),
                   "Stdev of Double from 6 last elements...");

  cpl_table_delete(copia);

  cpl_msg_info(cpl_func, "median of Integer: %f\n", cpl_table_get_column_median(table, "Integer"));
  cpl_msg_info(cpl_func, "median of Float: %f\n", cpl_table_get_column_median(table, "Float"));
  cpl_msg_info(cpl_func, "median of Double: %f\n", cpl_table_get_column_median(table, "Double"));
  cpl_msg_info(cpl_func, "median of Sequence: %f\n", cpl_table_get_column_median(table, "Sequence"));
  cpl_msg_info(cpl_func, "mean of Integer: %f\n", cpl_table_get_column_mean(table, "Integer"));
  cpl_msg_info(cpl_func, "mean of Float: %f\n", cpl_table_get_column_mean(table, "Float"));
  cpl_msg_info(cpl_func, "mean of Double: %f\n", cpl_table_get_column_mean(table, "Double"));
  cpl_msg_info(cpl_func, "mean of Sequence: %f\n", cpl_table_get_column_mean(table, "Sequence"));
  cpl_msg_info(cpl_func, "Stdev of Integer: %f\n", cpl_table_get_column_stdev(table, "Integer"));
  cpl_msg_info(cpl_func, "Stdev of Float: %f\n", cpl_table_get_column_stdev(table, "Float"));
  cpl_msg_info(cpl_func, "Stdev of Double: %f\n", cpl_table_get_column_stdev(table, "Double"));
  cpl_msg_info(cpl_func, "Stdev of Sequence: %f\n", cpl_table_get_column_stdev(table, "Sequence"));

/**- FIXME: RESTORE!!!  */
  error = cpl_table_fill_invalid_int(table, "Integer", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_fill_invalid_int(table, "New AInt", 320);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  error = cpl_table_save(table, NULL, NULL, BASE "22.fits", CPL_IO_CREATE);
  cpl_test_eq_error(error, CPL_ERROR_NONE);
  cpl_test_fits(BASE "22.fits");

  /* Test sorting table with only invalid elements (exposes DFS04044) */
  for (i = 0; i < cpl_table_get_nrow(table); i++) {
      error = cpl_table_set_invalid(table, "Integer",i);
      cpl_test_eq_error(error, CPL_ERROR_NONE);
  }
  cpl_table_sort(table, reflist);

  cpl_propertylist_delete(reflist);




}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test table 22
  @param  table The table to test
  @return   void
  @see cpl_table_test_main()
  @note A ppc-based Mac with gcc 4.0 cannot compile this code and that for
        table 20 + 21 if it is pasted into cpl_table_test_main()
 */
/*----------------------------------------------------------------------------*/
static void cpl_table_test_22(cpl_table * table)
{
    cpl_size           i;
    int                null;
    char               message[80];


  test_ivalue(18, cpl_table_get_nrow(table),
                       "Checking table length after decreasing sorting... ");

  test_ivalue(11, cpl_table_get_ncol(table),
                       "Checking table width after decreasing sorting... ");

  test_ivalue(7, cpl_table_count_invalid(table, "Integer"),
                     "Count \"Integer\" NULLs after decreasing sorting... ");

  test_ivalue(7, cpl_table_count_invalid(table, "Float"),
                     "Count \"Float\" NULLs after decreasing sorting... ");

  test_ivalue(2, cpl_table_count_invalid(table, "Double"),
                     "Count \"Double\" NULLs after decreasing sorting... ");

  test_ivalue(2, cpl_table_count_invalid(table, "String"),
                     "Count \"String\" NULLs after decreasing sorting... ");

  for (i = 0; i < 7; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"Integer\"... ", i + 1);
    test_ivalue(0, cpl_table_is_valid(table, "Integer", i), message);
  }

  for (i = 7; i < 13; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"Integer\"... ", i + 1);
    test_ivalue(999, cpl_table_get_int(table, "Integer", i, &null),
                       message);
    cpl_test_zero(null);
  }

  test_ivalue(7, cpl_table_get_int(table, "Integer", 13, &null),
                     "Check element 13 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(6, cpl_table_get_int(table, "Integer", 14, &null),
                     "Check element 14 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(4, cpl_table_get_int(table, "Integer", 15, &null),
                     "Check element 15 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(1, cpl_table_get_int(table, "Integer", 16, &null),
                     "Check element 16 of sorted \"Integer\"... ");
  cpl_test_zero(null);

  test_ivalue(-1, cpl_table_get_int(table, "Integer", 17, &null),
                     "Check element 17 of sorted \"Integer\"... ");
  cpl_test_zero(null);


  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 0, &null),
                     "Check element  1 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 1, &null),
                     "Check element  2 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Double", 2),
                     "Check element  3 of sorted \"Double\"... ");

  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 3, &null),
                     "Check element  4 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_fvalue(3.11, 0.00001,
                     cpl_table_get_double(table, "Double", 4, &null),
                     "Check element  5 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Double", 5),
                     "Check element  6 of sorted \"Double\"... ");

  test_fvalue(999.88, 0.00001,
                     cpl_table_get_double(table, "Double", 6, &null),
                     "Check element  7 of sorted \"Double\"... ");
                     cpl_test_zero(null);

  for (i = 7; i < 13; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"Double\"... ", i + 1);
    test_fvalue(999.88, 0.00001,
                cpl_table_get_double(table, "Double", i, &null), message);
                cpl_test_zero(null);
  }

  test_fvalue(7.11, 0.00001,
                    cpl_table_get_double(table, "Double", 13, &null),
                    "Check element 14 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  test_fvalue(6.11, 0.00001,
                    cpl_table_get_double(table, "Double", 14, &null),
                    "Check element 15 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  test_fvalue(4.11, 0.00001,
                    cpl_table_get_double(table, "Double", 15, &null),
                    "Check element 16 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  test_fvalue(1.11, 0.00001,
                    cpl_table_get_double(table, "Double", 16, &null),
                    "Check element 17 of sorted \"Double\"... ");
                    cpl_test_zero(null);

  test_fvalue(-1.11, 0.00001,
                     cpl_table_get_double(table, "Double", 17, &null),
                     "Check element 18 of sorted \"Double\"... ");
                     cpl_test_zero(null);


  test_svalue("999", cpl_table_get_string(table, "String", 0),
              "Check element  1 of sorted \"String\"... ");

  test_svalue("999", cpl_table_get_string(table, "String", 1),
              "Check element  2 of sorted \"String\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "String", 2),
              "Check element  3 of sorted \"String\"... ");

  test_svalue("999", cpl_table_get_string(table, "String", 3),
              "Check element  4 of sorted \"String\"... ");

  test_svalue("baaa", cpl_table_get_string(table, "String", 4),
              "Check element  5 of sorted \"String\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "String", 5),
              "Check element  6 of sorted \"String\"... ");

  test_svalue("999", cpl_table_get_string(table, "String", 6),
              "Check element  7 of sorted \"String\"... ");

  for (i = 7; i < 13; i++) {
    sprintf(message, "Check element  %" CPL_SIZE_FORMAT
            " of sorted \"String\"... ", i + 1);
    test_svalue("999", cpl_table_get_string(table, "String", i), message);
  }

  test_svalue("aaaa", cpl_table_get_string(table, "String", 13),
              "Check element 14 of sorted \"String\"... ");

  test_svalue("daaa", cpl_table_get_string(table, "String", 14),
              "Check element 15 of sorted \"String\"... ");

  test_svalue(" sss", cpl_table_get_string(table, "String", 15),
              "Check element 16 of sorted \"String\"... ");

  test_svalue("acde", cpl_table_get_string(table, "String", 16),
              "Check element 17 of sorted \"String\"... ");

  test_svalue("extra", cpl_table_get_string(table, "String", 17),
              "Check element 18 of sorted \"String\"... ");


  test_ivalue(0, cpl_table_is_valid(table, "Float", 0),
              "Check element  1 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 1),
              "Check element  2 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 2),
              "Check element  3 of sorted \"Float\"... ");

  test_fvalue(0.0, 0.00001, cpl_table_get_float(table, "Float", 3, &null),
              "Check element  4 of sorted \"Float\"... ");
  cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Float", 4),
              "Check element  5 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 5),
              "Check element  6 of sorted \"Float\"... ");

  test_ivalue(0, cpl_table_is_valid(table, "Float", 6),
              "Check element  7 of sorted \"Float\"... ");

  test_fvalue(0.0, 0.00001,
              cpl_table_get_float(table, "Float", 7, &null),
              "Check element  8 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(0.0, 0.00001,
              cpl_table_get_float(table, "Float", 8, &null),
              "Check element  9 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(0.0, 0.00001,
              cpl_table_get_float(table, "Float", 9, &null),
              "Check element 10 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-999880.0, 0.00001,
              cpl_table_get_float(table, "Float", 10, &null),
              "Check element 11 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_ivalue(0, cpl_table_is_valid(table, "Float", 11),
              "Check element 12 of sorted \"Float\"... ");

  test_fvalue(0.0, 0.00001,
              cpl_table_get_float(table, "Float", 12, &null),
              "Check element 13 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-28.44, 0.00001,
              cpl_table_get_float(table, "Float", 13, &null),
              "Check element 14 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-12.22, 0.00001,
              cpl_table_get_float(table, "Float", 14, &null),
              "Check element 15 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-12.33, 0.00001,
              cpl_table_get_float(table, "Float", 15, &null),
              "Check element 16 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(6.66, 0.00001,
              cpl_table_get_float(table, "Float", 16, &null),
              "Check element 17 of sorted \"Float\"... ");
              cpl_test_zero(null);

  test_fvalue(-1110.0, 0.00001,
              cpl_table_get_float(table, "Float", 17, &null),
              "Check element 18 of sorted \"Float\"... ");
              cpl_test_zero(null);


}
