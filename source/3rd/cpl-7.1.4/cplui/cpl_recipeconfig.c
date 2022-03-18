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

#include <string.h>

#include <cxmemory.h>
#include <cxmessages.h>
#include <cxlist.h>
#include <cxstrutils.h>

#include "cpl_error_impl.h"
#include "cpl_errorstate.h"
#include "cpl_recipeconfig.h"


/**
 * @defgroup cpl_recipeconfig Recipe Configurations
 *
 * This module implements the support for recipe configurations. A recipe
 * configuration stores information about the input data frames a recipe
 * can process, which other input frame are needed in addition, and
 * which output frames may be created by the recipe.
 *
 * For each input frame extra information, for instance, whether a
 * particular frame type is a required or optional recipe input, or
 * how many frames of a certain type are at least needed, can also
 * be stored.
 *
 * The information for the individual recipe configurations and also for
 * the individual frames can be accessed by means of a unique string
 * identifier for the configuration and the frame respectively. This
 * string identifier is called configuration tag in the former, and frame
 * tag in the latter case. In particular, the configuration tag is a frame
 * tag too, namely the frame tag of the recipe's "primary" input, or
 * trigger frame.
 *
 * The recipe configuration object stores a separate configuration for
 * each of the different frame types, indicated by its tag, it is able to
 * process. Each of these configurations can be retrieved, using the
 * appropriate configuration tag as a key.
 *
 * In the same way the information about individual frames can be retrieved
 * from the selected configuration.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_recipeconfig.h>
 * @endcode
 */

/**@{*/

typedef struct _cpl_frameinfo_ cpl_frameinfo;

struct _cpl_frameinfo_ {

    cpl_framedata data;

    cx_list* iframes;
    cx_list* oframes;

};


/*
 * The recipe configuration object
 */

struct _cpl_recipeconfig_ {
    cx_list* frames;
};



static void
_cpl_frameinfo_delete(cpl_frameinfo* self)
{

    if (self != NULL) {

        if (self->oframes != NULL) {
            cx_list_destroy(self->oframes,
                            (cx_free_func)cpl_framedata_delete);
            self->oframes = NULL;
        }

        if (self->iframes != NULL) {
            cx_list_destroy(self->iframes,
                            (cx_free_func)cpl_framedata_delete);
            self->iframes = NULL;
        }

        cpl_framedata_clear(&self->data);

        cx_free(self);
        self = NULL;

    }

    return;

}


inline static cpl_frameinfo*
_cpl_frameinfo_create(const cxchar* tag, cxint min_count, cxint max_count,
                      const cx_list* iframes, const cx_list* oframes)
{

    cpl_frameinfo* self = cx_calloc(1, sizeof *self);

    cpl_errorstate prevstate = cpl_errorstate_get();


    cpl_framedata_set((cpl_framedata*)self, tag, min_count, max_count);

    if (!cpl_errorstate_is_equal(prevstate)) {
        if (cpl_error_get_code() == CPL_ERROR_ILLEGAL_INPUT) {
            _cpl_frameinfo_delete(self);
            self = NULL;

            return NULL;
        } else {
            cpl_errorstate_set(prevstate);
        }
    }

    self->iframes = (cx_list*)iframes;
    self->oframes = (cx_list*)oframes;

    return self;

}


inline static cxchar**
_cpl_recipeconfig_get_tags(const cx_list* self)
{

    cxchar** tags = NULL;

    if (self == NULL) {
        return tags;
    }

    if (cx_list_empty(self) == TRUE) {

        tags = cx_malloc(sizeof(const cxchar*));
        tags[0] = NULL;

        return tags;

    }
    else {

        cxsize i = 0;
        cxsize sz = cx_list_size(self);

        cx_list_iterator entry = cx_list_begin(self);

        tags = cx_calloc(sz + 1, sizeof(const cxchar*));

        while (entry != cx_list_end(self)) {

            const cxchar** tag = cx_list_get(self, entry);

            tags[i] = cx_strdup(*tag);
            entry = cx_list_next(self, entry);
            ++i;
        }

        tags[sz] = NULL;

    }

    return tags;

}


