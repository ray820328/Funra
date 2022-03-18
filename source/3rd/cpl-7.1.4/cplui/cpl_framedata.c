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

#include "cpl_framedata.h"
#include "cpl_error_impl.h"


/**
 * @defgroup cpl_framedata  Auxiliary Frame Data
 *
 * @brief
 *   Auxiliary frame data for recipe configurations.
 *
 * This module implements a frame data object, which stores auxiliary
 * information of input and output frames of recipes. This information
 * is used to store the frame configurations of recipes which can be
 * queried by an application which is going to invoke the recipe.
 *
 * The objects stores a frame tag, a unique identifier for a certain
 * kind of frame, the minimum and maximum number of frames needed.
 *
 * A frame is required if the data member @em min_count is set to
 * a value greater than @c 0. The minimum and maximum number of frames is
 * unspecified if the respective member, @em min_count or @em max_count,
 * is set to @c -1.
 *
 * The data members of this structure are public to allow for a static
 * initialization. Any other access of the data members should still
 * be done using the member functions.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_framedata.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *  Create an new frame data object.
 *
 * @return
 *  On success the function returns a pointer to the newly created object,
 *  or @c NULL otherwise.
 *
 * The function allocates the memory for a frame data object, and initializes
 * the data members to their default values, i.e. @em tag is set to @c NULL,
 * and both, @em min_count and @em max_count are set to @c -1.
 */

cpl_framedata*
cpl_framedata_new(void)
{

    cpl_framedata* self = cx_calloc(1, sizeof *self);

    self->tag = NULL;
    self->min_count = -1;
    self->max_count = -1;

    return self;

}


/**
 * @brief
 *  Create a new frame data object and initialize it with the given values.
 *
 * @param tag        The frame tag initializer.
 * @param min_count  The initial value for the minimum number of frames.
 * @param max_count  The initial value for the maximum number of frames.
 *
 * @return
 *  On success the function returns a pointer to the newly created object,
 *  or @c NULL otherwise.
 *
 * The function allocates the memory for a frame data object, and initializes
 * its data members with the given values of the arguments @em tag,
 * @em min_count, and @em max_count.
 */

cpl_framedata*
cpl_framedata_create(const char* tag, cpl_size min_count, cpl_size max_count)
{

    cpl_framedata* self = cx_calloc(1, sizeof *self);

    self->tag = cx_strdup(tag);
    self->min_count = (min_count < -1) ? -1 : min_count;
    self->max_count = (max_count < -1) ? -1 : max_count;

    return self;

}


/**
 * @brief
 *  Create a duplicate of another frame data object.
 *
 * @param other  The frame data object to clone.
 *
 * @return
 *  On success the function returns a pointer to the newly created copy
 *  of the frame data object, or @c NULL otherwise.
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
 * The function creates a clone of the given frame data object @em other.
 * The created copy does not share any resources with the original object.
 */

cpl_framedata*
cpl_framedata_duplicate(const cpl_framedata* other)
{


    cpl_framedata* self = NULL;

    if (other == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }
    else {

        self = cx_calloc(1, sizeof *self);

        self->tag = cx_strdup(other->tag);
        self->min_count = other->min_count;
        self->max_count = other->max_count;

    }

    return self;

}


/**
 * @brief
 *  Clear a frame data object.
 *
 * @param self  The frame data object to clear.
 *
 * @return
 *  Nothing.
 *
 * The function clears the contents of the frame data object @em self, i.e.
 * resets the data members to their default values. If @em self is @c NULL,
 * nothing is done and no error is set.
 */

void
cpl_framedata_clear(cpl_framedata* self)
{

    if (self != NULL) {
        if (self->tag != NULL) {
            cx_free((cxptr)self->tag);
            self->tag = NULL;
        }

        self->min_count = -1;
        self->max_count = -1;
    }

    return;

}


/**
 * @brief
 *  Delete a frame data object.
 *
 * @param self  The frame data object to delete.
 *
 * @return
 *  Nothing.
 *
 * The function destroys the frame data object @em self. All resources used
 * by @em self are released. If @em self is @c NULL, nothing is done and no error is set.
 */

void
cpl_framedata_delete(cpl_framedata* self)
{

    if (self != NULL) {
        cpl_framedata_clear(self);

        cx_free(self);
        self = NULL;
    }

    return;

}


/**
 * @brief
 *  Get the frame tag.
 *
 * @param self  The frame data object.
 *
 * @return
 *  The function returns a pointer to the frame tag on success, or @c NULL
 *  if an error occurred.
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
 * The function returns a handle to the frame tag stored in the frame data
 * object @em self.
 */

