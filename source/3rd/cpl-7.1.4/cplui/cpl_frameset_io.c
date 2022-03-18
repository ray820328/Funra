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

#include "cpl_frameset_io.h"
#include "cpl_frame.h"
#include "cpl_frame_impl.h"

#include "cpl_imagelist_io.h"
#include "cpl_image_io_impl.h"
#include "cpl_errorstate.h"
#include "cpl_error_impl.h"
#include "cpl_io_fits.h"

#include <cxmessages.h>
#include <cxutils.h>

#include <stdio.h>
#include <string.h>

#include <fitsio.h>



/**
 * @defgroup cpl_frameset_io    Frame Sets IO functions
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_frameset_io.h>
 * @endcode
 */

/**@{*/

/*
 *  Private functions
 */


static cpl_error_code
cpl_imagelist_append_from_file(cpl_imagelist *, cpl_size *, const char *,
                               cpl_type, cpl_size, cpl_size);

/*----------------------------------------------------------------------------*/
/**
  @brief    Load an imagelist from a frameset
  @param    fset     The frames set
  @param    im_type  The required image type
  @param    pnum     The plane number, 1 for first, 0 for all planes
  @param    xtnum    The extension number, 0 for primary, n for nth, -1 for all
  @return   the loaded list of images or NULL on error.
  @note The returned cpl_imagelist must be deallocated using cpl_imagelist_delete()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if fset is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pnum is negative, xtnum is lower than -1
        or one image cannot be loaded as specified
 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * cpl_imagelist_load_frameset(const cpl_frameset * fset,
                                            cpl_type             im_type,
                                            cpl_size             pnum, 
                                            cpl_size             xtnum) 
{
    cpl_imagelist   * self;
    const cpl_frame * cur_frame;
    cpl_size          selfsize;


    /* Test entries */
    cpl_ensure(fset  != NULL, CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(pnum  >= 0,    CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(xtnum >= -1,   CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Create empty output image list */
    self = cpl_imagelist_new();
    selfsize = 0;

    cpl_frameset_iterator *it = cpl_frameset_iterator_new(fset);

    while ((cur_frame = cpl_frameset_iterator_get_const(it)) != NULL) {
        const char * filename = cpl_frame_get_filename(cur_frame);

        cpl_errorstate prestate;

        if (xtnum < 0) {
            /* Image(s) from all extensions requested.
             */

            int            nextensions, ixtnum;
            const cpl_size presize = selfsize;

            fitsfile     * fptr;
            int            error = 0;

            prestate = cpl_errorstate_get();

            /* Initialize to indicate that they need to be read from the file */
            int             naxis = 0;
            CPL_FITSIO_TYPE naxes[3] ={0, 0, 0};
            cpl_type        pix_type = im_type;

            /* FIXME: Version 3.2 of fits_open_diskfile() seg-faults on NULL.
               If fixed in CFITSIO, this check should be removed */

            if (filename == NULL) {
                cpl_frameset_iterator_delete(it);
                cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
                return NULL;
            }

            if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &error)) {
                (void)cpl_error_set_fits(CPL_ERROR_ILLEGAL_INPUT, error,
                                         fits_open_diskfile, "filename='%s', "
                                         "im_type=%u, pnum=%" CPL_SIZE_FORMAT
                                         ", xtnum=%" CPL_SIZE_FORMAT, filename,
                                         (unsigned)im_type, pnum, xtnum);
                break;
            }

            /* Get the number of extensions (the primary HDU counts as one) */
            if (fits_get_num_hdus(fptr, &nextensions, &error)) {
                (void)cpl_error_set_fits(CPL_ERROR_ILLEGAL_INPUT, error,
                                         fits_get_num_hdus, "filename='%s', "
                                         "im_type=%u, pnum=%" CPL_SIZE_FORMAT
                                         ", xtnum=%" CPL_SIZE_FORMAT, filename,
                                         (unsigned)im_type, pnum, xtnum);
                /* Ensure that the file is closed below */
                error = nextensions = 0;
            }

            for (ixtnum = 0; ixtnum < nextensions; ixtnum++) {
                /* If all planes are to be read, start with the first one */
                cpl_size iplane = pnum ? pnum - 1 : pnum;

                /* Load 1 image from the extension. This will set naxis and
                   naxes[] (and optionally the pixel type) for use in
                   subsequent calls */
                cpl_image * image = cpl_image_load_(fptr, &naxis, naxes,
                                                    &pix_type, filename,
                                                    iplane, ixtnum,
                                                    CPL_FALSE, 0, 0, 0, 0);

                if (!cpl_errorstate_is_equal(prestate)) {
                    if (cpl_error_get_code() == CPL_ERROR_DATA_NOT_FOUND
                        && ixtnum == 0) {
                        /* Main HDU allowed to not have image data */
                        cpl_errorstate_set(prestate);
                        continue;
                    }
                    break;
                }

                if (cpl_imagelist_set(self, image, selfsize)) {
                    cpl_image_delete(image);
                    break;
                }

                selfsize++;

                if (pnum == 0 && naxis == 3) {
                    /* Handle other planes in this extension, if any */
                    for (iplane = 1; iplane < naxes[2]; iplane++) {
                        image = cpl_image_load_(fptr, &naxis, naxes, &pix_type,
                                                filename, iplane, ixtnum,
                                                CPL_FALSE, 0, 0, 0, 0);
                        if (image == NULL) break;

                        if (cpl_imagelist_set(self, image, selfsize)) {
                            cpl_image_delete(image);
                            break;
                        }

                        selfsize++;
                    }
                    if (iplane < naxes[2]) break;
                }
            }

            if (cpl_io_fits_close_file(fptr, &error)) {
                (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                         fits_close_file, "filename='%s', "
                                         "im_type=%u, pnum=%" CPL_SIZE_FORMAT
                                         ", xtnum=%" CPL_SIZE_FORMAT, filename,
                                         (unsigned)im_type, pnum, xtnum);
            }

            if (ixtnum < nextensions || selfsize == presize) break;

        } else {
            if (cpl_imagelist_append_from_file(self, &selfsize, filename,
                                               im_type, pnum, xtnum)) break;
        }

        prestate = cpl_errorstate_get();

        cpl_frameset_iterator_advance(it, 1);

        if (!cpl_errorstate_is_equal(prestate)) {
            if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE) {
                cpl_errorstate_set(prestate);
            }
        }

    }


    if (cur_frame != NULL) {
        const char * filename = cpl_frame_get_filename(cur_frame);
        cpl_imagelist_delete(self);
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "file=%s, "
                                     "im_type=%u, pnum=%" CPL_SIZE_FORMAT
                                     ", xtnum=%" CPL_SIZE_FORMAT,
                                     filename ? filename : "<NULL>",
                                     (unsigned)im_type, pnum, xtnum);
        cpl_frameset_iterator_delete(it);
        return NULL;
    }

    cx_assert(cpl_imagelist_get_size(self) == selfsize);

    /* Require created imagelist to be non-empty */
    if (selfsize == 0) {
        const char * filename = cpl_frame_get_filename(cur_frame);
        cpl_imagelist_delete(self);
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "file=%s, "
                                     "im_type=%d, pnum=%" CPL_SIZE_FORMAT
                                     ", xtnum=%" CPL_SIZE_FORMAT,
                                     filename ? filename : "<NULL>",
                                     im_type, pnum, xtnum);
        cpl_frameset_iterator_delete(it);
        return NULL;
    }

    cpl_frameset_iterator_delete(it);

    return self;

}

