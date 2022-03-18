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
#  include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <fitsio.h>
#include <fitsio2.h>

#include <cpl_propertylist_impl.h>
#include <cpl_msg.h>
#include <cpl_memory.h>
#include <cpl_tools.h>
#include <cpl_error_impl.h>
#include <cpl_io_fits.h>

#include "cpl_dfs.h"

#include <string.h>
#include <assert.h>
#include <unistd.h>


/**
 * @defgroup cpl_dfs       DFS related functions
 *
 */

/*-----------------------------------------------------------------------------
                        Defines and Static variables
 -----------------------------------------------------------------------------*/

/*
 * Product keywords aliases
 */

#define DATE                        "DATE"
#define DATAMD5                     "DATAMD5"
#define ARCFILE                     "ARCFILE"
#define PIPEFILE                    "PIPEFILE"
#define CHECKSUM                    "CHECKSUM"
#define DATASUM                     "DATASUM"
#define PRO_DID                     "ESO PRO DID"
#define DPR_CATG                    "ESO DPR CATG"
#define PRO_CATG                    CPL_DFS_PRO_CATG 
#define PRO_TYPE                    CPL_DFS_PRO_TYPE
#define DPR_TECH                    "ESO DPR TECH"
#define PRO_TECH                    CPL_DFS_PRO_TECH
#define PRO_SCIENCE                 CPL_DFS_PRO_SCIENCE
#define PRO_ANCESTOR                "ESO PRO ANCESTOR"
#define PRO_DATE                    "ESO PRO DATE"
#define PRO_DATANCOM                "ESO PRO DATANCOM"
#define PRO_RECi_ID                 "ESO PRO REC%d ID"
#define PRO_RECi_DRS_ID             "ESO PRO REC%d DRS ID"
#define PRO_RECi_PIPE_ID            "ESO PRO REC%d PIPE ID"
#define PRO_RECi_RAWi_NAME          "ESO PRO REC%d RAW%d NAME"
#define PRO_RECi_RAWi_CATG          "ESO PRO REC%d RAW%d CATG"
#define PRO_RECi_CALi_NAME          "ESO PRO REC%d CAL%d NAME"
#define PRO_RECi_CALi_CATG          "ESO PRO REC%d CAL%d CATG"
#define PRO_RECi_CALi_DATAMD5       "ESO PRO REC%d CAL%d " DATAMD5
#define PRO_RECi_PARAMi_NAME        "ESO PRO REC%d PARAM%d NAME"
#define PRO_RECi_PARAMi_VALUE       "ESO PRO REC%d PARAM%d VALUE"

#define MAX_PLENGTH (64)

/* Size of a 128-bit MD5 hash in bytes */
#define MD5HASHSZ    32

/* Use at least this many places when printing the PAF key */
#define PAF_KEY_LEN 21
/* Right justify the PAF key and separate with an extra space */
#define PAF_KEY_FORMAT "%-21s "

/* Support for MD5 sums for large files and CFITSIO 3.X/2.51 */
/* OFF_T causes trouble with -std=c99, so avoid it when possible */
#ifdef fits_get_hduaddrll
/* fits_get_hduaddr() supports filesizes less than 2GiB,
   fits_get_hduaddrll() supports sizes less than 2^63 B, but is
   introduced after CFITSIO version 2.510 */
#define CPL_OFF_FUNC fits_get_hduaddrll
#define CPL_OFF_TYPE LONGLONG
#else
/* Using fits_get_hduoff() instead as per DFS05866 */
#define CPL_OFF_FUNC fits_get_hduoff
#define CPL_OFF_TYPE OFF_T
#endif

#define CPL_DFS_PRO_DID "PRO-1.16"


/*-----------------------------------------------------------------------------
 *                      Private function prototypes
 *-----------------------------------------------------------------------------
 */

/**@{*/

/* Declarations of static MD5 functions */
#include "md5.h"


#if defined CFITSIO_MAJOR && (CFITSIO_MAJOR > 3 || CFITSIO_MINOR >= 26)
/* Not needed from 3.26 */
#else
static char * cpl_dfs_extract_printable(const char *) CPL_ATTR_ALLOC;
#endif

inline static cpl_error_code _cpl_dfs_sign_product(const cpl_frame *frame,
                                                   cxuint flags);

static cpl_error_code cpl_dfs_update_product_header_(cpl_frameset *);

static cpl_error_code cpl_dfs_find_md5sum(char *, fitsfile *)
    CPL_ATTR_NONNULL;

static int cpl_is_fits(const char *);
static
const char *cpl_get_base_name(const char *) CPL_ATTR_PURE;

static FILE * cpl_dfs_paf_init(const char *, const char *, const char *);

static cpl_error_code cpl_dfs_paf_dump(const cpl_propertylist *, FILE *);

static
cpl_error_code cpl_dfs_paf_dump_string(const char *, const char *,
                                       const char *, FILE *);

static cpl_error_code cpl_dfs_paf_dump_double(const char *, double,
                                              const char *, FILE *);

static cpl_error_code cpl_dfs_paf_dump_int(const char *, cpl_size,
                                          const char *, FILE *);

static cpl_error_code cpl_dfs_product_save(cpl_frameset *,
                                           cpl_propertylist *,
                                           const cpl_parameterlist *,
                                           const cpl_frameset *,
                                           const cpl_frame *,
                                           const cpl_imagelist *,
                                           const cpl_image *,
                                           cpl_type,
                                           const cpl_table *,
                                           const cpl_propertylist *,
                                           const char *,
                                           const cpl_propertylist *,
                                           const char *,
                                           const char *,
                                           const char *);

/*-----------------------------------------------------------------------------
 *                             Function code
 *-----------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------*/
