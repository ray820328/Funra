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

#include <string.h>

#include "cpl_dfs.h"
#include "cpl_memory.h"
#include "cpl_test.h"
#include "cpl_image.h"

#include "cpl_io_fits.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define IMAGE_SIZE_X 1009
#define IMAGE_SIZE_Y 1031
#define IMAGE_NEXT   32
#define CPL_DFS_RAW_ASCII "ascii.txt"

#define CPL_DFS_FITSFILE "product.fits"

/*-----------------------------------------------------------------------------
                            Private declarations
 -----------------------------------------------------------------------------*/

static void cpl_dfs_product_tests(const char *);

static void cpl_dfs_save_tests(const char *);

static void cpl_dfs_save_txt(const char *);

static void cpl_dfs_parameterlist_fill(cpl_parameterlist *);

static void cpl_dfs_propertylist_append_dicb(cpl_propertylist *)
    CPL_ATTR_NONNULL;

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{

    cpl_propertylist * plist = NULL;
    cpl_error_code     code;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    cpl_dfs_save_txt(CPL_DFS_RAW_ASCII);

    plist = cpl_propertylist_new();

    /* This card is of floating point type, but make sure it is written
       as an integer, since that format is also valid */
    code = cpl_propertylist_append_int(plist, "EQUINOX", 2000);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_image_save(NULL, "inherit.fits",
           CPL_TYPE_FLOAT, plist, CPL_IO_CREATE);
    cpl_propertylist_delete(plist);

    /* Insert tests here */

    cpl_dfs_product_tests(CPL_DFS_RAW_ASCII);

    cpl_dfs_save_tests(CPL_DFS_RAW_ASCII);

    remove(CPL_DFS_RAW_ASCII);

    /* Testing finished */
    return cpl_test_end(0);

}