inline static cxptr
_cpl_recipeconfig_find(const cx_list* tags, const cxchar* tag)
{

    cxptr data = NULL;

    cx_list_iterator entry = NULL;


    cx_assert(tags != NULL);
    cx_assert(tag != NULL);

    entry = cx_list_begin(tags);

    while (entry != cx_list_end(tags)) {

        const cxchar** _data = cx_list_get(tags, entry);

        if (strcmp(*_data, tag) == 0) {
            data = _data;
            break;
        }

        entry = cx_list_next(tags, entry);

    }

    return data;

}


inline static cpl_frameinfo*
_cpl_recipeconfig_get_config(const cpl_recipeconfig* self, const cxchar* tag)
{

    cx_assert((self != NULL) && (self->frames != NULL));
    cx_assert(tag != NULL);

    return (cpl_frameinfo*)_cpl_recipeconfig_find(self->frames, tag);

}


/**
 * @brief
 *  Create a new recipe configuration object.
 *
 * @return
 *  The function returns a pointer to the newly created recipe configuration.
 *
 * The function creates a new, empty recipe configuration object.
 */

cpl_recipeconfig*
cpl_recipeconfig_new(void)
{

    cpl_recipeconfig* self = cx_calloc(1, sizeof *self);

    self->frames = cx_list_new();

    return self;

}


/**
 * @brief
 *  Delete a recipe configuration object.
 *
 * @param self  The recipe configuration object.
 *
 * @return
 *  Nothing.
 *
 * The function destroys the recipe configuration object @em self. Any
 * resources used by @em self are released. If @em self is @c NULL, nothing is
 * done and no error is set.
 */

void
cpl_recipeconfig_delete(cpl_recipeconfig* self)
{

    if (self != NULL) {
        if (self->frames != NULL) {
            cx_list_destroy(self->frames,
                            (cx_free_func)_cpl_frameinfo_delete);
            self->frames = NULL;
        }

        cx_free(self);
        self = NULL;
    }

    return;

}


/**
 * @brief
 *  Clear a recipe configuration object.
 *
 * @param self  The recipe configuration object.
 *
 * @return
 *  Nothing.
 *
 * The function clears the contents of the recipe configuration @em self.
 * After the return from this call, @em self is empty.
 *
 * @see cpl_recipeconfig_new()
 */

void
cpl_recipeconfig_clear(cpl_recipeconfig* self)
{

    if (self != NULL) {

        cx_assert(self->frames != NULL);

        if (cx_list_empty(self->frames) == FALSE) {

            cx_list_iterator position = cx_list_begin(self->frames);

            while (position != cx_list_end(self->frames)) {

                cpl_frameinfo* data = cx_list_get(self->frames, position);

                _cpl_frameinfo_delete(data);
                data = NULL;

                position = cx_list_next(self->frames, position);

            }

            cx_list_clear(self->frames);
            cx_assert(cx_list_empty(self->frames));

        }

    }

    return;

}


/**
 * @brief
 *  Get the list of supported configuration tags.
 *
 * @param self  The recipe configuration object.
 *
 * @return
 *  On success, the function returns a @c NULL terminated array of strings,
 *  and @c NULL if an error occurred. In the latter case an appropriate
 *  error code is also set.
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
 * The function retrieves the list of configuration tags stored in the
 * recipe configuration object @em self. The frame tags are returned as the
 * elements of an array of C strings. The last element of the array is
 * a @c NULL pointer indicating the end of the list.
 *
 * In case the recipe configuration object is empty, i.e. no configuration
 * tag has been added, or cpl_recipeconfig_clear() has been called for this
 * object, the function still returns the C string array. In this case the
 * first element is set to @c NULL.
 *
 * If the returned list is not used any more each element, and the array
 * itself must be deallocated using cpl_free().
 */

char**
cpl_recipeconfig_get_tags(const cpl_recipeconfig* self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->frames != NULL);

    return _cpl_recipeconfig_get_tags(self->frames);

}


