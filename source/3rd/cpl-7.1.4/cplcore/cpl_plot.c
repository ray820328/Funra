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

#include "cpl_plot.h"
#include "cpl_memory.h"
#include "cpl_error_impl.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef CPL_PLOT_TMPFILE
#define CPL_PLOT_TMPFILE "cpl_plot.txt"
#endif

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_plot   Plotting of CPL objects
 *
 * This module provides functions to plot basic CPL objects
 * 
 * This module is offered to help during the development process. 
 * The functions offered should NOT be used in any operational environment. 
 * For that reason, the support of those remains limited, and no functionality 
 * extension can be expected from the CPL team.
 *
 * The created plot windows can be closed by pressing the 'q' key like
 * you would do with a normal gnuplot window. 
 *
 * The default behaviour of the plotting is to use gnuplot (with option 
 * -persist). The user can control the actual plotting-command used to create 
 * the plot by setting the environment variable CPL_PLOTTER. Currently, if 
 * CPL_PLOTTER is set it must contain the string 'gnuplot'. Setting 
 * it to 'cat > my_gnuplot_$$.txt' causes a number of ASCII-files to be 
 * created, which each produce a plot when given as standard input to gnuplot.
 *
 * A finer control of the plotting options can be obtained by writing an 
 * executable script, e.g. my_gnuplot, that executes gnuplot after setting 
 * the desired gnuplot options (e.g. set terminal pslatex color) and then 
 * setting CPL_PLOTTER to my_gnuplot. 
 * 
 * Images can be plotted not only with gnuplot, but also using the pnm format. 
 * This is controlled with the environment variable CPL_IMAGER. If CPL_IMAGER 
 * is set to a string that does not contain the word gnuplot, the recipe 
 * will generate the plot in pnm format. E.g. setting CPL_IMAGER to 
 * 'display - &' will produce a gray-scale image using the image viewer display.
 *
 * @code
 *   #include "cpl_plot.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                   Type definition
 -----------------------------------------------------------------------------*/

typedef struct _cpl_plot_ cpl_plot;

struct _cpl_plot_ {
    /* File with data to be sent to plotting command's stdin */
    FILE * data;
    /* Name of command capable of reading the plot on stdin */
    const char * exe;
    /* Name of temporary file for storing the plot commands */
    const char * tmp;
};

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static cpl_error_code cpl_mplot_puts(cpl_plot *, const char *);
static cpl_error_code cpl_mplot_write(cpl_plot *, const char *, size_t);
static cpl_plot * cpl_mplot_open(const char *);
static cpl_plot * cpl_image_open(const char *);
static cpl_error_code cpl_mplot_close(cpl_plot *, const char *);
static const char * cpl_mplot_plotter(void);
static const char * cpl_mplot_imager(void);

