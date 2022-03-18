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

#include <cxmemory.h>
#include <cxmessages.h>
#include <cxstrutils.h>

#include "cpl_error_impl.h"
#include "cpl_frame.h"
#include "cpl_fits.h"
#include "cpl_frame_impl.h"
#include "cpl_io_fits.h"
#include "cpl_tools.h"

#include <stdio.h>
#include <string.h>

#include <fitsio.h>


/**
 * @defgroup cpl_frame Frames
 *
 * This module implements the @c cpl_frame type. A frame is a container for
 * descriptive attributes related to a data file. The attributes are related
 * to a data file through the file name member of the frame type. Among the
 * attributes which may be assigned to a data file is an attribute identifying
 * the type of the data stored in the file (image or table data), a
 * classification tag indicating the kind of data the file contains and an
 * attribute denoting to which group the data file belongs (raw, processed or
 * calibration file). For processed data a processing level indicates
 * whether the product is an temporary, intermediate or final product.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_frame.h>
 * @endcode
 */

/**@{*/

/*
 * The frame, attributes and fileinfo types.
 */

typedef struct _cpl_frame_attributes_ cpl_frame_attributes;
typedef struct _cpl_frame_fileinfo_ cpl_frame_fileinfo;

struct _cpl_frame_attributes_ {
    cpl_frame_type type;
    cpl_frame_group group;
    cpl_frame_level level;
};


struct _cpl_frame_fileinfo_ {
    cxchar *name;
};


struct _cpl_frame_ {
    cpl_frame_attributes attributes;
    cpl_frame_fileinfo *fileinfo;
    cxchar *tag;
};


/*
 * Private methods
 */

inline static cpl_frame_fileinfo *
_cpl_frame_fileinfo_new(void)
{

    cpl_frame_fileinfo *info = cx_malloc(sizeof *info);

    info->name = NULL;

    return info;

}


inline static void
_cpl_frame_fileinfo_delete(cpl_frame_fileinfo *info)
{

    if (info->name) {
        cx_free(info->name);
    }

    cx_free(info);
    return;
}

/**
 * @internal
 * @brief
 *   Query the number of planes of an extension.
 *
 * @param self       The frame to query
 * @param extnum     The extension number
 *
 * @return the number of planes.
 *
 * Counts how many planes are in the extension. Returns 0 if no plane
 * is found, and -1 if an error occurred.
 */

cpl_size
cpl_frame_get_nplanes(const cpl_frame *self, cpl_size extnum)
{
    cpl_size         next;
    cpl_size         nplanes;
    cxint            naxis;
    cxint status = 0;
    cxint hdu_type = ANY_HDU;
    cxint hdu_position = extnum + 1;
    fitsfile  *file = NULL;

    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (!self->fileinfo) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return -1;
    }
    cx_assert(self->fileinfo->name != NULL);

    next = cpl_frame_get_nextensions(self);
    if (extnum > next) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }


    cpl_io_fits_open_diskfile(&file, self->fileinfo->name, READONLY, &status);

    if (status == FILE_NOT_OPENED) {
        status = 0;
        cpl_io_fits_close_file(file, &status);
        cpl_error_set_(CPL_ERROR_FILE_IO);
        return -1;

    }
    else {

        if (status != 0) {
            status = 0;
            cpl_io_fits_close_file(file, &status);
            cpl_error_set_(CPL_ERROR_BAD_FILE_FORMAT);
            return -1;

        }
    }

    fits_movabs_hdu(file, hdu_position, &hdu_type, &status);

    if ((status == BAD_HDU_NUM) || (status == END_OF_FILE)) {
        status = 0;
        cpl_io_fits_close_file(file, &status);
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

        return -1;

    }

    /* Find the number of axes  */
    naxis = 0;
    fits_get_img_dim(file, &naxis, &status);

    /* Check validity of naxis */
    if ((naxis < 2) || (naxis > 3)) {
        cpl_io_fits_close_file(file, &status);
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    status = 0;

    /* Two dimensions cube */
    if (naxis == 2) {
        nplanes = 1;
    }
    else {
        /* For 3D cubes, get the third dimension size   */
        CPL_FITSIO_TYPE *naxes = cx_malloc(naxis * sizeof(CPL_FITSIO_TYPE));
        CPL_FITSIO_GET_SIZE(file, naxis, naxes, &status);
        nplanes = (cpl_size)naxes[2];
        if(naxes[2] < 1)
            nplanes = 0;

        cx_free(naxes);
    }

    cpl_io_fits_close_file(file, &status);
    cx_assert(status == 0);

    return nplanes;

}