/**
 * @brief
 *  Set the list of configuration tags.
 *
 * @param self  The recipe configuration object.
 * @param data  A frame data array from which the tags are set.
 *
 * @return
 *  The function returns @c 0 on success, or a non-zero value otherwise.
 *  Inparticular, if the @em self is @c NULL the function returns @c -1
 *  and sets the appropriate error code. If any configuration tag in the
 *  input array @em data is invalid, an empty string for instance, the
 *  function returns @c 1 and sets an appropriate error code.
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
 *         The frame data array <i>data</i> contains an invalid tag.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function is a convenience function to allow an initialization of
 * a recipe configuration from static data. The configuration tags to be
 * stored are taken from the frame data array @em data and are added to the
 * recipe configuration @em self. The configuration tag is copied to
 * @em self.
 *
 * In addition the tags can be configured using the remaining members of
 * the frame data structures of @em data.
 *
 * The function adds each configuration tag found in the array @em data
 * to the configuration @em self, until a configuration tag set to @em NULL
 * is reached. The array @em data must be terminated by such an entry,
 * which indicates the end of the array.
 */

int
cpl_recipeconfig_set_tags(cpl_recipeconfig* self, const cpl_framedata* data)
{



    cxint status = 0;

    cxsize i = 0;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (data == NULL) {
        return 0;
    }

    cx_assert(self->frames != NULL);

    while (data[i].tag != NULL) {

        if (data[i].tag[0] == '\0') {
            cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            status = 1;
        }
        else {
            cxptr info = _cpl_recipeconfig_find(self->frames, data[i].tag);

            if (info != NULL) {
                cx_list_remove(self->frames, info);

                _cpl_frameinfo_delete(info);
                info = NULL;
            }

            info = _cpl_frameinfo_create(data[i].tag, data[i].min_count,
                                         data[i].max_count, cx_list_new(),
                                         cx_list_new());
            cx_list_push_back(self->frames, info);

        }
        ++i;

    }

    return status;

}


/**
 * @brief
 *  Set a configuration tag.
 *
 * @param self       The recipe configuration object.
 * @param tag        The configuration tag.
 * @param min_count  The value to set as the minimum number of frames.
 * @param max_count  The value to set as the maximum number of frames.
 *
 * @return
 *  The function returns @c 0 on success and a non-zero value if an
 *  error occurred. The return value is @c -1 if @em self or @em tag is
 *  @c NULL, or if @em tag is an invalid string. If an error occurs an
 *  appropriate error code is set.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The frame tag <i>tag</i> is an invalid tag, i.e. the empty
 *         string.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function creates a configuration for the configuration tag @em tag
 * and adds it to the recipe configuration object @em self. The minimum and
 * maximum number of frames of this tag can be given using the arguments
 * @em min_count and @em max_count. Using a value of @c -1 for the minimum
 * and maximum number of frames, means that these two numbers are
 * unspecified. If the minimum number of frames is greater than @c 0, the
 * frame is considered to be required, otherwise it is optional.
 */

int
cpl_recipeconfig_set_tag(cpl_recipeconfig* self, const char* tag,
                         cpl_size min_count, cpl_size max_count)
{



    cxptr info = NULL;


    if ((self == NULL) || (tag == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (tag[0] == '\0') {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }


    cx_assert(self->frames != NULL);

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info != NULL) {
        cx_list_remove(self->frames, info);

        _cpl_frameinfo_delete(info);
        info = NULL;
    }

    info = _cpl_frameinfo_create(tag, min_count, max_count,
                                 cx_list_new(), cx_list_new());

    cx_list_push_back(self->frames, info);

    return 0;

}


/**
 * @brief
 *  Get the input configuration for a given tag.
 *
 * @param self  The recipe configuration object.
 * @param tag   The tag for which the input configuration is requested.
 *
 * @return
 *  On success, the function returns a @c NULL terminated array of strings,
 *  and @c NULL if an error occurred. In the latter case an appropriate
 *  error code is also set.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The configuration tag <i>tag</i> was not found.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the list of input frame tags stored in the
 * recipe configuration for the configuration tag @em tag from the
 * configuration object @em self.
 *
 * In case the input configuration for the tag @em tag is empty, i.e. no
 * input frame tag has been added the function still returns a C string
 * array. In this case the first element is set to @c NULL.
 *
 * The returned array and each of its elements must be deallocated using
 * cpl_free() if they are no longer used. The array is @c NULL terminated,
 * i.e. the last element of the array is set to @c NULL.
 */

char**
cpl_recipeconfig_get_inputs(const cpl_recipeconfig* self, const char* tag)
{



    cxchar** tags = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->frames != NULL);

    if (tag == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }
    else {

        cpl_frameinfo* info = _cpl_recipeconfig_get_config(self, tag);

        if (info == NULL) {
            cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
            return NULL;
        }

        tags = _cpl_recipeconfig_get_tags(info->iframes);

    }

    return tags;

}