static void cpl_dfs_product_tests(const char * rawname)
{
    cpl_frame         * rawframe;
    cpl_frame         * inhframe;
    cpl_frame         * proframe;
    cpl_frame         * emptyframe;
    cpl_frameset      * frameset = NULL;
    cpl_propertylist  * plist = NULL;
    cpl_parameterlist * parlist = NULL;
    cpl_image         * image;
    const char        * md5sum;

    cpl_error_code      error;
    const int           next  = IMAGE_NEXT;
    int                 fstatus = 0; /* CFITSIO error status */
    int                 i;


    /* Test 1: Check the error handling of NULL-pointer(s) */
    error  = cpl_dfs_setup_product_header(NULL, NULL, NULL, NULL, 
                                          NULL, NULL, NULL, NULL);

    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT);

    /* Test 1a: Check the error handling of NULL-pointer(s) */
    error = cpl_dfs_update_product_header(NULL);

    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT);

    /* Test 2: Check the error handling of an empty frame/frameset */

    proframe = cpl_frame_new();
    frameset = cpl_frameset_new();
    plist = cpl_propertylist_new();
    parlist = cpl_parameterlist_new();


    /* Insert CHECKSUM and DATASUM (with wrong values) */
    error = cpl_propertylist_append_string(plist, "CHECKSUM",
                                           "BADAKIZUEUSKARAZ");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_propertylist_set_comment(plist, "CHECKSUM", "HDU checksum");
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_propertylist_append_string(plist, "DATASUM",
                                           "3141592653");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_propertylist_set_comment(plist, "DATASUM",
                                         "data unit checksum");
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_dfs_parameterlist_fill(parlist);

    error = cpl_dfs_setup_product_header(plist, proframe, frameset, parlist,
                                         "Recipe", "Pipeline", "PRO-1.16",
                                         NULL);

    cpl_test_eq_error( error, CPL_ERROR_DATA_NOT_FOUND);

    /* Test 2a: Check the handling of an empty frameset */

    error = cpl_dfs_update_product_header(frameset);

    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Test 3: Check the error handling of a rawframe without a filename */

    rawframe = cpl_frame_new();
    error = cpl_frame_set_tag(rawframe, "TAG");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_group(rawframe, CPL_FRAME_GROUP_RAW);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frameset_insert(frameset, rawframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_dfs_setup_product_header(plist, proframe, frameset, parlist,
                                         "Recipe", "Pipeline", "PRO-1.16",
                                         NULL);

    cpl_test_eq_error( error, CPL_ERROR_DATA_NOT_FOUND);
    cpl_frameset_delete(frameset);

    /* Test 4: Check the error handling of an empty product frame
       (and a valid, non-fits rawframe) - should fail on missing filename */

    frameset = cpl_frameset_new();
    rawframe = cpl_frame_new();
    error = cpl_frame_set_tag(rawframe, "TAG");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_group(rawframe, CPL_FRAME_GROUP_RAW);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_filename(rawframe, rawname);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frameset_insert(frameset, rawframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);


    error = cpl_dfs_setup_product_header(plist, proframe, frameset, parlist,
                                         "Recipe", "Pipeline", "PRO-1.16",
                                         NULL);

    cpl_test_eq_error( error, CPL_ERROR_DATA_NOT_FOUND);


    /* Test 4c: Check the error handling of an product frame with a (non-fits)
       filename as the only attribute (and a valid, non-fits rawframe)
       - should fail on missing tag */

    error = cpl_frame_set_filename(proframe, CPL_DFS_FITSFILE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_dfs_setup_product_header(plist, proframe, frameset, parlist,
                                         "Recipe", "Pipeline", "PRO-1.16",
                                         NULL);
    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT);

    /* Test 4d: Check the error handling of an product frame with a filename
       and tagged  (and a valid, non-fits rawframe) - should fail on missing
       group */

    error = cpl_frame_set_tag(proframe, "PRODUCT");
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_dfs_setup_product_header(plist, proframe, frameset, parlist,
                                         "Recipe", "Pipeline", "PRO-1.16",
                                         NULL);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    /* Set product group */
    error = cpl_frame_set_group(proframe, CPL_FRAME_GROUP_PRODUCT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Test 5c: Check the error handling of an inherit-frame not present in
       the frameset */

    error = cpl_dfs_setup_product_header(plist, proframe, frameset,
                                         parlist, "Recipe", "Pipeline",
                                         "PRO-1.16", proframe);

    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT);

    /* Test 5: 2 simple successful calls */

    error = cpl_dfs_setup_product_header(plist, proframe, frameset,
                                         parlist, "Recipe", "Pipeline",
                                         "PRO-1.16", NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_propertylist_has(plist, "CHECKSUM"));
    cpl_test_zero(cpl_propertylist_has(plist, "DATASUM"));

    cpl_msg_debug("","Size of product header: %" CPL_SIZE_FORMAT,
                  cpl_propertylist_get_size(plist));

    inhframe = cpl_frame_new();
    error = cpl_frame_set_tag(inhframe, "TAG");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_group(inhframe, CPL_FRAME_GROUP_RAW);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_filename(inhframe, "inherit.fits");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frameset_insert(frameset, inhframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* This card is of floating point type, but make sure it is written
       as an integer, since that format is also valid */
    cpl_propertylist_append_int(plist, "EQUINOX", 2000);

    error = cpl_dfs_setup_product_header(plist, proframe, frameset,
                                         parlist, "Recipe", "Pipeline",
                                         "PRO-1.16", inhframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_propertylist_has(plist, "CHECKSUM"));
    cpl_test_zero(cpl_propertylist_has(plist, "DATASUM"));

    cpl_msg_debug("","Size of product header: %" CPL_SIZE_FORMAT,
                  cpl_propertylist_get_size(plist));

    cpl_propertylist_append_char(plist, "ESO QC MYCHAR", 42);
    cpl_propertylist_set_comment(plist, "ESO QC MYCHAR", "42");

    cpl_propertylist_append_string(plist, "ARCFILE",
                                   "IIINSTRUME.2013-08-28T12:50:43.3381");

    error = cpl_dfs_save_paf("IIINSTRUME", "RRRECIPE", plist,
                             "cpl_dfs-test.paf");
    cpl_test_eq_error(error, CPL_ERROR_NONE);

#if 0
    /*
     * FIXME: It fails if inherit_frame is an ASCII file, what is 
     * allowed for the first frame. See below: rawframe points to the
     * first frame in frameset.
     */
    cpl_test_zero(cpl_dfs_setup_product_header(plist, proframe, frameset,
                                               parlist, "Recipe", "Pipeline",
                                               "PRO-1.16", rawframe));

    cpl_msg_debug("","Size of product header: %" CPL_SIZE_FORMAT,
                  cpl_propertylist_get_size(plist));
#endif
    /* Test 5a: A failure due to a missing product file */

    error = cpl_frameset_insert(frameset, proframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_dfs_update_product_header(frameset);
    cpl_test_eq_error( error, CPL_ERROR_BAD_FILE_FORMAT);

    /* Test 5b: Check the error handling of an inherit-frame present in
       the frameset, but not present on the filesystem */

    error = cpl_dfs_setup_product_header(plist, proframe, frameset,
                                         parlist, "Recipe", "Pipeline",
                                         "PRO-1.16", proframe);

    cpl_test_eq_error( error, CPL_ERROR_FILE_NOT_FOUND);
    
    /* Test 5c: Check error handling when given an empty frame with no
       filename set. */
    emptyframe = cpl_frame_new();
    error = cpl_dfs_setup_product_header(plist, proframe, frameset,
                                         parlist, "Recipe", "Pipeline",
                                         "PRO-1.16", emptyframe);
    cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);
    cpl_frame_delete(emptyframe);

    /* Test 6a: A failure on file format (missing file) */

    /* Need to close file due to non CPL access */
    cpl_test_zero(cpl_io_fits_close(CPL_DFS_FITSFILE, &fstatus));
    cpl_test_zero(fstatus);

    /* Make sure file does not exist (from a previously failed test) */
    (void)remove(CPL_DFS_FITSFILE);

    error = cpl_frame_set_type(proframe, CPL_FRAME_TYPE_IMAGE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_dfs_update_product_header(frameset);

    cpl_test_eq_error( error, CPL_ERROR_BAD_FILE_FORMAT);


    /* Test 6a: Also support non-fits */

    cpl_dfs_save_txt(CPL_DFS_FITSFILE);

    error = cpl_dfs_update_product_header(frameset);

    cpl_test_error(error);

    /* Test 7a: A successful call of cpl_dfs_update_product_header()
       with an empty data-unit */

    /* This card is of floating point type, but make sure it is written
       as an integer, since that format is also valid */
    cpl_propertylist_append_int(plist, "EQUINOX", 2000);

    cpl_test_zero(cpl_image_save(NULL, CPL_DFS_FITSFILE, CPL_TYPE_UCHAR,
                                 plist, CPL_IO_CREATE));
    cpl_test_fits(CPL_DFS_FITSFILE);

    error = cpl_dfs_update_product_header(frameset);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(CPL_DFS_FITSFILE);

    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(CPL_DFS_FITSFILE, 0);

    cpl_test_nonnull(plist);

    cpl_test(cpl_propertylist_has(plist, "EQUINOX"));

    md5sum = cpl_propertylist_get_string(plist, "DATAMD5");

    cpl_test_nonnull(md5sum);

    /* The created Data Unit is empty */
    /* The corresponding 32-byte reference MD5 sum is found with:
       md5sum - < /dev/null
    */
    cpl_test_eq_string(md5sum, "d41d8cd98f00b204e9800998ecf8427e");

    /* Test 7b: A successful call of cpl_dfs_update_product_header() */

    image = cpl_image_new(2, 3, CPL_TYPE_INT);

    /* This card is of floating point type, but make sure it is written
       as an integer, since that format is also valid */
    cpl_propertylist_append_int(plist, "EQUINOX", 2000);

    cpl_test(cpl_propertylist_has(plist, "EQUINOX"));

    cpl_test_zero(cpl_image_save(image, CPL_DFS_FITSFILE, CPL_TYPE_UCHAR,
                                      plist, CPL_IO_CREATE));
    cpl_test_fits(CPL_DFS_FITSFILE);

    error = cpl_dfs_update_product_header(frameset);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(CPL_DFS_FITSFILE);

    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(CPL_DFS_FITSFILE, 0);

    cpl_test_nonnull(plist);

    md5sum = cpl_propertylist_get_string(plist, "DATAMD5");

    cpl_test_nonnull(md5sum);

    /* The created Data Unit consists of one FITS block of zero-bytes */
    /* The 32-byte reference MD5 sum of such a string is found with:
       perl -e 'print "\0" x 2880' | md5sum -
    */
    cpl_test_eq_string(md5sum, "b4a11922757e107a2c10306867f52601");

    /* Test 7c: A successful call of cpl_dfs_update_product_header()
                with a main HDU and one data-less extension */

    cpl_test_zero(cpl_image_save(NULL, CPL_DFS_FITSFILE, CPL_TYPE_UCHAR,
                                      plist, CPL_IO_EXTEND));
    cpl_test_fits(CPL_DFS_FITSFILE);

    error = cpl_dfs_update_product_header(frameset);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(CPL_DFS_FITSFILE);

    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(CPL_DFS_FITSFILE, 0);

    cpl_test_nonnull(plist);

    md5sum = cpl_propertylist_get_string(plist, "DATAMD5");

    cpl_test_nonnull(md5sum);

    /* The created Data Unit consists of one FITS block of zero-bytes */
    /* The 32-byte reference MD5 sum of such a string is found with:
       perl -e 'print "\0" x 2880' | md5sum -
    */
    cpl_test_eq_string(md5sum, "b4a11922757e107a2c10306867f52601");

    /* Test 7d: A successful call of cpl_dfs_update_product_header()
                with a main HDU, one data-less extension and
                one non-empty extension. */

    cpl_test_zero(cpl_image_save(image, CPL_DFS_FITSFILE, CPL_TYPE_UCHAR,
                                      plist, CPL_IO_EXTEND));
    cpl_test_fits(CPL_DFS_FITSFILE);

    error = cpl_dfs_update_product_header(frameset);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(CPL_DFS_FITSFILE);

    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(CPL_DFS_FITSFILE, 0);

    cpl_test_nonnull(plist);

    cpl_test(cpl_propertylist_has(plist, "EQUINOX"));

    md5sum = cpl_propertylist_get_string(plist, "DATAMD5");

    cpl_test_nonnull(md5sum);

    /* The created Data Unit consists of two FITS blocks of zero-bytes */
    /* The 32-byte reference MD5 sum of such a string is found with:
       perl -e 'print "\0" x (2880*2)' | md5sum -
    */
    cpl_test_eq_string(md5sum, "1c94f1009ff65c0a499040c0d1ae082f");

    cpl_image_delete(image);


    /* Test 8: A successful call of cpl_dfs_update_product_header()
       - on a multi-extension file of flat integer images*/

    image = cpl_image_new(IMAGE_SIZE_X, IMAGE_SIZE_Y, CPL_TYPE_INT);

    for (i = 0; i < next; i++) {

        error = cpl_image_add_scalar(image, 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_save(image, i % 2 ? CPL_DFS_FITSFILE
                               : "./" CPL_DFS_FITSFILE, i >= 255 ? CPL_TYPE_INT
                               : CPL_TYPE_UCHAR, plist, CPL_IO_EXTEND);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_fits(CPL_DFS_FITSFILE);

    }

    error = cpl_dfs_update_product_header(frameset);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(CPL_DFS_FITSFILE);

    cpl_propertylist_delete(plist);

    plist = cpl_propertylist_load(CPL_DFS_FITSFILE, 0);

    cpl_test_nonnull(plist);

    md5sum = cpl_propertylist_get_string(plist, "DATAMD5");

    cpl_test_nonnull(md5sum);

    /* The created Data Units are two FITS blocks of zero-bytes,
       and 32 each with a BITPIX-8 1009 by 1031 flat image with values
       1 through 32 */
    /* The 32-byte reference MD5 sum of such a string is found with:
       perl -e 'print "\0" x ( 2880*2);'
       -e 'grep(print(sprintf("%c",$_) x (1009*1031)."\0"x(1042560-1009*1031)),'
       -e '1..32)' | md5sum -
    */
#if defined IMAGE_NEXT && IMAGE_NEXT == 32
#if defined IMAGE_SIZE_X && IMAGE_SIZE_X == 1009
#if defined IMAGE_SIZE_Y && IMAGE_SIZE_X == 1031
    cpl_test_eq_string(md5sum, "503bcbb6fd6606ac1d035e2a43f0229c");
#endif
#endif
#endif

    cpl_image_delete(image);

    /* Test 9: A successful call of cpl_dfs_update_product_header()
       - after renaming the file */
    do {

        /* Renaming the file attribute of a frame belonging to a frameset
           is not allowed. This strange limitation requires a cumbersome
           work-around :-( */

        cpl_frameset * newframes = cpl_frameset_new();
        const char * newformat = "newname_%04d.fits";
        cpl_frame * newframe;

        cpl_frameset_iterator *it = cpl_frameset_iterator_new(frameset);
        const cpl_frame *frame = cpl_frameset_iterator_get_const(it);

        /*
         *  Create new frameset of products with new filenames
         *  - and rename product files
         */

        i = 0;
        while (frame != NULL) {
            cpl_errorstate status;

            if (cpl_frame_get_group(frame) == CPL_FRAME_GROUP_PRODUCT) {

                const cpl_frame_type type = cpl_frame_get_type(frame);

                if (type == CPL_FRAME_TYPE_TABLE ||
                    type == CPL_FRAME_TYPE_IMAGE) {
                    const char * oldname = cpl_frame_get_filename(frame);
                    char * newname = cpl_sprintf(newformat, ++i);

                    /* Rename product */
                    cpl_test_zero(rename(oldname, newname));

                    newframe = cpl_frame_duplicate(frame);

                    error = cpl_frame_set_filename(newframe, newname);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                    cpl_free(newname);

                    error = cpl_frameset_insert(newframes, newframe);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                }
            }

            status = cpl_errorstate_get();

            cpl_frameset_iterator_advance(it, 1);

            if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE) {
                cpl_errorstate_set(status);
            }

            frame = cpl_frameset_iterator_get_const(it);

        }

        cpl_frameset_iterator_delete(it);
        it = NULL;

        cpl_msg_info("", "Renamed %d product(s)", i);
        error = cpl_dfs_update_product_header(newframes);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_frameset_delete(newframes);

        /* Do not remove file(s) if there was a failure */
        if (cpl_error_get_code() == CPL_ERROR_NONE) {
            for (; i; i--) {
                char * newname = cpl_sprintf(newformat, i);

                error = remove(newname);

                cpl_free(newname);
                if (error) break;
            }
            cpl_test_zero(i);
        }

    } while (0);

    cpl_frameset_delete(frameset);
    cpl_propertylist_delete(plist);
    cpl_parameterlist_delete(parlist);

    return;

}


static void cpl_dfs_save_tests(const char * rawname)
{

    cpl_error_code      error;
    const char        * remregexp = "DROP";
    cpl_frameset      * frames   = NULL;
    cpl_propertylist  * tlist    = NULL;
    cpl_propertylist  * qclist   = NULL;
    cpl_parameterlist * parlist  = NULL;
    cpl_image         * image    = NULL;
    cpl_imagelist     * imlist   = NULL;
    cpl_table         * table    = NULL;
    cpl_frame         * rawframe = cpl_frame_new();
    int                 framesetsize;


    /* Test 1: Check the error handling of NULL-pointer(s) */
    error = cpl_dfs_save_image(frames, NULL, parlist, frames, NULL, image,
                               CPL_TYPE_UCHAR,
                               "recipe", qclist, "none", "pipe_id",
                               CPL_DFS_FITSFILE);

    cpl_test_eq(error, CPL_ERROR_NULL_INPUT);

    error = cpl_dfs_save_imagelist(frames, NULL, parlist, frames, NULL, imlist,
                                   CPL_TYPE_UCHAR, "recipe",
                                   qclist, "none", "pipe_id", CPL_DFS_FITSFILE);

    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT);

    error = cpl_dfs_save_table(frames, NULL, parlist, frames, NULL, table,
                               tlist, "recipe", qclist, "none", "pipe_id",
                               CPL_DFS_FITSFILE);

    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT);

    error = cpl_dfs_save_paf("INSTRUME", "recipe", qclist, CPL_DFS_FITSFILE);

    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT);


    frames  = cpl_frameset_new();
    tlist   = cpl_propertylist_new();
    qclist  = cpl_propertylist_new();
    parlist = cpl_parameterlist_new();
    table = cpl_table_new(1);
    imlist = cpl_imagelist_new();

    cpl_dfs_parameterlist_fill(parlist);

    /* This card is of floating point type, but make sure it is written
       as an integer, since that format is also valid */
    error = cpl_propertylist_append_int(qclist, "EQUINOX", 2000);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_propertylist_append_string(qclist, CPL_DFS_PRO_CATG,
                                                 "procat"));

    cpl_dfs_propertylist_append_dicb(qclist);
    cpl_dfs_propertylist_append_dicb(tlist);

    /* Test 1a: Try an illegal filename */
    error = cpl_dfs_save_paf("INSTRUME", "recipe", qclist, ".");

    cpl_test_eq_error( error, CPL_ERROR_FILE_IO);

    /* Test 2: Check handling of empty objects (frameset) */
    error = cpl_dfs_save_image(frames, NULL, parlist, frames, NULL, image,
                               CPL_TYPE_UCHAR,
                               "recipe", qclist, "none", "pipe_id",
                               "image" CPL_DFS_FITS);

    cpl_test_eq_error( error, CPL_ERROR_DATA_NOT_FOUND);

    error = cpl_dfs_save_imagelist(frames, NULL, parlist, frames, NULL, imlist,
                               CPL_TYPE_UCHAR, "recipe",
                               qclist, "none", "pipe_id", "imlist" CPL_DFS_FITS);

    cpl_test_eq_error( error, CPL_ERROR_DATA_NOT_FOUND);

    error = cpl_dfs_save_table(frames, NULL, parlist, frames, NULL, table,
                               tlist, "recipe", qclist, "none", "pipe_id",
                               "table" CPL_DFS_FITS);

    cpl_test_eq_error( error, CPL_ERROR_DATA_NOT_FOUND);

    /* Test 3A: Check handling of empty objects (except frameset)
                - using a non-existing input file */

    /* Make sure file does not exist */
    (void)remove("table" CPL_DFS_FITS);

    error = cpl_frame_set_tag(rawframe, "TAG");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_group(rawframe, CPL_FRAME_GROUP_RAW);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_filename(rawframe, "table" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frameset_insert(frames, rawframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    framesetsize = cpl_frameset_get_size(frames);

    error = cpl_dfs_save_image(frames, NULL, parlist, frames, NULL, image,
                               CPL_TYPE_UCHAR, "recipe", qclist, "none",
                               "pipe_id", "image" CPL_DFS_FITS);
    cpl_test_eq_error( error, CPL_ERROR_FILE_NOT_FOUND);
    cpl_test_eq( cpl_frameset_get_size(frames), framesetsize);

    error = cpl_dfs_save_imagelist(frames, NULL, parlist, frames, NULL, imlist,
                                   CPL_TYPE_UCHAR, "recipe",
                                   qclist, "none", "pipe_id",
                                   "imlist" CPL_DFS_FITS);
    cpl_test_eq_error( error, CPL_ERROR_FILE_NOT_FOUND);
    cpl_test_eq( cpl_frameset_get_size(frames), framesetsize);

    /* Test 3B: Check handling of empty objects (except frameset) */

    cpl_frameset_delete(frames);
    frames = cpl_frameset_new();
    rawframe = cpl_frame_new();

    error = cpl_frame_set_tag(rawframe, "TAG");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_group(rawframe, CPL_FRAME_GROUP_RAW);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_filename(rawframe, rawname);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frameset_insert(frames, rawframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    framesetsize = cpl_frameset_get_size(frames);

    /* Make sure file does not exist */
    (void)remove("imlist" CPL_DFS_FITS);
    error = cpl_dfs_save_imagelist(frames, NULL, parlist, frames, NULL, imlist,
                                   CPL_TYPE_UCHAR, "recipe",
                                   qclist, "none", "pipe_id",
                                   "imlist" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_eq( cpl_frameset_get_size(frames), framesetsize);

    /* Test 3: Check handling of empty objects (except frameset + imagelist) */

    /* Insert an image into the imagelist */
    error = cpl_imagelist_set(imlist, cpl_image_new(3, 2, CPL_TYPE_INT), 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Make sure file does not exist */
    (void)remove("image" CPL_DFS_FITS);
    error = cpl_dfs_save_image(frames, NULL, parlist, frames, NULL, image,
                               CPL_TYPE_UCHAR,
                               "recipe", qclist, "none", "pipe_id",
                               "image" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("image" CPL_DFS_FITS);

    /* Make sure file does not exist */
    (void)remove("imlist" CPL_DFS_FITS);
    error = cpl_dfs_save_imagelist(frames, NULL, parlist, frames, NULL, imlist,
                                   CPL_TYPE_UCHAR, "recipe",
                                   qclist, "none", "pipe_id",
                                   "imlist" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("imlist" CPL_DFS_FITS);

    /* Make sure file does not exist */
    (void)remove("table" CPL_DFS_FITS);
    error = cpl_dfs_save_table(frames, NULL, parlist, frames, NULL, table,
                               tlist, "recipe", qclist, "none", "pipe_id",
                               "table" CPL_DFS_FITS);

    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("table" CPL_DFS_FITS);

    /* Make sure file does not exist */
    (void)remove("recipe" CPL_DFS_PAF);
    error = cpl_dfs_save_paf("INSTRUME", "recipe", qclist, "recipe" CPL_DFS_PAF);

    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_dfs_update_product_header(frames);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Test 4: Check handling of empty objects (except frameset and qclist) */
    /*         And remove one property */
    cpl_test_zero(cpl_propertylist_append_string(qclist, "ESO QC STRING",
                                                      "'Lorem ipsum'"));
    error = cpl_propertylist_append_int(qclist, "ESO QC INT", 42);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    /* Use the value 1/3 to see the accuracy in the FITS header */
    error = cpl_propertylist_append_float(qclist, "ESO QC FLOAT", 1.0F/3.0F);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero(cpl_propertylist_append_double(qclist, "ESO QC DOUBLE",
                                                      1.0/3.0));
    error = cpl_propertylist_append_string(qclist, "ESO QC DROP", "image");
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_propertylist_set_comment(qclist, "ESO QC STRING",
                                                    "string"));
    error = cpl_propertylist_set_comment(qclist, "ESO QC INT", "int");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero(cpl_propertylist_set_comment(qclist, "ESO QC FLOAT",
                                                    "float"));
    cpl_test_zero(cpl_propertylist_set_comment(qclist, "ESO QC DOUBLE",
                                                    "double"));

    /* Make sure file does not exist */
    (void)remove("image" CPL_DFS_FITS);
    image = cpl_image_new(1, 41, CPL_TYPE_FLOAT);
    error = cpl_dfs_save_image(frames, NULL, parlist, frames, NULL, image,
                               CPL_TYPE_UCHAR, "recipe",
                               qclist, remregexp, "pipe_id",
                               "image" CPL_DFS_FITS);

    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("image" CPL_DFS_FITS);


    /* Insert the image also as a calibration frame */

    rawframe = cpl_frame_new();

    error = cpl_frame_set_tag(rawframe, "IMAGECALIB");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_group(rawframe, CPL_FRAME_GROUP_CALIB);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_filename(rawframe, "image" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frameset_insert(frames, rawframe);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    framesetsize++;

    /* Make sure file does not exist */
    (void)remove("table" CPL_DFS_FITS);
    error = cpl_dfs_save_table(frames, NULL, parlist, frames, NULL, table,
                               tlist, "recipe", qclist, "none", "pipe_id",
                               "table" CPL_DFS_FITS);

    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("table" CPL_DFS_FITS);

    /* Make sure file does not exist */
    (void)remove("imlist" CPL_DFS_FITS);
    error = cpl_dfs_save_imagelist(frames, NULL, parlist, frames, NULL, imlist,
                                   CPL_TYPE_UCHAR, "recipe",
                                   qclist, "none", "pipe_id",
                                   "imlist" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("imlist" CPL_DFS_FITS);

    /* Create an extension, with the QC list */
    error = cpl_image_save(image, "imlist" CPL_DFS_FITS, CPL_TYPE_UNSPECIFIED,
                           qclist, CPL_IO_EXTEND);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits("imlist" CPL_DFS_FITS);

    /* Make sure file does not exist */
    (void)remove("nullimage" CPL_DFS_FITS);
    error = cpl_dfs_save_image(frames, NULL, parlist, frames, NULL, NULL,
                               CPL_TYPE_UCHAR,
                               "recipe", qclist, remregexp, "pipe_id",
                               "nullimage" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("nullimage" CPL_DFS_FITS);

    /* Make sure file does not exist */
    (void)remove("dataless" CPL_DFS_FITS);
    error = cpl_dfs_save_propertylist(frames, NULL, parlist, frames, NULL,
                               "recipe", qclist, remregexp, "pipe_id",
                               "dataless" CPL_DFS_FITS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq( cpl_frameset_get_size(frames), ++framesetsize);
    cpl_test_fits("dataless" CPL_DFS_FITS);

    /* Make sure file does not exist */
    (void)remove("recipe" CPL_DFS_PAF);
    error = cpl_dfs_save_paf("INSTRUME", "recipe", qclist, "recipe" CPL_DFS_PAF);

    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Final test: Update the headers as well */

    error = cpl_dfs_update_product_header(frames);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_msg_info(cpl_func, "New size of frameset: %d", framesetsize);

    if (cpl_test_get_failed() == 0 &&
        cpl_msg_get_level() > CPL_MSG_INFO) {
        cpl_test_zero(remove("imlist" CPL_DFS_FITS));
        cpl_test_zero(remove("image" CPL_DFS_FITS));
        cpl_test_zero(remove("table" CPL_DFS_FITS));
        cpl_test_zero(remove("recipe" CPL_DFS_PAF));
    }

    cpl_frameset_delete(frames);
    cpl_propertylist_delete(tlist);
    cpl_propertylist_delete(qclist);
    cpl_parameterlist_delete(parlist);
    cpl_image_delete(image);
    cpl_imagelist_delete(imlist);
    cpl_table_delete(table);

    return;

}

static void cpl_dfs_save_txt(const char * self)
{

    FILE * stream = fopen(self, "w");

    cpl_test_nonnull( stream );

    cpl_test_leq(1, fprintf(stream, self, "SIMPLE ASCII file - not FITS\n"));

    cpl_test_zero( fclose(stream));

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Fill a parameterlist with all supported parameter types
  @param  self Parameterlist to fill
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_dfs_parameterlist_fill(cpl_parameterlist * self)
{

    cpl_error_code error;

    error = cpl_parameterlist_append
        (self, cpl_parameter_new_value("my_bool", CPL_TYPE_BOOL, "test bool",
                                       "bool context", CPL_TRUE));
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_parameterlist_append
        (self, cpl_parameter_new_value("my_int", CPL_TYPE_INT, "test integer",
                                       "int context", 42));
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Use the value 1/3 to see the accuracy in the FITS header */
    error = cpl_parameterlist_append
        (self, cpl_parameter_new_value("my_double", CPL_TYPE_DOUBLE,
                                       "test double", "double context",
                                       1.0/3.0));
    cpl_test_eq_error(error, CPL_ERROR_NONE);


    error = cpl_parameterlist_append
        (self, cpl_parameter_new_value("my_string", CPL_TYPE_STRING,
                                       "test string", "string context",
                                       "Hello, World"));
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* The help text here has a newline and a tab both of which are not
       allowed in a FITS card.  */
    error = cpl_parameterlist_append
        (self, cpl_parameter_new_range("my_range", CPL_TYPE_INT,
                                       "1 == Median,\n 2 == Mean.\t last word.",
                                       "range context", 1, 1, 2));

    cpl_test_eq_error(error, CPL_ERROR_NONE);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Append various card, to be ordered according to DICB
  @param  self Propertylist to fill
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_dfs_propertylist_append_dicb(cpl_propertylist * self)
{

    cpl_error_code code;


    /* Some FITS cards, ordered reversely according to DICB */

    code = cpl_propertylist_append_string(self, "NAXISKEYS",
                                           "Nine letter card");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "COMMENT",
                                           "Comment card");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "HISTORY",
                                           "History card");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "NAXISKEY",
                                           "NAX IS KEY 8-letter card!");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "TBCOLKEY",
                                           "TBCOL KEY 8-letter card!");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "TFORMKEY",
                                           "TFORM KEY 8-letter card!");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "ESO AA",
                                           "ESO card, after standard ones");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "ESO DET DID",
                                           "ESO-VLT-DIC.NGCDCS-123456");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "ESO INS DATE",
                                           "2018-10-25");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "ESO TEL DID",
                                           "ESO-VLT-DIC.TCS");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_double(self, "ESO GEN MOON PHASE", 0.5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "ESO TPL DID",
                                           "ESO-VLT-DIC.TPL-1.9");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "ESO OBS DID",
                                           "ESO-VLT-DIC.OBS-1.12");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_propertylist_append_string(self, "ESO DPR CATG",
                                           "CALIB");
    cpl_test_eq_error(code, CPL_ERROR_NONE);

}