/*
 * Public methods
 */

/**
 * @brief
 *   Create a new, empty frame.
 *
 * @return
 *   A handle for the newly created frame.
 *
 * The function allocates the memory for the new frame and initializes it
 * to an empty frame, i.e. it is created without tag and file information,
 * and the type, group and level set to @c CPL_FRAME_TYPE_NONE,
 * @c CPL_FRAME_GROUP_NONE, and @c CPL_FRAME_LEVEL_NONE, respectively.
 */

cpl_frame *
cpl_frame_new(void)
{
    cpl_frame *self = cx_malloc(sizeof *self);

    self->tag = NULL;
    self->fileinfo = NULL;

    self->attributes.type = CPL_FRAME_TYPE_NONE;
    self->attributes.group = CPL_FRAME_GROUP_NONE;
    self->attributes.level = CPL_FRAME_LEVEL_NONE;

    return self;
}


/**
 * @brief
 *   Create a copy of a frame.
 *
 * @param other  The frame to copy.
 *
 * @return
 *   The function returns a handle for the created clone. If an error
 *   occurs the function returns @c NULL and sets an appropriate error
 *   code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>other</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function creates a clone of the input frame @em other. All members
 * of the input frame are copied.
 */

cpl_frame *
cpl_frame_duplicate(const cpl_frame *other)
{


    cpl_frame *copy;


    if (other == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    copy = cx_malloc_clear(sizeof *copy);

    if (other->tag) {
        copy->tag = cx_strdup(other->tag);
    }

    if (other->fileinfo) {

        copy->fileinfo = cx_malloc(sizeof(cpl_frame_fileinfo));

        if (other->fileinfo->name) {
            copy->fileinfo->name = cx_strdup(other->fileinfo->name);
        }
        else {
            copy->fileinfo->name = NULL;
        }

    }

    copy->attributes.type = other->attributes.type;
    copy->attributes.group = other->attributes.group;
    copy->attributes.level = other->attributes.level;


    return copy;

}


/**
 * @brief
 *   Destroy a frame.
 *
 * @param self  A frame.
 *
 * @return Nothing.
 *
 * The function deallocates the memory used by the frame @em self.
 * If @em self is @c NULL, nothing is done, and no error is set.
 */

void
cpl_frame_delete(cpl_frame *self)
{

    if (self) {
        if (self->tag) {
            cx_free(self->tag);
        }

        if (self->fileinfo) {
            _cpl_frame_fileinfo_delete(self->fileinfo);
        }

        cx_free(self);
    }

    return;

}


/**
 * @brief
 *   Get the file name to which a frame refers.
 *
 * @param self  A frame.
 *
 * @return
 *   The file name to which the frame refers, or @c NULL if a file name
 *   has not been set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The frame <i>self</i> is not associated to a file.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the read-only name of a file associated to @em self. A file
 * is associated to @em self by calling @b cpl_frame_set_filename() for
 * @em self. If @em self is not associated to a file this function returns
 * @c NULL.
 */

const char *
cpl_frame_get_filename(const cpl_frame *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (!self->fileinfo) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return NULL;
    }

    cx_assert(self->fileinfo->name != NULL);

    return self->fileinfo->name;

}