/**
 * @brief
 *  Set the input configuration for a given tag.
 *
 * @param self  The recipe configuration object.
 * @param tag   The tag for which the input configuration is set.
 * @param data  An array containing the configuration informations.
 *
 * @return
 *  The function returns @c 0 on success, or a non-zero value otherwise.
 *  If @em self or @em tag is @c NULL, or if @em tag is invalid, i.e. the
 *  empty string the function returns @c -1 and sets an appropriate error
 *  code. If no configuration tags were previously configured using
 *  cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags() the function
 *  returns @c 1. If no configuration was found for the given tag @em tag
 *  the return value is @c 2. Finally, if the frame tag to add to the
 *  configuration is invalid, the function returns @c 3.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The frame tag <i>tag</i> is an invalid tag, i.e. the empty
 *         string or the input frame tag read from @em data is invalid,
 *         or the same as @em tag.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         No configuration for the tag <i>tag</i> was found, or <i>self</i>
 *         was not properly initialized.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the input configuration for the tag @em tag in the
 * recipe configuration object @em self. The minimum and maximum number
 * of frames of this tag can be given using the arguments @em min_count
 * and @em max_count. Using a value of @c -1 for the minimum and maximum
 * number of frames, means that these two numbers are unspecified. Using
 * a value greater than @c 0 for the minimum number of frames makes the
 * input frame a required frame.
 *
 * The function sets the configuration data for each input tag specified
 * in the array @em data, until a tag set to @c NULL is reached. The array
 * @em data must contain such an entry as last element, in order to indicate
 * the end of the array.
 *
 * Before an input configuration can be set using this function, the
 * configuration tag @em tag must have been added to @em self previously
 * using cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags().
 *
 * @see cpl_recipeconfig_set_tag(), cpl_recipeconfig_set_tags()
 */

int
cpl_recipeconfig_set_inputs(cpl_recipeconfig* self, const char* tag,
                            const cpl_framedata* data)
{



    cxint status = 0;

    cxsize i = 0;

    cpl_frameinfo* info = NULL;


    if ((self == NULL) || (tag == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (tag[0] == '\0') {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    if (data == NULL) {
        return 0;
    }

    cx_assert(self->frames != NULL);

    if (cx_list_empty(self->frames) == TRUE) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 1;
    }

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info == NULL) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 2;
    }

    while (data[i].tag != NULL) {

        if ((data[i].tag[0] == '\0') || (strcmp(data[i].tag, tag) == 0)) {
            cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            status = 3;
        }
        else {

            cxptr _data = _cpl_recipeconfig_find(info->iframes, data[i].tag);

            if (_data != NULL) {
                cx_list_remove(info->iframes, _data);
                cpl_framedata_delete(_data);
                _data = NULL;
            }

            _data = cpl_framedata_duplicate(&data[i]);
            cx_list_push_back(info->iframes, _data);
        }
        ++i;

    }

    return status;

}