/**
  @brief  Save an image as a DFS-compliant pipeline product
  @param  allframes  The list of input frames for the recipe
  @param  header     NULL, or filled with properties written to product header
  @param  parlist    The list of input parameters
  @param  usedframes The list of raw/calibration frames used for this product
  @param  inherit    NULL or product frames inherit their header from this frame
  @param  image      The image to be saved
  @param  type       The type used to represent the data in the file
  @param  recipe     The recipe name
  @param  applist    Propertylist to append to primary header, w. PRO.CATG
  @param  remregexp  Optional regexp of properties not to put in main header
  @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
  @param  filename   Filename of created product
  @note The image may be NULL in which case only the header information is saved
        but passing a NULL image is deprecated, use cpl_dfs_save_propertylist().
  @note remregexp may be NULL
  @note applist must contain a string-property with key CPL_DFS_PRO_CATG
  @note On success and iff header is non-NULL, it will be emptied and then
        filled with the properties written to the primary header of the product
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_dfs_setup_product_header(), cpl_image_save().

  The FITS header of the created product is created from the provided applist
  and the cards copied by cpl_dfs_setup_product_header(), with exception of
  the cards whose keys match the provided remregexp.

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_dfs_save_image(cpl_frameset            * allframes,
                                  cpl_propertylist        * header,
                                  const cpl_parameterlist * parlist,
                                  const cpl_frameset      * usedframes,
                                  const cpl_frame         * inherit,
                                  const cpl_image         * image,
                                  cpl_type                  type,
                                  const char              * recipe,
                                  const cpl_propertylist  * applist,
                                  const char              * remregexp,
                                  const char              * pipe_id,
                                  const char              * filename)
{
    return cpl_dfs_product_save(allframes, header, parlist, usedframes, inherit,
                                NULL, image, type, NULL, NULL, recipe, applist,
                                remregexp, pipe_id, filename)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
  @brief  Save a propertylist as a DFS-compliant pipeline product
  @param  allframes  The list of input frames for the recipe
  @param  header     NULL, or filled with properties written to product header
  @param  parlist    The list of input parameters
  @param  usedframes The list of raw/calibration frames used for this product
  @param  inherit    NULL or product frames inherit their header from this frame
  @param  recipe     The recipe name
  @param  applist    Propertylist to append to primary header, w. PRO.CATG
  @param  remregexp  Optional regexp of properties not to put in main header
  @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
  @param  filename   Filename of created product
  @note remregexp may be NULL
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_dfs_save_image(), cpl_propertylist_save().

  The FITS header of the created product is created from the provided applist
  and the cards copied by cpl_dfs_setup_product_header(), with exception of
  the cards whose keys match the provided remregexp.

  The FITS data unit will be empty.

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_dfs_save_propertylist(cpl_frameset            * allframes,
                                         cpl_propertylist        * header,
                                         const cpl_parameterlist * parlist,
                                         const cpl_frameset      * usedframes,
                                         const cpl_frame         * inherit,
                                         const char              * recipe,
                                         const cpl_propertylist  * applist,
                                         const char              * remregexp,
                                         const char              * pipe_id,
                                         const char              * filename)
{
    /* Use CPL_TYPE_INVALID to ensure it is not referenced */
    return cpl_dfs_product_save(allframes, header, parlist, usedframes, inherit,
                                NULL, NULL, CPL_TYPE_INVALID, NULL, NULL,
                                recipe, applist, remregexp, pipe_id, filename)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Save an imagelist as a DFS-compliant pipeline product
  @param  allframes  The list of input frames for the recipe
  @param  header     NULL, or filled with properties written to product header
  @param  parlist    The list of input parameters
  @param  usedframes The list of raw/calibration frames used for this product
  @param  inherit    NULL or product frames inherit their header from this frame
  @param  imagelist  The imagelist to be saved
  @param  type       The type used to represent the data in the file
  @param  recipe     The recipe name
  @param  applist    Propertylist to append to primary header, w. PRO.CATG
  @param  remregexp  Optional regexp of properties not to put in main header
  @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
  @param  filename   Filename of created product
  @note remregexp may be NULL
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_dfs_save_image(), cpl_imagelist_save().

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_dfs_save_imagelist(cpl_frameset            * allframes,
                                      cpl_propertylist        * header,
                                      const cpl_parameterlist * parlist,
                                      const cpl_frameset      * usedframes,
                                      const cpl_frame         * inherit,
                                      const cpl_imagelist     * imagelist,
                                      cpl_type                  type,
                                      const char              * recipe,
                                      const cpl_propertylist  * applist,
                                      const char              * remregexp,
                                      const char              * pipe_id,
                                      const char              * filename)
{
    cpl_ensure_code(imagelist != NULL, CPL_ERROR_NULL_INPUT);

    return cpl_dfs_product_save(allframes, header, parlist, usedframes,
                                inherit, imagelist, NULL, type, NULL, NULL,
                                recipe, applist, remregexp, pipe_id, filename)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Save a table as a DFS-compliant pipeline product
  @param  allframes  The list of input frames for the recipe
  @param  header     NULL, or filled with properties written to product header
  @param  parlist    The list of input parameters
  @param  usedframes The list of raw/calibration frames used for this product
  @param  inherit    NULL or product frames inherit their header from this frame
  @param  table      The table to be saved
  @param  tablelist  Optional propertylist to use in table extension or NULL
  @param  recipe     The recipe name
  @param  applist    Propertylist to append to primary header, w. PRO.CATG
  @param  remregexp  Optional regexp of properties not to put in main header
  @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
  @param  filename   Filename of created product
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_dfs_save_image(), cpl_table_save().

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_dfs_save_table(cpl_frameset            * allframes,
                                  cpl_propertylist        * header,
                                  const cpl_parameterlist * parlist,
                                  const cpl_frameset      * usedframes,
                                  const cpl_frame         * inherit,
                                  const cpl_table         * table,
                                  const cpl_propertylist  * tablelist,
                                  const char              * recipe,
                                  const cpl_propertylist  * applist,
                                  const char              * remregexp,
                                  const char              * pipe_id,
                                  const char              * filename)
{

    cpl_ensure_code(table != NULL, CPL_ERROR_NULL_INPUT);

    /* Use CPL_TYPE_INVALID to ensure it is not referenced */
    return cpl_dfs_product_save(allframes, header, parlist, usedframes, inherit,
                                NULL, NULL, CPL_TYPE_INVALID, table, tablelist,
                                recipe, applist, remregexp, pipe_id, filename)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief   Create a new PAF file
  @param   instrume Name of instrument in capitals (NACO, VISIR, etc.)
  @param   recipe   Name of recipe
  @param   paflist  Propertylist to save
  @param   filename Filename of created PArameter File
  @return  CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_dfs_save_image().

  The example below shows how to create a PAF from some FITS cards from the file
  ref_file and QC parameters in a propertylist qclist. Please note that qclist
  can be used also in calls to cpl_dfs_save_image() and cpl_dfs_save_table().
  Error handling is omitted for brevity:

   @code

   const char pafcopy[] = "^(DATE-OBS|ARCFILE|ESO TPL ID|ESO DET DIT|MJD-OBS)$";
   cpl_propertylist * paflist = cpl_propertylist_load_regexp(ref_file, 0,
                                                             pafcopy, 0);

   cpl_propertylist_append(paflist, qclist);

   cpl_dfs_save_paf("IIINSTRUMENT", "rrrecipe", paflist, "rrrecipe.paf");

   cpl_propertylist_delete(paflist);

   @endcode

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_dfs_save_paf(const char * instrume, const char *recipe,
                               const cpl_propertylist * paflist,
                               const char * filename)
{

    FILE * paf;
    cpl_error_code status;


    cpl_ensure_code(instrume  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(recipe    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(paflist   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(filename  != NULL, CPL_ERROR_NULL_INPUT);


    paf = cpl_dfs_paf_init(instrume, recipe, filename);

    cpl_ensure_code(paf != NULL, cpl_error_get_code());

    status = cpl_dfs_paf_dump(paflist, paf);
    if (status == CPL_ERROR_NONE && fprintf(paf, "\n") != 1)
        status = CPL_ERROR_FILE_IO;

    if (status == CPL_ERROR_NONE) {
        cpl_ensure_code(fclose(paf) == 0, CPL_ERROR_FILE_IO);
    } else {
        (void)fclose(paf);
    }

    return status;

}


/**
 * @brief    Add product keywords to a pipeline product property list.
 *
 * @param    header          Property list where keywords must be written
 * @param    product_frame   Frame describing the product
 * @param    framelist       List of frames including all input frames
 * @param    parlist         Recipe parameter list
 * @param    recid           Recipe name
 * @param    pipeline_id     Pipeline unique identifier
 * @param    dictionary_id   PRO dictionary identifier
 * @param    inherit_frame   Frame from which header information is inherited
 *
 * @return   @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         An input pointer is <tt>NULL</tt>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The input <i>framelist</i> contains no input frames or
 *         a frame in the input <i>framelist</i> does not specify a file.
 *         In the former case the string "Empty set-of-frames" is appended
 *         to the error message returned by cpl_error_get_message().
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The product frame is not tagged or not grouped 
 *         as <tt>CPL_FRAME_GROUP_PRODUCT</tt>.
 *         A specified @em inherit_frame doesn't belong to the input frame 
 *         list, or it is not in FITS format.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_FOUND</td>
 *       <td class="ecr">
 *         A frame in the input <i>framelist</i> specifies a non-existing file.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_BAD_FILE_FORMAT</td>
 *       <td class="ecr">
 *         A frame in the input <i>framelist</i> specifies an invalid file.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function checks the @em header associated to a pipeline product,
 * to ensure that it is DICB compliant. In particular, this function does
 * the following:
 *
 *    -# Selects a reference frame from which the primary and secondary 
 *       keyword information is inherited. The primary information is
 *       contained in the FITS keywords ORIGIN, TELESCOPE, INSTRUME, 
 *       OBJECT, RA, DEC, EPOCH, EQUINOX, RADECSYS, DATE-OBS, MJD-OBS, 
 *       UTC, LST, PI-COI, OBSERVER, while the secondary information is
 *       contained in all the other keywords. If the @em inherit_frame 
 *       is just a NULL pointer, both primary and secondary information 
 *       is inherited from the first frame in the input framelist with 
 *       group CPL_FRAME_GROUP_RAW, or if no such frames are present 
 *       the first frame with group CPL_FRAME_GROUP_CALIB. 
 *       If @em inherit_frame is non-NULL, the secondary information
 *       is inherited from @em inherit_frame instead.
 *
 *    -# Copy to @em header, if they are present, the following primary
 *       FITS keywords from the first input frame in the @em framelist:
 *       ORIGIN, TELESCOPE, INSTRUME, OBJECT, RA, DEC, EPOCH, EQUINOX,
 *       RADECSYS, DATE-OBS, MJD-OBS, UTC, LST, PI-COI, OBSERVER. If those
 *       keywords are already present in the @em header property list, they
 *       are overwritten only in case they have the same type. If any of
 *       these keywords are present with an unexpected type, a warning is
 *       issued, but the keywords are copied anyway (provided that the
 *       above conditions are fulfilled), and no error is set.
 *
 *    -# Copy all the HIERARCH.ESO._ keywords from the primary FITS header
 *       of the @em inherit_frame in @em framelist, with the exception of
 *       the HIERARCH.ESO.DPR._, and of the .PRO._ and .DRS._ keywords if
 *       the @em inherit_frame is a calibration. If those keywords are
 *       already present in @em header, they are overwritten.
 *
 *    -# If found, remove the HIERARCH.ESO.DPR._ keywords from @em header.
 *
 *    -# If found, remove the ARCFILE and ORIGFILE keywords from @em header.
 *
 *    -# Add to @em header the following mandatory keywords from the PRO
 *       dictionary: PIPEFILE, PRO.DID, PRO.REC1.ID, PRO.REC1.DRS.ID,
 *       PRO.REC1.PIPE.ID, and PRO.CATG. If those keywords are already 
 *       present in @em header, they are overwritten. The keyword
 *       PRO.CATG is always set identical to the tag in @em product_frame.
 *
 *    -# Only if missing, add to @em header the following mandatory keywords 
 *       from the PRO dictionary: PRO.TYPE, PRO.TECH, and PRO.SCIENCE. 
 *       The keyword PRO.TYPE will be set to "REDUCED". If the keyword 
 *       DPR.TECH is found in the header of the first frame, PRO.TECH is
 *       given its value, alternatively if the keyword PRO.TECH is found
 *       it is copied instead, and if all fails the value "UNDEFINED" is 
 *       set. Finally, if the keyword DPR.CATG is found in the header of 
 *       the first frame and is set to "SCIENCE", the boolean keyword 
 *       PRO.SCIENCE will be set to "true", otherwise it will be copied
 *       from an existing PRO.SCIENCE keyword, while it will be set to 
 *       "false" in all other cases.
 *
 *    -# Check the existence of the keyword PRO.DATANCOM in @em header. If
 *       this keyword is missing, one is added, with the value of the total
 *       number of raw input frames.
 *
 *    -# Add to @em header the keywords PRO.REC1.RAW1.NAME, PRO.REC1.RAW1.CATG,
 *       PRO.REC1.CAL1.NAME, PRO.REC1.CAL1.CATG, to describe the content of
 *       the input set-of-frames.
 *
 * See the DICB PRO dictionary to have details on the mentioned PRO keywords.
 *
 * @note
 *   Non-FITS files are handled as files with an empty FITS header.
 */

cpl_error_code cpl_dfs_setup_product_header(cpl_propertylist *header,
                                            const cpl_frame *product_frame,
                                            const cpl_frameset *framelist,
                                            const cpl_parameterlist *parlist,
                                            const char *recid,
                                            const char *pipeline_id,
                                            const char *dictionary_id,
                                            const cpl_frame *inherit_frame)
{

    char              cval[FLEN_VALUE];
    const char       *datamd5;
    const cpl_frame  *frame;
    const cpl_frame  *first_frame = NULL;
    cpl_frameset_iterator *it = NULL;
    cpl_propertylist *plist;
    const cpl_parameter *param;
    int               keylen;
    int               level = 0;
    int               empty = 0;
    int               nraw;
    int               ncal;
    int               npar;
    int               i;

    const char       *kname;


    /*
     * Here is the list of mandatory keywords, first in input header,
     * second in output _image_ header:
     */

    typedef struct {
        const cpl_cstr *key;
        cpl_type type;
    } klist;

    klist mandatory[] =  {{CXSTR("ORIGIN", 6), CPL_TYPE_STRING},
                          {CXSTR("TELESCOP", 8), CPL_TYPE_STRING},
                          {CXSTR("INSTRUME", 8), CPL_TYPE_STRING},
                          {CXSTR("OBJECT", 6), CPL_TYPE_STRING},
                          {CXSTR("RA", 2), CPL_TYPE_DOUBLE},
                          {CXSTR("DEC", 3), CPL_TYPE_DOUBLE},
                          {CXSTR("EPOCH", 5), CPL_TYPE_STRING},
                          {CXSTR("EQUINOX", 7), CPL_TYPE_DOUBLE},
                          {CXSTR("RADECSYS", 8), CPL_TYPE_STRING},
                          {CXSTR("DATE-OBS", 8), CPL_TYPE_STRING},
                          {CXSTR("MJD-OBS", 7), CPL_TYPE_DOUBLE},
                          {CXSTR("UTC", 3), CPL_TYPE_DOUBLE},
                          {CXSTR("LST", 3), CPL_TYPE_DOUBLE},
                          {CXSTR("PI-COI", 6), CPL_TYPE_STRING},
                          {CXSTR("OBSERVER", 8), CPL_TYPE_STRING}};

    const int count_mandatory = (int)CX_N_ELEMENTS(mandatory);


    /* Here are the mandatory keys again + ARCFILE
       - without their associated type */

    const cpl_cstr * exactkeys[] = {CXSTR("ORIGIN", 6),
                                     CXSTR("TELESCOP", 8),
                                     CXSTR("INSTRUME", 8),
                                     CXSTR("OBJECT", 6),
                                     CXSTR("RA", 2),
                                     CXSTR("DEC", 3),
                                     CXSTR("EPOCH", 5),
                                     CXSTR("EQUINOX", 7),
                                     CXSTR("RADECSYS", 8),
                                     CXSTR("DATE-OBS", 8),
                                     CXSTR("MJD-OBS", 7),
                                     CXSTR("UTC", 3),
                                     CXSTR("LST", 3),
                                     CXSTR("PI-COI", 6),
                                     CXSTR("OBSERVER", 8),
                                     CXSTR("ARCFILE", 7)};

    /* The same list, will use a counter one less */
    const cpl_cstr ** mandatorykeys = exactkeys;

    /* ESO HIERACH keys are also to be loaded */
    const cpl_cstr * esokeys[] = {CXSTR("ESO ", 4)};

    const int nesokeys   = (int)CX_N_ELEMENTS(esokeys);
    const int nexactkeys = (int)CX_N_ELEMENTS(exactkeys);
    const int nmandatorykeys = nexactkeys - 1;

    int pro_science = 0;


    assert(count_mandatory == nmandatorykeys);
    assert(nexactkeys      == nmandatorykeys + 1);
    assert(nesokeys        == 1);

    cpl_ensure_code(header        != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(product_frame != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(framelist     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(recid         != NULL, CPL_ERROR_NULL_INPUT);

    /*
     * Forbidden keywords are removed from product header.
     */

    cpl_propertylist_erase_regexp(header,
                                  "^ESO DPR |^ARCFILE$|^ORIGFILE$|"
                                  "^CHECKSUM$|^DATASUM$", 0);

    /*
     * Get the first input frame in the input set-of-frames.
     */

    it = cpl_frameset_iterator_new(framelist);
    frame = cpl_frameset_iterator_get_const(it);

    while (frame != NULL) {

    	cpl_errorstate status;

        if (cpl_frame_get_group(frame) == CPL_FRAME_GROUP_RAW) {
            first_frame = frame;
            break;
        }

        status = cpl_errorstate_get();

        cpl_frameset_iterator_advance(it, 1);

        if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE) {
        	cpl_errorstate_set(status);
        }

        frame = cpl_frameset_iterator_get_const(it);

    }

    if (first_frame == NULL) {

    	cpl_frameset_iterator_reset(it);
        frame = cpl_frameset_iterator_get_const(it);

        while (frame != NULL) {

        	cpl_errorstate status;

        	if (cpl_frame_get_group(frame) == CPL_FRAME_GROUP_CALIB) {
                first_frame = frame;
                break;
            }

            status = cpl_errorstate_get();

            cpl_frameset_iterator_advance(it, 1);

            if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE) {
            	cpl_errorstate_set(status);
            }

            frame = cpl_frameset_iterator_get_const(it);

        }
    }

	cpl_frameset_iterator_delete(it);
	it = NULL;

    if (first_frame == NULL) {
        return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND,
                                     "Empty set-of-frames");
    }

    if (inherit_frame) {
        int iframe = 0;

        if (cpl_frame_get_filename(inherit_frame) == NULL) {
            return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND,
                                          "Inherit frame has no filename");
        }

        it = cpl_frameset_iterator_new(framelist);
        frame = cpl_frameset_iterator_get_const(it);

        while (frame != NULL) {

        	cpl_errorstate status;

            iframe++;

            if (cpl_frame_get_filename(frame) == NULL) {

            	cpl_frameset_iterator_delete(it);

                return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "Frame "
                                              "%d/%d has no filename", iframe,
                                              (int)cpl_frameset_get_size
                                              (framelist));

            }

            if (strcmp(cpl_frame_get_filename(frame), 
                       cpl_frame_get_filename(inherit_frame)) == 0) {
                break;
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

        if (frame != NULL) {
            switch (cpl_is_fits(cpl_frame_get_filename(inherit_frame))) {
            case 0:
                return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                "Cannot inherit information from non-FITS file");
            case 1:
                break;
            default:
                return cpl_error_set_(CPL_ERROR_FILE_NOT_FOUND);
            }
        }
        else {
            return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
            "The reference frame does not belong to the input set-of-frames");
        }
    }
    else {
        inherit_frame = first_frame;
    }
    

    /*
     * Now copy all the required entries, if present, from input to
     * product header.
     */

    switch (cpl_is_fits(cpl_frame_get_filename(first_frame))) {
    case 0:
        plist = cpl_propertylist_new();
        break;
    case 1:
        /* Load only the mandatory keywords and the HIERACH ESO keywords */
        if (first_frame == inherit_frame) {
            plist = cpl_propertylist_load_name_(
                                         cpl_frame_get_filename(first_frame), 0,
                                         nesokeys, esokeys,
                                         nexactkeys, exactkeys, 0);

            cpl_ensure_code(plist != NULL, cpl_error_get_code());
        }
        else {
            cpl_propertylist *plist2;
            const cpl_cstr * exactkey[] = {CXSTR("ARCFILE", 7)};

            plist = cpl_propertylist_load_name_(
                    cpl_frame_get_filename(first_frame), 0,
                    nmandatorykeys, mandatorykeys, 0, NULL, 0);
            cpl_ensure_code(plist != NULL, cpl_error_get_code());
            plist2 = cpl_propertylist_load_name_(
                    cpl_frame_get_filename(inherit_frame), 0,
                    nesokeys, esokeys, 1, exactkey, 0);
            cpl_ensure_code(plist2 != NULL, cpl_error_get_code());
            if (cpl_propertylist_copy_property_regexp(plist, plist2, 
                                                      "^ESO|^ARCFILE$", 0)) {
                cpl_propertylist_delete(plist2);
                return cpl_error_set_where_();
            }
            cpl_propertylist_delete(plist2);
        }
        break;
    default:
        return cpl_error_get_code() == CPL_ERROR_NULL_INPUT
            ? cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND) : cpl_error_set_where_();
    }

    for (i = 0; i < count_mandatory; i++) {
        if (cpl_propertylist_has_cx(plist, mandatory[i].key)) {
            if (cpl_propertylist_get_type_cx(plist, mandatory[i].key) !=
                mandatory[i].type) {
                cpl_msg_warning(cpl_func, "Unexpected type for keyword %s in "
                                "input header", mandatory[i].key->data);
            }
            if (cpl_propertylist_has_cx(header, mandatory[i].key)) {
                if (cpl_propertylist_get_type_cx(header, mandatory[i].key) !=
                    mandatory[i].type) {
                    cpl_msg_warning(cpl_func, "Unexpected type for keyword %s "
                                    "in output header", mandatory[i].key->data);
                }
                if (cpl_propertylist_get_type_cx(plist, mandatory[i].key) !=
                    cpl_propertylist_get_type_cx(header, mandatory[i].key)) {
                    /* Do not copy this keyword */
                    cpl_propertylist_erase_cx(plist, mandatory[i].key);
                }
            }
        }
    }


    /*
     * Determine the product level from the inherited frame properties, i.e.
     * the next free slot in the product processing history.
     */

    while (!empty) {
        ++level;

        keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_ID, level);
        empty = !cpl_propertylist_has_cx(plist, CXSTR(cval, keylen));
    }


    /*
     * Here copy all the HIERARCH.ESO._ keywords, excluding the
     * HIERARCH.ESO.DPR._, .DRS._, and .QC._ keywords.
     *
     * HIERARCH.ESO.PRO._ keywords are also ignored with the exception of
     * the processing history PRO.RECi._
     */

    /* Also copy the mandatory keywords, if any */

    cpl_propertylist_copy_property_regexp(header, plist,
                                          "^ESO (DPR|DRS|PRO|QC) ", 1);

    /* Copy the processing history */

    cpl_propertylist_copy_property_regexp(header, plist,
                                          "^ESO PRO (REC[1-9]*) ", 0);

    /* Copy or create the product's ancestor */

    if (!cpl_propertylist_has_cx(header, CXSTR(PRO_ANCESTOR, 16))) {

        if (cpl_propertylist_has_cx(plist, CXSTR(PRO_ANCESTOR, 16))) {
            cpl_propertylist_copy_property(header, plist, PRO_ANCESTOR);
        }
        else if (cpl_propertylist_has_cx(plist, CXSTR(ARCFILE, 7))) {

            const char *s = cpl_propertylist_get_string(plist, ARCFILE);

            cpl_propertylist_append_string(header, PRO_ANCESTOR, s);

        }

    }

    cpl_propertylist_delete(plist);


    /*
     * DATAMD5 (placeholder: will be computed by esorex)
     */

    cpl_propertylist_update_string(header, DATAMD5, "Not computed");
    cpl_propertylist_set_comment_cx(header, CXSTR(DATAMD5, 7),
                                    CXSTR("MD5 checksum", 12));


    /*
     * PIPEFILE
     */

    kname = cpl_frame_get_filename(product_frame);
    cpl_ensure_code(kname != NULL, cpl_error_get_code());
    cpl_propertylist_update_string(header, PIPEFILE, kname);
    cpl_propertylist_set_comment_cx(header, CXSTR(PIPEFILE, 8),
                                    CXSTR("Filename of data product", 24));

    /*
     * PRO DID
     */

    cpl_propertylist_update_string(header, PRO_DID, dictionary_id);
    cpl_propertylist_set_comment_cx(header, CXSTR(PRO_DID, 11),
                                    CXSTR("Data dictionary for PRO", 23));


    /*
     * PRO CATG
     */

    kname = cpl_frame_get_tag(product_frame);
    if (kname == NULL) {
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                     "Product frame has no tag");
    }
    cpl_propertylist_update_string(header, PRO_CATG, kname);
    cpl_propertylist_set_comment_cx(header, CXSTR(PRO_CATG, 12),
                                    CXSTR("Category of pipeline product frame",
                                          34));

    /* The product frame must be grouped correctly */
    cpl_ensure_code(cpl_frame_get_group(product_frame)
                    == CPL_FRAME_GROUP_PRODUCT, CPL_ERROR_ILLEGAL_INPUT);

    /* 
     * Load only the HIERACH ESO DPR keywords 
     */

    if (cpl_is_fits(cpl_frame_get_filename(first_frame))) {
        const cpl_cstr * startkey[] = {CXSTR("ESO DPR ", 8)};
        plist = cpl_propertylist_load_name_(cpl_frame_get_filename(first_frame),
                                            0, 1, startkey, 0, NULL, 0);
        cpl_ensure_code(plist != NULL, cpl_error_get_code());
    }
    else
        plist = NULL;

    /* 
     * PRO TYPE
     * Check presence of PRO.TYPE already provided in the input header:
     * leave it alone if present, provide a default if missing.
     */

    if (!cpl_propertylist_has_cx(header, CXSTR(PRO_TYPE, 12))) {
        cpl_propertylist_update_string(header, PRO_TYPE, "REDUCED");
        cpl_propertylist_set_comment_cx(header, CXSTR(PRO_TYPE, 12),
                                        CXSTR("Product type", 12));
    }

    /*
     * PRO TECH
     * Check presence of PRO.TECH already provided in the input header:
     * leave it alone if present, copy it from the first frame DPR.TECH
     * if present there, copy from the first frame PRO.TECH if present
     * there, set a default "UNDEFINED" if missing even there.
     */

    if (!cpl_propertylist_has_cx(header, CXSTR(PRO_TECH, 12))) {
        if (plist && cpl_propertylist_has_cx(plist, CXSTR(DPR_TECH, 12))) {
            cpl_propertylist_update_string(header, PRO_TECH, 
                       cpl_propertylist_get_string(plist, DPR_TECH));
        }
        else if (plist && cpl_propertylist_has_cx(plist, CXSTR(PRO_TECH, 12))) {
            cpl_propertylist_update_string(header, PRO_TECH, 
                       cpl_propertylist_get_string(plist, PRO_TECH));
        }
        else {
            cpl_propertylist_update_string(header, PRO_TECH, "UNDEFINED");
        }
        cpl_propertylist_set_comment_cx(header, CXSTR(PRO_TECH, 12),
                                        CXSTR("Observation technique", 21));
    }

    /*
     * PRO SCIENCE
     * Check presence of PRO.SCIENCE already provided in the input header:
     * leave it alone if present, set it to "true" if DPR.CATG from the
     * first frame is "SCIENCE", set it to the value of PRO.SCIENCE if 
     * present, set it to "false" in all other cases.
     */

    if (!cpl_propertylist_has_cx(header, CXSTR(PRO_SCIENCE, 15))) {
        if (plist && cpl_propertylist_has_cx(plist, CXSTR(DPR_CATG, 12))) {
            pro_science = !strncmp(cpl_propertylist_get_string(plist, DPR_CATG),
                                   "SCIENCE", 7);
        }
        else if (plist && cpl_propertylist_has_cx(plist,
                                                  CXSTR(PRO_SCIENCE, 15))) {
            pro_science = cpl_propertylist_get_bool(plist, PRO_SCIENCE);
        }
        cpl_propertylist_update_bool(header, PRO_SCIENCE, pro_science);
        cpl_propertylist_set_comment_cx(header, CXSTR(PRO_SCIENCE, 15), 
                                        CXSTR("Scientific product if T",
                                                  23));
    }

    cpl_propertylist_delete(plist);

    /*
     * PRO REC1 ID
     */

    keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_ID, level);
    cpl_propertylist_update_string(header, cval, recid);
    cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                    CXSTR("Pipeline recipe (unique) identifier",
                                          35));


    /*
     * PRO REC1 DRS ID
     */

    /* cpl_propertylist_update_string(header,
                                      PRO_REC_DRS_ID, cpl_get_version()); */

    keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_DRS_ID, level);
    cpl_propertylist_update_string(header, cval,
                                   PACKAGE "-" PACKAGE_VERSION);
    cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                    CXSTR("Data Reduction System identifier",
                                          32));

    /*
     * PRO REC1 PIPE ID
     */

    keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_PIPE_ID, level);
    cpl_propertylist_update_string(header, cval, pipeline_id);
    cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                    CXSTR("Pipeline (unique) identifier", 28));


    /*
     * PRO REC1 RAWi NAME  and  PRO REC1 RAWi CATG
     */

    nraw = 0;

    it = cpl_frameset_iterator_new(framelist);
    frame = cpl_frameset_iterator_get_const(it);

    while (frame != NULL) {

    	cpl_errorstate status;

        if (cpl_frame_get_group(frame) == CPL_FRAME_GROUP_RAW) {

            ++nraw;

            keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_RAWi_NAME, level,
                              nraw);
            cpl_propertylist_update_string(header, cval,
                     cpl_get_base_name(cpl_frame_get_filename(frame)));
            cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                            CXSTR("File name of raw frame",
                                                      22));

            keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_RAWi_CATG, level,
                              nraw);
            cpl_propertylist_update_string(header, cval,
                                           cpl_frame_get_tag(frame));
            cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                            CXSTR("Category of raw frame", 21));

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


    /*
     * PRO DATANCOM
     */

    if (!cpl_propertylist_has_cx(header, CXSTR(PRO_DATANCOM, 16))) {
        cpl_propertylist_update_int(header, PRO_DATANCOM, nraw);
        cpl_propertylist_set_comment_cx(header, CXSTR(PRO_DATANCOM, 16),
                                        CXSTR("Number of combined frames", 25));
    }


    /*
     * PRO REC1 CALi NAME,  PRO REC1 CALi CATG,  and  PRO REC1 CALi DATAMD5
     */

    ncal = 0;

    it = cpl_frameset_iterator_new(framelist);
    frame = cpl_frameset_iterator_get_const(it);

    while (frame != NULL) {

    	cpl_errorstate status;

        if (cpl_frame_get_group(frame) == CPL_FRAME_GROUP_CALIB) {
            const cpl_cstr * exactkey[] = {CXSTR(DATAMD5, 7)};

            ++ncal;

            keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_CALi_NAME, level,
                              ncal);
            cpl_propertylist_update_string(header, cval,
                     cpl_get_base_name(cpl_frame_get_filename(frame)));
            cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                            CXSTR("File name of calibration "
                                                  "frame", 30));

            keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_CALi_CATG, level,
                              ncal);
            cpl_propertylist_update_string(header, cval,
                                           cpl_frame_get_tag(frame));
            cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                            CXSTR("Category of calibration "
                                                  "frame", 29));

            keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_CALi_DATAMD5, level,
                              ncal);

            switch (cpl_is_fits(cpl_frame_get_filename(frame))) {
            case 0:
                plist = cpl_propertylist_new();
                break;
            case 1:
                plist =
                    cpl_propertylist_load_name_(cpl_frame_get_filename(frame),
                                                0, 0, NULL, 1, exactkey, 0);
                cpl_ensure_code(plist != NULL, cpl_error_get_code());
                break;
            default:
            	cpl_frameset_iterator_delete(it);
                return cpl_error_get_code() == CPL_ERROR_NULL_INPUT
                    ? cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND)
                    : cpl_error_set_where_();
                break;
            }

            if (cpl_propertylist_has_cx(plist, CXSTR(DATAMD5, 7))) {
                if (cpl_propertylist_get_type(plist, DATAMD5) !=
                    CPL_TYPE_STRING) {
                    cpl_msg_warning(cpl_func, "Unexpected type for keyword "
                                    DATAMD5 " in input header");
                }
                else {
                    datamd5 = cpl_propertylist_get_string(plist, DATAMD5);
                    cpl_propertylist_update_string(header, cval, datamd5);
                    cpl_propertylist_set_comment_cx(header,
                                                    CXSTR(cval, keylen),
                                                    CXSTR("MD5 signature of "
                                                          "calib frame", 28));
                }
            }
            cpl_propertylist_delete(plist);
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


    npar = 0;
    param = cpl_parameterlist_get_first_const(parlist);

    while (param != NULL) {
        char * pval;
        char * dval; /* The default value for value comment */
#if defined CFITSIO_MAJOR && (CFITSIO_MAJOR > 3 || CFITSIO_MINOR >= 26)
        char * help = NULL;
        const char * comment = cpl_parameter_get_help(param);
#else
        char * help = cpl_dfs_extract_printable(cpl_parameter_get_help(param));
        const char * comment = help;
#endif
        int dvallen = 0;

        ++npar;

        kname = cpl_parameter_get_alias(param, CPL_PARAMETER_MODE_CLI);

        switch (cpl_parameter_get_type(param)) {
        case CPL_TYPE_BOOL:
            pval = cpl_strdup(cpl_parameter_get_bool(param) == 1
                              ? "true" : "false");
            dval = cpl_sprintf("Default: %s%n",
                               cpl_parameter_get_default_bool(param) == 1
                               ? "true" : "false", &dvallen);
            if (!comment) comment = "Boolean recipe parameter";
            break;
        case CPL_TYPE_INT:
            pval = cpl_sprintf("%d", cpl_parameter_get_int(param));
            dval = cpl_sprintf("Default: %d%n",
                               cpl_parameter_get_default_int(param), &dvallen);
            if (!comment) comment = "Integer recipe parameter";
            break;
        case CPL_TYPE_DOUBLE:
            pval = cpl_sprintf("%g", cpl_parameter_get_double(param));
            dval = cpl_sprintf("Default: %g%n",
                               cpl_parameter_get_default_double(param),
                               &dvallen);
            if (!comment) comment = "Floating point recipe parameter";
            break;
        case CPL_TYPE_STRING:
            pval = cpl_strdup(cpl_parameter_get_string(param));
            dval = cpl_sprintf("Default: '%s'%n",
                               cpl_parameter_get_default_string(param),
                               &dvallen);
            if (!comment) comment = "String recipe parameter";
            break;
        default:

            /*
             * Theoretically impossible to get here
             */

            cpl_free((void*)help);
            return cpl_error_set_(CPL_ERROR_UNSPECIFIED);
        }

        snprintf(cval, FLEN_VALUE, PRO_RECi_PARAMi_NAME, level, npar);
        cpl_propertylist_update_string(header, cval, kname);
        cpl_propertylist_set_comment(header, cval, comment);

        keylen = snprintf(cval, FLEN_VALUE, PRO_RECi_PARAMi_VALUE, level, npar);
        cpl_propertylist_update_string(header, cval, pval);
        cpl_propertylist_set_comment_cx(header, CXSTR(cval, keylen),
                                        CXSTR(dval, dvallen));

        cpl_free((void*)help);
        cpl_free((void*)pval);
        cpl_free((void*)dval);

        param = cpl_parameterlist_get_next_const(parlist);
    }


    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Perform any DFS-compliancy required actions (DATAMD5/PIPEFILE update)
  @param  self     The list of frames with FITS products created by the recipe
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @note Each product frame must correspond to a FITS file created with a CPL
        FITS saving function.


   @error
     <table class="ec" align="center">
       <tr>
         <td class="ecl">CPL_ERROR_NULL_INPUT</td>
         <td class="ecr">
            An input pointer is <tt>NULL</tt>.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
         <td class="ecr">
           The input <i>framelist</i> contains a frame of type product with a
           missing filename.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_BAD_FILE_FORMAT</td>
         <td class="ecr">
           The input <i>framelist</i> contains a frame of type product without
           a FITS card with key 'DATAMD5'.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_FILE_IO</td>
         <td class="ecr">
           The input <i>framelist</i> contains a frame of type product for which
           the FITS card with key 'DATAMD5' could not be updated.
         </td>
       </tr>
     </table>
   @enderror

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_dfs_update_product_header(cpl_frameset * self)
{
    return cpl_dfs_update_product_header_(self)
        | cpl_io_fits_close_tid(CPL_IO_FITS_ALL) /* FIXME: Close only writers */
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief
    Update DFS and DICB required header information of product frames.

  @param set    The frameset from which the product frames are taken.
  @param flags  Bit mask for selecting the digital signatures to be written.

  @return
   The function returns @c CPL_ERROR_NONE on success, or an appropriate CPL
   error code otherwise.

  @note
   Each product frame must correspond to a FITS file created with a CPL
   FITS saving function.

  The function takes all frames marked as products from the input frameset
  @em set. For each product the header information @c PIPEFILE is updated
  unconditionally. In addition, depending on the bit mask @em flags, the
  @c DATAMD5 data hash and/or the standard FITS checksums are computed and
  written to the product header. If a digital signature is not selected by
  @em flags when the function is called, its corresponding header keyword(s)
  are removed from the product frame.

   @error
     <table class="ec" align="center">
       <tr>
         <td class="ecl">CPL_ERROR_NULL_INPUT</td>
         <td class="ecr">
            An input pointer is <tt>NULL</tt>.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
         <td class="ecr">
           The input <i>framelist</i> contains a frame of type product with a
           missing filename.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_BAD_FILE_FORMAT</td>
         <td class="ecr">
           The input <i>framelist</i> contains a frame of type product without
           a FITS card with key 'DATAMD5'.
         </td>
       </tr>
       <tr>
         <td class="ecl">CPL_ERROR_FILE_IO</td>
         <td class="ecr">
           The input <i>framelist</i> contains a frame of type product for which
           the FITS card with key 'DATAMD5' could not be updated.
         </td>
       </tr>
     </table>
   @enderror

*/
/*----------------------------------------------------------------------------*/

cpl_error_code
cpl_dfs_sign_products(const cpl_frameset *set, unsigned int flags)
{
    const cpl_frame *frame;

    cpl_error_code status = CPL_ERROR_NONE;

    cpl_frameset_iterator *it;

    cpl_ensure_code(set != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_frameset_is_empty(set) || (flags == CPL_DFS_SIGNATURE_NONE)) {
        return CPL_ERROR_NONE;
    }


    it = cpl_frameset_iterator_new(set);

    while (((frame = cpl_frameset_iterator_get_const(it)) != NULL) &&
            (status == CPL_ERROR_NONE)) {
        cpl_errorstate _status;

        if (cpl_frame_get_group(frame) == CPL_FRAME_GROUP_PRODUCT) {
            status = _cpl_dfs_sign_product(frame, flags);
        }

        _status = cpl_errorstate_get();

        cpl_frameset_iterator_advance(it, 1);

        if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE) {
            cpl_errorstate_set(_status);
        }

    }

    cpl_frameset_iterator_delete(it);


    status |= cpl_io_fits_close_tid(CPL_IO_FITS_ALL);

    return status ? cpl_error_set_where_() : CPL_ERROR_NONE;

}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal

  @brief
    Write DFS and DICB required header information to pipeline products.

  @param frame  The frame to be updated.
  @param flags  Bit mask for selecting the digital signatures to be written.

  @return
   The function returns @c CPL_ERROR_NONE on success, or an appropriate CPL
   error code otherwise.

  @see cpl_dfs_sign_products()
 */
