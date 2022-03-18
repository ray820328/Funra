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

#include "cpl_apertures.h"

#include <cpl_memory.h>
#include <cpl_image.h>
#include <cpl_stats.h>
#include <cpl_mask.h>
#include <cpl_matrix.h>
#include <cpl_tools.h>
#include <cpl_error_impl.h>

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <assert.h>

/* Needed by memcpy() */
#include <string.h>

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_apertures  High level functions to handle apertures
 *
 * The aperture object contains a list of zones in an image. It is
 * typically used to contain the results of an objects detection, or if
 * one wants to work on a very specific zone in an image.
 *
 * This module provides functions to handle @em cpl_apertures.
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                Type definition
 -----------------------------------------------------------------------------*/

struct _cpl_apertures_ {
    /* Number of apertures */
    cpl_size    naperts;

    /* All positions are in FITS conventions : (1,1) is the lower left) */
    /* Aperture center: x=sum(x*1)/sum(1) */
    double  *   x;
    double  *   y;
    /* The position of the aperture maximum */
    cpl_size*   maxpos_x;
    cpl_size*   maxpos_y;
    /* The position of the aperture minimum */
    cpl_size*   minpos_x;
    cpl_size*   minpos_y;
    /* Aperture weighted center : xcentroid=sum(x*f(x)) / sum(f(x)) */
    double  *   xcentroid;
    double  *   ycentroid;

    cpl_size*   npix;
    cpl_size*   left_x;
    cpl_size*   left_y;
    cpl_size*   right_x;
    cpl_size*   right_y;
    cpl_size*   top_x;
    cpl_size*   top_y;
    cpl_size*   bottom_x;
    cpl_size*   bottom_y;
    double  *   max_val;
    double  *   min_val;
    double  *   mean;
    double  *   varsum;
    double  *   median;
    double  *   stdev;
    double  *   flux;
};


/*-----------------------------------------------------------------------------
                                Private functions
 -----------------------------------------------------------------------------*/

static cpl_apertures * cpl_apertures_new(cpl_size) CPL_ATTR_ALLOC;
static cpl_apertures * cpl_apertures_new_from_image_(const cpl_image *,
                                                     const cpl_image *,
                                                     cpl_size, cpl_boolean);

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Destructor for cpl_apertures
  @param    self The object to delete.
  @return   void
  @note If @em self is @c NULL, nothing is done and no error is set.

  This function deallocates all memory allocated for the object.
  
 */