/**
 * @brief
 *  Add the configuration for the given input and configuration tag.
 *
 * @param self       The recipe configuration object.
 * @param tag        The configuration tag.
 * @param input      The input frame tag.
 * @param min_count  The value to set as the minimum number of frames.
 * @param max_count  The value to set as the maximum number of frames.
 *
 * @return
 *  The function returns @c 0 on success and a non-zero value if an
 *  error occurred. The return value is @c -1 if @em self, @em tag or
 *  @em input is @c NULL, or, if @em tag or @em input is an invalid string.
 *  The function returns @c 1 if @em self was not properly initialized using
 *  cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags(). If no
 *  tag @em tag is found in @em self the function returns @c 2. If an error
 *  occurs an appropriate error code is also set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i>, <i>tag</i> or <i>input</i> is a
 *         <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The frame tag <i>tag</i> or <i>input</i> is an empty string,
 *         or <i>tag</i> and <i>input</i> are equal.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         No configuration for the tag <i>tag</i> was found, or <i>self</i>
 *         was not properly initialized.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the configuration for the input frame tag @em input
 * of the configuration associated with the tag @em tag in the recipe
 * configuration object @em self. The minimum and maximum number of frames
 * of this input frame tag are given using the @em min_count and
 * @em max_count arguments. Using a value of @c -1 for the minimum and
 * maximum number of frames, means that these two numbers are unspecified.
 * Using a minimum number @em min_count greater then @c 0 makes the frame
 * a required input.
 *
 * Before an input configuration can be set using this function, the
 * configuration tag @em tag must have been added to @em self previously
 * using cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags().
 *
 * @see cpl_recipeconfig_set_tag(), cpl_recipeconfig_set_tags()
 */

int
cpl_recipeconfig_set_input(cpl_recipeconfig* self,
                           const char* tag, const char* input,
                           cpl_size min_count, cpl_size max_count)
{



    cxptr data = NULL;

    cpl_frameinfo* info = NULL;


    if ((self == NULL) || (tag == NULL) || (input == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (tag[0] == '\0') {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    if ((input[0] == '\0') || (strcmp(input, tag) == 0)) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    cx_assert(self->frames != NULL);

    if (cx_list_empty(self->frames) == TRUE) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 1;
    }

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info == NULL) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 2;
    }

    data = _cpl_recipeconfig_find(info->iframes, input);

    if (data != NULL) {
        cx_list_remove(info->iframes, data);
        cpl_framedata_delete(data);
        data = NULL;
    }

    data = cpl_framedata_create(input, min_count, max_count);
    cx_list_push_back(info->iframes, data);

    return 0;

}


/**
 * @brief
 *  Get the output configuration for a given tag.
 *
 * @param self  The recipe configuration object.
 * @param tag   The tag for which the ouput configuration is requested.
 *
 * @return
 *  On success, the function returns a @c NULL terminated array of strings,
 *  and @c NULL if an error occurred. In the latter case an appropriate
 *  error code is also set.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The configuration tag <i>tag</i> was not found.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the list of all possible output frame tags
 * stored in the recipe configuration object @em self for the
 * configuration tag @em tag.
 *
 * In case the output configuration for the tag @em tag is empty, i.e. no
 * output frame tag has been added the function still returns a C string
 * array. In this case the first element is set to @c NULL.
 *
 * The returned array and each of its elements must be deallocated using
 * cpl_free() if they are no longer used. The array is @c NULL terminated,
 * i.e. the last element of the array is set to @c NULL.
 */

char**
cpl_recipeconfig_get_outputs(const cpl_recipeconfig* self, const char* tag)
{



    cxchar** tags = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->frames != NULL);

    if (tag == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }
    else {

        cpl_frameinfo* info = _cpl_recipeconfig_get_config(self, tag);

        if (info == NULL) {
            cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
            return NULL;
        }

        tags = _cpl_recipeconfig_get_tags(info->oframes);

    }

    return tags;

}


/**
 * @brief
 *  Set the output configuration for a given tag.
 *
 * @param self  The recipe configuration object.
 * @param tag   The tag for which the output configuration is set.
 * @param data  An array of strings containing the output frame tags to set.
 *
 * @return
 *  The function returns @c 0 on success, or a non-zero value otherwise.
 *  If @em self or @em tag is @c NULL, or if @em tag is invalid, i.e. the
 *  empty string the function returns @c -1 and sets an appropriate error
 *  code. If no configuration tags were previously configured using
 *  cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags() the function
 *  returns @c 1. If no configuration was found for the given tag @em tag
 *  the return value is @c 2.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The frame tag <i>tag</i> is an invalid tag, i.e. the empty string.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         No configuration for the tag <i>tag</i> was found, or <i>self</i>
 *         was not properly initialized.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function sets the output configuration for the tag @em tag in the
 * recipe configuration object @em self. The output configuration is a list
 * of all possible frame tags which could result from the execution of the
 * corresponding recipe.
 *
 * The function stores each output frame tag found in the array @em data,
 * until an array element set to @c NULL is reached. The array @em data must
 * contain such an entry as last element, in order to indicate the end of
 * the array.
 *
 * Before an output configuration can be set using this function, the
 * configuration tag @em tag must have been added to @em self previously
 * using cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags().
 *
 * @see cpl_recipeconfig_set_tag(), cpl_recipeconfig_set_tags()
 */