/**
 * @brief
 *   Get the category tag of a frame.
 *
 * @param self  A frame.
 *
 * @return
 *   The frame's category tag or @c NULL if the tag is not set. The function
 *   returns @c NULL if an error occurs and an appropriate error code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the read-only frame's category tag. If a tag has not yet
 * been set a @c NULL pointer is returned.
 */

const char *
cpl_frame_get_tag(const cpl_frame *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->tag;

}


/**
 * @brief
 *   Get the type of a frame.
 *
 * @param self  A frame.
 *
 * @return
 *   The frame's type. The returns @c CPL_FRAME_TYPE_NONE if an error occurs
 *   and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the type of the data object to which it currently
 * refers.
 */

cpl_frame_type
cpl_frame_get_type(const cpl_frame *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return CPL_FRAME_TYPE_NONE;
    }

    return self->attributes.type;

}


/**
 * @brief
 *   Get the current group of a frame.
 *
 * @param self  A frame.
 *
 * @return
 *   The frame's current group. The function returns @c CPL_FRAME_GROUP_NONE
 *   if an error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the group attribute of the frame @em self.
 */

cpl_frame_group
cpl_frame_get_group(const cpl_frame *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return CPL_FRAME_GROUP_NONE;
    }

    return self->attributes.group;

}


/**
 * @brief
 *   Get the current level of a frame.
 *
 * @param self  A frame.
 *
 * @return
 *   The frame's current level. The function returns @c CPL_FRAME_LEVEL_NONE
 *   if an error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the level attribute of the frame @em self.
 */

cpl_frame_level
cpl_frame_get_level(const cpl_frame *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return CPL_FRAME_LEVEL_NONE;
    }

    return self->attributes.level;

}


/**
 * @brief
 *   Set the file name to which a frame refers.
 *
 * @param self      A frame.
 * @param filename  The new file name.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>filename</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the name of the file, to which the frame @em self
 * refers. Any file name which was previously set by a call to this
 * function is replaced. If no file name is present yet it is created
 * and initialised to @em filename.
 */

