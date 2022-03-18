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

#include "cpl_image_io_impl.h"

#include "cpl_error_impl.h"
#include "cpl_memory.h"
#include "cpl_propertylist_impl.h"
#include "cpl_mask_defs.h"
#include "cpl_io_fits.h"
#include "cpl_cfitsio.h"
#include "cpl_image_bpm_impl.h"
#include "cpl_image_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include <fitsio.h>


/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define CPL_IMAGE_IO_GET_PIXELS    1
#define CPL_IMAGE_IO_SET_BADPIXEL  2
#define CPL_IMAGE_IO_GET           3
#define CPL_IMAGE_IO_SET           4

#define CPL_IMAGE_IO_RE_IM         5
#define CPL_IMAGE_IO_RE            6
#define CPL_IMAGE_IO_IM            7

#define CPL_IMAGE_IO_ABS_ARG       8
#define CPL_IMAGE_IO_ABS           9
#define CPL_IMAGE_IO_ARG          10

/* Needed for cpl_image_floodfill() */
#define FFSTACK_MAXLINES 10
#define FFSTACK_STACKSZ(IMAGE) (FFSTACK_MAXLINES * IMAGE->ny)
#define FFSTACK_BYTES(IMAGE) \
    ((size_t)FFSTACK_STACKSZ(IMAGE) * 4 * sizeof(cpl_size))

#define CPL_MSG "Image with %" CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT   \
    " pixel(s) of type '%s' and %" CPL_SIZE_FORMAT " bad pixel(s)\n"

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static
void cpl_image_floodfill(cpl_image *, void *, cpl_size, cpl_size, cpl_size);

static
cpl_image * cpl_image_load_one(const char *, cpl_type, cpl_size, cpl_size,
                               cpl_boolean, cpl_size, cpl_size, cpl_size,
                               cpl_size) CPL_ATTR_ALLOC;