int
cpl_recipeconfig_set_outputs(cpl_recipeconfig* self, const char* tag,
                            const char** data)
{



    cxint status = 0;

    cxsize i = 0;

    cpl_frameinfo* info = NULL;


    if ((self == NULL) || (tag == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (tag[0] == '\0') {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    if (data == NULL) {
        return 0;
    }

    cx_assert(self->frames != NULL);

    if (cx_list_empty(self->frames) == TRUE) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 1;
    }

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info == NULL) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 2;
    }

    while (data[i] != NULL) {

        if (*data[i] == '\0') {
            cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
            status = 3;
        }
        else {

            cxptr _data = _cpl_recipeconfig_find(info->oframes, data[i]);

            if (_data != NULL) {
                cx_list_remove(info->oframes, _data);
                cx_free(_data);
                _data = NULL;
            }

            _data = cpl_framedata_create(data[i], -1, -1);
            cx_list_push_back(info->oframes, _data);

        }
        ++i;

    }

    return status;

}


/**
 * @brief
 *  Add an output frame tag for the given configuration tag.
 *
 * @param self       The recipe configuration object.
 * @param tag        The configuration tag.
 * @param output     The output frame tag to add.
 *
 * @return
 *  The function returns @c 0 on success and a non-zero value if an
 *  error occurred. The return value is @c -1 if @em self, @em tag or
 *  @em output is @c NULL, or if @em tag or @em input is an invalid string.
 *  The function returns @c 1 if @em self was not properly initialized using
 *  cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags(). If no
 *  tag @em tag is found in @em self the function returns @c 2. If an error
 *  occurs an appropriate error code is also set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i>, <i>tag</i> or <i>output</i> is
 *         a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The frame tag <i>tag</i> or <i>input</i> is an invalid string,
 *         i.e. the empty string.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         No configuration for the tag <i>tag</i> was found, or <i>self</i>
 *         was not properly initialized.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function adds the output frame tag @em ouput to the configuration
 * associated with the tag @em tag in the recipe configuration object
 * @em self.
 *
 * Before an output frame tag can be set using this function, the
 * configuration tag @em tag must have been added to @em self previously,
 * using cpl_recipeconfig_set_tag() or cpl_recipeconfig_set_tags().
 *
 * @see cpl_recipeconfig_set_tag(), cpl_recipeconfig_set_tags()
 */

int
cpl_recipeconfig_set_output(cpl_recipeconfig* self, const char* tag,
                            const char* output)
{



    cxptr data = NULL;

    cpl_frameinfo* info = NULL;


    if ((self == NULL) || (tag == NULL) || (output == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if (tag[0] == '\0') {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    if (output[0] == '\0') {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return -1;
    }

    cx_assert(self->frames != NULL);

    if (cx_list_empty(self->frames) == TRUE) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 1;
    }

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info == NULL) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return 2;
    }

    data = _cpl_recipeconfig_find(info->oframes, output);

    if (data != NULL) {
        cx_list_remove(info->oframes, data);
        cpl_framedata_delete(data);
        data = NULL;
    }

    data = cpl_framedata_create(output, -1, -1);
    cx_list_push_back(info->oframes, data);

    return 0;

}


/**
 * @brief
 *  Get the minimum number of frames for the given configuration and tag.
 *
 * @param self  The recipe configuration object.
 * @param tag   The configuration tag to look up.
 * @param input The frame tag to search for.
 *
 * @return
 *  The function returns the minimum number of frames with the tag @em input,
 *  or @c -1, if an error occurred. In the latter case an appropriate error
 *  code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i>, <i>tag</i> or <i>input</i> is a
 *         <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The configuration tag <i>tag</i> is <tt>NULL</tt>, or an invalid
 *         string, the empty string for instance.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         No frame tag <i>tag</i> or <i>input</i> was found, or
 *         <i>self</i> was not properly initialized.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function queries the recipe configuration @em self for the
 * configuration tag @em tag, searches this configuration for the frame tag
 * @em input and returns the minimum number of frames required for this
 * frame type.
 *
 * If the same string is passed as @em tag and @em input, the settings for
 * the frame with tag @em tag are returned.
 */