/*----------------------------------------------------------------------------*/

inline static cpl_error_code
_cpl_dfs_sign_product(const cpl_frame *frame, cxuint flags)
{

    const cxchar *filename = cpl_frame_get_filename(frame);

    if (filename == NULL) {

        cpl_error_set(cpl_func, CPL_ERROR_DATA_NOT_FOUND);
        return CPL_ERROR_DATA_NOT_FOUND;

    }

    if (cpl_is_fits(filename)) {

        cxint _status = 0;
        fitsfile *fptr;
        cxint chdu;
        cxint nhdu;


        if (cpl_io_fits_open_diskfile(&fptr, filename, READWRITE, &_status)) {

            return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT,
                                      _status, fits_open_diskfile,
                                      "filename='%s'", filename);

        }


        fits_get_num_hdus(fptr, &nhdu, &_status);

        if (_status) {

            return cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                      _status, fits_get_num_hdus,
                                      "filename='%s'", filename);

        }


        /*
         * Move file pointer to the primary HDU
         */

        fits_movabs_hdu(fptr, 1, NULL, &_status);

        if (_status) {

            return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT,
                                      _status, fits_movabs_hdu,
                                      "filename='%s'", filename);

        }


        /*
         * Write primary HDU signatures
         */

        fits_update_key_str(fptr, PIPEFILE, filename,
                            "Filename of data product", &_status);

        if (_status) {

            return cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                      _status, fits_update_key_str,
                                      "filename='%s', key='"
                                      PIPEFILE "'", filename);

        }


        if (flags & CPL_DFS_SIGNATURE_DATAMD5) {

            cxchar md5sum[MD5HASHSZ + 1];

            if (cpl_dfs_find_md5sum(md5sum, fptr)) {
                return cpl_error_set_where_();
            }

            fits_update_key_str(fptr, DATAMD5, md5sum,
                                "MD5 checksum", &_status);

            if (_status) {

                return cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                          _status, fits_update_key_str,
                                          "filename='%s', key='"
                                          DATAMD5 "', value='%s'",
                                          filename, md5sum);

            }

        }
        else {

            fits_delete_key(fptr, DATAMD5, &_status);

            if (_status == KEY_NO_EXIST) {
                _status = 0;
            }

        }


        if (flags & CPL_DFS_SIGNATURE_CHECKSUM) {

            fits_write_chksum(fptr, &_status);

            if (_status) {

                return cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                          _status, fits_write_chksum,
                                          "filename='%s'", filename);

            }

        }
        else {

            fits_delete_key(fptr, CHECKSUM, &_status);

            if (_status == KEY_NO_EXIST) {
                _status = 0;
            }

            fits_delete_key(fptr, DATASUM, &_status);

            if (_status == KEY_NO_EXIST) {
                _status = 0;
            }

        }


        /*
         * Write extension HDU signatures
         */

        if (nhdu > 1) {

            /*
             * Move file pointer to the first extension
             */

            fits_movabs_hdu(fptr, 2, NULL, &_status);

            if (_status) {

                return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT,
                                          _status, fits_movabs_hdu,
                                          "filename='%s'", filename);

            }

            for (chdu = 2; chdu <= nhdu; ++chdu) {

                if (flags & CPL_DFS_SIGNATURE_CHECKSUM) {

                    fits_write_chksum(fptr, &_status);

                    if (_status) {

                        return cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                                  _status, fits_write_chksum,
                                                  "filename='%s'", filename);

                    }

                }
                else {

                    fits_delete_key(fptr, CHECKSUM, &_status);

                    if (_status == KEY_NO_EXIST) {
                        _status = 0;
                    }

                    fits_delete_key(fptr, DATASUM, &_status);

                    if (_status == KEY_NO_EXIST) {
                        _status = 0;
                    }

                    if (_status) {

                        return cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                                  _status, fits_delete_key,
                                                  "filename='%s'", filename);

                    }

                }

                if (chdu < nhdu) {
                    fits_movrel_hdu(fptr, 1, NULL, &_status);
                }

            }

        }


        if (cpl_io_fits_close_file(fptr, &_status)) {

            return cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                      _status, fits_close_file,
                                      "filename='%s'", filename);

        }

    }

    return CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Perform any DFS-compliancy required actions (DATAMD5/PIPEFILE update)
  @param  self     The list of frames with FITS products created by the recipe
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_dfs_update_product_header()
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_dfs_update_product_header_(cpl_frameset * self)
{
    const cpl_frame * frame;

    cpl_frameset_iterator *it;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);


    it = cpl_frameset_iterator_new(self);

    while ((frame = cpl_frameset_iterator_get_const(it)) != NULL) {

    	cpl_errorstate status;


        if (cpl_frame_get_group(frame) == CPL_FRAME_GROUP_PRODUCT) {

            const char * filename = cpl_frame_get_filename(frame);

            if (filename == NULL) {

            	cpl_frameset_iterator_delete(it);

            	cpl_error_set(cpl_func, CPL_ERROR_DATA_NOT_FOUND);
            	return CPL_ERROR_DATA_NOT_FOUND;

            }

            if (cpl_is_fits(filename)) {

                fitsfile     * fproduct = NULL;
                char           card[FLEN_CARD];
                int            error = 0; /* CFITSIO requires this to be zero */
                cpl_error_code code = CPL_ERROR_NONE;


                if (cpl_io_fits_open_diskfile(&fproduct, filename, READWRITE,
                                              &error)) {

                	cpl_frameset_iterator_delete(it);

                    return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                              fits_open_diskfile,
                                              "filename='%s'", filename);

                }

#ifndef CPL_IO_FITS_REWIND
                /* The previous call may be reusing file handle opened for
                   previous I/O, so the file pointer needs to be moved */
                if (fits_movabs_hdu(fproduct, 1, NULL, &error)) {
                    code = cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                              fits_movabs_hdu,
                                              "filename='%s'", filename);
                }
#endif

                if (fits_read_card(fproduct, DATAMD5, card, &error)) {
                    code = cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                              fits_read_card, "filename='%s', "
                                              "key='" DATAMD5 "'", filename);
                    error = 0;
                }
                else {

                    /* The fits header has the MD5-card, update it */
                    char md5sum[MD5HASHSZ + 1];

                    if (cpl_dfs_find_md5sum(md5sum, fproduct)) {
                        code = cpl_error_set_where_();
                    } else {
                        char* inclist;

                        if (fits_update_key_str(fproduct, DATAMD5, md5sum,
                                                "MD5 checksum", &error)) {
                            code = cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                                      fits_update_key_str,
                                                      "filename='%s', key='"
                                                      DATAMD5 "', value='%s'",
                                                      filename, md5sum);
                            error = 0;
                        } else if (fits_update_key_str(fproduct, PIPEFILE,
                                                       filename,
                                                       "Filename of data"
                                                       " product", &error)) {
                            code = cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                                      fits_update_key_str,
                                                      "filename='%s', key='"
                                                      PIPEFILE "'", filename);
                            error = 0;
                        }

                        /* Check if FITS standard CHECKSUM was present in the
                         * primary header we just modified. If it was then we
                         * must also update that checksum. */

                        /* CFITSIO (up to ver. 3450) has missing
                           const correctness in fits_find_nextkey()/ffgnxk() */
                        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
                        inclist = (char*)CHECKSUM;
                        CPL_DIAG_PRAGMA_POP;

                        if (!fits_find_nextkey(fproduct, &inclist, 1,
                                               NULL, 0, card, &error)) {

                            if (fits_write_chksum(fproduct, &error)) {
                                code = cpl_error_set_fits(CPL_ERROR_FILE_IO,
                                                        error,
                                                        fits_update_key_str,
                                                        "filename='%s', key='"
                                                        CHECKSUM "'", filename);
                                error = 0;
                            }
                        } else if (error == KEY_NO_EXIST) {
                            error = 0; /* Reset error if key not found */
                        } else {
                            code = cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                                      fits_update_key_str,
                                                      "filename='%s', key='"
                                                      CHECKSUM "'", filename);
                            error = 0;
                        }
                    }
                }

                if (cpl_io_fits_close_file(fproduct, &error)) {

                    cpl_frameset_iterator_delete(it);

                    return cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                              fits_close_file,
                                              "filename='%s'", filename);

                }

                if (code) {
                    cpl_frameset_iterator_delete(it);
                    return code;
                }
            }
        }

        status = cpl_errorstate_get();

        cpl_frameset_iterator_advance(it, 1);

        if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE) {
        	cpl_errorstate_set(status);
        }

    }

    cpl_frameset_iterator_delete(it);
    it = NULL;

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Compute the MD5 hash of the data units in a FITS file.
  @param  md5sum   A preallocated string long enough to hold the MD5 digest
  @param  fproduct A CFITSIO FITS file structure, only passed on to CFITSIO
  @return CPL_ERROR_NONE or the relevant _cpl_error_code_ on error
  @note   The input string must have (at least) length of 1 + MD5HASHSZ. Upon
          success the CFITSIO FITS file structure is reset to the 1st HDU

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_dfs_find_md5sum(char * md5sum, fitsfile * fproduct)
{
    struct MD5Context ctx;
    unsigned char     digest[(MD5HASHSZ)>>1];
    int               error = 0;
    int               next = 0;

    /* For performance reasons the number of bytes read and passed on
       by this function should be:
       1) A multiple of a FITS block, 2880 (64 * 45, IOBUFLEN) bytes
       2) A multiple of file-system block size, likely a power of two.
       3) Not exceed the size of the CPU cache (assuming that
          ffgbyt() will cause the loaded data to be placed in the
          cache where it will be readily available for MD5Update().
          If this does not happen, nothing is gained - and no harm is done.
       4) On cache-less architectures, the block size should simply prevent
          significant amounts of memory to be allocated.
    */

    /* The block size is set to 16 FITS blocks (45 kB). This will fit in any
       L1 cache from Intel from about year 2006. It is also a whole number
       (90) of 512 bytes, a common disk sector size. */
    const long maxblocksize = 2880 * 16;
    void     * buffer = NULL;

    /* Iterate through main HDU and all extensions */
    do {

        CPL_OFF_TYPE datastart, datapos, dataend;

        if (CPL_OFF_FUNC(fproduct, NULL, &datastart, &dataend, &error)) {
            cpl_free(buffer);
            return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                      CPL_OFF_FUNC, "HDU#=%d", next);
        }

        datapos = datastart;

        while (datapos < dataend) {
            const long     datatodo  = (long)(dataend - datapos);
            const LONGLONG blocksize = CX_MIN(datatodo, maxblocksize);

            if (buffer == NULL) {
                buffer = cpl_malloc((long)maxblocksize);
                MD5Init(&ctx);
            }

            /* Seek to beginning of Data Unit block (i.e. skip the header) */
            if (datapos == datastart &&
                ffmbyt(fproduct, datastart, 0, &error)) {
                cpl_free(buffer);
                return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                          ffmbyt, "HDU#=%d, datapos=%lu",
                                          next, (unsigned long)datapos);
            }


            /* Read blocksize bytes */
            if (ffgbyt(fproduct, blocksize, buffer, &error)) {
                cpl_free(buffer);
                return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                          ffgbyt, "HDU#=%d, blocksize=%ld",
                                          next, (long)blocksize);
            }

            /* Update MD5 sum with data in current buffer */
            MD5Update(&ctx, (const unsigned char *)buffer,
                      (unsigned)blocksize);

            datapos += blocksize;
        }

        if (dataend != datapos) {
            /* Something went really wrong */
            cpl_free(buffer);
            return cpl_error_set_message_(CPL_ERROR_BAD_FILE_FORMAT,
                                          "HDU#=%d, datapos=%ld != "
                                          "dataend=%ld, sizeof("
                                          CPL_STRINGIFY(CPL_OFF_TYPE) ")=%zu",
                                          next, (long)datapos, (long)dataend,
                                          sizeof(CPL_OFF_TYPE));
        }
        next++;

    } while (!fits_movabs_hdu(fproduct, 1 + next, NULL, &error));

    if (buffer != NULL) {
        int nsize;

        cpl_free(buffer);

        MD5Final(digest, &ctx);

        /* Write digest into a string */
        nsize = sprintf(md5sum,
                        "%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x",
                        digest[ 0],
                        digest[ 1],
                        digest[ 2],
                        digest[ 3],
                        digest[ 4],
                        digest[ 5],
                        digest[ 6],
                        digest[ 7],
                        digest[ 8],
                        digest[ 9],
                        digest[10],
                        digest[11],
                        digest[12],
                        digest[13],
                        digest[14],
                        digest[15]);

        assert(nsize == MD5HASHSZ);
    } else {
        /* The FITS file has no data, which has the below checksum */
        (void)memcpy(md5sum, "d41d8cd98f00b204e9800998ecf8427e",
                     1 + MD5HASHSZ);
    }

    if (error == END_OF_FILE) {
        error = 0; /* Reset CFITSIO error */
    } else {
        return cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                  fits_movabs_hdu, "HDU#=%d", next);
    }

    /* Move back to beginning */
    if (fits_movabs_hdu(fproduct, 1, NULL, &error)) {
        return cpl_error_set_fits(CPL_ERROR_FILE_IO, error,
                                 fits_movabs_hdu, "HDU#=%d", next);
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Determine whether a given file is a FITS file
  @param  self     The name of the fits
  @return 1 if the file is FITS, 0 if not, negative on error

 */
/*----------------------------------------------------------------------------*/
static int cpl_is_fits(const char * self)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    int            error = 0;
    int            is_fits = 1;
    fitsfile     * fptr;


    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, -1);

    if (cpl_io_fits_open_diskfile(&fptr, self, READONLY, &error)) {
        is_fits = error == FILE_NOT_OPENED ? -2 : 0;

        if (is_fits < 0) {
            (void)cpl_error_set_message_(CPL_ERROR_FILE_NOT_FOUND,
                                         "filename='%s'", self);
        } else {
            cpl_errorstate_set(prestate);
        }
        error = 0;
#ifndef CPL_IO_FITS_REWIND
    } else if (fits_movabs_hdu(fptr, 1, NULL, &error)) {
        /* The open call may be reusing file handle opened for
           previous I/O, so the file pointer needs to be moved */
        is_fits = error == FILE_NOT_OPENED ? -2 : 0;

        if (is_fits < 0)
            (void)cpl_error_set_fits(CPL_ERROR_FILE_NOT_FOUND, error,
                                     fits_movabs_hdu, "filename='%s'", self);
        error = 0;
#endif
    } else if (fits_read_imghdr(fptr, 0,  NULL, NULL, NULL, NULL, NULL, NULL,
                                NULL, &error)) {
        is_fits = error == FILE_NOT_OPENED ? -2 : 0;

        if (is_fits < 0)
            (void)cpl_error_set_fits(CPL_ERROR_FILE_NOT_FOUND, error,
                                     fits_read_imghdr, "filename='%s'", self);
        error = 0;
    }

    if (fptr != NULL && cpl_io_fits_close_file(fptr, &error) != 0) {
        /* FIXME: Is this even possible ? */
        is_fits = -1;
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_close_file, "filename='%s'", self);
    }

    return is_fits;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Return a pointer to the basename of a full-path filename
  @param  self   The filename
  @return The pointer (possibly self, which may be NULL)
  @note   The pointer returned by this function may not be used after self is
          de/re-allocated, nor after a call of the function with another string.

 */
