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

#ifdef HAVE_SETENV
extern int setenv(const char *, const char *, int);
#endif

#include <float.h>
#include <math.h>

#include "cpl_plot.h"
#include "cpl_math_const.h"
#include "cpl_test.h"
#include "cpl_memory.h"

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

#define     VEC_SIZE    1024
#define     IMA_SIZE    100
#define     TAB_SIZE    100

#define CPL_PLOT_MIN(A, B) ((A) < (B) ? (A) : (B))

/*-----------------------------------------------------------------------------
                                   Private functions
 -----------------------------------------------------------------------------*/

static
void cpl_plot_test_all(int, int, int);

static
void cpl_plot_test(const cpl_image *, const cpl_mask *, const cpl_table *,
                   int, const char *[],
                   const cpl_vector *, const cpl_bivector *,
                   int, const cpl_vector * const *);


/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    cpl_plot_test_all(IMA_SIZE, VEC_SIZE, TAB_SIZE);
    cpl_plot_test_all(1, 1, 1); /* Some unusual sizes */

    return cpl_test_end(0);
}

/**@}*/


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the plotting functions
  @param n2d  The size of the 2D-objects
  @param nvec The length of the vector-objects
  @param nrow The number of tables rows/columns lengths
  @return     void
  
 */
/*----------------------------------------------------------------------------*/
static
void cpl_plot_test_all(int n2d, int nvec, int nrow)
{


    cpl_vector      *   sinus;
    cpl_vector      *   cosinus;
    cpl_vector      *   x_deg;
    cpl_vector      *   vec_array[3];
    cpl_bivector    *   sin_deg;
    cpl_image       *   ima;
    double              med_dist, median, threshold;
    cpl_mask        *   mask;
    cpl_table       *   tab;
    double          *   px;
    double          *   py;
    const char      *   names[] = {"COLUMN1", "COLUMN2", "COLUMN3"};
    int                 i;

    /* Create the vector sinus */
    sinus = cpl_vector_new(nvec);
    x_deg = cpl_vector_new(nvec);

    px = cpl_vector_get_data(x_deg);
    py = cpl_vector_get_data(sinus);
    for (i=0; i < nvec; i++) {
        px[i] = i * 360.0 / nvec;
        py[i] = sin( i * CPL_MATH_2PI / nvec);
    }

    /* Create the vector cosinus */
    cosinus = cpl_vector_new(nvec);
    py = cpl_vector_get_data(cosinus);
    for (i=0; i < nvec; i++) py[i] = cos( i * CPL_MATH_2PI / nvec);
    
    /* Create the vectors array */
    vec_array[0] = x_deg;
    vec_array[1] = sinus;
    vec_array[2] = cosinus;

    /* Create the bivector */
    sin_deg = cpl_bivector_wrap_vectors(x_deg, sinus);

    /* Create the image */
    ima = cpl_image_fill_test_create(n2d, n2d); 

    /* Compute the median and the av. dist. to the median of the loaded image */
    median = cpl_image_get_median_dev(ima, &med_dist);
    threshold = median + 2.0 * med_dist;
    mask = cpl_mask_threshold_image_create(ima, threshold, DBL_MAX);

    /* Create the table */
    tab = cpl_table_new(nrow);
    cpl_table_new_column(tab, names[0], CPL_TYPE_DOUBLE);
    cpl_table_new_column(tab, names[1], CPL_TYPE_DOUBLE);
    cpl_table_new_column(tab, names[2], CPL_TYPE_DOUBLE);
    for (i=0; i < nrow; i++) {
        cpl_table_set_double(tab, names[0], i, (double)(10*i+1));
        cpl_table_set_double(tab, names[1], i, sin(10*i+1)/(10*i+1));
        cpl_table_set_double(tab, names[2], i, log1p(i)/(i+1));
    }

    /* No errors should have happened in the plot preparation */
    cpl_test_error(CPL_ERROR_NONE);
 
    /* No plotting is done unless the message level is modified */
    if (cpl_msg_get_level() <= CPL_MSG_INFO) {

        cpl_plot_test(ima, mask, tab, 3, names, sinus, sin_deg, 3,
                      (const cpl_vector* const *)vec_array);

#ifdef HAVE_SETENV
        if (!setenv("CPL_IMAGER", "display - &", 1) &&
            !setenv("CPL_PLOTTER", "cat > /dev/null; echo gnuplot > /dev/null",
                    1)) {
            cpl_plot_test(ima, mask, tab, 3, names, sinus, sin_deg, 3,
                          (const cpl_vector* const *)vec_array);
        }
    } else {
        if (!setenv("CPL_IMAGER", "cat > /dev/null", 1) &&
            !setenv("CPL_PLOTTER", "cat > /dev/null; echo gnuplot > /dev/null",
                    1)) {
            cpl_plot_test(ima, mask, tab, 3, names, sinus, sin_deg, 3,
                          (const cpl_vector* const *)vec_array);
        }
#endif
    }

    /* Free and return */
    cpl_table_delete(tab);
    cpl_image_delete(ima);
    cpl_mask_delete(mask);
    cpl_bivector_unwrap_vectors(sin_deg);
    cpl_vector_delete(sinus);
    cpl_vector_delete(cosinus);
    cpl_vector_delete(x_deg);


}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Test plotting of the given CPL objects
  @param image     image
  @param mask      mask
  @param table     table w. at least three columns
  @param names     Column labels of table
  @param vector    vector
  @param bivector  bivector
  @param nvector   Number of vectors in vector array
  @param vec_array Array of vectors
  @return     void
  
 */