cpl_error_code
cpl_frame_set_filename(cpl_frame *self, const char *filename)
{


    cpl_frame_fileinfo *info;


    if (self == NULL || filename == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (!self->fileinfo) {
        self->fileinfo = _cpl_frame_fileinfo_new();
        cx_assert(self->fileinfo != NULL);
    }

    info = self->fileinfo;

    if (info->name) {
        cx_free(info->name);
        info->name = NULL;
    }

    info->name = cx_strdup(filename);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Set a frame's category tag
 *
 * @param self   A frame.
 * @param tag    The new category tag.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>tag</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the category tag of @em self, replacing any
 * previously set tag. If the frame does not yet have a tag is is
 * created and initialised to @em tag.
 */

cpl_error_code
cpl_frame_set_tag(cpl_frame *self, const char *tag)
{



    if (self == NULL || tag == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->tag) {
        cx_free(self->tag);
        self->tag = NULL;
    }

    self->tag = cx_strdup(tag);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Set the type of a frame.
 *
 * @param self   A frame.
 * @param type   New frame type.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the type of the frame @em self to @em type.
 */

cpl_error_code
cpl_frame_set_type(cpl_frame *self, cpl_frame_type type)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->attributes.type = type;
    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Set the group attribute of a frame.
 *
 * @param self   A frame.
 * @param group  New group attribute.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the group attribute of the frame @em self to @em group.
 */

cpl_error_code
cpl_frame_set_group(cpl_frame *self, cpl_frame_group group)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->attributes.group = group;
    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Set the level attribute of a frame.
 *
 * @param self   A frame.
 * @param level  New level attribute.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the level attribute of the frame @em self to @em level.
 */

cpl_error_code
cpl_frame_set_level(cpl_frame *self, cpl_frame_level level)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->attributes.level = level;
    return CPL_ERROR_NONE;

}

/**
 * @brief
 *   Get the number of extensions of this frame
 *
 * @param self  A frame.
 *
 * @return
 *  The number of extensions in the file
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The frame <i>self</i> is not associated to a file.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the number of extensions in the frame or -1 in
 * case of error.
 */

cpl_size
cpl_frame_get_nextensions(const cpl_frame *self)
{
    cpl_size next;

    if (self == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (!self->fileinfo) {
        cpl_error_set(cpl_func, CPL_ERROR_DATA_NOT_FOUND);
        return -1;
    }

    if((next = cpl_fits_count_extensions(self->fileinfo->name)) == -1)
        return -1;

    return next;

}


/**
 * @brief
 *   Dump the frame debugging information to the given stream.
 *
 * @param frame    The frame.
 * @param stream   The output stream to use.
 *
 * @return Nothing.
 *
 * The function dumps the contents of the frame @em frame to the
 * output stream @em stream. If @em stream is @c NULL the function writes
 * to the standard output. If @em frame is @c NULL the function does nothing.
 */

void
cpl_frame_dump(const cpl_frame *frame, FILE *stream)
{

    const char *name;
    const char *tag;

    cpl_frame_type type;
    cpl_frame_group group;
    cpl_frame_level level;

    if (frame == NULL)
        return;

    if (stream == NULL) {
        stream = stdout;
    }

    name = cpl_frame_get_filename(frame);
    tag = cpl_frame_get_tag(frame);

    type = cpl_frame_get_type(frame);
    group = cpl_frame_get_group(frame);
    level = cpl_frame_get_level(frame);

    fprintf(stream, "%s  \t%s  ",name, tag);

    switch (type) {
        case CPL_FRAME_TYPE_IMAGE:
            fprintf(stream, "%s  ","CPL_FRAME_TYPE_IMAGE");
            break;

        case CPL_FRAME_TYPE_MATRIX:
            fprintf(stream, "%s  ","CPL_FRAME_TYPE_MATRIX");
            break;

        case CPL_FRAME_TYPE_TABLE:
            fprintf(stream, "%s  ","CPL_FRAME_TYPE_TABLE");
            break;

        case CPL_FRAME_TYPE_PAF:
            fprintf(stream, "%s  ","CPL_FRAME_TYPE_PAF");
            break;

        case CPL_FRAME_TYPE_ANY:
            fprintf(stream, "%s  ","CPL_FRAME_TYPE_ANY");
            break;

        default:
            fprintf(stream, "%s  ","CPL_FRAME_TYPE_NONE");
            break;
    }


    switch (group) {
        case CPL_FRAME_GROUP_RAW:
            fprintf(stream, "%s  ","CPL_FRAME_GROUP_RAW");
            break;

        case CPL_FRAME_GROUP_CALIB:
            fprintf(stream, "%s  ", "CPL_FRAME_GROUP_CALIB");
            break;

        case CPL_FRAME_GROUP_PRODUCT:
            fprintf(stream, "%s  ", "CPL_FRAME_GROUP_PRODUCT");
            break;

        default:
            fprintf(stream, "%s  ","CPL_FRAME_GROUP_NONE");
            break;
    }


    switch (level) {
        case CPL_FRAME_LEVEL_TEMPORARY:
            fprintf(stream, "%s  ","CPL_FRAME_LEVEL_TEMPORARY");
            break;

        case CPL_FRAME_LEVEL_INTERMEDIATE:
            fprintf(stream, "%s  ","CPL_FRAME_LEVEL_INTERMEDIATE");
            break;

        case CPL_FRAME_LEVEL_FINAL:
            fprintf(stream, "%s  ", "CPL_FRAME_LEVEL_FINAL");
            break;

        default:
            fprintf(stream, "%s  ","CPL_FRAME_LEVEL_NONE");
            break;

    }

    fprintf(stream, "\n");

    return;

}


/**@}*/
