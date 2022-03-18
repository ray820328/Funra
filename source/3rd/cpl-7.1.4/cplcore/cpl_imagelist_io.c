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

#include "cpl_tools.h"
#include "cpl_error_impl.h"
#include "cpl_propertylist_impl.h"
#include "cpl_memory.h"
#include "cpl_imagelist_io.h"
#include "cpl_image_io_impl.h"
#include "cpl_io_fits.h"

#include "cpl_imagelist_defs.h"

/* Verify self-sufficiency of CPL header files by including system files last */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <fitsio.h>

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#define CPL_MSG  "Imagelist with %d image(s)\n"
#define CPL_IMSG "Image nb %d of %d in imagelist\n"

static
cpl_imagelist * cpl_imagelist_load_one(const char *, cpl_type, cpl_size,
                                       cpl_boolean, cpl_size, cpl_size,
                                       cpl_size, cpl_size) CPL_ATTR_ALLOC;

/**@{*/

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Create an empty imagelist
  @return   1 newly allocated cpl_imagelist
  @see      cpl_imagelist_set()

  The returned cpl_imagelist must be deallocated using cpl_imagelist_delete()

 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * cpl_imagelist_new(void)
{
    return (cpl_imagelist *) cpl_calloc(1, sizeof(cpl_imagelist));
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Load a FITS file extension into a list of images
  @param    filename   The FITS file name
  @param    im_type    Type of the images in the created image list
  @param    xtnum      The extension number (0 for primary HDU)
  @return   The loaded list of images or NULL on error.
  @see      cpl_image_load()

  This function loads all the images of a specified extension (NAXIS=2
  or 3) into an image list.

  Type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or CPL_TYPE_INT.
  The loaded images have an empty bad pixel map.

  The returned cpl_imagelist must be deallocated using cpl_imagelist_delete()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xtnum is negative
  - CPL_ERROR_INVALID_TYPE if the passed type is not supported
  - CPL_ERROR_FILE_IO If the file cannot be opened or read, or if xtnum is
                      bigger than the number of extensions in the FITS file
  - CPL_ERROR_BAD_FILE_FORMAT if the file cannot be parsed
  - CPL_ERROR_DATA_NOT_FOUND if the data cannot be read from the file
 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * cpl_imagelist_load(const char * filename,
                                   cpl_type     im_type,
                                   cpl_size     xtnum)
{
    cpl_imagelist * self = cpl_imagelist_load_one(filename, im_type, xtnum,
                                                  CPL_FALSE, 0, 0, 0, 0);

    if (self == NULL) cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief  Load images windows from a FITS file extension into an image list
  @param  filename    The FITS file name
  @param  im_type     Type of the images in the created image list
  @param  xtnum       The extension number (0 for primary HDU)
  @param  llx         Lower left  x position (FITS convention, 1 for leftmost)
  @param  lly         Lower left  y position (FITS convention, 1 for lowest)
  @param  urx         Upper right x position (FITS convention)
  @param  ury         Upper right y position (FITS convention)
  @return The loaded list of image windows or NULL on error.
  @see      cpl_imagelist_load(), cpl_image_load_window()
  @note The returned cpl_imagelist must be deallocated using
        cpl_imagelist_delete()

  This function loads all the image windows of a specified extension in an
  image list.

  Type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or CPL_TYPE_INT.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xtnum is negative
  - CPL_ERROR_INVALID_TYPE if the passed type is not supported
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if xtnum is bigger than the number of
    extensions in the FITS file
  - CPL_ERROR_BAD_FILE_FORMAT if the file cannot be parsed
  - CPL_ERROR_DATA_NOT_FOUND if the data cannot be read from the file
 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * cpl_imagelist_load_window(const char * filename,
                                          cpl_type     im_type,
                                          cpl_size     xtnum,
                                          cpl_size     llx,
                                          cpl_size     lly,
                                          cpl_size     urx,
                                          cpl_size     ury)

{
    cpl_imagelist * self = cpl_imagelist_load_one(filename, im_type, xtnum,
                                                  CPL_TRUE, llx, lly, urx, ury);

    if (self == NULL) cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Get the number of images in the imagelist
  @param    imlist    the list of image
  @return   The number of images or -1 on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_imagelist_get_size(const cpl_imagelist * imlist)
{
    cpl_ensure(imlist != NULL, CPL_ERROR_NULL_INPUT,
               -1);

    assert( imlist->ni >= 0 );

    return imlist->ni;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Get an image from a list of images
  @param    imlist    the image list
  @param    inum    the image id (from 0 to number of images-1)
  @return   A pointer to the image or NULL in error case.

  The returned pointer refers to already allocated data.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if inum is bigger thant the list size
  - CPL_ERROR_ILLEGAL_INPUT if inum is negative
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_imagelist_get(cpl_imagelist * imlist,
                              cpl_size        inum)
{

    cpl_ensure(imlist != NULL,    CPL_ERROR_NULL_INPUT,          NULL);
    cpl_ensure(inum >= 0,         CPL_ERROR_ILLEGAL_INPUT,       NULL);
    cpl_ensure(inum < imlist->ni, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);

    return imlist->images[inum];
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Get an image from a list of images
  @param    imlist    the image list
  @param    inum    the image id (from 0 to number of images-1)
  @return   A pointer to the image or NULL in error case.
  @see cpl_imagelist_get
 */
/*----------------------------------------------------------------------------*/
const cpl_image * cpl_imagelist_get_const(const cpl_imagelist * imlist,
                                          cpl_size              inum)
{
    cpl_ensure(imlist != NULL,    CPL_ERROR_NULL_INPUT,          NULL);
    cpl_ensure(inum >= 0,         CPL_ERROR_ILLEGAL_INPUT,       NULL);
    cpl_ensure(inum < imlist->ni, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);

    return imlist->images[inum];
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Insert an image into an imagelist
  @param    self   The imagelist
  @param    im     The image to insert
  @param    pos    The list position (from 0 to number of images)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  It is allowed to specify the position equal to the number of images in the
  list. This will increment the size of the imagelist.

  No action occurs if an image is inserted more than once into the same
  position. It is allowed to insert the same image into two different
  positions in a list.

  The image is inserted at the position pos in the image list. If the image
  already there is only present in that one location in the list, then the
  image is deallocated.

  It is not allowed to insert images of different size into a list.

  The added image is owned by the imagelist object, which deallocates it when
  cpl_imagelist_delete is called. Another option is to use cpl_imagelist_unset
  to recover ownership of the image, in which case the cpl_imagelist object is
  not longer responsible for deallocating it.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pos is negative
  - CPL_ERROR_TYPE_MISMATCH if im and self are of different types
  - CPL_ERROR_INCOMPATIBLE_INPUT if im and self have different sizes
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if pos is bigger than the number of
    images in self
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_set(cpl_imagelist * self,
                                 cpl_image     * im,
                                 cpl_size        pos)
{

    cpl_ensure_code(self != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im   != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pos  >= 0,        CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(pos  <= self->ni, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    /* Do nothing if the image is already there */
    if (pos < self->ni && im == self->images[pos]) return CPL_ERROR_NONE;

    if (pos > 0 || self->ni > 1) {
        /* Require images to have the same size and type */
        cpl_ensure_code(cpl_image_get_size_x(im) ==
                        cpl_image_get_size_x(self->images[0]),
                        CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(cpl_image_get_size_y(im) ==
                        cpl_image_get_size_y(self->images[0]),
                        CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(cpl_image_get_type(im) ==
                        cpl_image_get_type(self->images[0]),
                        CPL_ERROR_TYPE_MISMATCH);
    }

    if (pos == self->ni) {
        self->ni++;
        self->images = cpl_realloc(self->images,
                                     (size_t)self->ni * sizeof(cpl_image*));
    } else {
        /* Check if the image at the position to be overwritten
           is present in only one position */
        int i;

        for (i = 0; i < self->ni; i++) {
            if (i != pos && self->images[i] == self->images[pos]) break;
        }
        if (i == self->ni) {
            /* The image at the position to be overwritten
               is present in only one position, so delete it */

            /* But first we must check whether its bpm is shared with others */
            const cpl_mask * bpm = cpl_image_get_bpm_const(self->images[pos]);

            if (bpm != NULL) {
                for (i = 0; i < self->ni; i++) {
                    if (i != pos &&
                        bpm == cpl_image_get_bpm_const(self->images[i]))
                        break;
                }
                if (i < self->ni) {
                    /* The same bad pixel map is used also by the ith image */
                    (void)cpl_image_unset_bpm(self->images[pos]);
                }
            }

            cpl_image_delete(self->images[pos]);
        }
    }

    self->images[pos] = im;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Remove an image from an imagelist
  @param    self   The imagelist
  @param    pos      The list position (from 0 to number of images-1)
  @return   The pointer to the removed image or NULL in error case

  The specified image is not deallocated, it is simply removed from the
  list. The pointer to the image is returned to let the user decide to
  deallocate it or not.
  Eventually, the image will have to be deallocated with cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pos is negative
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if pos is bigger than the number of
    images in self
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_imagelist_unset(cpl_imagelist * self,
                                cpl_size        pos)
{
    cpl_image * out;

    cpl_ensure(self != NULL,     CPL_ERROR_NULL_INPUT,          NULL);
    cpl_ensure(pos  >= 0,        CPL_ERROR_ILLEGAL_INPUT,       NULL);
    cpl_ensure(pos  <  self->ni, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);

    /* Get pointer to image to be removed */
    out = self->images[pos];

    /* Decrement of the size */
    self->ni--;

    if (pos < self->ni) {
        /* Move the following images one position towards zero */
        (void)memmove(self->images + (size_t)pos,
                      self->images + (size_t)pos + 1,
                      sizeof(cpl_image*) * (size_t)(self->ni - pos));
    }

    return out;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Empty an imagelist and deallocate all its images
  @param    self  The image list or NULL
  @return   Nothing
  @see  cpl_imagelist_empty(), cpl_imagelist_delete()
  @note If @em self is @c NULL nothing is done and no error is set.

  After the call the image list can be populated again. It must eventually
  be deallocted with a call to cpl_imagelist_delete().

 */
/*----------------------------------------------------------------------------*/
void cpl_imagelist_empty(cpl_imagelist * self)
{

    if (self != NULL) {

        while (self->ni > 0) { /* An iteration may unset more than 1 image */
            cpl_size i = self->ni - 1;
            cpl_image * del = cpl_imagelist_unset(self, i);
            const cpl_mask * bpm = cpl_image_get_bpm_const(del);

            cpl_image_delete(del);

            /* If this image was inserted more than once into the list,
               the other insertions must be unset without a delete. */
            while (--i >= 0) {
                if (bpm != NULL &&
                    bpm == cpl_image_get_bpm_const(self->images[i])) {
                    /* The same bad pixel map is used also by this image */
                    const cpl_mask * samebpm
                        = cpl_image_unset_bpm(self->images[i]);
                    assert(samebpm == bpm);
                }
               if (self->images[i] == del) {
                    /* This image was inserted more than once in the list */
                   const cpl_image * sameimg = cpl_imagelist_unset(self, i);
                   assert(sameimg == del);
                }
            }
        }
    }

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Free memory used by a cpl_imagelist object, except the images
  @param    self    The image list or NULL
  @return   Nothing
  @see cpl_imagelist_empty()
  @note The caller must have pointers to all images in the list and is
        reponsible for their deallocation. If @em self is @c NULL nothing is
        done and no error is set.
 */
/*----------------------------------------------------------------------------*/
void cpl_imagelist_unwrap(cpl_imagelist * self)
{

    if (self != NULL) {

        cpl_free(self->images);
        cpl_free(self);
    }

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Free all memory used by a cpl_imagelist object including the images
  @param    self    The image list or NULL
  @return   Nothing
  @see      cpl_imagelist_empty(), cpl_imagelist_unwrap()

 */
/*----------------------------------------------------------------------------*/
void cpl_imagelist_delete(cpl_imagelist * self)
{
    cpl_imagelist_empty(self);
    cpl_imagelist_unwrap(self);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Cast an imagelist, optionally in-place
  @param    self   Destination imagelist
  @param    other  Source imagelist, or NULL to cast in-place
  @param    type   If called with empty self, cast to this pixel-type
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_image_cast()
  @note    If called with a non-empty self in an out-of-place cast,
  the input images are cast to the type already present in self and appended to
  the output list. In this case the parameter type is ignored.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the destination pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the same pointer is passed twice
  - CPL_ERROR_ILLEGAL_INPUT if the passed type is invalid
  - CPL_ERROR_TYPE_MISMATCH if the passed image type is complex and requested
  casting type is non-complex.
  - CPL_ERROR_INVALID_TYPE if the passed pixel type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_cast(cpl_imagelist * self,
                                  const cpl_imagelist * other,
                                  cpl_type type)
{

    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    } else if (self == other) {
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    } else {
        const cpl_boolean     inplace = other == NULL ? CPL_TRUE : CPL_FALSE;
        const cpl_imagelist * src     = inplace ? self : other;
        cpl_imagelist       * dest    = inplace ? cpl_imagelist_new() : self;
        const size_t          n       = (size_t)cpl_imagelist_get_size(src);
        const size_t          m       = (size_t)cpl_imagelist_get_size(dest);
        const cpl_type        ctype   = inplace || m == 0 ? type
            : cpl_image_get_type(cpl_imagelist_get_const(dest, 0));
        cpl_error_code        code    = CPL_ERROR_NONE;
        size_t                i, j;

        for (i = j = 0; i < n; i++) {

            /* For in-place, empty src along the way */
            const cpl_image * srcimg = cpl_imagelist_get_const(src,(cpl_size)j);
            cpl_image       * tmpimg = cpl_image_cast(srcimg, ctype);

            if (tmpimg == NULL) break;

            if (cpl_imagelist_set(dest, tmpimg, m + (cpl_size)i)) break;

            if (inplace) {
                cpl_size k;
                cpl_image* del = cpl_imagelist_unset(self, 0);
                const cpl_mask * bpm = cpl_image_get_bpm_const(del);

                /* If this image was inserted more than once into the list,
                   it will be deleted in a subsequent iteration */
                for (k = 0; k < self->ni; k++) {
                    if (bpm != NULL &&
                        bpm == cpl_image_get_bpm_const(self->images[k])) {
                        /* The same bad pixel map is used also by this image */
                        (void)cpl_image_unset_bpm(del);
                    }
                    if (self->images[k] == del) {
                        /* This image was inserted more than once in the list */
                        break;
                    }
                }
                if (k == self->ni)
                    cpl_image_delete(del);
            } else {
                j++;
            }
        }

        if (i < n) {
            /* An error happened */
            code = cpl_error_set_where_();
            if (inplace) {
                cpl_imagelist_delete(dest);
            }
        } else if (inplace) {
            /* Need to move dest to self (which is empty) */
            cpl_imagelist * tmp = (cpl_imagelist*)cpl_malloc(sizeof(*tmp));

            (void)memcpy(tmp,  self, sizeof(*tmp));
            (void)memcpy(self, dest, sizeof(*tmp));

            cpl_free(dest);
            cpl_imagelist_delete(tmp);
        }

        return code;
    }
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Copy an image list
  @param    imlist    Source image list.
  @return   1 newly allocated image list, or NULL on error.

  Copy an image list into a new image list object.
  The returned image list must be deallocated using cpl_imagelist_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * cpl_imagelist_duplicate(const cpl_imagelist * imlist)
{
    cpl_imagelist *   out;
    cpl_size          i;

    cpl_ensure(imlist != NULL, CPL_ERROR_NULL_INPUT, NULL);

    /* Create the new imagelist */
    out = cpl_imagelist_new();

    /* Duplicate the images */
    for (i=0; i<imlist->ni; i++) {
        cpl_imagelist_set(out, cpl_image_duplicate(imlist->images[i]), i);
    }

    return out;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Reject one or more images in a list according to an array of flags.
  @param    imlist  Non-empty imagelist to examine for image rejection.
  @param    valid   Vector of flags (>=-0.5: valid, <-0.5: invalid)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  This function takes an imagelist and a vector of flags. The imagelist and
  vector must have equal lengths.

  Images flagged as invalid are removed from the list.

  The removal of image(s) will reduce the length of the list accordingly.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the vector size and the image list
    size are different
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_erase(
        cpl_imagelist    * imlist,
        const cpl_vector * valid)
{
    cpl_size     nkeep = 0;
    cpl_size     i;

    /* Check entries */
    cpl_ensure_code(imlist, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(valid,  CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cpl_vector_get_size(valid) == imlist->ni,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    for (i=0; i < imlist->ni; i++) {
        if (cpl_vector_get(valid, i) >= -0.5) {
            /* image is to be kept, place it in the 1st free position */
            imlist->images[nkeep] = imlist->images[i];
            nkeep++;
        } else {
            /* image is to be erased, delete it */
            cpl_size k;
            const cpl_mask * bpm = cpl_image_get_bpm_const(imlist->images[i]);

            /* If this image was inserted more than once into the list,
               it will be deleted in a subsequent iteration */
            for (k = 0; k < imlist->ni; k++) {
                if (k < nkeep || i < k) {
                    if (bpm != NULL &&
                        bpm == cpl_image_get_bpm_const(imlist->images[k])) {
                        /* The same bad pixel map is used also by this image */
                        (void)cpl_image_unset_bpm(imlist->images[i]);
                    }
                    if (imlist->images[k] == imlist->images[i]) {
                        /* This image was inserted more than once in the list */
                        break;
                    }
                }
            }
            if (k == imlist->ni)
                cpl_image_delete(imlist->images[i]);
        }
    }

    /* Update the size of the altered list */
    imlist->ni = nkeep;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Save an imagelist to disk in FITS format.
  @param    self      Imagelist to save
  @param    filename  Name of the FITS file to write
  @param    type      The type used to represent the data in the file
  @param    pl        Property list for the output header or NULL
  @param    mode      The desired output options (combined with bitwise or)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see cpl_image_save()

  This function saves an image list to a FITS file. If a
  property list is provided, it is written to the named file before the
  pixels are written.

  Supported image lists types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT.

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

  Supported output modes are CPL_IO_CREATE (create a new file),
  CPL_IO_EXTEND (extend an existing file with a new extension) and
  CPL_IO_APPEND (append a list of images to the last data unit, which
  must already contain compatible image(s)).

  For the CPL_IO_APPEND mode it is recommended to pass a NULL pointer for the
  output header, since updating the already existing header incurs significant
  overhead.

  When the data written to disk are of an integer type, the output mode
  CPL_IO_EXTEND can be combined (via bit-wise or) with an option for
  tile-compression. This compression of integer data is lossless.
  The options are:
  CPL_IO_COMPRESS_GZIP, CPL_IO_COMPRESS_RICE, CPL_IO_COMPRESS_HCOMPRESS,
  CPL_IO_COMPRESS_PLIO.
  With compression the type must be CPL_TYPE_UNSPECIFIED or CPL_TYPE_INT.
 
  In extend and append mode, make sure that the file has write permissions.
  You may have problems if you create a file in your application and append
  something to it with the umask set to 222. In this case, the file created
  by your application would not be writable, and the append would fail.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the type or the mode is not supported
  - CPL_ERROR_FILE_IO if the file cannot be written
  - CPL_ERROR_INVALID_TYPE if the passed image list type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_save(const cpl_imagelist    * self,
                                  const char             * filename,
                                  cpl_type                 type,
                                  const cpl_propertylist * pl,
                                  unsigned                 mode)
{

    cpl_ensure_code(self     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(filename != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cpl_imagelist_is_uniform(self)==0,
                    CPL_ERROR_ILLEGAL_INPUT);

    return cpl_image_save_((const cpl_image* const*)self->images, self->ni,
                           CPL_TRUE, filename, type, pl, mode)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Determine if an imagelist contains images of equal size and type
  @param    imlist    The imagelist to check
  @return   Zero if uniform, positive if non-uniform and negative on error.

  The function returns 1 if the list is empty.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
int cpl_imagelist_is_uniform(const cpl_imagelist * imlist)
{
    cpl_type     type;
    cpl_size     nx, ny;
    cpl_size     i;

    cpl_ensure(imlist != NULL, CPL_ERROR_NULL_INPUT, -1);
    if (imlist->ni == 0) return 1;

    /* Check the images */
    nx   = cpl_image_get_size_x(imlist->images[0]);
    ny   = cpl_image_get_size_y(imlist->images[0]);
    type = cpl_image_get_type  (imlist->images[0]);

    for (i=1; i < imlist->ni; i++) {
        if (cpl_image_get_size_x(imlist->images[i]) != nx)   return (int)i+1;
        if (cpl_image_get_size_y(imlist->images[i]) != ny)   return (int)i+1;
        if (cpl_image_get_type  (imlist->images[i]) != type) return (int)i+1;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Dump structural information of images in an imagelist
  @param    self    Imagelist to dump
  @param    stream  Output stream, accepts @c stdout or @c stderr
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if a write operation fails
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_dump_structure(const cpl_imagelist * self,
                        FILE * stream)
{
    const int    msgmin = (int)strlen(CPL_MSG) - 5;

    int i;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(stream != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code( fprintf(stream,  CPL_MSG, (int)self->ni) >= msgmin,
             CPL_ERROR_FILE_IO );

    for (i = 0; i < self -> ni; i++) {
    const cpl_image * image   = cpl_imagelist_get_const(self, i);
    const int         imsgmin = (int)strlen(CPL_IMSG) - 5;

    cpl_ensure_code( fprintf(stream,  CPL_IMSG, i, (int)self->ni) >= imsgmin,
             CPL_ERROR_FILE_IO );

    cpl_ensure_code( !cpl_image_dump_structure(image, stream),
             cpl_error_get_code() );
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Dump pixel values of images in a CPL imagelist
  @param    self    Imagelist to dump
  @param    llx     Specifies the window position
  @param    lly     Specifies the window position
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
cpl_error_code cpl_imagelist_dump_window(const cpl_imagelist * self,
                                         cpl_size llx, cpl_size lly,
                                         cpl_size urx, cpl_size ury,
                                         FILE * stream)
{
    cpl_size i;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(stream != NULL, CPL_ERROR_NULL_INPUT);

    for (i = 0; i < self -> ni; i++) {
    const cpl_image * image   = cpl_imagelist_get_const(self, i);
    const int         imsgmin = (int)strlen(CPL_IMSG) - 5;

    cpl_ensure_code( fprintf(stream,  CPL_IMSG, (int)i,
                             (int)self->ni) >= imsgmin, CPL_ERROR_FILE_IO );

    cpl_ensure_code( !cpl_image_dump_window(image, llx, lly, urx, ury,
                           stream),
             cpl_error_get_code() );
    }

    return CPL_ERROR_NONE;
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_imagelist
  @brief  Load an imagelist from a FITS file.
  @param  filename    Name of the file to load from.
  @param  im_type     Type of the created images
  @param  xtnum       Extension number in the file (0 for primary HDU)
  @param  do_window   True for (and only for) a windowed load
  @param  llx         Lower left  x position (FITS convention, 1 for leftmost)
  @param  lly         Lower left  y position (FITS convention, 1 for lowest)
  @param  urx         Upper right x position (FITS convention)
  @param  ury         Upper right y position (FITS convention)
  @return 1 newly allocated imagelist or NULL on error
  @see cpl_imagelist_load()
*/
/*----------------------------------------------------------------------------*/
static cpl_imagelist * cpl_imagelist_load_one(const char * filename,
                                              cpl_type     im_type,
                                              cpl_size     xtnum,
                                              cpl_boolean  do_window,
                                              cpl_size     llx,
                                              cpl_size     lly,
                                              cpl_size     urx,
                                              cpl_size     ury)
{

    /* Count number of images read - use also to indicate failure */
    cpl_size        selfsize;
    cpl_imagelist * self;
    cpl_image     * image;
    fitsfile      * fptr;
    int             status = 0; /* CFITSIO status, must be set to zero */
    /* Initialize to indicate that they need to be read from the file */
    int             naxis = 0;
    CPL_FITSIO_TYPE naxes[3] ={0, 0, 0};
    cpl_type        pix_type = im_type;

    /* FIXME: Version 3.2 of fits_open_diskfile() seg-faults on NULL.
       If fixed in CFITSIO, this check should be removed */
    cpl_ensure(filename != NULL, CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(xtnum    >= 0,    CPL_ERROR_ILLEGAL_INPUT, NULL);

    if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &status)) {
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, status, fits_open_diskfile,
                                 "filename='%s', im_type=%u, xtnum=%"
                                 CPL_SIZE_FORMAT, filename, im_type, xtnum);
        return NULL;
    }

    /* Load 1st image from the extension. This will set naxis and naxes[]
       (and optionally the pixel type) for use in subsequent calls */
    image = cpl_image_load_(fptr, &naxis, naxes, &pix_type, filename, 0, xtnum,
                            do_window, llx, lly, urx, ury);

    self = cpl_imagelist_new();
    selfsize = 0;

    if (image == NULL || cpl_imagelist_set(self, image, selfsize)) {
        cpl_image_delete(image);
    } else {
        selfsize++;
    }

    if (selfsize > 0 && naxis == 3) {
        /* Handle other planes in this extension, if any */
        int iplane;

        for (iplane = 1; iplane < naxes[2]; iplane++) {
            image = cpl_image_load_(fptr, &naxis, naxes, &pix_type,
                                    filename, iplane, xtnum,
                                    do_window, llx, lly, urx, ury);
            if (image == NULL) break;

            if (cpl_imagelist_set(self, image, selfsize)) {
                cpl_image_delete(image);
                break;
            }

            selfsize++;
        }
        if (iplane < naxes[2]) {
            selfsize = 0; /* Indicate failure */
        }
    }

    if (cpl_io_fits_close_file(fptr, &status)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status,
                                 fits_close_file, "filename='%s', "
                                 "im_type=%u, xtnum=%" CPL_SIZE_FORMAT,
                                 filename, (unsigned)im_type, xtnum);
        selfsize = 0; /* Indicate failure */
    } else if (selfsize == 0) {
        (void)cpl_error_set_where_();
    }

    if (selfsize == 0) {
        cpl_imagelist_delete(self);
        self = NULL;
    }

    return self;
}