/*----------------------------------------------------------------------------*/
static
void cpl_plot_test(const cpl_image * image, const cpl_mask * mask,
                   const cpl_table * table, int ncolumns, const char *names[],
                   const cpl_vector * vector, const cpl_bivector * bivector,
                   int nvector, const cpl_vector * const *vec_array)
{

    /* An error may occur, e.g. in the absence of gnuplot.
       The unit test should still succeed, and it is only verified
       that the return code equals the CPL error code. */
    /* - and that all plotting calls either succeed or fail in the same way */

    cpl_error_code error;
    const cpl_error_code error0 = cpl_plot_vector("", "w lines", "", vector);

    cpl_test_error(error0);

    cpl_test_eq(cpl_table_get_ncol(table), ncolumns);
    cpl_test_leq(3, ncolumns);
    
    error = cpl_plot_bivector("", "w lines", "", bivector);
    cpl_test_eq_error(error, error0);

    error = cpl_plot_vector("", "w lines", "", vector);
    cpl_test_eq_error(error, error0);

    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    error = cpl_plot_vectors("", "w lines", "",
                             (const cpl_vector**)vec_array, nvector);
    CPL_DIAG_PRAGMA_POP;
    cpl_test_eq_error(error, error0);

    error = cpl_plot_image("", "w lines", "", image);
    cpl_test_eq_error(error, error0);

    error = cpl_plot_mask("", "w lines", "", mask);
    cpl_test_eq_error(error, error0);

    error = cpl_plot_column("set grid;", "w lines", "", table, names[0],
                            names[1]);
    cpl_test_eq_error(error, error0);
    error = cpl_plot_column("set grid;", "w lines", "", table, NULL,
                            names[2]);
    cpl_test_eq_error(error, error0);

    error = cpl_plot_columns("set grid;", "w lines", "", table, names, ncolumns);
    cpl_test_eq_error(error, error0);

    /* Example of some plotting using the options */
    error = cpl_plot_image_row("set xlabel 'X Position (pixels)';"
                               "set ylabel 'Counts (ADU)';",  "", "", 
                               image, 1,
                               CPL_PLOT_MIN(10, cpl_image_get_size_y(image)), 1);
    cpl_test_eq_error(error, error0);

    error = cpl_plot_image_col("set xlabel 'Y Position (pixels)';"
                               "set ylabel 'Counts (ADU)';",  "", "", 
                               image, 1, 1, 1);
    cpl_test_eq_error(error, error0);

}