/**@}*/


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Append the specified images to an imagelist from a file
  @param    self      The imagelist to append to
  @param    pselfsize Increase with number of images appended
  @param    filename  The file to read from
  @param    im_type     The required image type
  @param    pnum      The plane number, 1 for first, 0 for all planes
  @param    xtnum    The extension number, 0 for primary, n for nth, -1 for all
  @return   CPL_ERROR_NONE, or the relevant error code on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if fset is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pnum is negative, xtnum is lower than -1
        or one image cannot be loaded as specified
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_imagelist_append_from_file(cpl_imagelist * self,
                                                     cpl_size      * pselfsize,
                                                     const char    * filename,
                                                     cpl_type        im_type,
                                                     cpl_size        pnum, 
                                                     cpl_size        xtnum)
{

    cpl_size selfsize = cpl_imagelist_get_size(self);

    cpl_ensure_code(self      != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pselfsize != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(filename  != NULL, CPL_ERROR_NULL_INPUT);

    if (pnum == 0) {
        /* All planes required */
        cpl_imagelist * onelist = cpl_imagelist_load(filename, im_type,
                                                     xtnum);
        const cpl_size onesize = cpl_imagelist_get_size(onelist);
        cpl_size       j;

        cpl_ensure_code(onelist != NULL, CPL_ERROR_ILLEGAL_INPUT);

        /* Move the images from onelist to self */
        for (j = 0; j < onesize; j++) {
            cpl_image * img = cpl_imagelist_unset(onelist, 0);

            if (cpl_imagelist_set(self, img, selfsize)) {
                /* Type or size could differ */
                cpl_imagelist_delete(onelist);
                cpl_image_delete(img);
                return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            }
            selfsize++;
        }
        cx_assert(cpl_imagelist_get_size(onelist) == 0);
        cpl_imagelist_delete(onelist);
    } else {
        /* Only one plane required */
        cpl_image * img = cpl_image_load(filename, im_type, pnum-1, xtnum);
        cpl_ensure_code(img != NULL, CPL_ERROR_ILLEGAL_INPUT);
        if (cpl_imagelist_set(self, img, selfsize)) {
            /* Type or size could differ */
            cpl_image_delete(img);
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        selfsize++;
    }

    cx_assert(cpl_imagelist_get_size(self) == selfsize);

    *pselfsize = selfsize;

    return CPL_ERROR_NONE;

}