/*----------------------------------------------------------------------------*/
void cpl_apertures_delete(cpl_apertures * self)
{

    if (self !=NULL) {
        cpl_free(self->x);
        cpl_free(self->y);
        cpl_free(self->maxpos_x);
        cpl_free(self->maxpos_y);
        cpl_free(self->minpos_x);
        cpl_free(self->minpos_y);
        cpl_free(self->xcentroid);
        cpl_free(self->ycentroid);
        cpl_free(self->npix);
        cpl_free(self->left_x);
        cpl_free(self->left_y);
        cpl_free(self->right_x);
        cpl_free(self->right_y);
        cpl_free(self->top_x);
        cpl_free(self->top_y);
        cpl_free(self->bottom_x);
        cpl_free(self->bottom_y);
        cpl_free(self->max_val);
        cpl_free(self->min_val);
        cpl_free(self->mean);
        cpl_free(self->varsum);
        cpl_free(self->median);
        cpl_free(self->stdev);
        cpl_free(self->flux);
        cpl_free(self);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a cpl_apertures to an opened file pointer.
  @param    self   The cpl_apertures to dump
  @param    fp     File pointer, may use @em stdout or @em stderr
  @return   void

  This function dumps all information in a cpl_apertures to the
  passed file pointer. If the object is unallocated or contains nothing,
  this function does nothing.
 */
/*----------------------------------------------------------------------------*/
void cpl_apertures_dump(const cpl_apertures * self,
                        FILE            * fp)
{
    if (self !=NULL && fp !=NULL && self->naperts > 0) {
        cpl_size i;

        fprintf(fp, "#        X      Y");
        fprintf(fp, " XCENTROID YCENTROID");
        fprintf(fp, "   XMAX   YMAX");
        fprintf(fp, "   XMIN   YMIN");
        fprintf(fp, "    pix");
        fprintf(fp, "    max");
        fprintf(fp, "    min");
        fprintf(fp, "   mean");
        fprintf(fp, "    med");
        fprintf(fp, "    dev");
        fprintf(fp, "   flux");
        fprintf(fp, "\n");

        for (i = 0; i < self->naperts; i++) {
            fprintf(fp, "% 3" CPL_SIZE_FORMAT " %6.1f %6.1f", i+1,
                    self->x[i], self->y[i]);
            fprintf(fp, "    %6.1f    %6.1f",
                    self->xcentroid[i], self->ycentroid[i]);
            fprintf(fp, " %6" CPL_SIZE_FORMAT " %6" CPL_SIZE_FORMAT,
                    self->maxpos_x[i], self->maxpos_y[i]);
            fprintf(fp, " %6" CPL_SIZE_FORMAT " %6" CPL_SIZE_FORMAT,
                    self->minpos_x[i], self->minpos_y[i]);
            fprintf(fp, " % 6" CPL_SIZE_FORMAT, self->npix[i]);
            fprintf(fp, " %6.2f", self->max_val[i]);
            fprintf(fp, " %6.2f", self->min_val[i]);
            fprintf(fp, " %6.2f", self->mean[i]);
            fprintf(fp, " %6.2f", self->median[i]);
            fprintf(fp, " %6.2f", self->stdev[i]);
            fprintf(fp, " %8.2f", self->flux[i]);
            fprintf(fp, "\n");
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute statistics on selected apertures
  @param    self    Reference image
  @param    lab     Labelized image (of type CPL_TYPE_INT)
  @return   An CPL apertures object or @em NULL on error
  @note The returned object must be deleted using cpl_apertures_delete().
  @see cpl_image_labelise_mask_create()

  The labelized image must contain at least one pixel for each value from 1
  to the maximum value in the image.

  For the centroiding computation of an aperture, if some pixels have
  values lower or equal to 0, all the values of the aperture are locally
  shifted such as the minimum value of the aperture has a value of
  epsilon. The centroid is then computed on these positive values. In
  principle, centroid should always be computed on positive values, this
  is done to avoid raising an error in case the caller of the function
  wants to use it on negative values images without caring about the
  centroid results. In such cases, the centroid result would be
  meaningful, but slightly depend on the hardcoded value chosen for
  epsilon (1e-10).

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_TYPE_MISMATCH if lab is not of type CPL_TYPE_INT or
                            if self is of a complex type 
  - CPL_ERROR_ILLEGAL_INPUT if lab has a negative value or zero maximum
  - CPL_ERROR_INCOMPATIBLE_INPUT if lab and self have different sizes.
  - CPL_ERROR_DATA_NOT_FOUND if one of the lab values is missing.
 */
/*----------------------------------------------------------------------------*/
cpl_apertures * cpl_apertures_new_from_image(const cpl_image * self,
                                             const cpl_image * lab)
{

    cpl_apertures * aperts = cpl_apertures_new_from_image_(self, lab, 0,
                                                           CPL_TRUE);

    if (aperts == NULL) (void)cpl_error_set_where_();

    return aperts;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of apertures
  @param    self      The cpl_apertures object
  @return   The number of apertures or -1 on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_size(const cpl_apertures * self)
{
    cpl_ensure(self, CPL_ERROR_NULL_INPUT, -1);
    return self->naperts;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the average X-position of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The average X-position of the aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_centroid_x()

 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_pos_x(const cpl_apertures * self,
                               cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3.0);

    return self->x[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the average Y-position of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The average Y-position of the aperture or negative on error
  @see      cpl_apertures_get_pos_x()
 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_pos_y(const cpl_apertures * self,
                               cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3.0);

    return self->y[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the average X-position of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The average X-position of the aperture or negative on error
  @deprecated Replace this function with cpl_apertures_get_pos_x()

 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_max_x(const cpl_apertures * self,
                               cpl_size              ind)
{
    return cpl_apertures_get_pos_x(self, ind);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Get the average Y-position of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The average Y-position of the aperture or negative on error
  @deprecated Replace this function with cpl_apertures_get_pos_y()

 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_max_y(const cpl_apertures * self,
                               cpl_size              ind)
{
    return cpl_apertures_get_pos_y(self, ind);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Get the X-centroid of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The X-centroid position of the aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set

  For a concave aperture the centroid may not belong to the aperture.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if ind is non-positive
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if ind exceeds the number of apertures in self
 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_centroid_x(const cpl_apertures * self,
                                    cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3.0);

    return self->xcentroid[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the Y-centroid of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The X-centroid position of the aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_centroid_x()

 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_centroid_y(const cpl_apertures * self,
                                    cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3.0);

    return self->ycentroid[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the X-position of the aperture maximum value
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The X-position of the aperture maximum value or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_centroid_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_maxpos_x(const cpl_apertures * self,
                                    cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->maxpos_x[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the Y-position of the aperture maximum value
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The Y-position of the aperture maximum value or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_maxpos_y(const cpl_apertures * self,
                                    cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->maxpos_y[ind-1];
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Get the X-position of the aperture minimum value
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The X-position of the aperture minimum value or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_minpos_x(const cpl_apertures * self,
                                    cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->minpos_x[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the Y-position of the aperture minimum value
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The Y-position of the aperture minimum value or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_minpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_minpos_y(const cpl_apertures * self,
                                    cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->minpos_y[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of pixels of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The number of pixels of the aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if ind is non-positive
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if ind exceeds the number of apertures in self
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_npix(const cpl_apertures * self,
                                cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->npix[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the leftmost x position in an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   the leftmost x position of the aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_left(const cpl_apertures * self,
                                cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->left_x[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief   Get the y position of the leftmost x position in an aperture
  @param   self    The cpl_apertures object
  @param   ind     The aperture index (1 for the first one)
  @return  the y position of the leftmost x position or negative on error
  @note An aperture may have multiple leftmost x positions, in which case one
  of these is returned.
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_left_y(const cpl_apertures * self,
                                  cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->left_y[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the rightmost x position in an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   the rightmost x position in an aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_right(const cpl_apertures * self,
                                 cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->right_x[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the y position of the rightmost x position in an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   the y position of the rightmost x position or negative on error
  @note An aperture may have multiple rightmost x positions, in which case one
  of these is returned.
  @see cpl_apertures_get_maxpos_x()
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_right_y(const cpl_apertures * self,
                                   cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->right_y[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the x position of the bottommost y position in an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   the bottommost x position of the aperture or negative on error
  @note An aperture may have multiple bottom x positions, in which case one
  of these is returned.
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_bottom_x(const cpl_apertures * self,
                                    cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->bottom_x[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the bottommost y position in an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   the bottommost y position in the aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_bottom(const cpl_apertures * self,
                                  cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->bottom_y[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the x position of the topmost y position in an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   the x position of the topmost y position or negative on error
  @note An aperture may have multiple topmost x positions, in which case one
  of these is returned.
  @see cpl_apertures_get_maxpos_x()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_top_x(const cpl_apertures * self,
                                 cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->top_x[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the topmost y position in an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   the topmost y position in the aperture or negative on error
  @note In case of an error the #_cpl_error_code_ code is set
  @see cpl_apertures_get_maxpos_x()

*/
/*----------------------------------------------------------------------------*/
cpl_size cpl_apertures_get_top(const cpl_apertures * self,
                               cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);

    return self->top_y[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the maximum value of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The maximum value of the aperture or undefined on error
  @note In case of an error the #_cpl_error_code_ code is set

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if ind is non-positive
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if ind exceeds the number of apertures in self
 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_max(const cpl_apertures * self,
                             cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          0.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       0.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, 0.0);

    return self->max_val[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the minimum value of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The minimum value of the aperture or undefined on error
  @see cpl_apertures_get_max()

 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_min(const cpl_apertures * self,
                             cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          0.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       0.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, 0.0);

    return self->min_val[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the mean value of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The mean value of the aperture or undefined on error
  @see cpl_apertures_get_max()

 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_mean(const cpl_apertures * self,
                              cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          0.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       0.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, 0.0);

    return self->mean[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the median value of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The median value of the aperture or undefined on error
  @see cpl_apertures_get_max()
 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_median(const cpl_apertures * self,
                                cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          0.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       0.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, 0.0);

    return self->median[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the standard deviation of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The standard deviation of the aperture or negative on error
  @see cpl_apertures_get_max()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if ind is non-positive
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if ind exceeds the number of apertures in self
  - CPL_ERROR_DATA_NOT_FOUND if the aperture comprises less than two pixels

 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_stdev(const cpl_apertures * self,
                               cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          -1.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       -2.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, -3.0);

    /* A negative value is used as internal representation of an undefined
       stdev. An error is raised only when the stdev is asked for */
    cpl_ensure(self->stdev[ind-1] >= 0.0, CPL_ERROR_DATA_NOT_FOUND, -4.0);

    return self->stdev[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the flux of an aperture
  @param    self    The cpl_apertures object
  @param    ind     The aperture index (1 for the first one)
  @return   The flux of the aperture or undefined on error
  @see      cpl_apertures_get_max()
 */
/*----------------------------------------------------------------------------*/
double cpl_apertures_get_flux(const cpl_apertures * self,
                              cpl_size              ind)
{

    cpl_ensure(self != NULL,          CPL_ERROR_NULL_INPUT,          0.0);
    cpl_ensure(ind  >  0,             CPL_ERROR_ILLEGAL_INPUT,       0.0);
    cpl_ensure(ind  <= self->naperts, CPL_ERROR_ACCESS_OUT_OF_RANGE, 0.0);

    return self->flux[ind-1];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Sort by decreasing aperture size
  @param    self      Apertures to sort (MODIFIED)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_apertures_sort_by_npix(cpl_apertures * self)
{
    cpl_apertures *   sorted_apert;
    void          *   swap;
    cpl_size          naperts;
    cpl_size      *   sorted_ind;
    cpl_size      *   computed;
    cpl_size        max_npix;
    cpl_size        max_ind = -1; /* Avoid (false) uninit warning */
    cpl_size        i, j;

    /* Test entries */
    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);

    /* Initialize */
    naperts = self->naperts;

    /* Compute the sorted index array - FIXME: Use better sort algorithm ? */
    sorted_ind = cpl_malloc((size_t)naperts * sizeof(*sorted_ind));
    computed   = cpl_calloc((size_t)naperts,  sizeof(*computed));
    for (j = 0; j < naperts; j++) {
        max_npix = -1;
        for (i = 0; i < naperts; i++) {
            if ((computed[i]==0) && (max_npix < self->npix[i])) {
                max_npix = self->npix[i];
                max_ind = i;
            }
        }
        computed[max_ind] = 1;
        sorted_ind[j] = max_ind;
    }
    cpl_free(computed);

    /* Now sort the input apertures */
    sorted_apert = cpl_apertures_new(naperts);
    for (i = 0; i < sorted_apert->naperts; i++) {
        sorted_apert->x[i]          = self->x[sorted_ind[i]];
        sorted_apert->y[i]          = self->y[sorted_ind[i]];
        sorted_apert->maxpos_x[i]   = self->maxpos_x[sorted_ind[i]];
        sorted_apert->maxpos_y[i]   = self->maxpos_y[sorted_ind[i]];
        sorted_apert->minpos_x[i]   = self->minpos_x[sorted_ind[i]];
        sorted_apert->minpos_y[i]   = self->minpos_y[sorted_ind[i]];
        sorted_apert->xcentroid[i]  = self->xcentroid[sorted_ind[i]];
        sorted_apert->ycentroid[i]  = self->ycentroid[sorted_ind[i]];
        sorted_apert->npix[i]       = self->npix[sorted_ind[i]];
        sorted_apert->left_x[i]     = self->left_x[sorted_ind[i]];
        sorted_apert->left_y[i]     = self->left_y[sorted_ind[i]];
        sorted_apert->right_x[i]    = self->right_x[sorted_ind[i]];
        sorted_apert->right_y[i]    = self->right_y[sorted_ind[i]];
        sorted_apert->top_x[i]      = self->top_x[sorted_ind[i]];
        sorted_apert->top_y[i]      = self->top_y[sorted_ind[i]];
        sorted_apert->bottom_x[i]   = self->bottom_x[sorted_ind[i]];
        sorted_apert->bottom_y[i]   = self->bottom_y[sorted_ind[i]];
        sorted_apert->max_val[i]    = self->max_val[sorted_ind[i]];
        sorted_apert->min_val[i]    = self->min_val[sorted_ind[i]];
        sorted_apert->mean[i]       = self->mean[sorted_ind[i]];
        sorted_apert->median[i]     = self->median[sorted_ind[i]];
        sorted_apert->stdev[i]      = self->stdev[sorted_ind[i]];
        sorted_apert->flux[i]       = self->flux[sorted_ind[i]];
    }
    cpl_free(sorted_ind);

    /* Swap the sorted and the input apertures */
    swap = cpl_malloc(sizeof(cpl_apertures));
    memcpy(swap,         sorted_apert, sizeof(cpl_apertures));
    memcpy(sorted_apert, self,           sizeof(cpl_apertures));
    memcpy(self,           swap,         sizeof(cpl_apertures));

    cpl_free(swap);
    cpl_apertures_delete(sorted_apert);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Sort by decreasing aperture peak value
  @param    self      Apertures to sort (MODIFIED)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_apertures_sort_by_npix()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_apertures_sort_by_max(cpl_apertures * self)
{
    cpl_apertures * sorted_apert;
    void          * swap;
    cpl_size        naperts;
    cpl_size    *   sorted_ind;
    cpl_size    *   computed;
    double          max_max = DBL_MAX; /* Avoid (false) uninit warning */
    cpl_size        max_ind;
    cpl_size        i, j;

    /* Test entries */
    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);

    /* Initialize */
    naperts = self->naperts;

    /* Compute the sorted index array - FIXME: Use better sort algorithm ? */
    sorted_ind = cpl_malloc((size_t)naperts * sizeof(*sorted_apert));
    computed   = cpl_calloc((size_t)naperts,  sizeof(*computed));
    for (j = 0; j < naperts; j++) {
        max_ind = -1;
        for (i = 0; i < naperts; i++) {
            if ((computed[i]==0) && (max_ind < 0 || max_max < self->max_val[i])) {
                max_max = self->max_val[i];
                max_ind = i;
            }
        }
        computed[max_ind] = 1;
        sorted_ind[j] = max_ind;
    }
    cpl_free(computed);

    /* Now sort the input apertures */
    sorted_apert = cpl_apertures_new(naperts);
    for (i = 0; i < sorted_apert->naperts; i++) {
        sorted_apert->x[i]          = self->x[sorted_ind[i]];
        sorted_apert->y[i]          = self->y[sorted_ind[i]];
        sorted_apert->maxpos_x[i]   = self->maxpos_x[sorted_ind[i]];
        sorted_apert->maxpos_y[i]   = self->maxpos_y[sorted_ind[i]];
        sorted_apert->minpos_x[i]   = self->minpos_x[sorted_ind[i]];
        sorted_apert->minpos_y[i]   = self->minpos_y[sorted_ind[i]];
        sorted_apert->xcentroid[i]  = self->xcentroid[sorted_ind[i]];
        sorted_apert->ycentroid[i]  = self->ycentroid[sorted_ind[i]];
        sorted_apert->npix[i]       = self->npix[sorted_ind[i]];
        sorted_apert->left_x[i]     = self->left_x[sorted_ind[i]];
        sorted_apert->left_y[i]     = self->left_y[sorted_ind[i]];
        sorted_apert->right_x[i]    = self->right_x[sorted_ind[i]];
        sorted_apert->right_y[i]    = self->right_y[sorted_ind[i]];
        sorted_apert->top_x[i]      = self->top_x[sorted_ind[i]];
        sorted_apert->top_y[i]      = self->top_y[sorted_ind[i]];
        sorted_apert->bottom_x[i]   = self->bottom_x[sorted_ind[i]];
        sorted_apert->bottom_y[i]   = self->bottom_y[sorted_ind[i]];
        sorted_apert->max_val[i]    = self->max_val[sorted_ind[i]];
        sorted_apert->min_val[i]    = self->min_val[sorted_ind[i]];
        sorted_apert->mean[i]       = self->mean[sorted_ind[i]];
        sorted_apert->median[i]     = self->median[sorted_ind[i]];
        sorted_apert->stdev[i]      = self->stdev[sorted_ind[i]];
        sorted_apert->flux[i]       = self->flux[sorted_ind[i]];
    }
    cpl_free(sorted_ind);

    /* Swap the sorted and the input apertures */
    swap = cpl_malloc(sizeof(cpl_apertures));
    memcpy(swap,         sorted_apert, sizeof(cpl_apertures));
    memcpy(sorted_apert, self,           sizeof(cpl_apertures));
    memcpy(self,           swap,         sizeof(cpl_apertures));

    cpl_free(swap);
    cpl_apertures_delete(sorted_apert);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Sort by decreasing aperture flux
  @param    self      Apertures to sort (MODIFIED)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_apertures_sort_by_npix()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_apertures_sort_by_flux(cpl_apertures * self)
{
    cpl_apertures * sorted_apert;
    void          * swap;
    cpl_size        naperts;
    cpl_size      * sorted_ind;
    cpl_size      * computed;
    double          max_flux = DBL_MAX; /* Avoid (false) uninit warning */
    cpl_size        max_ind;
    cpl_size        i, j;

    /* Test entries */
    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);

    /* Initialize */
    naperts = self->naperts;

    /* Compute the sorted index array - FIXME: Use better sort algorithm ? */
    sorted_ind = cpl_malloc((size_t)naperts * sizeof(*sorted_ind));
    computed   = cpl_calloc((size_t)naperts, sizeof(*computed));
    for (j = 0; j < naperts; j++) {
        max_ind = -1;
        for (i = 0; i < naperts; i++) {
            if ((computed[i]==0) && (max_ind < 0 || max_flux < self->flux[i])) {
                max_flux = self->flux[i];
                max_ind = i;
            }
        }
        computed[max_ind] = 1;
        sorted_ind[j] = max_ind;
    }
    cpl_free(computed);

    /* Now sort the input apertures */
    sorted_apert = cpl_apertures_new(naperts);
    for (i = 0; i < sorted_apert->naperts; i++) {
        sorted_apert->x[i]          = self->x[sorted_ind[i]];
        sorted_apert->y[i]          = self->y[sorted_ind[i]];
        sorted_apert->maxpos_x[i]   = self->maxpos_x[sorted_ind[i]];
        sorted_apert->maxpos_y[i]   = self->maxpos_y[sorted_ind[i]];
        sorted_apert->minpos_x[i]   = self->minpos_x[sorted_ind[i]];
        sorted_apert->minpos_y[i]   = self->minpos_y[sorted_ind[i]];
        sorted_apert->xcentroid[i]  = self->xcentroid[sorted_ind[i]];
        sorted_apert->ycentroid[i]  = self->ycentroid[sorted_ind[i]];
        sorted_apert->npix[i]       = self->npix[sorted_ind[i]];
        sorted_apert->left_x[i]     = self->left_x[sorted_ind[i]];
        sorted_apert->left_y[i]     = self->left_y[sorted_ind[i]];
        sorted_apert->right_x[i]    = self->right_x[sorted_ind[i]];
        sorted_apert->right_y[i]    = self->right_y[sorted_ind[i]];
        sorted_apert->top_x[i]      = self->top_x[sorted_ind[i]];
        sorted_apert->top_y[i]      = self->top_y[sorted_ind[i]];
        sorted_apert->bottom_x[i]   = self->bottom_x[sorted_ind[i]];
        sorted_apert->bottom_y[i]   = self->bottom_y[sorted_ind[i]];
        sorted_apert->max_val[i]    = self->max_val[sorted_ind[i]];
        sorted_apert->min_val[i]    = self->min_val[sorted_ind[i]];
        sorted_apert->mean[i]       = self->mean[sorted_ind[i]];
        sorted_apert->median[i]     = self->median[sorted_ind[i]];
        sorted_apert->stdev[i]      = self->stdev[sorted_ind[i]];
        sorted_apert->flux[i]       = self->flux[sorted_ind[i]];
    }
    cpl_free(sorted_ind);

    /* Swap the sorted and the input apertures */
    swap = cpl_malloc(sizeof(cpl_apertures));
    memcpy(swap,         sorted_apert, sizeof(cpl_apertures));
    memcpy(sorted_apert, self,           sizeof(cpl_apertures));
    memcpy(self,           swap,         sizeof(cpl_apertures));

    cpl_free(swap);
    cpl_apertures_delete(sorted_apert);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Simple detection of apertures in an image
  @param    self        The image to process
  @param    sigmas      Positive, decreasing sigmas to apply
  @param    pisigma     Index of the sigma that was used or unchanged on error
  @return   The detected apertures or NULL on error
  @see cpl_apertures_extract_sigma()

  pisigma may be NULL.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if self or sigmas is NULL
  - CPL_ERROR_DATA_NOT_FOUND if the apertures cannot be detected
 */
/*----------------------------------------------------------------------------*/
cpl_apertures * cpl_apertures_extract(const cpl_image  * self,
                                      const cpl_vector * sigmas,
                                      cpl_size         * pisigma)
{
    cpl_errorstate  prestate = cpl_errorstate_get();
    cpl_apertures * aperts = NULL;
    const cpl_size  n = cpl_vector_get_size(sigmas);
    cpl_size        i;

    cpl_ensure(self   != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(sigmas != NULL, CPL_ERROR_NULL_INPUT, NULL);

    for (i = 0; i < n; i++) {
        const double sigma = cpl_vector_get(sigmas, i);

        if (sigma <= 0.0) break;

        /* Apply the detection  */
        aperts = cpl_apertures_extract_sigma(self, sigma);
        if (aperts != NULL) break;
    }

    cpl_ensure(aperts != NULL, CPL_ERROR_DATA_NOT_FOUND, NULL);

    /* Recover from any errors set in cpl_apertures_extract_sigma() */
    cpl_errorstate_set(prestate);

    if (pisigma) *pisigma = i;

    return aperts;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Simple detection of apertures in an image window
  @param    self        The image to process
  @param    sigmas      Positive, decreasing sigmas to apply
  @param    llx         Lower left x position (FITS convention)
  @param    lly         Lower left y position (FITS convention)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @param    pisigma     Index of the sigma that was used or undefined on error
  @return   The list of detected apertures or NULL on error
  @see cpl_apertures_extract()
  @see cpl_image_extract()
 */
/*----------------------------------------------------------------------------*/
cpl_apertures * cpl_apertures_extract_window(const cpl_image  * self,
                                             const cpl_vector * sigmas,
                                             cpl_size           llx,
                                             cpl_size           lly,
                                             cpl_size           urx,
                                             cpl_size           ury,
                                             cpl_size         * pisigma)
{
    cpl_apertures * aperts;
    cpl_image     * extracted;
    cpl_size        i;

    /* Test entries */
    cpl_ensure(self != NULL,     CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(sigmas != NULL, CPL_ERROR_NULL_INPUT, NULL);

    /* Extract the subwindow - and check entries */
    extracted = cpl_image_extract(self, llx, lly, urx, ury);
    cpl_ensure(extracted, cpl_error_get_code(), NULL);

    /* Get the apertures */
    aperts = cpl_apertures_extract(extracted, sigmas, pisigma);
    cpl_image_delete(extracted);
    cpl_ensure(aperts, cpl_error_get_code(), NULL);

    /* Update the detected positions */
    for (i = 0; i < aperts->naperts; i++) {
        aperts->x[i] += (double)(llx - 1);
        aperts->y[i] += (double)(lly - 1);
        aperts->maxpos_x[i] += (double)(llx - 1);
        aperts->maxpos_y[i] += (double)(lly - 1);
        aperts->minpos_x[i] += (double)(llx - 1);
        aperts->minpos_y[i] += (double)(lly - 1);
        aperts->xcentroid[i] += (double)(llx - 1);
        aperts->ycentroid[i] += (double)(lly - 1);
        aperts->left_x[i] += llx - 1;
        aperts->left_y[i] += lly - 1;
        aperts->right_x[i] += llx - 1;
        aperts->right_y[i] += lly - 1;
        aperts->top_x[i] += llx - 1;
        aperts->top_y[i] += lly - 1;
        aperts->bottom_x[i] += llx - 1;
        aperts->bottom_y[i] += lly - 1;
    }

    return aperts;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Simple apertures creation from a user supplied selection mask
  @param    self    The image to process
  @param    selection  The mask of selected pixels
  @return   The list of detected apertures or NULL if nothing detected or on
            error.
  @see cpl_image_labelise_mask_create(), cpl_apertures_new_from_image()

  The values selected for inclusion in the apertures must have the non-zero
  value in the selection mask, and must not be flagged as bad in the bad pixel
  map of the image.

  The input image type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or CPL_TYPE_INT.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if self and selection have different sizes.
  - CPL_ERROR_TYPE_MISMATCH if self is of a complex type 
  - CPL_ERROR_DATA_NOT_FOUND if the selection mask is empty
 */
/*----------------------------------------------------------------------------*/
cpl_apertures * cpl_apertures_extract_mask(const cpl_image * self,
                                           const cpl_mask  * selection)
{
    cpl_errorstate  prestate = cpl_errorstate_get();
    cpl_apertures * aperts = NULL;
    cpl_size        nlabels;
    /* Labelise the user provided selection */
    cpl_image     * labels = cpl_image_labelise_mask_create(selection,
                                                            &nlabels);

    if (labels != NULL && nlabels > 0) {

        /* Create the detected apertures list */
        aperts = cpl_apertures_new_from_image_(self, labels, nlabels,
                                               CPL_FALSE);

    }

    if (aperts == NULL) {
        cpl_errorstate_is_equal(prestate)
            ? (void)cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND)
            : (void)cpl_error_set_where_();
    }

    cpl_image_delete(labels);

    return aperts;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Simple apertures detection in an image using a provided sigma
  @param    self    The image to process
  @param    sigma   Detection level
  @return   The list of detected apertures or NULL on error
  @note In order to avoid (the potentially many) detections of small objects the
  mask of detected pixels is subjected to a 3x3 morphological opening filter.
  @see cpl_apertures_extract_mask(), cpl_mask_filter()

  The threshold used for the detection is the median plus the average distance
  to the median times sigma.

  The input image type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or CPL_TYPE_INT.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if sigma is non-positive
  - CPL_ERROR_TYPE_MISMATCH if self is of a complex type
  - CPL_ERROR_DATA_NOT_FOUND if the no apertures are found
 */
/*----------------------------------------------------------------------------*/
cpl_apertures * cpl_apertures_extract_sigma(const cpl_image * self,
                                            double            sigma)
{
    double          median, med_dist;
    double          threshold;
    cpl_mask    *   selection;
    cpl_apertures * aperts = NULL;

    /* Check entries */
    cpl_ensure(self  != NULL, CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(sigma >  0.0,  CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Compute the threshold */
    median = cpl_image_get_median_dev(self, &med_dist);
    threshold = median + sigma * med_dist;

    /* Binarise the image */
    selection = cpl_mask_threshold_image_create(self, threshold, DBL_MAX);
    if (selection != NULL) {
        /* Use a morphological opening to remove the single pixel detections */
        cpl_mask * kernel = cpl_mask_new(3, 3);

        /* These two should not be able to fail */
        const int  error  = cpl_mask_not(kernel) ||
            cpl_mask_filter(selection, selection, kernel, CPL_FILTER_OPENING,
                            CPL_BORDER_ZERO);
        cpl_mask_delete(kernel);

        if (!error) {
            /* Create the detected apertures list */
            aperts = cpl_apertures_extract_mask(self, selection);
        }

        cpl_mask_delete(selection);
    }

    if (aperts == NULL) {
        (void)cpl_error_set_where_();
    }

    return aperts;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Compute FWHM values in x and y for a list of apertures
  @param    self    The image to process
  @param    aperts  The list of apertures
  @return   A newly allocated object containing the fwhms in x and y or NULL
  @see cpl_image_get_fwhm()
  @deprecated Replace this call with a loop over cpl_image_get_fwhm()

 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_apertures_get_fwhm(const cpl_image     * self,
                                      const cpl_apertures * aperts)
{
    cpl_errorstate prevstate = cpl_errorstate_get();
    cpl_bivector * fwhms;
    double       * fwhms_x;
    double       * fwhms_y;
    cpl_size       naperts;
    cpl_size       i, nok = 0;


    cpl_ensure(self     != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(aperts != NULL, CPL_ERROR_NULL_INPUT, NULL);

    naperts = cpl_apertures_get_size(aperts);
    cpl_ensure(naperts >= 1, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Allocate storage */
    fwhms = cpl_bivector_new(naperts);
    fwhms_x = cpl_bivector_get_x_data(fwhms);
    fwhms_y = cpl_bivector_get_y_data(fwhms);


    /* Compute FWHM on all apertures */
    for (i=0; i < naperts; i++) {
        const cpl_error_code error
            = cpl_image_get_fwhm(self,
                                 (cpl_size)cpl_apertures_get_pos_x(aperts, i+1),
                                 (cpl_size)cpl_apertures_get_pos_y(aperts, i+1),
                                 &(fwhms_x[i]),
                                 &(fwhms_y[i]));

        if (!error) {
            nok++;
        } else if (error == CPL_ERROR_DATA_NOT_FOUND) {
            /* This error can be ignored */
            cpl_errorstate_set(prevstate);
        } else {
            /* This unexpected error cannot be ignored */
            cpl_bivector_delete(fwhms);
            (void)cpl_error_set_where_();
            return NULL;
        }
    }

    if (!nok) {
        /* Require at least one fwhm to be OK */
        cpl_bivector_delete(fwhms);
        (void)cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        fwhms = NULL;
    }

    return fwhms;
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Constructor for cpl_apertures
  @param    naperts     Number of apertures in the structure
  @return   1 newly allocated cpl_apertures or NULL on error

  The returned object must be deleted using cpl_apertures_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if naperts is not strictly positive
 */
/*----------------------------------------------------------------------------*/
static cpl_apertures * cpl_apertures_new(cpl_size naperts)
{
    cpl_apertures * aperts;

    /* Check entries */
    cpl_ensure(naperts > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Create a cpl_apertures */
    aperts = (cpl_apertures *)cpl_malloc(sizeof(cpl_apertures));
    aperts->naperts = naperts;

    /* Allocate data holders */
    aperts->x           = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->y           = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->maxpos_x    = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->maxpos_y    = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->minpos_x    = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->minpos_y    = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->xcentroid   = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->ycentroid   = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->npix        = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->left_x      = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->left_y      = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->right_x     = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->right_y     = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->top_x       = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->top_y       = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->bottom_x    = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->bottom_y    = cpl_calloc((size_t)naperts, sizeof(cpl_size));
    aperts->max_val     = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->min_val     = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->mean        = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->varsum      = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->median      = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->stdev       = cpl_calloc((size_t)naperts, sizeof(double));
    aperts->flux        = cpl_calloc((size_t)naperts, sizeof(double));

    return aperts;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Compute statistics on selected apertures
  @param  self      Reference image
  @param  lab       Labelized image (of type CPL_TYPE_INT)
  @param  maperts   Number of apertures, or zero for unknown
  @param  bpm_check Iff true, any bpms must be deselected from the labels
  @return  An CPL apertures object or @em NULL on error
  @note The returned object must be deleted using cpl_apertures_delete().
  @see cpl_apertures_new_from_image()

 */
/*----------------------------------------------------------------------------*/
cpl_apertures * cpl_apertures_new_from_image_(const cpl_image * self,
                                              const cpl_image * lab,
                                              cpl_size          maperts,
                                              cpl_boolean       bpm_check)
{
    cpl_apertures   *   aperts;
    const cpl_image *   dimage;
    cpl_image       *   cimage = NULL;
    cpl_image       *   labcopy = NULL;
    const cpl_image *   labuse;
    cpl_size            naperts;
    const double    *   pd;
    const int       *   plab;
    double          *   medians;
    cpl_size        *   medianc;
    const cpl_size      nx = cpl_image_get_size_x(self);
    const cpl_size      ny = cpl_image_get_size_y(self);
    const double        ep = 1e-10;
    cpl_size            i, j, k;

    /* Test entries */
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(lab  != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_image_get_type(lab) == CPL_TYPE_INT,
               CPL_ERROR_TYPE_MISMATCH, NULL);
    cpl_ensure(nx == cpl_image_get_size_x(lab),
               CPL_ERROR_INCOMPATIBLE_INPUT, NULL);
    cpl_ensure(ny == cpl_image_get_size_y(lab),
               CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    if (bpm_check) {
        const cpl_mask * bpm = cpl_image_get_bpm_const(self);

        if (bpm != NULL && !cpl_mask_is_empty(bpm)) {
            /* Set the bad pixels of the input image to 0 in the lab image */
            /* FIXME: Is this even necessary ? */

            labcopy = cpl_image_duplicate(lab);
            cpl_image_reject_from_mask(labcopy, bpm);
            cpl_image_fill_rejected(labcopy, 0.0);
        }
    }
    labuse = labcopy ? labcopy : lab;

    plab = cpl_image_get_data_int_const(labuse);

    /* Get number of apertures from the labelised image */
    naperts = maperts != 0 ? maperts : (cpl_size)cpl_image_get_max(labuse);
    if (naperts <= 0) {
        cpl_image_delete(labcopy);
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    /* Convert input image in a double image */
    dimage = cpl_image_get_type(self) == CPL_TYPE_DOUBLE ? self
        : (cimage = cpl_image_cast(self, CPL_TYPE_DOUBLE));

    if (dimage == NULL) {
        cpl_image_delete(labcopy);
        cpl_image_delete(cimage);
        (void)cpl_error_set_where_();
        return NULL;
    }

    pd = cpl_image_get_data_double_const(dimage);

    /* Create a cpl_apertures */
    aperts = cpl_apertures_new(naperts);

    /* Initialise min and max */
    for (k = 0; k < naperts; k++) {
        aperts->max_val[k] = -DBL_MAX;
        aperts->min_val[k] = DBL_MAX;
    }

    /* Compute some stats */
    for (j = 0; j < ny; j++) {
        for (i = 0; i < nx; i++) {
            double pix, delta;

            k = plab[i+j*nx]-1;
            /* Background: do nothing */
            if (k==-1) continue;
            if (k < 0) {
                cpl_apertures_delete(aperts);
                cpl_image_delete(cimage);
                cpl_image_delete(labcopy);
                (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                return NULL;
            }

            assert( k < naperts );

            /* Current pixel value */
            pix = pd[i+j*nx];

            /* See cpl_tools_get_variance_double() */
            delta = pix - aperts->mean[k];

            aperts->varsum[k] += (double)aperts->npix[k] * delta * delta
                / (double)(aperts->npix[k] + 1);
            aperts->mean[k]   += delta / (double)(aperts->npix[k] + 1);

            /* Accumulate positions */
            aperts->x[k] += (double)(i+1);
            aperts->y[k] += (double)(j+1);
            /* Increase number of pixels */
            aperts->npix[k] ++;
            /* Store pixel sum and squared sum */
            aperts->flux[k] += pix;
            /* Check max pos value */
            if (pix > aperts->max_val[k]) {
                aperts->max_val[k] = pix;
                aperts->maxpos_x[k] = i+1;
                aperts->maxpos_y[k] = j+1;
            }
            /* Check min pos value */
            if (pix < aperts->min_val[k]) {
                aperts->min_val[k] = pix;
                aperts->minpos_x[k] = i+1;
                aperts->minpos_y[k] = j+1;
            }

            /* Check object extremities */
            if ((i+1 < aperts->left_x[k]) || (aperts->left_x[k] == 0)) {
                aperts->left_x[k] = i+1;
                aperts->left_y[k] = j+1;
            }
            if (i+1 > aperts->right_x[k]) {
                aperts->right_x[k] = i+1;
                aperts->right_y[k] = j+1;
            }
            if ((j+1 < aperts->bottom_y[k]) || (aperts->bottom_y[k] == 0)) {
                aperts->bottom_x[k] = i+1;
                aperts->bottom_y[k] = j+1;
            }
            if (j+1 > aperts->top_y[k]) {
                aperts->top_x[k] = i+1;
                aperts->top_y[k] = j+1;
            }
        }
    }

    /* Centroiding */
    for (j = 0; j < ny; j++) {
        for (i = 0; i < nx; i++) {
            k = plab[i+j*nx]-1;

            if (k >= 0) { /* Ignore background */
                /* Current pixel value */
                const double pix = pd[i+j*nx];

                if (aperts->min_val[k] <= 0) {
                    /* Accumulate weighted positions */
                    aperts->xcentroid[k] +=
                        (double)(i+1)*(pix-aperts->min_val[k]+ep);
                    aperts->ycentroid[k] += 
                        (double)(j+1)*(pix-aperts->min_val[k]+ep);
                } else {
                    /* Accumulate weighted positions */
                    aperts->xcentroid[k] += (double)(i+1) * pix;
                    aperts->ycentroid[k] += (double)(j+1) * pix;
                }
            }
        }
    }

    /* Allocate buffers for the aperture medians, this will typically
       require much less memory than the pixel buffer and in the worst
       case on the same order */
    medianc = cpl_malloc((size_t)naperts * sizeof(cpl_size));

    /* Compute average and std dev for each aperture, normalize centers */
    for (k = 0; k < aperts->naperts; k++) {
        double weight;
        const cpl_size npix = aperts->npix[k];
        if (npix <= 0) {
            cpl_apertures_delete(aperts);
            cpl_image_delete(cimage);
            cpl_image_delete(labcopy);
            cpl_free(medianc);
            (void)cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "Label image"
                                         " of size %d X %d with %d label(s) "
                                         "has no pixel labeled %d", (int)nx,
                                         (int)ny, (int)naperts, (int)k);
            return NULL;
        }

        /* Determine end index for each median */
        medianc[k] = k > 0 ? npix + medianc[k - 1] : npix;

        aperts->stdev[k] = npix > 1 ?
            sqrt(aperts->varsum[k]/((double)npix-1.0)) : -1.0;

        aperts->x[k] /= (double)npix;
        aperts->y[k] /= (double)npix;
        weight = aperts->flux[k];
        if (aperts->min_val[k] <= 0) {
            weight -= (double)npix * aperts->min_val[k];
            weight += (double)npix * ep;
        }

        if (aperts->xcentroid[k] >= weight * (double)aperts->left_x[k] &&
            aperts->xcentroid[k] <= weight * (double)aperts->right_x[k] &&
            weight != 0.0) {
            aperts->xcentroid[k] /= weight;
        } else {
            aperts->xcentroid[k] = 0.0;
        }

        if (aperts->ycentroid[k] >= weight * (double)aperts->bottom_y[k] &&
            aperts->ycentroid[k] <= weight * (double)aperts->top_y[k] &&
            weight != 0.0) {
            aperts->ycentroid[k] /= weight;
        } else {
            aperts->ycentroid[k] = 0.0;
        }
    }

    /* Allocate space for all pixels belonging to an aperture */
    medians = cpl_malloc((size_t)medianc[aperts->naperts-1] * sizeof(*medians));

    /* Repack all pixels belonging to an aperture
       into an array specific to that aperture */
    for (i = 0; i < nx * ny; i++) {
        k = plab[i]-1;

        if (k >= 0) { /* Ignore background */
            /* Insert value into its array, while adjusting the index for the
               next access */
            medians[--medianc[k]] = pd[i];
        }
    }

    /* Compute median for each aperture */
    assert(medianc[0] == 0);
    aperts->median[0] = cpl_tools_get_median_double(medians, aperts->npix[0]);
    for (k = 1; k < aperts->naperts; k++) {

        assert(medianc[k] == medianc[k - 1] + aperts->npix[k - 1]);

        aperts->median[k] =
            cpl_tools_get_median_double(medians + medianc[k], aperts->npix[k]);
    }

    cpl_free(medians);
    cpl_free(medianc);
    cpl_image_delete(cimage);
    cpl_image_delete(labcopy);

    return aperts;
}