/*----------------------------------------------------------------------------*/
static const char * cpl_get_base_name(const char * self)
{
    const char * p = self ? strrchr(self, '/') : NULL;

    return p ? p + 1 : self;
}



/*----------------------------------------------------------------------------*/
/**
  @brief   Open a new PAF file, output a default header.
  @param   instrume  Name of instrument in capitals (NACO, VISIR, etc.)
  @param   recipe    Name of recipe
  @param   filename  Filename of created product
  @return  PAF stream or NULL on error

  This function creates a new PAF file with the requested file name.
  If another file already exists with the same name, it will be overwritten
  (if the file access rights allow it).

  This function returns an opened file pointer, ready to receive more data
  through fprintf()'s. The caller is responsible for fclose()ing the file.

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if fopen(), or fprintf() fails

 */
/*----------------------------------------------------------------------------*/
static FILE * cpl_dfs_paf_init(const char * instrume,
                               const char * recipe,
                               const char * filename)
{
    FILE * paf = NULL;
    char * paf_id = NULL;
    const char paf_desc[] = "QC file";
    int nlen;
    cpl_error_code error = CPL_ERROR_NONE;


    cpl_ensure(instrume  != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(recipe    != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(filename  != NULL, CPL_ERROR_NULL_INPUT, NULL);


    cpl_msg_info(cpl_func, "Writing PAF: %s" , filename);

    paf_id = cpl_sprintf("%s/%s", instrume, recipe);
    assert( paf_id != NULL);

    paf = fopen(filename, "w");

    if (paf == NULL) error = CPL_ERROR_FILE_IO;

    /* Some ugly, traditional error handling that obscures
       the actual functionality (i.e. to fprintf() some strings) */
    
    if (!error) {
        nlen = fprintf(paf, "PAF.HDR.START         ;# start of header\n");
        if (nlen <= PAF_KEY_LEN) error = CPL_ERROR_FILE_IO;
    }

    if (!error) {
        nlen = fprintf(paf, "PAF.TYPE              \"pipeline product\" ;\n");
        if (nlen <= PAF_KEY_LEN) error = CPL_ERROR_FILE_IO;
    }

    if (!error) {
        nlen = fprintf(paf, "PAF.ID                \"%s\"\n", paf_id);
        if (nlen <= PAF_KEY_LEN) error = CPL_ERROR_FILE_IO;
    }
    cpl_free(paf_id);

    if (!error) {
        nlen = fprintf(paf, "PAF.NAME              \"%s\"\n", filename);
        if (nlen <= PAF_KEY_LEN) error = CPL_ERROR_FILE_IO;
    }

    if (!error) {
        nlen = fprintf(paf, "PAF.DESC              \"%s\"\n", paf_desc);
        if (nlen <= PAF_KEY_LEN) error = CPL_ERROR_FILE_IO;
    }

    if (!error) {
        nlen = fprintf(paf, "PAF.CHCK.CHECKSUM     \"\"\n");
        if (nlen <= PAF_KEY_LEN) error = CPL_ERROR_FILE_IO;
    }

    if (!error) {
        nlen = fprintf(paf, "PAF.HDR.END           ;# end of header\n");
        if (nlen <= PAF_KEY_LEN) error = CPL_ERROR_FILE_IO;
    }

    if (!error) {
        nlen = fprintf(paf, "\n");
        if (nlen != 1) error = CPL_ERROR_FILE_IO;
    }

    if (error) {
        if (paf != NULL) {
            (void)fclose(paf);
            paf = NULL;
        }
        (void)cpl_error_set_message_(error, "Could not write PAF, instrume=%s, "
                                     "recipe=%s, filename=%s", instrume,
                                     recipe, filename);
    }

    return paf;

}

/*----------------------------------------------------------------------------*/
/**
  @brief   Print a propertylist as PAF
  @param   self  Propertylist to be printed
  @param   paf   PAF stream
  @return  CPL_ERROR_NONE or the relevant _cpl_error_code_ on error

  The property names are printed with these modifications:
  1) The prefix "ESO " is dropped.
  2) Any space-character is replaced by a . (dot).

  Thus a property name
  ESO QC PART1 PART2 PART3
  will cause the printing of a line with QC.PART1.PART2.PART3 as key.

  Supported property-types:
  CPL_TYPE_CHAR       (cast to cpl_size)
  CPL_TYPE_INT        (cast to cpl_size)
  CPL_TYPE_LONG       (cast to cpl_size)
  CPL_TYPE_LONG_LONG  (cast to cpl_size)
  CPL_TYPE_FLOAT      (cast to double)
  CPL_TYPE_DOUBLE
  CPL_TYPE_STRING
 
  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT       if an input pointer is NULL
  - CPL_ERROR_FILE_IO          if fprintf() fails
  - CPL_ERROR_UNSUPPORTED_MODE if the list contains unsupported property-types

 */
/*----------------------------------------------------------------------------*/
static
cpl_error_code cpl_dfs_paf_dump(const cpl_propertylist * self, FILE * paf)
{
    const cpl_size nproperties = cpl_propertylist_get_size(self);
    cpl_size i;

    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(paf,  CPL_ERROR_NULL_INPUT);

    for (i=0; i < nproperties; i++) {
        const cpl_property * prop = cpl_propertylist_get_const(self, i);
        const char * name    = cpl_property_get_name(prop);
        const char * comment = cpl_property_get_comment(prop);
        const char * usekey;
        const char * usecom;
        char * qckey = NULL;
        char * qccom = NULL;
        /* Initialization needed in absence of #if for CPL_TYPE_LONG */
        cpl_error_code err = CPL_ERROR_UNSUPPORTED_MODE;


        if (strstr(name, "ESO ") == name) {
            /* Drop prefix "ESO " */
            qckey = cpl_sprintf("%s", name+4);
        } else if (strchr(name, ' ')) {
            qckey = cpl_strdup(name);
        }

        if (qckey != NULL) {
            /* Replace <space> with . */
            char * p;
            for (p = qckey; *p != '\0'; p++)
                if (*p == ' ') *p = '.';
            usekey = qckey;
        } else {
            usekey = name;
        }

        if (comment != NULL && strchr(comment, '\n')) {
            char * p;
            /* Replace line-feed with space */
            qccom = cpl_strdup(comment);
            for (p = qccom; *p != '\0'; p++)
                if (*p == '\n') *p = ' ';
            usecom = qccom;
        } else {
            usecom = comment;
        }


        /* Print the property as PAF */

        switch (cpl_property_get_type(prop)) {
        case CPL_TYPE_CHAR:
            err = cpl_dfs_paf_dump_int(usekey, cpl_property_get_char(prop),
                                      usecom, paf);
            break;
        case CPL_TYPE_BOOL:
            err = cpl_dfs_paf_dump_int(usekey, cpl_property_get_bool(prop),
                                      usecom, paf);
            break;
        case CPL_TYPE_INT:
            err = cpl_dfs_paf_dump_int(usekey, cpl_property_get_int(prop),
                                      usecom, paf);
            break;
        case CPL_TYPE_LONG:
            err = cpl_dfs_paf_dump_int(usekey, cpl_property_get_long(prop),
                                       usecom, paf);
            break;
        case CPL_TYPE_LONG_LONG:
            err = cpl_dfs_paf_dump_int(usekey, cpl_property_get_long_long(prop),
                                       usecom, paf);
            break;
        case CPL_TYPE_FLOAT:
            err = cpl_dfs_paf_dump_double(usekey, cpl_property_get_float(prop),
                                         usecom, paf);
            break;
        case CPL_TYPE_DOUBLE:
            err = cpl_dfs_paf_dump_double(usekey, cpl_property_get_double(prop),
                                         usecom, paf);
            break;
        case CPL_TYPE_STRING:
            err = cpl_dfs_paf_dump_string(usekey, cpl_property_get_string(prop),
                                         usecom, paf);
            break;
        default:
            err = CPL_ERROR_UNSUPPORTED_MODE;
        }

        cpl_free(qckey);
        cpl_free(qccom);

        if (err) return cpl_error_set_(err);

    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Print a string-property as PAF
  @param    key      Property key
  @param    value    Property value
  @param    comment  Optional property comment
  @param    paf      PAF stream
  @return   CPL_ERROR_NONE or the relevant _cpl_error_code_ on error

  Possible _cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if a non-optional input pointer is NULL
  - CPL_ERROR_FILE_IO if fprintf() fails
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_dfs_paf_dump_string(const char * key,
                                             const char * value,
                                             const char * comment, FILE * paf)
{
    cpl_ensure_code(paf,   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(key,   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(value, CPL_ERROR_NULL_INPUT);

    if (comment == NULL)
        cpl_ensure_code(fprintf(paf, PAF_KEY_FORMAT "\"%s\"\n",
                                key, value) > PAF_KEY_LEN,
                        CPL_ERROR_FILE_IO);
    else
        cpl_ensure_code(fprintf(paf, PAF_KEY_FORMAT "\"%s\" ; # %s\n",
                                key, value, comment) > PAF_KEY_LEN,
                        CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Print a double-property as PAF
  @param    key      Property key
  @param    value    Property value
  @param    comment  Optional property comment
  @param    paf      PAF stream
  @return   CPL_ERROR_NONE or the relevant _cpl_error_code_ on error
  @see cpl_dfs_paf_dump_string

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_dfs_paf_dump_double(const char * key,
                                             double value,
                                             const char * comment, FILE * paf)
{
    cpl_ensure_code(paf,   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(key,   CPL_ERROR_NULL_INPUT);

    /* The number of decimals for double keywords in CFITSIO is 15
     * (see putkey.c in cfitsio). So we match here that number not to
     * lose precision */
    if (comment == NULL)
        cpl_ensure_code(fprintf(paf, PAF_KEY_FORMAT "%.15g\n",
                                key, value) > PAF_KEY_LEN,
                        CPL_ERROR_FILE_IO);
    else
        cpl_ensure_code(fprintf(paf, PAF_KEY_FORMAT "%.15g ; # %s\n",
                                key, value, comment) > PAF_KEY_LEN,
                        CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Print an int-property as PAF
  @param    key      Property key
  @param    value    Property value
  @param    comment  Optional property comment
  @param    paf      PAF stream
  @return   CPL_ERROR_NONE or the relevant _cpl_error_code_ on error
  @see cpl_dfs_paf_dump_string

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_dfs_paf_dump_int(const char * key, cpl_size value,
                                           const char * comment, FILE * paf)
{
    cpl_ensure_code(paf,   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(key,   CPL_ERROR_NULL_INPUT);

    if (comment == NULL)
        cpl_ensure_code(fprintf(paf, PAF_KEY_FORMAT "%" CPL_SIZE_FORMAT "\n",
                                key, value) > PAF_KEY_LEN,
                        CPL_ERROR_FILE_IO);
    else
        cpl_ensure_code(fprintf(paf, PAF_KEY_FORMAT "%" CPL_SIZE_FORMAT
                                "; # %s\n", key, value, comment) > PAF_KEY_LEN,
                        CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;

}




/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Save either an image or table as a pipeline product
  @param  allframes  The list of input frames for the recipe
  @param  header     NULL, or filled with properties written to product header
  @param  parlist    The list of input parameters
  @param  usedframes The list of raw/calibration frames used for this product
  @param  inherit    NULL, or frame from which header information is inherited
  @param  imagelist  The imagelist to be saved or NULL
  @param  image      The image to be saved or NULL
  @param  type       The type used to represent the data in the file
  @param  table      The table to be saved or NULL
  @param  tablelist  Optional propertylist to use in table extension or NULL
  @param  recipe     The recipe name
  @param  applist    Propertylist to append to primary header, w. PRO.CATG
  @param  remregexp  Optional regexp of properties not to put in main header
  @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
  @param  filename   Filename of created product
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @note At most one of imagelist, image and table may be non-NULL, if all are
        NULL the product frame type will be that of an image
  @see cpl_dfs_save_image()

 */
/*----------------------------------------------------------------------------*/

static cpl_error_code cpl_dfs_product_save(cpl_frameset            * allframes,
                                           cpl_propertylist        * header,
                                           const cpl_parameterlist * parlist,
                                           const cpl_frameset      * usedframes,
                                           const cpl_frame         * inherit,
                                           const cpl_imagelist     * imagelist,
                                           const cpl_image         * image,
                                           cpl_type                  type,
                                           const cpl_table         * table,
                                           const cpl_propertylist  * tablelist,
                                           const char              * recipe,
                                           const cpl_propertylist  * applist,
                                           const char              * remregexp,
                                           const char              * pipe_id,
                                           const char              * filename) {

    const char       * procat;
    cpl_propertylist * plist;
    cpl_frame        * product_frame;
    /* Inside this function the product-types are numbered:
       0: imagelist
       1: table
       2: image
       3: propertylist only
    */
    const unsigned     pronum
        = imagelist != NULL ? 0 : table != NULL ? 1 :  (image != NULL ? 2 : 3);
    const char       * proname[] = {"imagelist", "table", "image",
                                    "propertylist"};
    /* FIXME: Define a frame type for an imagelist and when data-less */
    const int          protype[] = {CPL_FRAME_TYPE_ANY, CPL_FRAME_TYPE_TABLE,
                                    CPL_FRAME_TYPE_IMAGE, CPL_FRAME_TYPE_ANY};
    cpl_error_code     error = CPL_ERROR_NONE;


    /* No more than one of imagelist, table and image may be non-NULL */
    /* tablelist may only be non-NULL when table is non-NULL */
    if (imagelist != NULL) {
        assert(pronum == 0);
        assert(image == NULL);
        assert(table == NULL);
        assert(tablelist == NULL);
    } else if (table != NULL) {
        assert(pronum == 1);
        assert(imagelist == NULL);
        assert(image == NULL);
    } else if (image != NULL) {
        assert(pronum == 2);
        assert(imagelist == NULL);
        assert(table == NULL);
        assert(tablelist == NULL);
    } else {
        assert(pronum == 3);
        assert(imagelist == NULL);
        assert(table == NULL);
        assert(tablelist == NULL);
        assert(image == NULL);
    }

    cpl_ensure_code(allframes  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(parlist    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(usedframes != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(recipe     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(applist    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pipe_id    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(filename   != NULL, CPL_ERROR_NULL_INPUT);

    procat = cpl_propertylist_get_string(applist, CPL_DFS_PRO_CATG);

    cpl_ensure_code(procat     != NULL, cpl_error_get_code());

    cpl_msg_info(cpl_func, "Writing FITS %s product(%s): %s", proname[pronum],
                 procat, filename);

    product_frame = cpl_frame_new();

    /* Create product frame.
       NB: With multiple errors, error will differ from the actual CPL error */
    error |= cpl_frame_set_filename(product_frame, filename);
    error |= cpl_frame_set_tag(product_frame, procat);
    error |= cpl_frame_set_type(product_frame, protype[pronum]);
    error |= cpl_frame_set_group(product_frame, CPL_FRAME_GROUP_PRODUCT);
    error |= cpl_frame_set_level(product_frame, CPL_FRAME_LEVEL_FINAL);

    if (error) {
        cpl_frame_delete(product_frame);
        return cpl_error_set_where_();
    }

    if (header != NULL) {
        cpl_propertylist_empty(header);
        plist = header;
    } else {
        plist = cpl_propertylist_new();
    }

    /* Add any QC parameters here */
    error = cpl_propertylist_append(plist, applist);

    /* Add DataFlow keywords */
    if (!error)
        error = cpl_dfs_setup_product_header(plist, product_frame, usedframes,
                                             parlist, recipe, pipe_id,
                                             CPL_DFS_PRO_DID, inherit);

    if (remregexp != NULL && !error) {
        cpl_errorstate prestate = cpl_errorstate_get();
        (void)cpl_propertylist_erase_regexp(plist, remregexp, 0);
        if (!cpl_errorstate_is_equal(prestate)) error = cpl_error_get_code();
    }

    if (!error) {
        switch (pronum) {
            case 0:
                error = cpl_imagelist_save(imagelist, filename, type, plist,
                                           CPL_IO_CREATE);
                break;
            case 1:
                error = cpl_table_save(table, plist, tablelist, filename,
                                       CPL_IO_CREATE);
                break;
            case 2:
                error = cpl_image_save(image, filename, type, plist,
                                       CPL_IO_CREATE);
                break;
            default:
                /* case 3: */
                error = cpl_propertylist_save(plist, filename, CPL_IO_CREATE);
        }
    }

    if (!error) {
        /* Insert the frame of the saved file in the input frameset */
        error = cpl_frameset_insert(allframes, product_frame);

    } else {
        cpl_frame_delete(product_frame);
    }

    if (plist != header) cpl_propertylist_delete(plist);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;

}    

#if defined CFITSIO_MAJOR && (CFITSIO_MAJOR > 3 || CFITSIO_MINOR >= 26)
/* Not needed from 3.26 */
#else
/*----------------------------------------------------------------------------*/
/**
  @brief  Extract the initial, printable part of a NULL-terminated string
  @param  card    The NULL-terminated input string to extract from
  @return The created, printable string or NULL on empty input or NULL input
  @see fftrec() of CFITSIO v. 3.090.
  @note The returned string can be written to a FITS card.

  A single space replaces various (forbidden) white-space characters:
            TAB char
            Line Feed char
            Vertical Tab
            Form Feed char
            Carriage Return

  With CFITSIO 3.09 and 3.24 this extraction is necessary, while for 3.26
  it is not necessary.

 */
/*----------------------------------------------------------------------------*/

static char * cpl_dfs_extract_printable(const char * self)
{

    char * card = NULL;

    if (self != NULL) {
        int ii;

        card = cpl_strdup(self);

        for (ii = 0; ; ii++) {

            /* Transform various white-space characters to a space */
            /* The if-statements have been copied from fftrec() in fitscore.c */
            if (card[ii] == 9)
                card[ii] = ' ';
            else if (card[ii] == 10)
                card[ii] = ' ';
            else if (card[ii] == 11)
                card[ii] = ' ';
            else if (card[ii] == 12)
                card[ii] = ' ';
            else if (card[ii] == 13)
                card[ii] = ' ';
            else /* All of the above cases are OK and need no check below */

            /* This line has been copied from fftrec() in fitscore.c */
            if (card[ii] < 32 || card[ii] > 126)
                break; /* Will at the latest break on the NULL-terminator */
        }
        card[ii] = '\0';
    }

    return card;
}
#endif


/* Definitions of static MD5 functions */
#include "md5.c"