#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Allocate an image structure and pixel buffer for a image.
  @param    nx          Size in x (the number of columns)
  @param    ny          Size in y (the number of rows)
  @param    type        The pixel type
  @return   1 newly allocated cpl_image or NULL in case of an error

  Allocates space for the cpl_image structure and sets the dimensions and
  type of pixel data. The pixel buffer is allocated and initialised to zero.
  The pixel array will contain nx*ny values being the image pixels from the
  lower left to the upper right line by line. 

  Supported pixel types are CPL_TYPE_INT, CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE,
  CPL_TYPE_FLOAT_COMPLEX and CPL_TYPE_DOUBLE_COMPLEX.

  The returned cpl_image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if nx or ny is non-positive
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported

 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_new(cpl_size nx, cpl_size ny, cpl_type type)
{

    cpl_image * self = cpl_image_wrap_(nx, ny, type, NULL);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Create an image using an existing pixel buffer of the specified type
  @param    nx      Size in x (the number of columns)
  @param    ny      Size in y (the number of rows)
  @param    type    Pixel type
  @param    pixels  Pixel data of the specified type
  @return   1 newly allocated cpl_image or NULL on error
  @see      cpl_image_new()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if nx or ny is non-positive or their product is
                            too big
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_wrap(cpl_size nx, cpl_size ny, cpl_type type,
                           void * pixels)
{
    cpl_image  * self;

    cpl_ensure(pixels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_wrap_(nx, ny, type, pixels);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a double image using an existing pixel buffer.
  @param    nx          Size in x (the number of columns)
  @param    ny          Size in y (the number of rows)
  @param    pixels      double * pixel data
  @return   1 newly allocated cpl_image or NULL in case of an error
  @see      cpl_image_new

  The pixel array is set to point to that of the argument.
  The pixel array must contain nx*ny doubles. 

  The allocated image must be deallocated with cpl_image_unwrap().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if nx or ny is non-positive or zero.
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_wrap_double(cpl_size nx, 
                                  cpl_size ny,
                                  double * pixels)
{
    cpl_image * self;

    cpl_ensure(pixels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_wrap_(nx, ny, CPL_TYPE_DOUBLE, pixels);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a float image using an existing pixel buffer.
  @param    nx          Size in x (the number of columns)
  @param    ny          Size in y (the number of rows)
  @param    pixels      float * pixel data.
  @return   1 newly allocated cpl_image or NULL on error
  @see      cpl_image_wrap_double()
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_wrap_float(cpl_size nx, 
                                 cpl_size ny,
                                 float  * pixels)
{
    cpl_image * self;

    cpl_ensure(pixels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_wrap_(nx, ny, CPL_TYPE_FLOAT, pixels);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a double complex image using an existing pixel buffer.
  @param    nx          Size in x (the number of columns)
  @param    ny          Size in y (the number of rows)
  @param    pixels      double complex * pixel data.
  @return   1 newly allocated cpl_image or NULL on error
  @see      cpl_image_wrap_double()
  @note This function is available iff the application includes complex.h
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_wrap_double_complex(cpl_size         nx, 
                                          cpl_size         ny,
                                          double complex * pixels)
{
    cpl_image * self;

    cpl_ensure(pixels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_wrap_(nx, ny, CPL_TYPE_DOUBLE_COMPLEX, pixels);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a float complex image using an existing pixel buffer.
  @param    nx          Size in x (the number of columns)
  @param    ny          Size in y (the number of rows)
  @param    pixels      float complex * pixel data.
  @return   1 newly allocated cpl_image or NULL on error
  @see      cpl_image_wrap_double_complex()
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_wrap_float_complex(cpl_size        nx,
                                         cpl_size        ny,
                                         float complex * pixels)
{
    cpl_image * self;

    cpl_ensure(pixels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_wrap_(nx, ny, CPL_TYPE_FLOAT_COMPLEX, pixels);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create an integer image using an existing pixel buffer.
  @param    nx          Size in x (the number of columns)
  @param    ny          Size in y (the number of rows)
  @param    pixels      int * pixel data.
  @return   1 newly allocated cpl_image or NULL on error 
  @see      cpl_image_wrap_double()
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_wrap_int(
        cpl_size           nx, 
        cpl_size           ny,
        int       *   pixels)
{
    cpl_image * self;

    cpl_ensure(pixels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_wrap_(nx, ny, CPL_TYPE_INT, pixels);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Load an image from a FITS file.
  @param    filename    Name of the file to load from.
  @param    im_type     Type of the created image       
  @param    pnum        Plane number in the Data Unit (0 for first)
  @param    xtnum       Extension number in the file (0 for primary HDU)
  @return   1 newly allocated image or NULL on error

  This function loads an image from a FITS file (NAXIS=2 or 3).

  The returned image has to be deallocated with cpl_image_delete().

  The passed type for the output image can be : CPL_TYPE_FLOAT, 
  CPL_TYPE_DOUBLE or CPL_TYPE_INT.

  This type is there to specify the type of the cpl_image that will be created
  by the function. It is totally independant from the way the data are stored
  in the FITS file. A FITS image containg float pixels can be loaded as a
  cpl_image of type double. In this case, the user would specify 
  CPL_TYPE_DOUBLE as im_type.

  Additionally, CPL_TYPE_UNSPECIFIED can be passed to inherit the FITS file
  type. The type of the image is defined based on the BITPIX information of the
  FITS file. After a successful call, the type of the created image can be
  accessed via cpl_image_get_type().
  
  'xtnum' specifies from which extension the image should be loaded.
  This could be 0 for the main data section (files without extension),
  or any number between 1 and N, where N is the number of extensions
  present in the file.

  The requested plane number runs from 0 to nplanes-1, where nplanes is
  the number of planes present in the requested data section.

  The created image has an empty bad pixel map.
  
  Examples:
  @code
  // Load as a float image the only image in FITS file (a.fits) without ext. 
  // and NAXIS=2.
  cpl_image * im = cpl_image_load("a.fits", CPL_TYPE_FLOAT, 0, 0);
  // Load as a double image the first plane in a FITS cube (a.fits) without 
  // extension, NAXIS=3 and NAXIS3=128
  cpl_image * im = cpl_image_load("a.fits", CPL_TYPE_DOUBLE, 0, 0);
  // Load as an integer image the third plane in a FITS cube (a.fits) without 
  // extension, NAXIS=3 and NAXIS3=128
  cpl_image * im = cpl_image_load("a.fits", CPL_TYPE_INT, 2, 0);
  // Load as a double image the first plane from extension 5
  cpl_image * im = cpl_image_load("a.fits", CPL_TYPE_DOUBLE, 0, 5);
  // Load as a double image the third plane in extension 5
  cpl_image * im = cpl_image_load("a.fits", CPL_TYPE_DOUBLE, 2, 5);
  @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO if the file cannot be opened or does not exist
  - CPL_ERROR_BAD_FILE_FORMAT if the data cannot be loaded from the file 
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported
  - CPL_ERROR_ILLEGAL_INPUT if the passed extension number is negative
  - CPL_ERROR_DATA_NOT_FOUND if the specified extension has no image data
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_load(const char * filename,
                           cpl_type     im_type,
                           cpl_size     pnum,
                           cpl_size     xtnum)
{
    cpl_image * self = cpl_image_load_one(filename, im_type, pnum, xtnum,
                                          CPL_FALSE, 0, 0, 0, 0);

    if (self == NULL) cpl_error_set_where_();

    return self;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Load an image from a FITS file.
  @param  filename    Name of the file to load from.
  @param  im_type     Type of the created image       
  @param  pnum        Plane number in the Data Unit (0 for first)
  @param  xtnum       Extension number in the file (0 for primary HDU)
  @param  do_window   True for (and only for) a windowed load
  @param  llx         Lower left  x position (FITS convention, 1 for leftmost)
  @param  lly         Lower left  y position (FITS convention, 1 for lowest)
  @param  urx         Upper right x position (FITS convention)
  @param  ury         Upper right y position (FITS convention)
  @return 1 newly allocated image or NULL on error
  @see cpl_image_load()
*/
/*----------------------------------------------------------------------------*/
static cpl_image * cpl_image_load_one(const char * filename,
                                      cpl_type     im_type,
                                      cpl_size     pnum,
                                      cpl_size     xtnum,
                                      cpl_boolean  do_window,
                                      cpl_size     llx,
                                      cpl_size     lly,
                                      cpl_size     urx,
                                      cpl_size     ury)
{

    int             error = 0;
    int             naxis = 0;
    /* May read from Data Unit w. NAXIS=[23] */
    CPL_FITSIO_TYPE naxes[3] ={0, 0, 0};
    cpl_type        pix_type = im_type;
    fitsfile      * fptr;
    cpl_image     * self;

    /* FIXME: Version 3.24 of fits_open_diskfile() seg-faults on NULL.
       If ever fixed in CFITSIO, this check should be removed */
    cpl_ensure(filename != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(xtnum   >= 0,     CPL_ERROR_ILLEGAL_INPUT, NULL);

    if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &error)) {
        /* If the file did not exist:    error = 104 (FILE_NOT_OPENED) */
        /* If the file had permission 0: error = 104 (FILE_NOT_OPENED) */
        /* If the file was empty:        error = 107 (END_OF_FILE) */
        /* If the file was a directory:  error = 108 (READ_ERROR) */
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_open_diskfile,
                                 "filename='%s', im_type=%u, pnum=%"
                                 CPL_SIZE_FORMAT ", xtnum=%" CPL_SIZE_FORMAT,
                                 filename, im_type, pnum, xtnum);
        return NULL;
    }

    self = cpl_image_load_(fptr, &naxis, naxes, &pix_type, filename, pnum,
                           xtnum, do_window, llx, lly, urx, ury);

    if (cpl_io_fits_close_file(fptr, &error)) {
        cpl_image_delete(self);
        self = NULL;
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_close_file, "filename='%s', "
                                 "im_type=%u, pnum=%" CPL_SIZE_FORMAT
                                 ", xtnum=%" CPL_SIZE_FORMAT,
                                 filename, im_type, pnum, xtnum);
    } else if (self == NULL) {
        cpl_error_set_where_();
    }

    return self;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Load an image from a FITS file.
  @param  filename    Name of the file to load from.
  @param  im_type     Type of the created image       
  @param  pnum        Plane number in the Data Unit (0 for first)
  @param  xtnum       Extension number in the file.
  @param  llx         Lower left  x position (FITS convention, 1 for leftmost)
  @param  lly         Lower left  y position (FITS convention, 1 for lowest)
  @param  urx         Upper right x position (FITS convention)
  @param  ury         Upper right y position (FITS convention)
  @return 1 newly allocated image or NULL on error
  @see    cpl_image_load()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO if the file does not exist
  - CPL_ERROR_BAD_FILE_FORMAT if the data cannot be loaded from the file 
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported
  - CPL_ERROR_ILLEGAL_INPUT if the passed position is invalid
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_load_window(const char * filename,
                                  cpl_type     im_type,
                                  cpl_size     pnum,
                                  cpl_size     xtnum,
                                  cpl_size     llx,
                                  cpl_size     lly,
                                  cpl_size     urx,
                                  cpl_size     ury)
{

    cpl_image * self = cpl_image_load_one(filename, im_type, pnum, xtnum,
                                          CPL_TRUE, llx, lly, urx, ury);

    if (self == NULL) cpl_error_set_where_();

    return self;

}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create an int image from a mask
  @param    mask    the original mask
  @return   1 newly allocated cpl_image or NULL on error 

  The created image is of type CPL_TYPE_INT.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_new_from_mask(const cpl_mask * mask)
{
    const size_t       nx   = cpl_mask_get_size_x(mask);
    const size_t       ny   = cpl_mask_get_size_y(mask);
    const cpl_binary * data = cpl_mask_get_data_const(mask);
    cpl_image        * out;
    int              * pout;
    size_t             i;


    cpl_ensure(mask, CPL_ERROR_NULL_INPUT, NULL);

    /* Create the INT image */
    pout = (int*)cpl_malloc(nx * ny * sizeof(*pout));
    out = cpl_image_wrap_int(nx, ny, pout);

    /* Fill the image */
    for (i= 0; i < nx * ny; i++) {
        pout[i] = (data[i] == CPL_BINARY_0) ? 0 : 1;
    }

    return out;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Labelise a mask to differentiate different objects
  @param    in      mask to labelise
  @param    nbobjs  If non-NULL, number of objects found on success
  @return   A newly allocated label image or NULL on error
  
  This function labelises all blobs in a mask. All 4-neighbour connected zones 
  set to 1 in the input mask will end up in the returned integer image as zones
  where all pixels are set to the same (unique for this blob in this image) 
  label.
  A non-recursive flood-fill is applied to label the zones. The flood-fill is 
  dimensioned by the number of lines in the image, and the maximal number of
  lines possibly covered by a blob.
  The returned image must be deallocated with cpl_image_delete()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input mask is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_labelise_mask_create(const cpl_mask * in,
                                           cpl_size       * nbobjs)
{
    cpl_image        * intimage;
    int              * pio;
    cpl_size           label = 1; /* The first label value */
    const size_t       nx  = cpl_mask_get_size_x(in);
    const size_t       ny  = cpl_mask_get_size_y(in);
    const cpl_binary * pin = cpl_mask_get_data_const(in);
    size_t             i, j;
    cpl_boolean        has_data = CPL_TRUE;

    /* Duplicated body from cpl_image_new_from_mask() :-((((((((((((((((*/

    /* Test entries */
    cpl_ensure(in != NULL, CPL_ERROR_NULL_INPUT, NULL);

    /* Create output integer image and initialise it with the input map */
    intimage = cpl_image_new(nx, ny, CPL_TYPE_INT);
    pio = cpl_image_get_data_int(intimage);

    for (i = 0; i < nx * ny; i++) {
        if (pin[i] != CPL_BINARY_0) {
            pio[i] = -1;
            has_data = CPL_TRUE;
        }
    }


    if (has_data) {
        /* Now work on intimage */

        /* Allocate temporary work-space for cpl_image_floodfill() */
        void * fftemp = cpl_malloc(FFSTACK_BYTES(intimage));

        for (j = 0; j < ny; j++, pio += nx) {
            for (i = 0; i < nx; i++) {
                /* Look up if unprocessed pixel */
                if (pio[i] == -1) {
                    /* Flood fill from this pixel with the assigned label */
                    cpl_image_floodfill(intimage, fftemp, i, j, label);
                    label++;
                }
            }
        }
        cpl_free(fftemp);
    }

    if (nbobjs!=NULL) *nbobjs = label-1;

    return intimage;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the image type
  @param    img     a cpl_image object 
  @return   The image type or CPL_TYPE_INVALID on NULL input.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_type cpl_image_get_type(const cpl_image * img)
{
    cpl_ensure(img, CPL_ERROR_NULL_INPUT, CPL_TYPE_INVALID);
    return img->type;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the image x size
  @param    img     a cpl_image object 
  @return   The image x size, or -1 on NULL input

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_image_get_size_x(const cpl_image * img)
{
    cpl_ensure(img, CPL_ERROR_NULL_INPUT, -1);
    return img->nx;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the image y size
  @param    img     a cpl_image object 
  @return   The image y size, or -1 on NULL input

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_image_get_size_y(const cpl_image * img)
{
    cpl_ensure(img, CPL_ERROR_NULL_INPUT, -1);
    return img->ny;
}

#define CPL_OPERATION CPL_IMAGE_IO_GET
#define CPL_TYPE_RETURN double
/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Get the value of a pixel at a given position
  @param    image          Input image.
  @param    xpos           Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos           Pixel y position (FITS convention, 1 for lowest)
  @return   The pixel value (cast to a double)
  @see cpl_image_get()
  @note No input validation in this function

 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_(const cpl_image * image,
                      cpl_size          xpos,
                      cpl_size          ypos) 
{
    const cpl_size pos = (xpos - 1) + image->nx * (ypos - 1);
    double value;

    assert( image->pixels );

    /* Get the value */
    switch (image->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

        default:
            assert(0);
    }

    return value;
}
#undef CPL_TYPE_RETURN
#define CPL_TYPE_RETURN double complex

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the value of a pixel at a given position
  @param    image          Input image.
  @param    xpos           Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos           Pixel y position (FITS convention, 1 for lowest)
  @param    pis_rejected   1 if the pixel is bad, 0 if good, negative on error
  @return   The pixel value (cast to a double) or undefined if *pis_rejected
  @see cpl_image_get_complex()

  The return value is defined if the pixel is not flagged as rejected, i. e.
  when *pis_rejected == 0.

  In case of an error, the #_cpl_error_code_ code is set.

  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the passed position is not in the image
  - CPL_ERROR_INVALID_TYPE if the image pixel type is not supported
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get(const cpl_image * image,
                     cpl_size          xpos,
                     cpl_size          ypos,
                     int             * pis_rejected) 
{
    cpl_size pos;

    /* Check entries */
    cpl_ensure(pis_rejected != NULL, CPL_ERROR_NULL_INPUT, -1.0);

    *pis_rejected = -1;
    cpl_ensure(image        != NULL, CPL_ERROR_NULL_INPUT, -2.0);

    /* Check the validity of image, xpos and ypos */
    cpl_ensure(xpos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);
    cpl_ensure(ypos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);
    cpl_ensure(xpos <= image->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);
    cpl_ensure(ypos <= image->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);
    cpl_ensure(!(image->type & CPL_TYPE_COMPLEX), CPL_ERROR_INVALID_TYPE, -3.0);

    pos = (xpos - 1) + image->nx * (ypos - 1);

    *pis_rejected = image->bpm != NULL && image->bpm->data[pos] != CPL_BINARY_0
        ? 1 : 0;

    return *pis_rejected ? 0.0 : cpl_image_get_(image, xpos, ypos);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the value of a complex pixel at a given position
  @param    image          Input complex image.
  @param    xpos           Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos           Pixel y position (FITS convention, 1 for lowest)
  @param    pis_rejected   1 if the pixel is bad, 0 if good, negative on error
  @return   The pixel value (cast to a double complex) or
     undefined if *pis_rejected
  @see cpl_image_get()
  @note This function is available iff the application includes complex.h

  The return value is defined if the pixel is not flagged as rejected, i. e.
  when *pis_rejected == 0.

  In case of an error, the #_cpl_error_code_ code is set.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the passed position is not in the image
 */ 
/*----------------------------------------------------------------------------*/
double complex cpl_image_get_complex(const cpl_image * image,
                                     cpl_size          xpos,
                                     cpl_size          ypos,
                                     int             * pis_rejected) 
{
    double complex value;
    cpl_size       pos;

    /* Check entries */
    cpl_ensure(pis_rejected != NULL, CPL_ERROR_NULL_INPUT, -1.0);

    *pis_rejected = -1;
    cpl_ensure(image        != NULL, CPL_ERROR_NULL_INPUT, -1.0);

    cpl_ensure(xpos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);
    cpl_ensure(ypos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);
    cpl_ensure(xpos <= image->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);
    cpl_ensure(ypos <= image->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE, -2.0);

    pos = (xpos - 1) + image->nx * (ypos - 1);

    /* Check the validity of image, xpos and ypos */
    *pis_rejected = image->bpm != NULL && image->bpm->data[pos] != CPL_BINARY_0
        ? 1 : 0;

    /* The pixel is flagged as rejected */
    if (*pis_rejected) return 0.0;

    assert( image->pixels );

    /* Get the value */
    switch (image->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS

    default:
        (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        value = -3.0;
    }

    return value;
}
#undef CPL_OPERATION
#undef CPL_TYPE_RETURN


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Split a complex image into its real and/or imaginary part(s)
  @param  im_real Pre-allocated image to hold the real part, or @c NULL
  @param  im_imag Pre-allocated image to hold the imaginary part, or @c NULL
  @param  self    Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   At least one output image must be non-NULL. The images must match
          in size and precision

  Any bad pixels are also processed.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT         If the input image or both output images are
                                 @c NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT If the images have different sizes
  - CPL_ERROR_INVALID_TYPE       If the input image is not of complex type
  - CPL_ERROR_TYPE_MISMATCH      If the images have different precision
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_re_im(cpl_image * im_real, cpl_image * im_imag,
                                    const cpl_image * self)
{

    cpl_error_code error = CPL_ERROR_NONE; /* Set locally in this function */
    cpl_error_code code  = CPL_ERROR_NONE; /* Set in call, propagate */

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(self->type & CPL_TYPE_COMPLEX, CPL_ERROR_INVALID_TYPE);

    if (im_real != NULL && im_imag != NULL) {
        cpl_ensure_code(self->nx == im_real->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_real->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->nx == im_imag->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_imag->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

        switch (self->type) {
        case CPL_TYPE_DOUBLE_COMPLEX:
            code = cpl_image_fill_re_im_double(im_real, im_imag, self);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            code = cpl_image_fill_re_im_float(im_real, im_imag, self);
            break;
        default:
            error = cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    } else if (im_real != NULL) {
        cpl_ensure_code(self->nx == im_real->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_real->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

        switch (self->type) {
        case CPL_TYPE_DOUBLE_COMPLEX:
            code = cpl_image_fill_re_double(im_real, self);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            code = cpl_image_fill_re_float(im_real, self);
            break;
        default:
            error = cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    } else if (im_imag != NULL) {
        cpl_ensure_code(self->nx == im_imag->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_imag->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

        switch (self->type) {
        case CPL_TYPE_DOUBLE_COMPLEX:
            code = cpl_image_fill_im_double(im_imag, self);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            code = cpl_image_fill_im_float(im_imag, self);
            break;
        default:
            error = cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    } else {
        error = cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    return code ? cpl_error_set_where_() : error;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Split a complex image into its absolute and argument part(s)
  @param  im_abs Pre-allocated image to hold the absolute part, or @c NULL
  @param  im_arg Pre-allocated image to hold the argument part, or @c NULL
  @param  self    Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   At least one output image must be non-NULL. The images must match
          in size and precision

  Any bad pixels are also processed.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT         If the input image or both output images are
                                 @c NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT If the images have different sizes
  - CPL_ERROR_INVALID_TYPE       If the input image is not of complex type
  - CPL_ERROR_TYPE_MISMATCH      If the images have different precision
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_abs_arg(cpl_image * im_abs, cpl_image * im_arg,
                                      const cpl_image * self)
{

    cpl_error_code error;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(self->type & CPL_TYPE_COMPLEX, CPL_ERROR_INVALID_TYPE);

    if (im_abs != NULL && im_arg != NULL) {
        cpl_ensure_code(self->nx == im_abs->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_abs->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->nx == im_arg->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_arg->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

        switch (self->type) {
        case CPL_TYPE_DOUBLE_COMPLEX:
            error = cpl_image_fill_abs_arg_double(im_abs, im_arg, self);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            error = cpl_image_fill_abs_arg_float(im_abs, im_arg, self);
            break;
        default:
            error = CPL_ERROR_INVALID_TYPE;
        }
    } else if (im_abs != NULL) {
        cpl_ensure_code(self->nx == im_abs->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_abs->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

        switch (self->type) {
        case CPL_TYPE_DOUBLE_COMPLEX:
            error = cpl_image_fill_abs_double(im_abs, self);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            error = cpl_image_fill_abs_float(im_abs, self);
            break;
        default:
            error = CPL_ERROR_INVALID_TYPE;
        }
    } else if (im_arg != NULL) {
        cpl_ensure_code(self->nx == im_arg->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == im_arg->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

        switch (self->type) {
        case CPL_TYPE_DOUBLE_COMPLEX:
            error = cpl_image_fill_arg_double(im_arg, self);
            break;
        case CPL_TYPE_FLOAT_COMPLEX:
            error = cpl_image_fill_arg_float(im_arg, self);
            break;
        default:
            error = CPL_ERROR_INVALID_TYPE;
        }
    } else {
        error = CPL_ERROR_NULL_INPUT;
    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Get the real part of the pixels from a complex image
  @param  self  Pre-allocated non-complex image to hold the result
  @param  other The complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @deprecated Replace this call with cpl_image_fill_re_im()
  @see cpl_image_fill_re_im()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_real(cpl_image * self, const cpl_image * other)
{

    const cpl_error_code error = cpl_image_fill_re_im(self, NULL, other);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Get the imaginary part of the pixels from a complex image
  @param  self  Pre-allocated non-complex image to hold the result
  @param  other The complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @deprecated Replace this call with cpl_image_fill_re_im()
  @see cpl_image_fill_re_im()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_imag(cpl_image * self, const cpl_image * other)
{

    const cpl_error_code error = cpl_image_fill_re_im(NULL, self, other);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Get the absolute values of the pixels in a complex image
  @param  self  Pre-allocated non-complex image to hold the result
  @param  other The complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @deprecated Replace this call with cpl_image_fill_abs_arg()
  @see cpl_image_fill_abs_arg()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_mod(cpl_image * self, const cpl_image * other)
{

    const cpl_error_code error = cpl_image_fill_abs_arg(self, NULL, other);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Get the phase of the pixels in a complex image
  @param  self  Pre-allocated non-complex image to hold the result
  @param  other The complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @deprecated Replace this call with cpl_image_fill_abs_arg()
  @see cpl_image_fill_abs_arg()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_phase(cpl_image * self, const cpl_image * other)
{

    const cpl_error_code error = cpl_image_fill_abs_arg(NULL, self, other);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Complex conjugate the pixels in a complex image
  @param  self  Pre-allocated complex image to hold the result
  @param  other The complex image to conjugate, may equal self
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   The two images must match in size and precision

  Any bad pixels are also conjugated.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT         If an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT If the images have different sizes
  - CPL_ERROR_INVALID_TYPE       If an input image is not of complex type
  - CPL_ERROR_TYPE_MISMATCH      If the images have different precision
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_conjugate(cpl_image * self, const cpl_image * other)
{

    cpl_error_code error;

    cpl_ensure_code(self       != NULL,      CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other      != NULL,      CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(self->nx   == other->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(self->ny   == other->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(self->type == other->type, CPL_ERROR_TYPE_MISMATCH);

    switch (self->type) {
    case CPL_TYPE_DOUBLE_COMPLEX:
        error = cpl_image_conjugate_double(self, other);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        error = cpl_image_conjugate_float(self, other);
        break;
    default:
        error = CPL_ERROR_INVALID_TYPE;
    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Fill an image window with a constant
  @param  self  The image to fill
  @param  llx     Lower left x position (FITS convention, 1 for leftmost)
  @param  lly     Lower left y position (FITS convention, 1 for lowest)
  @param  urx     Specifies the window position
  @param  ury     Specifies the window position
  @param  value The value to fill with
  @return The #_cpl_error_code_ or CPL_ERROR_NONE
  @note   Any bad pixels are accepted

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT    if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the specified window is not valid
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_window(cpl_image * self,
                                     cpl_size     llx,
                                     cpl_size     lly,
                                     cpl_size     urx,
                                     cpl_size     ury,
                                     double value)
{
    cpl_ensure_code(self != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(llx >= 1,        CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(lly >= 1,        CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(llx <= urx,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(lly <= ury,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(urx <= self->nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(ury <= self->ny, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(!(self->type & CPL_TYPE_COMPLEX), CPL_ERROR_INVALID_TYPE);

    for (cpl_size j = lly; j <= ury; j++) {
        for (cpl_size i = llx; i <= urx; i++) {
            cpl_image_set_(self, i, j, value);
        }
    }

    if (self->bpm != NULL) {
        cpl_mask_fill_window(self->bpm, llx, lly, urx, ury, CPL_BINARY_0);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Fill an image of type integer with a constant
  @param  self  The image to fill
  @param  value The value to fill with
  @return The #_cpl_error_code_ or CPL_ERROR_NONE
  @note   Any bad pixels are accepted

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT    if an input pointer is NULL
  - CPL_ERROR_TYPE_MISMATCH if the image type is not int
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_int(cpl_image * self, int value)
{

    cpl_ensure_code(self       != NULL,         CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(self->type == CPL_TYPE_INT, CPL_ERROR_TYPE_MISMATCH);

    if (value == 0) {
        (void)memset(self->pixels, 0, (size_t)self->nx * (size_t)self->ny
                     * sizeof(int));
    } else {
        size_t i = (size_t)(self->nx * self->ny);

        do {
            ((int*)self->pixels)[--i] = value;
        } while (i);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Extract the real part image of a complex image
  @param    image       input image
  @return   a newly allocated real image or NULL on error
  @see cpl_image_fill_real()

  Image can be CPL_TYPE_FLOAT_COMPLEX or CPL_TYPE_DOUBLE_COMPLEX.

  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is non-complex
 */ 
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_extract_real(const cpl_image * image)
{
    cpl_image * self;

    cpl_ensure(image != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_new(image->nx, image->ny, image->type & ~CPL_TYPE_COMPLEX);

    if (cpl_image_fill_real(self, image)) {
        (void)cpl_error_set_where_();
        cpl_image_delete(self);
        self = NULL;
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Extract the imaginary part image of a complex image
  @param    image       input image
  @return   a newly allocated imaginary image or NULL on error
  @see cpl_image_fill_imag()

  Image can be CPL_TYPE_FLOAT_COMPLEX or CPL_TYPE_DOUBLE_COMPLEX.

  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is non-complex
 */ 
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_extract_imag(const cpl_image * image)
{
    cpl_image * self;

    cpl_ensure(image != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_new(image->nx, image->ny, image->type & ~CPL_TYPE_COMPLEX);

    if (cpl_image_fill_imag(self, image)) {
        (void)cpl_error_set_where_();
        cpl_image_delete(self);
        self = NULL;
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Extract the absolute part (modulus) of a complex image
  @param    image       input image
  @return   a newly allocated imaginary image or NULL on error
  @see cpl_image_fill_mod()

  Image can be CPL_TYPE_FLOAT_COMPLEX or CPL_TYPE_DOUBLE_COMPLEX.

  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is non-complex
 */ 
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_extract_mod(const cpl_image * image)
{
    cpl_image * self;

    cpl_ensure(image != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_new(image->nx, image->ny, image->type & ~CPL_TYPE_COMPLEX);

    if (cpl_image_fill_mod(self, image)) {
        (void)cpl_error_set_where_();
        cpl_image_delete(self);
        self = NULL;
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Extract the argument (phase) of a complex image
  @param    image       input image
  @return   a newly allocated imaginary image or NULL on error
  @see cpl_image_fill_phase()

  Image can be CPL_TYPE_FLOAT_COMPLEX or CPL_TYPE_DOUBLE_COMPLEX.

  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is non-complex
 */ 
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_extract_phase(const cpl_image * image)
{
    cpl_image * self;

    cpl_ensure(image != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_new(image->nx, image->ny, image->type & ~CPL_TYPE_COMPLEX);

    if (cpl_image_fill_phase(self, image)) {
        (void)cpl_error_set_where_();
        cpl_image_delete(self);
        self = NULL;
    }

    return self;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_IO_SET

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @internal
  @brief    Set the pixel at the given position to the given value
  @param    image       input image.
  @param    xpos        Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos        Pixel y position (FITS convention, 1 for lowest)
  @param    value       New pixel value 
  @return   Nothing
  @see cpl_image_set()
  @note BPM not touched, no input validation

 */ 
/*----------------------------------------------------------------------------*/
inline void cpl_image_set_(cpl_image * image,
                           cpl_size    xpos,
                           cpl_size    ypos,
                           double      value)
{
    const cpl_size pos = (xpos - 1) + image->nx * (ypos - 1);

    assert( image->pixels );

    switch (image->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_io_body.h"
#undef CPL_CLASS
        default:
            assert(0);
    }
}
#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Set the pixel at the given position to the given value
  @param    image       input image.
  @param    xpos        Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos        Pixel y position (FITS convention, 1 for lowest)
  @param    value       New pixel value 
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT, CPL_TYPE_DOUBLE.

  If the pixel is flagged as rejected, this flag is removed.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the passed position is not in the image
  - CPL_ERROR_INVALID_TYPE if the image pixel type is not supported
 */ 
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_set(cpl_image * image,
                             cpl_size    xpos,
                             cpl_size    ypos,
                             double      value) 
{
    /* Check entries */
    cpl_ensure_code(image != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(xpos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(ypos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(xpos <= image->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(ypos <= image->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(!(image->type & CPL_TYPE_COMPLEX), CPL_ERROR_INVALID_TYPE);

    cpl_image_set_(image, xpos, ypos, value);
    cpl_image_accept_(image, xpos, ypos);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Set the pixel at the given position to the given complex value
  @param    image       input image.
  @param    xpos        Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos        Pixel y position (FITS convention, 1 for lowest)
  @param    value       New pixel value 
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_image_set()
  @note This function is available iff the application includes complex.h

  Images can be CPL_TYPE_FLOAT_COMPLEX or CPL_TYPE_DOUBLE_COMPLEX.

  If the pixel is flagged as rejected, this flag is removed.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the passed position is not in the image
  - CPL_ERROR_INVALID_TYPE if the image pixel type is not supported
 */ 
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_set_complex(cpl_image *    image,
                                     cpl_size       xpos,
                                     cpl_size       ypos,
                                     double complex value) 
{
    cpl_size pos;

    /* Check entries */
    cpl_ensure_code(image != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(xpos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(ypos >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(xpos <= image->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(ypos <= image->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);
/*    cpl_ensure_code(image -> type & CPL_TYPE_COMPLEX,
            CPL_ERROR_INVALID_TYPE);*/

    assert( image->pixels );

    pos = (xpos - 1) + image->nx * (ypos - 1);

    if (image-> type == CPL_TYPE_FLOAT_COMPLEX) {
        float complex * pi = (float complex *)image->pixels;
        pi[pos] = (float complex)value;
    } else {
        double complex * pi = (double complex *)image->pixels;
        pi[pos] = value;
    }
    cpl_image_accept_(image, xpos, ypos);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Gets the pixel data.
  @param    img  Image to query.
  @return   A pointer to the image pixel data or NULL on error.
  
  The returned pointer refers to already allocated data. 

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
void * cpl_image_get_data(cpl_image * img)
{
    cpl_ensure(img, CPL_ERROR_NULL_INPUT, NULL);
    return img->pixels;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Gets the pixel data.
  @param    img  Image to query.
  @return   A pointer to the image pixel data or NULL on error.
  @see cpl_image_get_data
 */
/*----------------------------------------------------------------------------*/
const void * cpl_image_get_data_const(const cpl_image * img)
{
    cpl_ensure(img, CPL_ERROR_NULL_INPUT, NULL);
    return img->pixels;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Replace the bad pixel map of the image
  @param    self  Image to modfify
  @param    bpm   Bad Pixel Map (BPM) to set, replacing the old one, or NULL
  @return   A pointer to the old mask of bad pixels, or NULL
  @note NULL is returned if the image had no bad pixel map, while a non-NULL
        returned mask must be deallocated by the caller using cpl_mask_delete().
  @see cpl_image_get_bpm_const()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if self is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_image_set_bpm(cpl_image * self, cpl_mask * bpm)
{
    cpl_mask * oldbpm;

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);

    oldbpm = self->bpm;
    self->bpm = bpm;
 
    return oldbpm;
}



/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Remove the bad pixel map from the image
  @param    self  image to process
  @return   A pointer to the cpl_mask of bad pixels, or NULL
  @note NULL is returned if the image has no bad pixel map
  @note The returned mask must be deallocated using cpl_mask_delete().
  @see cpl_image_set_bpm()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_image_unset_bpm(cpl_image * self)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);

    /* Cannot fail now */
    return cpl_image_set_bpm(self, NULL);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Gets the bad pixels map
  @param    img  Image to query.
  @return   A pointer to the mask identifying the bad pixels or NULL.
  
  The returned pointer refers to already allocated data. 
  If the bad pixel map is NULL, an empty one is created.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_image_get_bpm(cpl_image * img)
{
    cpl_ensure(img != NULL, CPL_ERROR_NULL_INPUT, NULL);

    if (img->bpm == NULL) img->bpm = cpl_mask_new(img->nx, img->ny);

    return img->bpm;
}
    
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Gets the bad pixels map
  @param    img  Image to query.
  @return   A pointer to the mask identifying the bad pixels or NULL.
  @note NULL is returned if the image has no bad pixel map
  @see cpl_image_get_bpm

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
const cpl_mask * cpl_image_get_bpm_const(const cpl_image * img)
{
    cpl_ensure(img != NULL, CPL_ERROR_NULL_INPUT, NULL);

    return img->bpm;
}
    
#define CPL_OPERATION CPL_IMAGE_IO_GET_PIXELS
#define CPL_CONST /* const */
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a double array
  @param    img     a cpl_image object 
  @return   pointer to the double data array or NULL on error.
  
  The returned pointer refers to already allocated data. 

  The pixels are stored in a one dimensional array. The pixel value PIXVAL at 
  position (i, j) in the image - (0, 0) is the lower left pixel, i gives the 
  column position from left to right, j gives the row position from bottom to
  top - is given by :
  PIXVAL = array[i + j*nx];
  where nx is the x size of the image and array is the data array returned by 
  this function.
  array can be used to access or modify the pixel value in the image.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_TYPE_MISMATCH if the passed image type is not double
 */
/*----------------------------------------------------------------------------*/
double * cpl_image_get_data_double(cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a float array
  @param    img     a cpl_image object 
  @return   pointer to the float data array or NULL on error.
  @see      cpl_image_get_data_double()
 */
/*----------------------------------------------------------------------------*/
float * cpl_image_get_data_float(cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a double complex array
  @param    img     a cpl_image object 
  @return   pointer to the double complex data array or NULL on error.
  @see      cpl_image_get_data_double()
  @note This function is available iff the application includes complex.h

 */
/*----------------------------------------------------------------------------*/
double complex * cpl_image_get_data_double_complex(cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a float complex array
  @param    img     a cpl_image object 
  @return   pointer to the float complex data array or NULL on error.
  @see      cpl_image_get_data_double_complex()
 */
/*----------------------------------------------------------------------------*/
float complex * cpl_image_get_data_float_complex(cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a integer array
  @param    img     a cpl_image object 
  @return   pointer to the integer data array or NULL on error.
  @see      cpl_image_get_data_double()
 */
/*----------------------------------------------------------------------------*/
int * cpl_image_get_data_int(cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}
   
#undef CPL_CONST
#define CPL_CONST const

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a double array
  @param    img     a cpl_image object 
  @return   pointer to the double data array or NULL on error.
  @see cpl_image_get_data_double
 */
/*----------------------------------------------------------------------------*/
const double * cpl_image_get_data_double_const(const cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a float array
  @param    img     a cpl_image object 
  @return   pointer to the float data array or NULL on error.
  @see      cpl_image_get_data_float()
 */
/*----------------------------------------------------------------------------*/
const float * cpl_image_get_data_float_const(const cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a double complex array
  @param    img     a cpl_image object 
  @return   pointer to the double complex data array or NULL on error.
  @see      cpl_image_get_data_double_complex()
 */
/*----------------------------------------------------------------------------*/
const double complex * 
cpl_image_get_data_double_complex_const(const cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a float complex array
  @param    img     a cpl_image object 
  @return   pointer to the float complex data array or NULL on error.
  @see      cpl_image_get_data_double_complex()
 */
/*----------------------------------------------------------------------------*/
const float complex * 
cpl_image_get_data_float_complex_const(const cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Get the data as a integer array
  @param    img     a cpl_image object 
  @return   pointer to the integer data array or NULL on error.
  @see      cpl_image_get_data_int()
 */
/*----------------------------------------------------------------------------*/
const int * cpl_image_get_data_int_const(const cpl_image * img)
{
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_io_body.h"
#undef CPL_CLASS
}
#undef CPL_OPERATION
  

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Free memory associated to an cpl_image object.
  @param    d    Image to destroy.
  @return   void

  Frees all memory associated with a cpl_image.
  If the passed image is NULL, the function returns without doing
  anything.
 */
/*----------------------------------------------------------------------------*/
void cpl_image_delete(cpl_image * d)
{
    if (d == NULL) return;

    /* Delete pixels and bad pixel map */
    cpl_free(d->pixels);
    cpl_mask_delete(d->bpm);

    cpl_free(d);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Free memory associated to an cpl_image object, but the pixel buffer.
  @param    d   Image to destroy.
  @return   A pointer to the data array or NULL if the input is NULL.

  Frees all memory associated to an cpl_image, except the pixel buffer.
  cpl_image_unwrap() is provided for images that are constructed by passing
  a pixel buffer to one of cpl_image_wrap_{double,float,int}().

  @note
     The pixel buffer must subsequently be deallocated. Failure to do so will
     result in a memory leak.
 */
/*----------------------------------------------------------------------------*/
void * cpl_image_unwrap(cpl_image * d)
{

    void * data;

    if (d == NULL) return NULL;

    /* Delete bad pixel map */
    cpl_mask_delete(d->bpm);

    data = (void *) d->pixels;

    cpl_free(d);

    return data;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Copy an image.
  @param    src    Source image.
  @return   1 newly allocated image, or NULL on error.
 
  Copy an image into a new image object. The pixels and the bad pixel map are
  also copied. The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_duplicate(const cpl_image * src)
{
    cpl_image * self;

    cpl_ensure(src != NULL, CPL_ERROR_NULL_INPUT, NULL);

    /* Allocate and copy the image properties into a new image
       - must update all (one or two) non-NULL pointer members */
    self = memcpy(cpl_malloc(sizeof(cpl_image)), src, sizeof(cpl_image));

    /* 1: Allocate and copy the pixel buffer */
    self->pixels = cpl_malloc((size_t)src->nx * (size_t)src->ny
                              * cpl_type_get_sizeof(src->type));
    (void)memcpy(self->pixels, src->pixels, (size_t)src->nx * (size_t)src->ny
                 * cpl_type_get_sizeof(src->type));

    /* 2: Duplicate the bad pixel map, if any */
    if (src->bpm != NULL) self->bpm = cpl_mask_duplicate(src->bpm);

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Convert a cpl_image to a given type
  @param    self  The image to convert
  @param    type  The destination type
  @return   the newly allocated cpl_image or NULL on error

  Casting to non-complex types is only supported for non-complex types.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the passed type is invalid
  - CPL_ERROR_TYPE_MISMATCH if the passed image type is complex and requested
  casting type is non-complex.
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_cast(const cpl_image * self,
                           cpl_type          type)
{
    cpl_image * new_im;
    void      * ppixels;

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);
    
    /* The source image has the destination type */
    if (self->type == type) return cpl_image_duplicate(self);

    cpl_ensure(type != CPL_TYPE_INVALID, CPL_ERROR_ILLEGAL_INPUT, NULL);

    ppixels = cpl_malloc((size_t)(self->nx * self->ny)
                         * cpl_type_get_sizeof(type));

    /* Switch on the destination type */
    switch (type) {
    case CPL_TYPE_FLOAT: {
        float * pfo = (float*)ppixels;

        /* Switch on the source type */
        switch(self->type) {
        case CPL_TYPE_DOUBLE: {
            const double * pdi = (const double*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pfo[i] = (float)pdi[i];
            break;
        }
        case CPL_TYPE_INT: {
            const int * pii = (const int*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pfo[i] = (float)pii[i];
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            return NULL;
        default:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
        }
        break;
    }
    case CPL_TYPE_DOUBLE: {
        double * pdo = (double*)ppixels;

        /* Switch on the source type */
        switch(self->type) {
        case CPL_TYPE_FLOAT: {
            const float * pfi = (const float*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pdo[i] = (double)pfi[i];
            break;
        }
        case CPL_TYPE_INT: {
            const int * pii = (const int*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pdo[i] = (double)pii[i];
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            return NULL;
        default:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
        }
        break;
    }
    case CPL_TYPE_INT: {
        int * pio = (int*)ppixels;

        /* Switch on the source type */
        switch(self->type) {
        case CPL_TYPE_FLOAT: {
            const float * pfi = (const float*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pio[i] = (int)pfi[i];
            break;
        }
        case CPL_TYPE_DOUBLE: {
            const double * pdi = (const double*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pio[i] = (int)pdi[i];
            break;
        }
        case CPL_TYPE_FLOAT_COMPLEX:
        case CPL_TYPE_DOUBLE_COMPLEX:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            return NULL;
        default:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
        }
        break;
    }
    case CPL_TYPE_FLOAT_COMPLEX: {
        float complex * pfco = (float complex*)ppixels;

        /* Switch on the source type */
        switch(self->type) {
        case CPL_TYPE_DOUBLE_COMPLEX: {
            const double complex * pdci =
                (const double complex *)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pfco[i] = (float complex)pdci[i];
            break;
        }
        case CPL_TYPE_INT: {
            const int * pii = (const int *)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pfco[i] = (float complex)pii[i];
            break;
        }
        case CPL_TYPE_FLOAT: {
            const float * pfi = (const float*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pfco[i] = (float complex)pfi[i];
            break;
        }
        case CPL_TYPE_DOUBLE: {
            const double * pdi = (const double*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pfco[i] = (float complex)pdi[i];
            break;
        }
        default:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
        }
        break;
    }
    case CPL_TYPE_DOUBLE_COMPLEX: {
        double complex * pdco = (double complex*)ppixels;

        /* Switch on the source type */
        switch(self->type) {
        case CPL_TYPE_FLOAT_COMPLEX: {
            const float complex * pfci = (const float complex*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++) 
                pdco[i] = (double complex)pfci[i];
            break;
        }
        case CPL_TYPE_INT: {
            const int * pii = (const int *)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pdco[i] = (double complex)pii[i];
            break;
        }
        case CPL_TYPE_FLOAT: {
            const float * pfi = (const float*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pdco[i] = (double complex)pfi[i];
            break;
        }
        case CPL_TYPE_DOUBLE: {
            const double * pdi = (const double*)self->pixels;
            for (cpl_size i = 0; i < self->nx * self->ny; i++)
                pdco[i] = (double complex)pdi[i];
            break;
        }
        default:
            cpl_free(ppixels);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
        }
        break;
    }
    default:
        cpl_free(ppixels);
        (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                     "source: %s <=> %s-destination",
                                     cpl_type_get_name(self->type),
                                     cpl_type_get_name(type));
        return NULL;
    }

    new_im = cpl_image_wrap_(self->nx, self->ny, type, ppixels);

    /* Bad pixel map */
    if (self->bpm != NULL) new_im->bpm = cpl_mask_duplicate(self->bpm);
    
    return new_im;
}

#define CPL_OPERATION CPL_IMAGE_IO_SET_BADPIXEL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Set the bad pixels in an image to a fixed value
  @param    im  The image to modify.
  @param    a   The fixed value
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT, CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_INVALID_TYPE if the image pixel type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_rejected(
        cpl_image   *   im, 
        double          a)
{
    cpl_binary  *       pbpm;

    /* Check entries and Initialise */
    cpl_ensure_code(im, CPL_ERROR_NULL_INPUT);
    
    /* If no bad pixel */
    if (im->bpm == NULL) return CPL_ERROR_NONE;
    
    /* Return if no bad pixel map to update */
    if (cpl_mask_is_empty(im->bpm)) return CPL_ERROR_NONE;

    pbpm = cpl_mask_get_data(im->bpm);
    
    switch (im->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_io_body.h"
#undef CPL_CLASS

      default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Save one or more images to a FITS file
  @param    self       Image(s) of identical size and type to write to disk
  @param    nz         Number of images
  @param    is_cube    CPL_TRUE: use NAXIS3, otherwise NAXIS2 (ignored: nz > 1)
  @param    filename   Name of the file to write to
  @param    type       The type used to represent the data in the file
  @param    pl         Property list for the output header or NULL
  @param    mode       The desired output options
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_image_save()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_save_(const cpl_image * const* self,
                               cpl_size nz,
                               cpl_boolean is_cube,
                               const char * filename,
                               cpl_type type,
                               const cpl_propertylist * pl,
                               unsigned mode)
{

    int               status = 0; /* CFITSIO status, must be set to zero */
    /* Count number of compression flags */
    const unsigned ncompress = ((mode & CPL_IO_COMPRESS_GZIP) != 0) +
        ((mode & CPL_IO_COMPRESS_HCOMPRESS) != 0) +
        ((mode & CPL_IO_COMPRESS_PLIO) != 0) +
        ((mode & CPL_IO_COMPRESS_RICE) != 0);
    fitsfile       *  fptr;
    cpl_size          i;
    const char      * badkeys = mode & CPL_IO_EXTEND ?
        CPL_FITS_BADKEYS_EXT  "|" CPL_FITS_COMPRKEYS :
        CPL_FITS_BADKEYS_PRIM "|" CPL_FITS_COMPRKEYS;
    const cpl_image * first = self ? self[0] : NULL;
    const cpl_size    nx = cpl_image_get_size_x(first);
    const cpl_size    ny = cpl_image_get_size_y(first);
    const cpl_type    ftype = cpl_image_get_type(first);
    const int         cfitsiotype = ftype == CPL_TYPE_DOUBLE ? TDOUBLE
        : (ftype == CPL_TYPE_FLOAT ? TFLOAT : TINT);
    const CPL_FITSIO_TYPE naxes[3] = {nx, ny, nz};
    long              plane1 = 0; /* First plane to write */
    const int         ibpp = cpl_tools_get_bpp(type == CPL_TYPE_UNSPECIFIED ?
                                               ftype : type);
    const int naxis23 = (nz > 1 || is_cube) ? 3 : 2;

    /* Test entries */
    cpl_ensure_code(filename != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(self     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(ibpp     != 0,    CPL_ERROR_ILLEGAL_INPUT);

    if (is_cube) {
        cpl_ensure_code(((mode & CPL_IO_CREATE) != 0) +
                        ((mode & CPL_IO_EXTEND) != 0) +
                        ((mode & CPL_IO_APPEND) != 0) == 1, CPL_ERROR_ILLEGAL_INPUT);
    } else {
        cpl_ensure_code(((mode & CPL_IO_CREATE) != 0) +
                        ((mode & CPL_IO_EXTEND) != 0) == 1, CPL_ERROR_ILLEGAL_INPUT);
    }

    cpl_ensure_code(mode < CPL_IO_MAX, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(ftype == CPL_TYPE_DOUBLE || ftype == CPL_TYPE_FLOAT ||
                    ftype == CPL_TYPE_INT, CPL_ERROR_INVALID_TYPE);

    if (mode & (CPL_IO_EXTEND | CPL_IO_APPEND)) {

        if (ncompress > 0) {
            if (ncompress > 1) {
                /* Only one type of compression is allowed */
                return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                              "Specify one compression method, "
                                              "not %d (mode=0x%x)", ncompress,
                                              mode);
            }
            if (mode & CPL_IO_APPEND) {
            /* A compression flag is not allowed in append mode - it would
               at best be redundant or else in conflict with the existing
               compression method (which includes no compression) */
                return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                              "Compression in append mode not "
                                              "allowed");
            }
            if (abs(ibpp) < 32) {
                return cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                              "Compression not supported "
                                              "with implicit conversion to "
                                              "BITPIX=%d (type='%s'), use e.g. "
                                              "type='CPL_TYPE_INT'", ibpp,
                                              cpl_type_get_name(type));
            }
#ifndef CPL_IO_COMPRESSION_LOSSY
            if (ibpp < 0) {
                /* No compression of floating-point data - yet */
                return cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                              "Lossy compression not supported "
                                              "(BITPIX=%d < 0)", ibpp);
            }
#endif
        }

        /* Open the file */
        if (cpl_io_fits_open_diskfile(&fptr, filename, READWRITE, &status)) {
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      fits_open_diskfile, "filename='%s', "
                                      "type=%d, mode=%d", filename, type, mode);
        }

        if (mode & CPL_IO_COMPRESS_GZIP) 
            fits_set_compression_type(fptr, GZIP_1, &status);
        else if (mode & CPL_IO_COMPRESS_HCOMPRESS) 
            fits_set_compression_type(fptr, HCOMPRESS_1, &status);
        else if (mode & CPL_IO_COMPRESS_PLIO) 
            fits_set_compression_type(fptr, PLIO_1, &status);
        else if (mode & CPL_IO_COMPRESS_RICE) 
            fits_set_compression_type(fptr, RICE_1, &status);

        if (status != 0) {
            int fio_status2 = 0;
            cpl_io_fits_close_file(fptr, &fio_status2);

            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      fits_set_compression_type,
                                      "filename='%s', type=%d, mode=%d",
                                      filename, type, mode);
        }

    } else if (ncompress > 0) {
        /* Compression is only allowed in extensions */
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    } else {
        /* Create the file */
        char * sval = cpl_sprintf("!%s", filename);
        cpl_io_fits_create_file(&fptr, sval, &status);
        cpl_free(sval);
        if (status != 0) {
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      fits_create_file, "filename='%s', "
                                      "type=%d, mode=%d", filename, type, mode);
        }
    }

    if (mode & CPL_IO_APPEND) {
        int next;
        int exttype;
        int naxis;
        int bitpix;
        /* Last element read on naxis=2 */
        CPL_FITSIO_TYPE fnaxes[3];

        if (fits_get_num_hdus(fptr, &next, &status)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      fits_get_num_hdus, "filename='%s', "
                                      "type=%d, mode=%d", filename, type, mode);
        } else if (fits_movabs_hdu(fptr, next, &exttype, &status)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status, fits_movabs_hdu,
                                      "filename='%s', type=%d, mode=%d",
                                      filename, type, mode);
        } else if (exttype != IMAGE_HDU) {
            int error2 = 0;
            return cpl_io_fits_close_file(fptr, &error2)
                ? cpl_error_set_fits(CPL_ERROR_FILE_IO, error2,
                                     fits_close_file, "filename='%s', "
                                     "type=%d, mode=%d", filename, type, mode)
                : cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Data-unit "
                                         "%d in file %s is not image-type (%d "
                                         "!= %d. type=%d, mode=%d)", next,
                                         filename, exttype, IMAGE_HDU, type,
                                         mode);
        } else if (fits_get_img_dim(fptr, &naxis, &status)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      fits_get_img_dim, "filename='%s', "
                                      "type=%d, mode=%d", filename, type, mode);
        } else if (naxis < 2 || naxis > 3) {
            int error2 = 0;
            return cpl_io_fits_close_file(fptr, &error2)
                ? cpl_error_set_fits(CPL_ERROR_FILE_IO, error2,
                                     fits_close_file, "filename='%s', "
                                     "type=%d, mode=%d", filename, type, mode)
                : cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Data-unit "
                                         "%d in file %s has wrong NAXIS (%d !="
                                         " 2/3). type=%d, mode=%d)", next,
                                         filename, naxis, type, mode);
        } else if (fits_get_img_type(fptr, &bitpix, &status)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      fits_get_img_type, "filename='%s', type="
                                      "%d, mode=%d", filename, type, mode);
        } else if (bitpix != ibpp) {
            int error2 = 0;
            return cpl_io_fits_close_file(fptr, &error2)
                ? cpl_error_set_fits(CPL_ERROR_FILE_IO, error2,
                                     fits_close_file, "filename='%s', "
                                     "type=%d, mode=%d", filename, type, mode)
                : cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Data-unit "
                                         "%d in file %s has wrong BITPIX (%d "
                                         "!= %d). type=%d, mode=%d)", next,
                                         filename, bitpix, ibpp, type, mode);
        } else if (CPL_FITSIO_GET_SIZE(fptr, naxis, fnaxes, &status)) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      CPL_FITSIO_GET_SIZE,
                                      "filename='%s', type=%d, mode=%d",
                                      filename, type, mode);
        } else if (naxes[0] != fnaxes[0] || naxes[1] != fnaxes[1]) {
            int error2 = 0;
            return cpl_io_fits_close_file(fptr, &error2)
                ? cpl_error_set_fits(CPL_ERROR_FILE_IO, error2,
                                     fits_close_file, "filename='%s', "
                                     "type=%d, mode=%d", filename, type, mode)
                : cpl_error_set_message_
                (CPL_ERROR_ILLEGAL_INPUT, "Data-unit %d in file %s has "
                 "wrong NAXIS1/2 (%ldX%ld != %ldX%ld). type=%d, mode=%d)",
                 next, filename, (long)fnaxes[0], (long)fnaxes[1],
                 (long)naxes[0], (long)naxes[1], type, mode);

        } else if (fits_is_compressed_image(fptr, &status) && !status) {
            return cpl_io_fits_close_file(fptr, &status)
                ? cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                     fits_close_file, "filename='%s', "
                                     "type=%d, mode=%d", filename, type, mode)
                : cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE, "Data-"
                                         "unit %d in file %s is compressed. "
                                         "type=%d, mode=%d)", next, filename,
                                         type, mode);
        } else if (status) {
            int error2 = 0;
            cpl_io_fits_close_file(fptr, &error2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                      fits_is_compressed_image,
                                      "filename='%s', type=%d, mode=%d",
                                      filename, type, mode);
        } else {
            plane1 = fnaxes[2];
            fnaxes[2] += naxes[2];
            if (CPL_FITSIO_RESIZE_IMG(fptr, bitpix, naxis, fnaxes,
                                      &status)) {
                int error2 = 0;
                cpl_io_fits_close_file(fptr, &error2);
                return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                          CPL_FITSIO_RESIZE_IMG,
                                          "filename='%s', type=%d, mode=%d",
                                          filename, type, mode);
            }
        }
    } else if 
          /* Create the imagelist in a new HDU appended to the file */
          (CPL_FITSIO_CREATE_IMG(fptr, ibpp, naxis23, naxes, &status)) {
        int error2 = 0;
        cpl_io_fits_close_file(fptr, &error2);
        return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                  CPL_FITSIO_CREATE_IMG_E,
                                  "filename='%s', type=%d, mode=%d",
                                  filename, type, mode);
    } else if
          /* Add Date, if creating */
          ((mode & CPL_IO_CREATE) && fits_write_date(fptr, &status)) {
        int error2 = 0;
        cpl_io_fits_close_file(fptr, &error2);
        return cpl_error_set_fits(CPL_ERROR_FILE_IO, status, fits_write_date,
                                  "filename='%s', type=%d, mode=%d",
                                  filename, type, mode);
    }

    /* Add the property list */
    if (cpl_fits_add_properties(fptr, pl, badkeys)) {
        return cpl_io_fits_close_file(fptr, &status)
            ? cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                 fits_close_file, "filename='%s', "
                                 "type=%d, mode=%d", filename, type, mode)
            : cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    /* Loop on the images */
    for (i = 0; i < nz; i++) {
        const CPL_FITSIO_TYPE fpixel[3] = {1, 1, plane1 + i + 1};
        const cpl_image     * image = self[i];
        const void          * data  = cpl_image_get_data_const(image);

        /* Write the pixels */
        if (CPL_FITSIO_WRITE_PIX(fptr, cfitsiotype, fpixel,
                                 (LONGLONG)(nx*ny), data, &status)) break;
    }

    /* Check  */
    if (status) {
        int error2 = 0;
        cpl_io_fits_close_file(fptr, &error2);
        return cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                                  CPL_FITSIO_WRITE_PIX_E, "filename='%s', "
                                  "type=%d, mode=%d, plane=%u", filename, type,
                                  mode, (unsigned)i);
    }

    /* Close (and write to disk) */
    return cpl_io_fits_close_file(fptr, &status)
        ? cpl_error_set_fits(CPL_ERROR_FILE_IO, status,
                             fits_close_file, "filename='%s', "
                             "type=%d, mode=%d", filename, type, mode)
        : CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Save an image to a FITS file
  @param    self       Image to write to disk or NULL
  @param    filename   Name of the file to write to
  @param    type       The type used to represent the data in the file
  @param    pl         Property list for the output header or NULL
  @param    mode       The desired output options
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_propertylist_save()

  This function saves an image to a FITS file. If a property list
  is provided, it is written to the header where the image is written.
  The image may be NULL, in this case only the propertylist is saved.
  
  Supported image types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT.

  The type used in the file can be one of:
     CPL_TYPE_UCHAR  (8 bit unsigned),
     CPL_TYPE_SHORT  (16 bit signed),
     CPL_TYPE_USHORT (16 bit unsigned),
     CPL_TYPE_INT    (32 bit signed),
     CPL_TYPE_FLOAT  (32 bit floating point), or
     CPL_TYPE_DOUBLE (64 bit floating point). Additionally, the special value
     CPL_TYPE_UNSPECIFIED is allowed. This value means that the type used
  for saving is the pixel type of the input image. Using the image pixel type
  as saving type ensures that the saving incurs no loss of information.

  Supported output modes are CPL_IO_CREATE (create a new file) and
  CPL_IO_EXTEND (append a new extension to an existing file).

  Upon success the image will reside in a FITS data unit with NAXIS = 2.
  Is it possible to save a single image in a FITS data unit with NAXIS = 3,
  see cpl_imagelist_save().

  When the data written to disk are of an integer type, the output mode
  CPL_IO_EXTEND can be combined (via bit-wise or) with an option for
  tile-compression. This compression of integer data is lossless.
  The options are:
  CPL_IO_COMPRESS_GZIP, CPL_IO_COMPRESS_RICE, CPL_IO_COMPRESS_HCOMPRESS,
  CPL_IO_COMPRESS_PLIO.
  With compression the type must be CPL_TYPE_UNSPECIFIED or CPL_TYPE_INT.
 
  Note that in append mode the file must be writable (and do not take for
  granted that a file is writable just because it was created by the same
  application, as this depends from the system @em umask).
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the type or the mode is not supported
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported
  - CPL_ERROR_FILE_NOT_CREATED if the output file cannot be created
  - CPL_ERROR_FILE_IO if the data cannot be written to the file
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_save(const cpl_image        * self,
                              const char             * filename,
                              cpl_type                 type,
                              const cpl_propertylist * pl,
                              unsigned                 mode)
{
    const cpl_error_code error = self == NULL ?
        cpl_propertylist_save(pl, filename, mode) :
        cpl_image_save_(&self, 1, CPL_FALSE, filename, type, pl, mode);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Dump structural information of a CPL image
  @param    self    Image to dump
  @param    stream  Output stream, accepts @c stdout or @c stderr
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if a write operation fails
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_dump_structure(const cpl_image * self, FILE * stream)
{

    const int msgmin = (int)(strlen(CPL_MSG) - 2 - 3 * strlen(CPL_SIZE_FORMAT));

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(stream != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code( fprintf(stream, CPL_MSG,
                             self->nx, 
                             self->ny,
                             cpl_type_get_name(self->type),
                             self->bpm ? cpl_mask_count(self->bpm) : 0)
                     >= msgmin, CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;

}
 
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Dump pixel values in a CPL image
  @param    self    Image to dump
  @param    llx     Lower left x position (FITS convention, 1 for leftmost)
  @param    lly     Lower left y position (FITS convention, 1 for lowest)
  @param    urx     Specifies the window position
  @param    ury     Specifies the window position
  @param    stream  Output stream, accepts @c stdout or @c stderr
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if a write operation fails
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the defined window is not in the image
  - CPL_ERROR_ILLEGAL_INPUT if the window definition is wrong (e.g llx > urx)
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_dump_window(const cpl_image * self, cpl_size llx,
                                     cpl_size lly,
                                     cpl_size urx, cpl_size ury, FILE * stream)
{
    const cpl_error_code err = CPL_ERROR_FILE_IO;
    cpl_size             i, j;
    cpl_boolean          has_bad;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(stream != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(llx > 0 && llx <= self->nx && urx > 0 && urx <= self->nx,
           CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(lly > 0 && lly <= self->ny && ury > 0 && ury <= self->ny,
           CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(urx >= llx && ury >= lly, CPL_ERROR_ILLEGAL_INPUT);

    has_bad = self->bpm != NULL && !cpl_mask_is_empty(self->bpm);

    cpl_ensure_code( fprintf(stream, "#----- image: %" CPL_SIZE_FORMAT
                             " <= x <= %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                             " <= y <= %" CPL_SIZE_FORMAT " -----\n",
                             llx, urx, lly, ury) > 0, err);

    cpl_ensure_code( fprintf(stream, "\tX\tY\tvalue\n") > 0, err);

    for (j = lly; j <= ury; j++) {
        for (i = llx; i <= urx; i++) {
            const char * badtxt = has_bad && cpl_mask_get(self->bpm, i, j)
                ? " (rejected)" : "";
            if (self->type == CPL_TYPE_INT) {
                const int * pint  = (const int*)self->pixels;
                const int   value = pint[(i-1) + (j-1) * self->nx];
                cpl_ensure_code( fprintf(stream, "\t%" CPL_SIZE_FORMAT "\t%"
                                         CPL_SIZE_FORMAT "\t%d%s\n", i, j,
                                         value, badtxt) > 0, err);
            } else if (self->type == CPL_TYPE_FLOAT_COMPLEX ||
               self->type == CPL_TYPE_DOUBLE_COMPLEX ) {
                int dummy;
                const double complex value = 
            cpl_image_get_complex(self, i, j, &dummy);
                cpl_ensure_code( fprintf(stream, "\t%" CPL_SIZE_FORMAT "\t%"
                                         CPL_SIZE_FORMAT "\t%g + %g I %s\n",
                     i, j, creal(value), cimag(value),
                     badtxt) > 0, err);
            } else {
                int dummy;
                const double value = cpl_image_get(self, i, j, &dummy);
                cpl_ensure_code( fprintf(stream, "\t%" CPL_SIZE_FORMAT "\t%"
                                         CPL_SIZE_FORMAT "\t%g%s\n", i, j,
                                         value, badtxt) > 0, err);
            }
        }
    }

    return CPL_ERROR_NONE;

}
 

#define FFSTACK_PUSH(Y, XL, XR, DY) \
    if (sp<stack+stacksz && Y+(DY)>=wy1 && Y+(DY)<=wy2) \
    {sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}
#define FFSTACK_POP(Y, XL, XR, DY) \
    {sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}

/*----------------------------------------------------------------------------*/
/*
  @internal
  @ingroup cpl_image
  @brief    Fill a zone with label.
  @param    intimage    input label image
  @param    fftemp      Pre-allocated work-space
  @param    x           x position
  @param    y           y position
  @param    label       current label
  
  This code was pulled out the Usenet and freely adapted to cpl. 
  Credits - Paul Heckbert (posted on comp.graphics 28 Apr 1988)
  It is highly unreadable and makes use of goto and other fairly bad 
  programming practices, but works fine and fast.
 */
/*----------------------------------------------------------------------------*/
static void cpl_image_floodfill(cpl_image   * lab,
                                void        * fftemp,
                                cpl_size      x,
                                cpl_size      y,
                                cpl_size      label)
{
    struct { cpl_size y, xl, xr, dy; } * stack, * sp;
    cpl_size        wx1, wx2, wy1, wy2;
    cpl_size        l, x1, x2, dy;
    cpl_size        ov;
    const cpl_size  stacksz = FFSTACK_STACKSZ(lab);
    int         *   pii;

    stack = fftemp;
    sp = stack;
    wx1 = 0;
    wx2 = lab->nx-1;
    wy1 = 0;
    wy2 = lab->ny-1;
    pii = cpl_image_get_data_int(lab);
    ov = pii[x+y*lab->nx];
    if (ov==label || x<wx1 || x>wx2 || y<wy1 || y>wy2) return;
    FFSTACK_PUSH(y, x, x, 1);           /* needed in some cases */
    FFSTACK_PUSH(y+1, x, x, -1);        /* seed segment (popped 1st) */

    while (sp>stack) {
        /* pop segment off stack and fill a neighboring scan line */
        FFSTACK_POP(y, x1, x2, dy);
        /*
         * segment of scan line y-dy for x1<=x<=x2 was previously filled,
         * now explore adjacent pixels in scan line y
         */
        for (x=x1; x>=wx1 && pii[x+y*lab->nx]==ov; x--)
            pii[x+y*lab->nx] = (int)label; /* FIXME: Need cpl_size pixel */
        if (x>=x1) goto skip;
        l = x+1;
        if (l<x1) FFSTACK_PUSH(y, l, x1-1, -dy);        /* leak on left? */
        x = x1+1;
        do {
            for (; x<=wx2 && pii[x+y*lab->nx]==ov; x++)
                pii[x+y*lab->nx] = (int)label; /* FIXME: Need cpl_size pixel */
            FFSTACK_PUSH(y, l, x-1, dy);
            if (x>x2+1) FFSTACK_PUSH(y, x2+1, x-1, -dy);    /* leak on right? */
skip:       for (x++; x<=x2 && pii[x+y*lab->nx]!=ov; x++) {
                ;
            }
            l = x;
        } while (x<=x2);
    }
}


/*----------------------------------------------------------------------------*/
/*
  @internal
  @brief Internal image loading function
  @param fptr      CFITSIO structure of the already opened FITS file
  @param pnaxis    If it points to 0, set *pnaxis to NAXIS else use as such
  @param naxes     If 1st is 0, fill w. NAXIS[12[3]] else use as such
  @param ppix_type If points to _UNSPECIFIED, set else use as such
  @param filename  The named of the opened file (only for error messages)
  @param pnum      Plane number in the Data Unit (0 for first)
  @param lhdumov   Absolute extension number to move to first (0 for primary)
  @param do_window True for (and only for) a windowed load
  @param llx       Lower left x position (FITS convention, 1 for leftmost)
  @param lly       Lower left y position (FITS convention, 1 for lowest)
  @param urx       Specifies the window position
  @param ury       Specifies the window position
  @return   1 newly allocated image or NULL if image cannot be loaded.
  @see cpl_image_load_one()

  This function reads from an already opened FITS-file, which is useful when
  multiple images are to be loaded from the same file.

  To avoid repeated calls to determine NAXIS, NAXISi and optionally the
  CPL pixel-type most suitable for the the actual FITS data, the parameters
  pnaxis, naxes and ppix_type can be used both for input and for output.

  The extension number lhdumov is numbered with 1 for the first extension,
  i.e. 0 moves to the primary HDU. Any negative value causes no move.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_BAD_FILE_FORMAT if a CFITSIO call fails or if in auto-pixel mode
                              the CFITSIO data type is unsupported.
  - CPL_ERROR_DATA_NOT_FOUND if the specified extension has no image data.
                             This code is relied on as being part of the API!
  - CPL_ERROR_INCOMPATIBLE_INPUT if NAXIS is OK but the data unit is empty
  - CPL_ERROR_ILLEGAL_INPUT If the plane number is out of range, or in a windowed
                             read the window is invalid
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported

 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_load_(fitsfile      * fptr,
                            int           * pnaxis,
                            CPL_FITSIO_TYPE naxes[],
                            cpl_type      * ppix_type,
                            const char    * filename,
                            cpl_size        pnum,
                            cpl_size        lhdumov,
                            cpl_boolean     do_window,
                            cpl_size        llx,
                            cpl_size        lly,
                            cpl_size        urx,
                            cpl_size        ury)
{

    const int      hdumov = (int)lhdumov;
    int            error = 0;
    cpl_image    * self;
    void         * pixels;
    const long int inc[3] = {1, 1, 1};
    long int       fpixel[3];
    long int       lpixel[3];
    int            loadtype;
    cpl_size       nx, ny;

    cpl_ensure(fptr             != NULL,    CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(pnaxis           != NULL,    CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(naxes            != NULL,    CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(ppix_type        != NULL,    CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(filename         != NULL,    CPL_ERROR_NULL_INPUT,    NULL);
    /* CFITSIO only supports int */
    cpl_ensure((cpl_size)(1+hdumov) == 1+lhdumov, CPL_ERROR_ILLEGAL_INPUT,
               NULL);


    /* The open call may be reusing file handle opened for previous I/O,
       so the file pointer needs to be moved also for hdumov = 0 */
    if (hdumov >= 0 && fits_movabs_hdu(fptr, 1+hdumov, NULL, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_movabs_hdu, "filename='%s', pnum=%"
                                 CPL_SIZE_FORMAT ", hdumov=%d",
                                 filename, pnum, hdumov);
        return NULL;
    }

    /* Get NAXIS, if needed */
    if (*pnaxis == 0 && fits_get_img_dim(fptr, pnaxis, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_get_img_dim, "filename='%s', pnum=%"
                                 CPL_SIZE_FORMAT ", hdumov=%d",
                                 filename, pnum, hdumov);
        return NULL;
    }
    /* Verify NAXIS before trying anything else */
    if (*pnaxis != 2 && *pnaxis != 3) {
        (void)cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d",
                                     filename, pnum, hdumov, *pnaxis);
        return NULL;
    }

    /* Get NAXIS[12[3]], if needed */
    if (naxes[0] == 0 && CPL_FITSIO_GET_SIZE(fptr, *pnaxis, naxes, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 CPL_FITSIO_GET_SIZE, "filename='%s', "
                                 "pnum=%" CPL_SIZE_FORMAT ", hdumov=%d, "
                                 "NAXIS=%d", filename, pnum, hdumov, *pnaxis);
        return NULL;
    }

    /* Verify NAXIS[123] */
    if (naxes[0] == 0 || naxes[1] == 0) {
        /* FIXME: Is this actually possible with a non-zero NAXIS ? */
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                     "NAXIS2=%ld", filename, pnum, hdumov,
                                    *pnaxis, (long)naxes[0], (long)naxes[1]);
        return NULL;
    }
    if (*pnaxis == 3 && naxes[2] == 0) {
        /* FIXME: Is this actually possible with a non-zero NAXIS ? */
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                     "NAXIS2=%ld NAXIS3=0", filename, pnum,
                                     hdumov, *pnaxis, (long)naxes[0],
                                     (long)naxes[1]);
        return NULL;
    }

    if (do_window) {
        /* Verify the window size */
        /* If the naxes[] passed is from a previous succesful call here,
           then this check is redundant. Don't rely on that. */
        if (llx < 1 || lly < 1 || urx > naxes[0] || ury > naxes[1]
            || urx < llx || ury < lly) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                         "filename='%s', pnum=%"
                                         CPL_SIZE_FORMAT ", hdumov=%d, "
                                         "llx=%" CPL_SIZE_FORMAT ", lly=%"
                                         CPL_SIZE_FORMAT ", urx=%"
                                         CPL_SIZE_FORMAT ", ury=%"
                                         CPL_SIZE_FORMAT ", NAXIS=%d, "
                                         "NAXIS1=%ld, NAXIS2=%ld",
                                         filename, pnum, hdumov, llx, lly,
                                         urx, ury, *pnaxis, (long)naxes[0],
                                         (long)naxes[1]);
            return NULL;
        }
    } else {
        llx = lly = 1;
        urx = naxes[0];
        ury = naxes[1];
    }

    /* Create the zone definition. The 3rd element not defined for NAXIS = 2 */
    fpixel[0] = llx;
    fpixel[1] = lly;
    lpixel[0] = urx;
    lpixel[1] = ury;
    if (*pnaxis == 3) {

        /* Verify plane number */
        if (pnum + 1 > naxes[2]) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                        "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                         ", hdumov=%d, NAXIS=3, NAXIS1=%ld, "
                                        "NAXIS2=%ld, NAXIS3=%ld",
                                         filename, pnum, hdumov,
                                        (long)naxes[0], (long)naxes[1],
                                        (long)naxes[2]);
            return NULL;
        }

        fpixel[2] = lpixel[2] = pnum + 1;
    } else if (pnum != 0) {
        /* May not ask for any plane but the first when NAXIS == 2 */
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                     "NAXIS2=%ld", filename, pnum, hdumov,
                                    *pnaxis, (long)naxes[0], (long)naxes[1]);
        return NULL;
    }

    if (*ppix_type == CPL_TYPE_UNSPECIFIED) {

        /* The pixel type of the created image is determined
           by the pixel type of the loaded FITS image */

        int imgtype;

        if (fits_get_img_type(fptr, &imgtype, &error)) {
            (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                     fits_get_img_type, "filename='%s', "
                                     "pnum=%" CPL_SIZE_FORMAT ", hdumov=%d, "
                                     "NAXIS=%d, NAXIS1=%ld, NAXIS2=%ld",
                                     filename, pnum, hdumov, *pnaxis,
                                     (long)naxes[0], (long)naxes[1]);
            return NULL;
        }
        
        switch (imgtype) {
        case BYTE_IMG    :
        case SHORT_IMG   :
        case LONG_IMG    :
        case LONGLONG_IMG:
            *ppix_type = CPL_TYPE_INT;
            break;
        case FLOAT_IMG   :
            *ppix_type = CPL_TYPE_FLOAT;
            break;
        case DOUBLE_IMG  :
            *ppix_type = CPL_TYPE_DOUBLE;
            break;
        default:
            break;
        }

        if (*ppix_type == CPL_TYPE_UNSPECIFIED) {
            (void)cpl_error_set_message_(CPL_ERROR_BAD_FILE_FORMAT,
                                        "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                         ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                         "NAXIS2=%ld, imgtype=%d", filename,
                                         pnum, hdumov, *pnaxis, (long)naxes[0],
                                        (long)naxes[1], imgtype);
            return NULL;
        }
    }

    if (*ppix_type == CPL_TYPE_DOUBLE) {
        loadtype = TDOUBLE;
    } else if (*ppix_type == CPL_TYPE_FLOAT) {
        loadtype = TFLOAT;
    } else if (*ppix_type == CPL_TYPE_INT) {
        loadtype = TINT;
    } else {
        (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                     "NAXIS2=%ld, im_type=%d", filename, pnum,
                                     hdumov,*pnaxis, (long)naxes[0],
                                    (long)naxes[1], *ppix_type);
        return NULL;
    }

    nx = urx - llx + 1;
    ny = ury - lly + 1;

    pixels = cpl_malloc((size_t)nx * (size_t)ny
                        * cpl_type_get_sizeof(*ppix_type));

    if (nx == (cpl_size)naxes[0]) {
        const LONGLONG nelem     =     naxes[0] * (LONGLONG)ny;
        const LONGLONG firstelem = 1 + naxes[0] * (LONGLONG)(lly - 1)
                                     + naxes[0] * naxes[1] * (LONGLONG)pnum;

        if (fits_read_img(fptr, loadtype, firstelem, nelem,
                          NULL, pixels, NULL, &error)) {
            cpl_free(pixels);
            (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                     fits_read_img, "filename='%s', "
                                     "pnum=%" CPL_SIZE_FORMAT ", hdumov=%d, "
                                     "NAXIS=%d, NAXIS1=%ld, NAXIS2=%ld",
                                     filename, pnum, hdumov, *pnaxis,
                                     (long)naxes[0], (long)naxes[1]);
            return NULL;
        }
    } else if (cpl_fits_read_subset(fptr, loadtype, fpixel, lpixel, inc,
                                    NULL, pixels, NULL, &error)) {
        cpl_free(pixels);
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_read_subset, "filename='%s', "
                                 "pnum=%" CPL_SIZE_FORMAT ", hdumov=%d, "
                                 "NAXIS=%d, NAXIS1=%ld, NAXIS2=%ld",
                                 filename, pnum, hdumov, *pnaxis,
                                 (long)naxes[0], (long)naxes[1]);
        return NULL;
    }

    self = cpl_image_wrap_(nx, ny, *ppix_type, pixels);

    if (self == NULL) {
        cpl_free(pixels);
        cpl_error_set_where_();
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Create an image, optionally using an existing pixel buffer.
  @param    nx      Size in x (the number of columns)
  @param    ny      Size in y (the number of rows)
  @param    type    Pixel type
  @param    pixels  Pixel data, or NULL
  @return   1 newly allocated cpl_image or NULL on error
  @see      cpl_image_new(), cpl_image_wrap_double()
  @note if pixels is NULL, a new buffer will be cpl_calloc'ed.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if nx or ny is non-positive or their product is too big
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_wrap_(cpl_size nx, cpl_size ny, cpl_type type,
                            void * pixels)
{
    cpl_image    * self;
    const cpl_size npix = nx * ny;
    const size_t   upix = (size_t)nx * (size_t)ny;

    cpl_ensure( nx > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure( ny > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    /* Check for overflow */
    cpl_ensure( npix > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure( (size_t)npix == upix, CPL_ERROR_ILLEGAL_INPUT, NULL);

    cpl_ensure( type == CPL_TYPE_INT    || type == CPL_TYPE_FLOAT ||
                type == CPL_TYPE_DOUBLE || type == CPL_TYPE_FLOAT_COMPLEX ||
                type == CPL_TYPE_DOUBLE_COMPLEX, CPL_ERROR_INVALID_TYPE, NULL);

    self         = cpl_malloc(sizeof(cpl_image));
    self->nx     = nx;
    self->ny     = ny;
    self->type   = type;
    self->bpm    = NULL;
    self->pixels = pixels != NULL ? pixels
        : cpl_calloc(upix, cpl_type_get_sizeof(type));

    return self;
}