cpl_size
cpl_recipeconfig_get_min_count(const cpl_recipeconfig* self,
                               const char* tag, const char* input)
{



    const cpl_frameinfo* info = NULL;
    const cpl_framedata* data = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    cx_assert(self->frames != NULL);

    if ((tag == NULL) || (input == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info == NULL) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return -1;
    }

    if (strcmp(tag, input) == 0) {
        data = (const cpl_framedata*)info;
    }
    else {
        data = _cpl_recipeconfig_find(info->iframes, input);

        if (data == NULL) {
            cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
            return -1;
        }
    }

    return data->min_count;

}


/**
 * @brief
 *  Get the maximum number of frames for the given configuration and tag.
 *
 * @param self  The recipe configuration object.
 * @param tag   The configuration tag to look up.
 * @param input The frame tag to search for.
 *
 * @return
 *  The function returns the maximum number of frames with the tag @em input,
 *  or @c -1, if an error occurred. In the latter case an appropriate error
 *  code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i>, <i>tag</i> or <i>input</i> is a
 *         <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The configuration tag <i>tag</i> is <tt>NULL</tt>, or an invalid
 *         string, the empty string for instance.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         No frame tag <i>tag</i> or <i>input</i> was found, or
 *         <i>self</i> was not properly initialized.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function queries the recipe configuration @em self for the
 * configuration tag @em tag, searches this configuration for the frame tag
 * @em input and returns the maximum number of frames required for this
 * frame type.
 *
 * If the same string is passed as @em tag and @em input, the settings for
 * the frame with tag @em tag are returned.
 */

cpl_size
cpl_recipeconfig_get_max_count(const cpl_recipeconfig* self,
                               const char* tag, const char* input)
{



    const cpl_frameinfo* info = NULL;
    const cpl_framedata* data = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    cx_assert(self->frames != NULL);

    if ((tag == NULL) || (input == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info == NULL) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return -1;
    }

    if (strcmp(tag, input) == 0) {
        data = (const cpl_framedata*)info;
    }
    else {
        data = _cpl_recipeconfig_find(info->iframes, input);

        if (data == NULL) {
            cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
            return -1;
        }
    }

    return data->max_count;

}


/**
 * @brief
 *  Check whether a frame with the given tag is required.
 *
 * @param self  A recipe configuration object.
 * @param tag   The configuration tag to look up.
 * @param input The frame tag to search for.
 *
 * @return
 *  The function returns @c 1 if the frame with the tag @em input is required,
 *  and @c 0 if the frame is not a required input. If an error occurred
 *  @c -1 is returned and an appropriate error code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i>, <i>tag</i> or <i>input</i> is a
 *         <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The frame tag <i>tag</i> is <tt>NULL</tt>, an invalid string,
 *         the empty string for instance.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         No frame tag <i>tag</i> or <i>input</i> was found, or
 *         <i>self</i> was not properly initialized.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function queries the recipe configuration @em self for the
 * configuration tag @em tag and searches this configuration for the frame
 * tag @em input. It returns @c 1 if one or more frames of type @em input
 * are a mandatory input for the recipe configuration selected by @em tag.
 *
 * If the same string is passed as @em tag and @em input, the settings for
 * the frame with tag @em tag are returned.
 */

int
cpl_recipeconfig_is_required(const cpl_recipeconfig* self,
                             const char* tag, const char* input)
{



    const cpl_frameinfo* info = NULL;
    const cpl_framedata* data = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    if ((tag == NULL) || (input == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    info = _cpl_recipeconfig_find(self->frames, tag);

    if (info == NULL) {
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return -1;
    }

    if (strcmp(tag, input) == 0) {
        data = (const cpl_framedata*)info;
    }
    else {
        data = _cpl_recipeconfig_find(info->iframes, input);

        if (data == NULL) {
            cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
            return -1;
        }
    }

    return (data->min_count > 0);

}
/**@}*/