const char*
cpl_framedata_get_tag(const cpl_framedata* self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->tag;

}


/**
 * @brief
 *  Get the minimum number of frames.
 *
 * @param self  The frame data object.
 *
 * @return
 *  The function returns the minimum number of frames on success. In case an
 *  error occurred, the return value is @c -2 and an appropriate error code
 *  is set.
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
 * The function returns the minimum number of frames value stored in
 * @em self. If the returned value is @c -1, the minimum number of frames
 * is undefined, i.e. any number may be used.
 */

cpl_size
cpl_framedata_get_min_count(const cpl_framedata* self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -2;
    }

    return self->min_count;

}


/**
 * @brief
 *  Get the maximum number of frames.
 *
 * @param self  The frame data object.
 *
 * @return
 *  The function returns the maximum number of frames on success. In case an
 *  error occurred, the return value is @c -2 and an appropriate error code
 *  is set.
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
 * The function returns the maximum number of frames value stored in
 * @em self. If the returned value is @c -1, the maximum number of frames
 * is undefined, i.e. any number may be used.
 */

cpl_size
cpl_framedata_get_max_count(const cpl_framedata* self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -2;
    }

    return self->max_count;

}


/**
 * @brief
 *  Set the frame tag to the given value.
 *
 * @param self  The frame data object.
 * @param tag   The tag to assign.
 *
 * @return
 *  The function returns @c CPL_ERROR_NONE on success or a CPL error code
 *  otherwise.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>tag</i> is a <tt>NULL</tt> pointer or the empty
 *         string.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function assigns the string @em tag to the corresponding data member
 * of the frame data object @em self by copying its contents. Any previous
 * tag stored in @em self is replaced.
 */

cpl_error_code
cpl_framedata_set_tag(cpl_framedata* self, const char* tag)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if ((tag == NULL) || (tag[0] == '\0')) {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    if (self->tag != NULL) {
        cx_free((cxptr)self->tag);
    }
    self->tag = cx_strdup(tag);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *  Set the minimum number of frames.
 *
 * @param self       The frame data object.
 * @param min_count  The value to set as minimum number of frames.
 *
 * @return
 *  The function returns @c CPL_ERROR_NONE on success or a CPL error code
 *  otherwise.
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
 * The function sets the minimum number of frames value of @em self to
 * @em min_count. If @em min_count is @c -1 the minimum number of frames is
 * unspecified.
 */

cpl_error_code
cpl_framedata_set_min_count(cpl_framedata* self, cpl_size min_count)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->min_count = (min_count < -1) ? -1 : min_count;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *  Set the maximum number of frames.
 *
 * @param self       The frame data object.
 * @param max_count  The value to set as maximum number of frames.
 *
 * @return
 *  The function returns @c CPL_ERROR_NONE on success or a CPL error code
 *  otherwise.
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
 * The function sets the maximum number of frames value of @em self to
 * @em max_count. If @em max_count is @c -1 the maximum number of frames is
 * unspecified.
 */

cpl_error_code
cpl_framedata_set_max_count(cpl_framedata* self, cpl_size max_count)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    self->max_count = (max_count < -1) ? -1 : max_count;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *  Assign new values to a frame data object.
 *
 * @param self       The frame data object.
 * @param tag        The tag to assign.
 * @param min_count  The value to set as minimum number of frames.
 * @param max_count  The value to set as maximum number of frames.
 *
 * @return
 *  The function returns @c CPL_ERROR_NONE on success or a CPL error code
 *  otherwise.
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
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>tag</i> is a <tt>NULL</tt> pointer or the empty
 *         string.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function updates the frame data object @em self with the given
 * values for @em tag, @em min_count, and @em max_count.
 * All previous values stored in @em self are replaced. The string
 * @em tag is assigned by copying its contents.
 *
 * @see cpl_framedata_set_tag()
 */

cpl_error_code
cpl_framedata_set(cpl_framedata* self, const char* tag, cpl_size min_count,
                  cpl_size max_count)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if ((tag == NULL) || (tag[0] == '\0')) {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    if (self->tag != NULL) {
        cx_free((cxptr)self->tag);
    }
    self->tag = cx_strdup(tag);

    self->min_count = (min_count < -1) ? -1 : min_count;
    self->max_count = (max_count < -1) ? -1 : max_count;

    return CPL_ERROR_NONE;

}
/**@}*/