/*-----------------------------------------------------------------------------
                                   Functions code
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot a vector
  @param    pre      An optional string with pre-plot commands
  @param    options  An optional string with plotting options
  @param    post     An optional string with post-plot commands
  @param    vector   The vector to plot
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  The vector must have a positive number of elements.

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_vector(const char          *   pre, 
                                        const char          *   options,
                                        const char          *   post, 
                                        const cpl_vector    *   vector)
{
    const double * pvec  = cpl_vector_get_data_const(vector);
    const cpl_size n     = cpl_vector_get_size(vector);
    cpl_error_code error = CPL_ERROR_NONE;
    cpl_size       i;
    cpl_plot     * plot;


    if (n <= 0) return cpl_error_set_where_();

    plot = cpl_mplot_open(pre);

    if (plot == NULL) return cpl_error_set_where_();

    error |= cpl_mplot_puts(plot, "plot '-' ");

    if (options != NULL) {
        error |= cpl_mplot_puts(plot, options);
    } else {
        char * myoptions = cpl_sprintf("t '%" CPL_SIZE_FORMAT"-vector (%p)", n,
                                       (const void*)vector);
        assert(myoptions != NULL);
        error |= cpl_mplot_puts(plot, myoptions);
        cpl_free(myoptions);
    }

    error |= cpl_mplot_puts(plot, ";\n");

    for (i = 0; i < n; i++) {
        char * snumber = cpl_sprintf("%g\n", pvec[i]);

        error |= cpl_mplot_puts(plot, snumber);
        cpl_free(snumber);
        if (error) break;
    }

    error |= cpl_mplot_puts(plot, "e\n");
    error |= cpl_mplot_close(plot, post);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot a bivector
  @param    pre      An optional string with pre-plot commands
  @param    options  An optional string with plotting options
  @param    post     An optional string with post-plot commands
  @param    bivector The bivector to plot
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  The bivector must have a positive number of elements.

  @see also @c cpl_mplot_open().

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_bivector(const char          *   pre, 
                                          const char          *   options,
                                          const char          *   post,
                                          const cpl_bivector  *   bivector)
{
    const cpl_size n         = cpl_bivector_get_size(bivector);
    const double * pvecx     = cpl_bivector_get_x_data_const(bivector);
    const double * pvecy     = cpl_bivector_get_y_data_const(bivector);
    cpl_error_code error     = CPL_ERROR_NONE;
    cpl_plot     * plot;
    cpl_size       i;


    if (n <= 0) return cpl_error_set_where_();

    plot = cpl_mplot_open(pre);

    if (plot == NULL) return cpl_error_set_where_();

    error |= cpl_mplot_puts(plot, "plot '-' ");

    if (options != NULL) {
        error |= cpl_mplot_puts(plot, options);
    } else {
        char * myoptions = cpl_sprintf("t '%" CPL_SIZE_FORMAT "-bivector (%p)",
                                       n, (const void*)bivector);
        assert(myoptions != NULL);
        error |= cpl_mplot_puts(plot, myoptions);
        cpl_free(myoptions);
    }

    error |= cpl_mplot_puts(plot, ";\n");

    for (i = 0; i < n; i++) {
        char * snumber = cpl_sprintf("%g %g\n", pvecx[i], pvecy[i]);

        error |= cpl_mplot_puts(plot, snumber);
        cpl_free(snumber);
        if (error) break;
    }

    error |= cpl_mplot_puts(plot, "e\n");
    error |= cpl_mplot_close(plot, post);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot an array of vectors
  @param    pre     An optional string with pre-plot commands
  @param    options An optional string with plotting options
  @param    post    An optional string with post-plot commands
  @param    vectors The vectors array to plot
  @param    nvec    The number of vectors
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  The array should contain at least 3 vectors, the first one can be
  NULL.

  The non-NULL vectors must have the same number of elements.
  The first vector gives the x-axis. If NULL, the index is used.

  @see also @c cpl_mplot_open().

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_vectors(const char          *   pre, 
                                const char          *   options,
                                const char          *   post,
                                const cpl_vector    **  vectors,
                                cpl_size                nvec)
{
    cpl_errorstate prestate = cpl_errorstate_get();

    const double    *   pvecx;
    const double    *   pvecy;
    cpl_size            vec_size = 0; /* Avoid uninit warning */
    char            **  names;
    char            *   sval;
    FILE            *   tmpfd;
    cpl_plot        *   plot;
    cpl_size            i, j;
    
    /* Check entries */
    cpl_ensure_code(vectors != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nvec>=3, CPL_ERROR_ILLEGAL_INPUT);
    for (i=1; i<nvec; i++) {
        cpl_ensure_code(vectors[i] != NULL, CPL_ERROR_NULL_INPUT);
        if (i==1) vec_size = cpl_vector_get_size(vectors[i]);
        else cpl_ensure_code(vec_size==cpl_vector_get_size(vectors[i]), 
                    CPL_ERROR_ILLEGAL_INPUT);
    }
    if (vectors[0] != NULL) {
        cpl_ensure_code(vec_size==cpl_vector_get_size(vectors[0]), 
                    CPL_ERROR_ILLEGAL_INPUT);
    }

    /* Get the X axis if passed */
    pvecx = vectors[0] ? cpl_vector_get_data_const(vectors[0]) : NULL;
    
    /* Hold the files names */
    names = cpl_malloc((size_t)(nvec-1)*sizeof(char*));
    for (i=1; i<nvec; i++)
        names[i-1] = cpl_sprintf("cpl_plot-%" CPL_SIZE_FORMAT, i);
    
    /* Open the plot */
    plot = cpl_mplot_open(pre);

    /* Loop on the signals to plot */
    for (i=1; i<nvec; i++) {
        
        /* Get the current Y axis */
        pvecy = cpl_vector_get_data_const(vectors[i]);

        /* Open temporary file for output   */
    if ((tmpfd=fopen(names[i-1], "w"))==NULL) {
            cpl_mplot_close(plot, post);
            for (i=1; i<nvec; i++) (void) remove(names[i-1]);
            for (i=1; i<nvec; i++) cpl_free(names[i-1]); 
            cpl_free(names); 
            return CPL_ERROR_FILE_IO;
        }

        /* Write data to this file  */
        for (j=0; j<vec_size; j++) {
            char * snumber = cpl_sprintf("%g %g\n",
                                         pvecx == NULL ? (double)(j+1)
                                         : pvecx[j], pvecy[j]);
            const int nstat = fputs(snumber, tmpfd);

            cpl_free(snumber);
            if (nstat < 0) break;

        }
        if (fclose(tmpfd) != 0) break;
        if (j != vec_size) break;

        /* Plot the file */
        if (i==1) {
            /* Plot the first signal */
            sval = cpl_sprintf("plot '%s' %s;\n", names[i-1],
                                                 options ? options : "");
        } else {
            sval = cpl_sprintf("replot '%s' t 'Vector %" CPL_SIZE_FORMAT
                               "' w lines;\n", names[i-1], i);
        }
        cpl_mplot_puts(plot, sval);
        cpl_free(sval);
    }
    cpl_mplot_close(plot, post);
    for (i=1; i<nvec; i++) (void) remove(names[i-1]);
    for (i=1; i<nvec; i++) cpl_free(names[i-1]); 
    cpl_free(names); 

    return cpl_errorstate_is_equal(prestate) ? CPL_ERROR_NONE
        : cpl_error_set_where_();
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot an array of bivectors
  @param    pre       An optional string with pre-plot commands
  @param    options   Array of strings with plotting options
  @param    post      An optional string with post-plot commands
  @param    bivectors The bivectors array to plot
  @param    nbvec     The number of bivectors, at least one is
                      required
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  Each bivector in the array defines a sequence of points to be plotted. The
  bivectors can have different size.

  The @em options array must be of same size as the @em bivectors array. The
  i'th string in the array specifies the plotting options for the i'th
  bivector.

  @see also @c cpl_mplot_open().

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_DATA_NOT_FOUND
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_bivectors(const char         *  pre, 
                                  const char         ** options,
                                  const char         *  post,
                                  const cpl_bivector ** bivectors,
                                  cpl_size              nbvec)
{
    cpl_errorstate prestate = cpl_errorstate_get();

    const double * pvecx;
    const double * pvecy;
    char        ** names;
    char         * sval;
    FILE         * tmpfd;
    cpl_plot     * plot;
    cpl_size       i;
    cpl_size       j;
    
    /* Check entries */
    cpl_ensure_code(bivectors != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(options   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nbvec     >= 1,    CPL_ERROR_DATA_NOT_FOUND);
    for (i=0; i<nbvec; i++) {
        cpl_ensure_code(bivectors[i] != NULL, CPL_ERROR_NULL_INPUT);
        cpl_ensure_code(  options[i] != NULL, CPL_ERROR_NULL_INPUT);
    }

    /* Hold the files names */
    names = cpl_malloc((size_t)nbvec * sizeof(char*));
    for (i=0; i<nbvec; i++)
        names[i] = cpl_sprintf("cpl_plot-%" CPL_SIZE_FORMAT, i + 1);
    
    /* Open the plot */
    plot = cpl_mplot_open(pre);

    /* Loop on the signals to plot */
    for (i=0; i<nbvec; i++) {
        cpl_size vec_size;
        
        /* Get current vectors */
        pvecx = cpl_bivector_get_x_data_const(bivectors[i]);
        pvecy = cpl_bivector_get_y_data_const(bivectors[i]);

        vec_size = cpl_bivector_get_size(bivectors[i]);

        /* Open temporary file for output */
        if ((tmpfd=fopen(names[i], "w"))==NULL) {
            cpl_mplot_close(plot, post);
            for (i=0; i<nbvec; i++) (void) remove(names[i]);
            for (i=0; i<nbvec; i++) cpl_free(names[i]);
            cpl_free(names);
            return CPL_ERROR_FILE_IO;
        }

        /* Write data to this file  */
        for (j=0; j<vec_size; j++) {
            char * snumber = cpl_sprintf("%g %g\n", pvecx[j], pvecy[j]);
            const int nstat = fputs(snumber, tmpfd);

            cpl_free(snumber);
            if (nstat < 0) break;
        }
        if (fclose(tmpfd) != 0) break;
        if (j != vec_size) break;

    /* Plot the file using 'plot' the first time,
       otherwise use 'replot' */
    sval = cpl_sprintf("%splot '%s' %s;\n", 
        i==0 ? "" : "re", names[i], options[i]);
    cpl_mplot_puts(plot, sval);
        cpl_free(sval);
    }
    cpl_mplot_close(plot, post);
    for (i=0; i<nbvec; i++) (void) remove(names[i]);
    for (i=0; i<nbvec; i++) cpl_free(names[i]);
    cpl_free(names);

    return cpl_errorstate_is_equal(prestate) ? CPL_ERROR_NONE
        : cpl_error_set_where_();
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot a mask
  @param    pre      An optional string with pre-plot commands
  @param    options  An optional string with plotting options
  @param    post     An optional string with post-plot commands
  @param    mask    The mask to plot
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  If the specified plotting command does not contain the string 'gnuplot',
  the plotting command is assumed to be able to parse a pgm
  (P5) mask from stdin. Valid examples of such a command may include
  'cat > myplot$$.pgm' and 'display - &'.

  The 'pre' and 'post' commands are ignored in PGM-plots, while the
  'options' string is written as a comment in the header of the mask.

  See also cpl_plot_vector().

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_mask(const char     *   pre, 
                             const char     *   options,
                             const char     *   post,
                             const cpl_mask *   mask)
{
    const cpl_size     nx = cpl_mask_get_size_x(mask);
    const cpl_size     ny = cpl_mask_get_size_y(mask);
    cpl_plot         * plot;
    const cpl_binary * pmask = cpl_mask_get_data_const(mask);
    char             * dvi_options = (char *)options;
    char             * myoptions   = (char *)options;
    cpl_error_code     error = CPL_ERROR_NONE;
    cpl_size           i, j;


    if (nx <= 0) return cpl_error_set_where_();

    plot = cpl_image_open(pre);

    if (plot == NULL) return cpl_error_set_where_();

    if (options == NULL || strlen(options) < 1) {
        dvi_options = cpl_sprintf("%" CPL_SIZE_FORMAT "X%" CPL_SIZE_FORMAT
                                  "-mask (%p)", nx, ny, (const void*)mask);
        assert( dvi_options != NULL);
    }

    assert(plot->exe != NULL);
    if (strstr(plot->exe, "gnuplot")) {

        error |= cpl_mplot_puts(plot, "splot '-' matrix ");

        if (myoptions == NULL || strlen(myoptions) < 1) {

            myoptions = cpl_sprintf("t '%s';", dvi_options);
            assert( myoptions != NULL);
        }
        error |= cpl_mplot_puts(plot, myoptions);
        error |= cpl_mplot_puts(plot, ";\n");

        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                char * snumber = cpl_sprintf("%d ", pmask[i + j * nx]);

                error |= cpl_mplot_puts(plot, snumber);
                cpl_free(snumber);
                if (error) break;
            }
            error |= cpl_mplot_puts(plot, "\n");
            if (error) break;
        }
        error |= cpl_mplot_puts(plot, "e\n");

    } else {
        const size_t bsize = (size_t)(ny * nx); /* Octets in raster */
        unsigned char * raster = (unsigned char *)cpl_malloc(bsize);

        /* Create a PGM P5 header - with a maxval of 1 */
        myoptions = cpl_sprintf("P5\n%" CPL_SIZE_FORMAT " %" CPL_SIZE_FORMAT
                                "\n", nx, ny);
        error |= cpl_mplot_puts(plot, myoptions);
        cpl_free(myoptions);

        myoptions = cpl_sprintf("# %s\n1\n", dvi_options);

        error |= cpl_mplot_puts(plot, myoptions);

        for (j = 0; j < ny; j++) for (i = 0; i < nx; i++)
            raster[i + (ny-j-1) * nx] = (unsigned char) pmask[i + j * nx];

        error |= cpl_mplot_write(plot, (const void *) raster, bsize);

        cpl_free(raster);
    }


    if (dvi_options != options) cpl_free(dvi_options);
    if (myoptions   != options) cpl_free(myoptions);

    error |= cpl_mplot_close(plot, post);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot an image
  @param    pre      An optional string with pre-plot commands
  @param    options  An optional string with plotting options
  @param    post     An optional string with post-plot commands
  @param    image    The image to plot
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  The image must have a positive number of pixels.

  @see also @c cpl_image_open().

  If the specified plotting command does not contain the string 'gnuplot',
  the plotting command is assumed to be able to parse a pgm
  (P5) image from stdin. Valid examples of such a command may include
  'cat > myplot$$.pgm' and 'display - &'.

  The 'pre' and 'post' commands are ignored in PGM-plots, while the
  'options' string is written as a comment in the header of the image.

  See also cpl_plot_vector().

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_image(const char      *   pre, 
                              const char      *   options,
                              const char      *   post,
                              const cpl_image *   image)
{
    const cpl_size  nx = cpl_image_get_size_x(image);
    const cpl_size  ny = cpl_image_get_size_y(image);
    cpl_plot      * plot;
    cpl_image     * temp;
    const double  * pimage;
    unsigned char * raster = NULL;
    char          * dvi_options = (char *)options;
    char          * myoptions   = (char *)options;
    cpl_error_code  error = CPL_ERROR_NONE;
    cpl_size        i, j;


    if (nx <= 0) return cpl_error_set_where_();

    plot = cpl_image_open(pre);

    if (plot == NULL) return cpl_error_set_where_();

    if (options == NULL || strlen(options) < 1) {
        dvi_options = cpl_sprintf("%" CPL_SIZE_FORMAT "X%" CPL_SIZE_FORMAT
                                  "-image-(%d) (%p)", nx, ny, 
                                  (int)cpl_image_get_type(image),
                                  (const void*)image);
        assert( dvi_options != NULL);
    }

    assert(plot->exe != NULL);
    if (strstr(plot->exe, "gnuplot")) {

        if (cpl_image_get_type(image) == CPL_TYPE_DOUBLE) {
            temp = NULL;
            pimage = cpl_image_get_data_double_const(image);
        } else {
            temp = cpl_image_cast(image, CPL_TYPE_DOUBLE);
            pimage = cpl_image_get_data_double(temp);
        }

        error |= cpl_mplot_puts(plot, "splot '-' matrix ");

        if (myoptions == NULL || strlen(myoptions) < 1) {

            myoptions = cpl_sprintf("t '%s';",
                                                      dvi_options);
            assert( myoptions != NULL);
        }
        error |= cpl_mplot_puts(plot, myoptions);
        error |= cpl_mplot_puts(plot, ";\n");

        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                char * snumber = cpl_sprintf("%g ", pimage[j*nx + i]);

                error |= cpl_mplot_puts(plot, snumber);
                cpl_free(snumber);
                if (error) break;
            }
            error |= cpl_mplot_puts(plot, "\n");
            if (error) break;
        }
        error |= cpl_mplot_puts(plot, "e\n");

    } else {
        const size_t bsize = (size_t)(ny * nx); /* Octets in raster */
        const double tmin = cpl_image_get_min(image);
        double       tmax;


        /* Convert the image to one in the range [0; 256[ */
        if (cpl_image_get_type(image) == CPL_TYPE_DOUBLE)  {
            temp = cpl_image_subtract_scalar_create(image, tmin);
        } else {
            temp = cpl_image_cast(image, CPL_TYPE_DOUBLE);
            error |= cpl_image_subtract_scalar(temp, tmin);
        }

        tmax = cpl_image_get_max(temp);

        if (tmax > 0.0)
            error |= cpl_image_multiply_scalar(temp, 256.0*(1.0-DBL_EPSILON)
                                               /tmax);

        pimage = cpl_image_get_data_double(temp);
        assert(pimage != NULL);

        /* Create a PGM P5 header - with a maxval of 255 */
        myoptions = cpl_sprintf("P5\n%" CPL_SIZE_FORMAT " %" CPL_SIZE_FORMAT
                                "\n", nx, ny);
        error |= cpl_mplot_puts(plot, myoptions);
        cpl_free(myoptions);

        myoptions = cpl_sprintf("# True Value Range: [%g;%g]\n",
                                   tmin, tmin+tmax);
        assert( myoptions != NULL);

        error |= cpl_mplot_puts(plot, myoptions);
        cpl_free(myoptions);

        myoptions = cpl_sprintf("# %s\n255\n", dvi_options);

        error |= cpl_mplot_puts(plot, myoptions);

        raster = (unsigned char *)cpl_malloc(bsize);
        for (j = 0; j < ny; j++) for (i = 0; i < nx; i++)
            raster[(ny-j-1)*nx + i] = (unsigned char) pimage[j*nx + i];

        error |= cpl_mplot_write(plot, (const char *) raster, bsize);

    }


    if (dvi_options != options) cpl_free(dvi_options);
    if (myoptions   != options) cpl_free(myoptions);
    cpl_free(raster);
    cpl_image_delete(temp);

    error |= cpl_mplot_close(plot, post);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot a range of image rows
  @param    pre      An optional string with pre-plot commands
  @param    options  An optional string with plotting options
  @param    post     An optional string with post-plot commands
  @param    image    The image to plot
  @param    firstrow The first row to plot (1 for first)
  @param    lastrow  The last row to plot
  @param    rowstep  The positive row stride
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  The image must have a positive number of pixels.

  lastrow shall be greater than or equal to firstrow.

  @see also @c cpl_mplot_open().

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_ACCESS_OUT_OF_RANGE (firstrow or lastrow are out of range)
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_image_row(const char      *   pre,
                                  const char      *   options,
                                  const char      *   post,
                                  const cpl_image *   image,
                                  cpl_size            firstrow, 
                                  cpl_size            lastrow,
                                  cpl_size            rowstep)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_size nx, ny;

    cpl_ensure_code(image != NULL, CPL_ERROR_NULL_INPUT);

    nx = cpl_image_get_size_x(image);
    ny = cpl_image_get_size_y(image);

    cpl_ensure_code( nx > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code( ny > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code( rowstep > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code( firstrow > 0,    CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code( firstrow <= lastrow, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code( lastrow  <= ny,  CPL_ERROR_ACCESS_OUT_OF_RANGE);
   
    do {

        const double * pimage;
        cpl_image * temp = NULL;
        char * myoptions = NULL;
        cpl_size i, j;

        cpl_plot * plot = cpl_mplot_open(pre);

        do {
            if (plot == NULL) break;

            if (cpl_mplot_puts(plot, "plot")) break;
            for (j = firstrow-1; j < lastrow; j += rowstep) {
                const char * useoptions = options;
                if (j > firstrow-1 && cpl_mplot_puts(plot, ",")) break;
                if (cpl_mplot_puts(plot, " '-' ")) break;

                if (useoptions == NULL || strlen(useoptions) < 1) {

                    cpl_free(myoptions);
                    myoptions = (j == firstrow-1)
                        ? cpl_sprintf("t 'Row %" CPL_SIZE_FORMAT " %"
                                      CPL_SIZE_FORMAT "X%" CPL_SIZE_FORMAT
                                      "-image-(%d) (%p)'",  j, nx, ny,
                                      (int)cpl_image_get_type(image),
                                      (const void*)image)
                        : cpl_sprintf("t 'Row %" CPL_SIZE_FORMAT
                                      " of the same image'", j);

                    assert( myoptions != NULL);
                        useoptions = myoptions;
                }
                if (cpl_mplot_puts(plot, useoptions)) break;
            }
            if (cpl_mplot_puts(plot, ";\n      ")) break;

            if (cpl_image_get_type(image) == CPL_TYPE_DOUBLE) {
                pimage = cpl_image_get_data_double_const(image);
            } else {
                temp = cpl_image_cast(image, CPL_TYPE_DOUBLE);
                pimage = cpl_image_get_data_double_const(temp);
            }

            assert(pimage != NULL);

            for (j = firstrow-1; j < lastrow; j += rowstep) {
                for (i = 0; i < nx; i++) {
                    char * snumber = cpl_sprintf("%g\n", pimage[j * nx + i]);
                    const int nstat = cpl_mplot_puts(plot, snumber);

                    cpl_free(snumber);
                    if (nstat) break;
                }
                if (i != nx) break;
                if (cpl_mplot_puts(plot, "e\n")) break;
            }
            if (j != lastrow) break;

        } while (0);

        cpl_free(myoptions);
        cpl_image_delete(temp);

        cpl_mplot_close(plot, post);

    } while (0);

    return cpl_errorstate_is_equal(prestate) ? CPL_ERROR_NONE
        : cpl_error_set_where_();
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot a range of image columns
  @param    pre      An optional string with pre-plot commands
  @param    options  An optional string with plotting options
  @param    post     An optional string with post-plot commands
  @param    image    The image to plot
  @param    firstcol The first column to plot (1 for first)
  @param    lastcol  The last column to plot
  @param    colstep  The positive column stride
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  The image must have a positive number of pixels.

  lastcol shall be greater than or equal to firstcol.

  @see also @c cpl_mplot_open().

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_ACCESS_OUT_OF_RANGE (firstcol or lastcol are out of range)
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_image_col(const char      * pre,
                                  const char      * options,
                                  const char      * post,
                                  const cpl_image * image,
                                  cpl_size          firstcol, 
                                  cpl_size          lastcol,
                                  cpl_size          colstep)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_size nx, ny;

    cpl_ensure_code(image != NULL, CPL_ERROR_NULL_INPUT);

    nx = cpl_image_get_size_x(image);
    ny = cpl_image_get_size_y(image);

    cpl_ensure_code( nx > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code( ny > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code( colstep > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code( firstcol > 0,    CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code( firstcol <= lastcol, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code( lastcol  <= nx,  CPL_ERROR_ACCESS_OUT_OF_RANGE);
   
    do {

        const double * pimage;
        cpl_image * temp = NULL;
        char * myoptions = NULL;
        cpl_size i, j;

        cpl_plot * plot = cpl_mplot_open(pre);

        do {
            if (cpl_error_get_code()) break;

            if (cpl_mplot_puts(plot, "plot")) break;
            for (i = firstcol-1; i < lastcol; i += colstep) {
                const char * useoptions = options;
                if (i > firstcol-1 && cpl_mplot_puts(plot, ",")) break;
                if (cpl_mplot_puts(plot, " '-' ")) break;
                if (useoptions == NULL || strlen(useoptions) < 1) {

                    cpl_free(myoptions);
                    myoptions = (i == firstcol-1)
                        ? cpl_sprintf("t 'Column %" CPL_SIZE_FORMAT " of a "
                                      "%" CPL_SIZE_FORMAT "X%" CPL_SIZE_FORMAT
                                      "-image-(%d) (%p) ", i, nx, ny,
                                      (int)cpl_image_get_type(image),
                                      (const void*)image)
                        : cpl_sprintf("t 'Column %" CPL_SIZE_FORMAT " of the "
                                      "same image'", i);
                    assert( myoptions != NULL);
                    useoptions = myoptions;

                }
                if (cpl_mplot_puts(plot, useoptions  )) break;
            }
            if (cpl_mplot_puts(plot, ";\n      ")) break;

            if (cpl_image_get_type(image) == CPL_TYPE_DOUBLE) {
                pimage = cpl_image_get_data_double_const(image);
            } else {
                temp = cpl_image_cast(image, CPL_TYPE_DOUBLE);
                pimage = cpl_image_get_data_double_const(temp);
            }

            for (i = firstcol-1; i < lastcol; i += colstep) {
                for (j = 0; j < ny; j++) {
                    char * snumber = cpl_sprintf("%g\n", pimage[j * nx + i]);
                    const int nstat = cpl_mplot_puts(plot, snumber);

                    cpl_free(snumber);
                    if (nstat) break;
                }
                if (j != ny) break;
                if (cpl_mplot_puts(plot, "e\n")) break;
            }
            if (i != lastcol) break;

        } while (0);

        cpl_free(myoptions);

        cpl_image_delete(temp);

        cpl_mplot_close(plot, post);

    } while (0);

    return cpl_errorstate_is_equal(prestate) ? CPL_ERROR_NONE
        : cpl_error_set_where_();
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Plot a column of a table 
  @param    pre      An optional string with pre-plot commands
  @param    options  An optional string with plotting options
  @param    post     An optional string with post-plot commands
  @param    tab      The table to plot 
  @param    xlab     The label of the column used in x
  @param    ylab     The label of the column used in y
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  @see also @c cpl_mplot_open().

  If xlab is NULL, the sequence number is used for X.

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_DATA_NOT_FOUND
  - CPL_ERROR_INVALID_TYPE
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_column(
        const char      *   pre, 
        const char      *   options,
        const char      *   post,
        const cpl_table *   tab,
        const char      *   xlab, 
        const char      *   ylab)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_size            n;
    cpl_table       *   tempx;
    cpl_table       *   tempy;
    cpl_bivector    *   bivect;
    cpl_vector      *   xvec;
    cpl_vector      *   yvec;
    cpl_type            type_x;
    cpl_type            type_y;
    cpl_size            invalid_x;
    cpl_size            invalid_y;

    /* Check input */
    cpl_ensure_code(tab, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(ylab, CPL_ERROR_NULL_INPUT);
    n = cpl_table_get_nrow(tab);
    cpl_ensure_code(n>0, CPL_ERROR_ILLEGAL_INPUT);

    if (xlab)
        cpl_ensure_code(cpl_table_has_column(tab, xlab), 
                CPL_ERROR_DATA_NOT_FOUND);
    cpl_ensure_code(cpl_table_has_column(tab, ylab), CPL_ERROR_DATA_NOT_FOUND);


    if (xlab != NULL) {
        invalid_x = cpl_table_count_invalid(tab, xlab);
        type_x = cpl_table_get_column_type(tab, xlab);
    } else {
        invalid_x = 0;
        type_x = CPL_TYPE_DOUBLE;
    }
    invalid_y = cpl_table_count_invalid(tab, ylab);
    type_y = cpl_table_get_column_type(tab, ylab);

    cpl_ensure_code(type_x == CPL_TYPE_INT ||
            type_x == CPL_TYPE_FLOAT ||
            type_x == CPL_TYPE_DOUBLE, CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(type_y == CPL_TYPE_INT ||
            type_y == CPL_TYPE_FLOAT ||
            type_y == CPL_TYPE_DOUBLE, CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(invalid_x < n && invalid_y < n, CPL_ERROR_DATA_NOT_FOUND);
  
    /* Cast columns to CPL_TYPE_DOUBLE and remove invalid entries */
    if (type_y != CPL_TYPE_DOUBLE || invalid_y > 0) {
        tempy = cpl_table_new(n);
        
        cpl_table_duplicate_column(tempy, "Y", tab, ylab);
        cpl_table_cast_column(tempy, "Y", "Ydouble", CPL_TYPE_DOUBLE);
        
        /* Remove rows with one or more invalid elements.
           No columns will be removed because each column
           contains at least one valid element. */
        cpl_table_erase_invalid(tempy);
        
        yvec = cpl_vector_wrap(cpl_table_get_nrow(tempy),
                   cpl_table_get_data_double(tempy, "Ydouble"));
    }
    else
    {
        tempy = NULL;
CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
        yvec = cpl_vector_wrap(n,
                               (double*)cpl_table_get_data_double_const(tab,
                                                                        ylab));
CPL_DIAG_PRAGMA_POP;

    }
     /* Cast columns to CPL_TYPE_DOUBLE and remove invalid entries */
    if (type_x != CPL_TYPE_DOUBLE || invalid_x > 0) {
        tempx = cpl_table_new(n);
        
        cpl_table_duplicate_column(tempx, "X", tab, xlab);
        cpl_table_cast_column(tempx, "X", "Xdouble", CPL_TYPE_DOUBLE);
        
        /* Remove rows with one or more invalid elements.
           No columns will be removed because each column
           contains at least one valid element. */

        cpl_table_erase_invalid(tempx);
        
        xvec = cpl_vector_wrap(cpl_table_get_nrow(tempx),
                   cpl_table_get_data_double(tempx, "Xdouble"));
    }
    else
    {
        tempx = NULL;
        
        if (xlab == NULL) {
            cpl_size i;
            xvec = cpl_vector_duplicate(yvec);
            for (i=0; i<cpl_vector_get_size(xvec); i++) {
                cpl_vector_set(xvec, i, (double)i+1);
            }
        } else {
CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
            xvec = cpl_vector_wrap(n,(double*)
                                   cpl_table_get_data_double_const(tab, xlab));
CPL_DIAG_PRAGMA_POP;
        }
    }
   
    bivect = cpl_bivector_wrap_vectors(xvec, yvec);
    cpl_plot_bivector(pre, options, post, bivect);
    
    cpl_bivector_unwrap_vectors(bivect);
    if (xlab == NULL) cpl_vector_delete(xvec);
    else (void)cpl_vector_unwrap(xvec);
    cpl_vector_unwrap(yvec);
    if (tempx != NULL) cpl_table_delete(tempx);
    if (tempy != NULL) cpl_table_delete(tempy);

    return cpl_errorstate_is_equal(prestate) ? CPL_ERROR_NONE
        : cpl_error_set_where_();
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Plot severals column of a table 
  @param    pre         An optional string with pre-plot commands
  @param    options     An optional string with plotting options
  @param    post        An optional string with post-plot commands
  @param    tab         The table to plot 
  @param    labels      The labels of the columns
  @param    nlabels     The number of labels
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  @see also @c cpl_mplot_open().

  If xlab is NULL, the sequence number is used for X.

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_DATA_NOT_FOUND
  - CPL_ERROR_INVALID_TYPE
  - CPL_ERROR_UNSUPPORTED_MODE if plotting is unsupported on the specific
    run-time system.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_plot_columns(const char      *  pre, 
                                const char      *  options,
                                const char      *  post,
                                const cpl_table *  tab,
                                const char      ** labels, 
                                cpl_size           nlabels)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_size            n;
    cpl_table       *   tempx;
    cpl_table       *   tempy;
    cpl_type            type_x;
    cpl_type            type_y;
    const double    *   px;
    const double    *   py;
    char            **  names;
    char            *   sval;
    FILE            *   tmpfd;
    cpl_plot        *   plot;
    cpl_size            i, j;

    /* Check input */
    cpl_ensure_code(tab     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(labels  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nlabels >= 3,    CPL_ERROR_DATA_NOT_FOUND);
    for (i = 1; i < nlabels; i++) {
        cpl_ensure_code(labels[i] != NULL, CPL_ERROR_NULL_INPUT);
    }

    /* Initialise */
    n = cpl_table_get_nrow(tab);
    tempx = cpl_table_new(n);
    tempy = cpl_table_new(n);

    /* Get the X axis if passed */
    if (labels[0] == NULL) px = NULL;
    else {
        cpl_ensure_code(cpl_table_has_column(tab, labels[0]), 
                CPL_ERROR_DATA_NOT_FOUND);
        type_x = cpl_table_get_column_type(tab, labels[0]);
        cpl_ensure_code(type_x == CPL_TYPE_INT || type_x == CPL_TYPE_FLOAT ||
                type_x == CPL_TYPE_DOUBLE, CPL_ERROR_INVALID_TYPE);
        
        if (type_x != CPL_TYPE_DOUBLE) {
            cpl_table_duplicate_column(tempx, "X", tab, labels[0]);
            cpl_table_cast_column(tempx, "X", "Xdouble", CPL_TYPE_DOUBLE);
            px = cpl_table_get_data_double_const(tempx, "Xdouble");
        } else {
            px = cpl_table_get_data_double_const(tab, labels[0]);
        }
    }

    /* Hold the files names */
    names = cpl_malloc((size_t)(nlabels-1) * sizeof(char*));
    for (i=1; i < nlabels; i++)
        names[i-1] = cpl_sprintf("cpl_plot-%" CPL_SIZE_FORMAT, i);

    /* Open the plot */
    plot = cpl_mplot_open(pre);

    /* Loop on the signals to plot */
    for (i=1; i<nlabels; i++) {

        /* Get the current Y axis */
        cpl_ensure_code(cpl_table_has_column(tab, labels[i]), 
                CPL_ERROR_DATA_NOT_FOUND);
        type_y = cpl_table_get_column_type(tab, labels[i]);
        if (type_y != CPL_TYPE_INT && type_y != CPL_TYPE_FLOAT &&
                type_y != CPL_TYPE_DOUBLE) {
            cpl_table_delete(tempx);
            cpl_table_delete(tempy);
            cpl_mplot_close(plot, post);
            for (j=1; j<nlabels; j++) (void) remove(names[j-1]);
            for (j=1; j<nlabels; j++) cpl_free(names[j-1]);
            cpl_free(names);
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
        
        if (type_y != CPL_TYPE_DOUBLE) {
            if (cpl_table_has_column(tempy, "Y"))
                cpl_table_erase_column(tempy, "Y");
            if (cpl_table_has_column(tempy, "Ydouble"))
                cpl_table_erase_column(tempy, "Ydouble");
            cpl_table_duplicate_column(tempy, "Y", tab, labels[i]);
            cpl_table_cast_column(tempy, "Y", "Ydouble", CPL_TYPE_DOUBLE);
            py = cpl_table_get_data_double_const(tempy, "Ydouble");
        } else {
            py = cpl_table_get_data_double_const(tab, labels[i]);
        }
    
        /* Open temporary file for output   */
        if ((tmpfd=fopen(names[i-1], "w"))==NULL) {
            cpl_table_delete(tempx);
            cpl_table_delete(tempy);
            cpl_mplot_close(plot, post);
            for (j=1; j<nlabels; j++) (void) remove(names[j-1]);
            for (j=1; j<nlabels; j++) cpl_free(names[j-1]);
            cpl_free(names);
            return cpl_error_set_(CPL_ERROR_FILE_IO);
        }

        /* Write data to this file  */
        for (j=0; j<n; j++) {
            char * snumber = cpl_sprintf("%g %g\n",
                                         px == NULL ? (double)(j+1)
                                         : px[j], py[j]);
            const int nstat = fputs(snumber, tmpfd);
            cpl_free(snumber);
            if (nstat < 0) break;
        }
        if (fclose(tmpfd) != 0) break;
        if (j != n) break;

        /* Plot the file */
        if (i==1) {
            /* Plot the first signal */
            sval = cpl_sprintf("plot '%s' %s;\n", names[i-1],
                                                 options ? options : "");
        } else {
            sval = cpl_sprintf("replot '%s' t 'Column %" CPL_SIZE_FORMAT
                               "' w lines;\n", names[i-1], i);
        }

        cpl_mplot_puts(plot, sval);
        cpl_free(sval);
    }
    cpl_mplot_close(plot, post);
    for (i=1; i<nlabels; i++) (void) remove(names[i-1]);
    for (i=1; i<nlabels; i++) cpl_free(names[i-1]);
    cpl_free(names);
    cpl_table_delete(tempx);
    cpl_table_delete(tempy);

    return cpl_errorstate_is_equal(prestate) ? CPL_ERROR_NONE
        : cpl_error_set_where_();
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the name of the plotting device
  @return   A string with the name of the plotting device

  The plotting device is set to either getenv("CPL_PLOTTER") or
  "gnuplot -persist".

 */
/*----------------------------------------------------------------------------*/
static const char * cpl_mplot_plotter(void)
{
    return getenv("CPL_PLOTTER") ? getenv("CPL_PLOTTER")
        : "gnuplot -persist";
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the name of the imaging device
  @return   A string with the name of the imaging device
  @see cpl_mplot_plotter()

  The imaging device is set to either getenv("CPL_IMAGER") or
  cpl_mplot_plotter().

  Examples of useful settings include
  'display - &'

 */
/*----------------------------------------------------------------------------*/
static const char * cpl_mplot_imager(void)
{
    return getenv("CPL_IMAGER") ? getenv("CPL_IMAGER")
        : cpl_mplot_plotter();
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Open a plotting device
  @param    options  An optional string to be sent to the plotter
  @return   A handle to the plotter or NULL on error
  @note This function fails if the real User ID unequals the effective UID.

  The plot is constructed by generating a file "cpl_plot.txt" which is a
  gnuplot script. This script is sent to the specified command using the call
  system("exec <cpl_plot.txt [plotter]").
  
  The command to execute
  can be controlled with the environment variable CPL_PLOTTER.
  If this is not set, 'gnuplot -persist' is used. In this case gnuplot
  must be in the path of the process.

  The plotting command must be able to parse gnuplot syntax on its stdin.
  Valid examples of such a command may include 'cat > myplot$$.gp' and
  '/usr/bin/gnuplot -persist 2> /dev/null'

  Consult your local documentation for the security caveats regarding
  the above usage of system().

  gnuplot is supported as plotting device. This means that if CPL_PLOTTER
  contains the string gnuplot it is assumed to correctly parse gnuplot syntax
  from stdin. Examples:
    perl ./my_gnuplot.pl
    /usr/bin/gnuplot -persist
    exec /usr/bin/gnuplot -persist > /dev/null 2>&1

  With a properly set CPL_PLOTTER, such a process is created and the
  options if any are sent to the plotter.

  When this function returns non-NULL cpl_mplot_close() must be called.

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_UNSUPPORTED_MODE
 */
/*----------------------------------------------------------------------------*/
static cpl_plot * cpl_mplot_open(const char * options)
{
    cpl_plot * plot;
    const char * exe = cpl_mplot_plotter();
    const char * tmp = CPL_PLOT_TMPFILE;
    FILE * data;


    cpl_ensure(exe != NULL, CPL_ERROR_NULL_INPUT, NULL);

    cpl_error_ensure(strstr(exe, "gnuplot"), CPL_ERROR_UNSUPPORTED_MODE,
                     return NULL, "%s", exe);

    data = fopen(tmp, "w");

    cpl_error_ensure(data != NULL, CPL_ERROR_FILE_IO, return NULL,
                     "%s", tmp);

    plot = cpl_malloc(sizeof(cpl_plot));

    plot->exe  = exe;
    plot->tmp  = tmp;
    plot->data = data;

    if (cpl_mplot_puts(plot, options)) {
        (void)cpl_mplot_close(plot, "");
        plot = NULL;
        (void)cpl_error_set_where_();
    }

    return plot;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Open an imaging device
  @param    options  An optional string to be sent to the imager
  @return   A handle to the imager or NULL on error

  gnuplot is supported as imaging device. This means that if CPL_IMAGER
  contains the string gnuplot it is assumed to correctly parse gnuplot syntax
  from stdin. Examples:
    perl ./my_gnuplot.pl
    /usr/bin/gnuplot -persist
    exec /usr/bin/gnuplot -persist > /dev/null 2>&1

  imagers that can parse a pgm (P5) image, f.ex. display are also supported.

  The plot is constructed by generating a file "cpl_plot.txt" which is a
  gnuplot script or a pgm image. This script is sent to the specified command
  using the call system("exec <cpl_plot.txt [imager]").

  With a properly set CPL_IMAGER, such a process is created and the
  options if any are sent to the plotter.

  When this function returns non-NULL cpl_mplot_close() must be called.

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO
 */
/*----------------------------------------------------------------------------*/
static cpl_plot * cpl_image_open(const char * options)
{
    cpl_plot * plot;
    const char * exe = cpl_mplot_imager();
    const char * tmp = CPL_PLOT_TMPFILE;
    FILE * data;


    cpl_ensure(exe != NULL, CPL_ERROR_NULL_INPUT, NULL);

    data = fopen(tmp, "w");

    cpl_error_ensure(data != NULL, CPL_ERROR_FILE_IO, return NULL,
                     "%s", tmp);

    plot = cpl_malloc(sizeof(cpl_plot));

    plot->exe  = exe;
    plot->tmp  = tmp;
    plot->data = data;

    if (cpl_mplot_puts(plot, options)) {
        (void)cpl_mplot_close(plot, "");
        plot = NULL;
        (void)cpl_error_set_where_();
    }

    return plot;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Close the plotting device and deallocate its resources
  @param    plot     Pointer to the plotting device
  @param    options  An optional string to be sent to the plotter
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_FILE_IO
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mplot_close(cpl_plot * plot,
                                         const char  * options)
{
    cpl_error_code error;


    cpl_ensure_code(plot != NULL, CPL_ERROR_NULL_INPUT);

    error = cpl_mplot_puts(plot, options);
    cpl_error_set_where_();

    assert(plot->data != NULL);
    assert(plot->tmp != NULL);
    assert(plot->exe != NULL);

    if (fclose(plot->data) != 0) error = cpl_error_set_(CPL_ERROR_FILE_IO);

    if (!error) {
        /* Note: The portability of this plotting module is limited by the
           portability of the following command (which is likely to be valid on
           any kind of UNIX platform). We use 'exec' to reduce the number of
           child processes created. */
        char * command = cpl_sprintf("exec <%s %s", plot->tmp, plot->exe);
        const int nstat = system(command);

        if (nstat != 0) error =
            cpl_error_set_message_(CPL_ERROR_FILE_IO, "system('%s') returned "
                                   "%d", command ? command : "<NULL>", nstat);
        cpl_free(command);
    }
    (void) remove(plot->tmp);

    cpl_free(plot);

    return error ? error : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Send a text string to the plotting device
  @param    plot Pointer to the plotting device
  @param    cmd  A string to be sent to the plotter
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_FILE_IO

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mplot_puts(cpl_plot * plot, const char * cmd)
{

    if (cmd == NULL || strlen(cmd) == 0) return CPL_ERROR_NONE;

    cpl_ensure_code(plot != NULL, CPL_ERROR_NULL_INPUT);

    assert(plot->data != NULL);

    cpl_ensure_code(fputs(cmd, plot->data) >= 0, CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Send n octets of data to the plotting device
  @param    plot    Pointer to the plotting device
  @param    buffer  Pointer to the data to be sent to the plotter
  @param    length  Number of octets to send
  @return   CPL_ERROR_NONE or the relevant CPL_ERROR_# on error

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_FILE_IO
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mplot_write(cpl_plot     *   plot, 
                                         const char      *   buffer,
                                         size_t              length)
{
    cpl_ensure_code(plot   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(buffer != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(length > 0,     CPL_ERROR_ILLEGAL_INPUT);

    assert(plot->data != NULL);

    cpl_ensure_code(fwrite(buffer, 1, length, plot->data) == length,
                    CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;
}
