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

#include <stdlib.h>
#include <string.h>

#include <cxmemory.h>
#include <cxmessages.h>
#include <cxslist.h>

#include "cpl_parameter.h"
#include "cpl_parameterlist.h"

#include "cpl_errorstate.h"
#include "cpl_error_impl.h"

/**
 * @defgroup cpl_parameterlist Parameter Lists
 *
 * The module implements a parameter list data type, a container for the
 * cpl_parameter type. It provides a convenient way to pass a set of
 * parameters, as a whole, to a function.
 *
 * It is used in the plugin interface (cf. @ref cpl_plugin), for instance,
 * to pass the parameters a recipe accepts from the plugin to the calling
 * application and vice versa.
 *
 * All functions expect a valid pointer to a parameter list as input, unless
 * otherwise specified.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_parameterlist.h>
 * @endcode
 */

/**@{*/

/*
 * Parameter list data type representation
 */

struct _cpl_parameterlist_ {
    cx_slist *data;
    cx_slist_iterator search;
};


/**
 * @brief
 *   Create a new parameter list.
 *
 * @return
 *   A pointer to the newly created parameter list.
 *
 * The function creates a new parameter list object. The created
 * object must be destroyed using the parameter list destructor
 * @b cpl_parameterlist_delete().
 */

cpl_parameterlist *
cpl_parameterlist_new(void)
{
    cpl_parameterlist *list = cx_malloc(sizeof(cpl_parameterlist));

    list->data = cx_slist_new();
    list->search = cx_slist_begin(list->data);

    return list;
}


/**
 * @brief
 *   Destroy a parameter list.
 *
 * @param self  The parameter list to destroy.
 *
 * @return Nothing.
 *
 * The function destroys the parameter list object @em self and all parameters
 * it possibly contains. The individual parameters are destroyed using
 * the parameter destructor (cf. @ref cpl_parameter). If @em self is @c NULL,
 * nothing is done and no error is set.
 */

void
cpl_parameterlist_delete(cpl_parameterlist *self)
{

    if (self) {
        cx_slist_destroy(self->data, (cx_free_func)cpl_parameter_delete);
        cx_free(self);
    }

    return;

}


/**
 * @brief
 *   Get the current size of a parameter list.
 *
 * @param self  A parameter list
 *
 * @return
 *   The parameter list's current size, or 0 if the list is empty.
 *   If an error occurs the function returns 0 and sets an appropriate
 *   error code.
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
 * The function reports the current number of elements stored in the
 * parameter list @em self.
 */

cpl_size
cpl_parameterlist_get_size(const cpl_parameterlist *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return (cpl_size)cx_slist_size(self->data);

}


/**
 * @brief
 *   Append a parameter to a parameter list.
 *
 * @param self       A parameter list.
 * @param parameter  The parameter to append.
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
 * The parameter @em parameter is appended to the parameter list @em self.
 */

cpl_error_code
cpl_parameterlist_append(cpl_parameterlist *self, cpl_parameter *parameter)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    cx_slist_push_back(self->data, parameter);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the first parameter in the given parameter list.
 *
 * @param self  A parameter list.
 *
 * @return
 *   The function returns a handle for the first parameter in the
 *   list, or @c NULL if the list is empty. If an error occurs the
 *   function returns @c NULL and sets an appropriate error code.
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
 * The function returns the first parameter in the parameter list @em self,
 * if it exists. If there is no first parameter, i.e. if the list is empty,
 * @c NULL is returned. The function updates the internal search position
 * cache.
 */

const cpl_parameter *
cpl_parameterlist_get_first_const(const cpl_parameterlist *self)
{


    cpl_parameterlist *_self = (cpl_parameterlist *)self;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    _self->search = cx_slist_begin(self->data);

    if (self->search == cx_slist_end(self->data)) {
        return NULL;
    }

    return cx_slist_get(self->data, self->search);

}

/**
 * @brief
 *   Get the first parameter in the given parameter list.
 *
 * @param self  A parameter list.
 *
 * @return
 *   The function returns a handle for the first parameter in the
 *   list, or @c NULL if the list is empty. If an error occurs the
 *   function returns @c NULL and sets an appropriate error code.
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
 * The function returns the first parameter in the parameter list @em self,
 * if it exists. If there is no first parameter, i.e. if the list is empty,
 * @c NULL is returned. The function updates the internal search position
 * cache.
 */

cpl_parameter *
cpl_parameterlist_get_first(cpl_parameterlist *self)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_parameter *par = (cpl_parameter *)cpl_parameterlist_get_first_const(self);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return par;

}

/**
 * @brief
 *   Get the next parameter in the given list.
 *
 * @param self  A parameter list.
 *
 * @return
 *   The function returns a handle for the next parameter in the
 *   list, or @c NULL if there are no more parameters in the list.
 *   If an error occurs the function returns @c NULL and sets an
 *   appropriate error code.
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
 * The function returns the next parameter in the parameter list @em self if
 * it exists and @c NULL otherwise. The function uses the last cached search
 * position to determine the most recently accessed parameter. This means
 * that the function only works as expected if the @em self has been
 * initialised by a call to @b cpl_parameterlist_get_first_const(), and if no
 * function updating the internal cache was called between two subsequent calls
 * to this function.
 */

const cpl_parameter *
cpl_parameterlist_get_next_const(const cpl_parameterlist *self)
{


    cx_slist_iterator next;

    cpl_parameterlist *_self = (cpl_parameterlist *)self;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    next = cx_slist_next(self->data, self->search);

    if (next == cx_slist_end(self->data)) {
        _self->search = cx_slist_begin(self->data);
        return NULL;
    }

    _self->search = next;
    return cx_slist_get(self->data, next);

}

/**
 * @brief
 *   Get the next parameter in the given list.
 *
 * @param self  A parameter list.
 *
 * @return
 *   The function returns a handle for the next parameter in the
 *   list, or @c NULL if there are no more parameters in the list.
 *   If an error occurs the function returns @c NULL and sets an
 *   appropriate error code.
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
 * The function returns the next parameter in the parameter list @em self if
 * it exists and @c NULL otherwise. The function uses the last cached search
 * position to determine the most recently accessed parameter. This means
 * that the function only works as expected if the @em self has been
 * initialised by a call to @b cpl_parameterlist_get_first(), and if no
 * function updating the internal cache was called between two subsequent calls
 * to this function.
 */

cpl_parameter *
cpl_parameterlist_get_next(cpl_parameterlist *self)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_parameter *par = (cpl_parameter *)cpl_parameterlist_get_next_const(self);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return par;

}

/**
 * @brief
 *   Get the last parameter in the given list.
 *
 * @param self  A parameter list.
 *
 * @return
 *   The function returns a handle for the last parameter in the
 *   list. If an error occurs the function returns @c NULL and sets
 *   an appropriate error code.
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
 *         The parameter <i>self</i> is an empty list.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the last parameter stored in the parameter list
 * @em self. The list @em self @b must @b not be empty.
 */

const cpl_parameter *
cpl_parameterlist_get_last_const(const cpl_parameterlist *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (cx_slist_empty(self->data)) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    return cx_slist_back(self->data);

}

/**
 * @brief
 *   Get the last parameter in the given list.
 *
 * @param self  A parameter list.
 *
 * @return
 *   The function returns a handle for the last parameter in the
 *   list. If an error occurs the function returns @c NULL and sets
 *   an appropriate error code.
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
 *         The parameter <i>self</i> is an empty list.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the last parameter stored in the parameter list
 * @em self. The list @em self @b must @b not be empty.
 */

cpl_parameter *
cpl_parameterlist_get_last(cpl_parameterlist *self)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_parameter *par = (cpl_parameter *)cpl_parameterlist_get_last_const(self);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return par;
}

/**
 * @brief
 *   Find a parameter with the given name in a parameter list.
 *
 * @param self  A parameter list.
 * @param name  The parameter name to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   name @em name, or @c NULL if no such parameter was found. If an
 *   error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>name</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter with the fully  qualified name @em name. If no parameter
 * with this name exists, the function returns @c NULL.
 */

const cpl_parameter *
cpl_parameterlist_find_const(const cpl_parameterlist *self, const char *name)
{


    cx_slist_iterator first, last;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    first = cx_slist_begin(self->data);
    last = cx_slist_end(self->data);

    while (first != last) {

        cpl_parameter *p = cx_slist_get(self->data, first);


        if (strcmp(cpl_parameter_get_name(p), name) == 0) {
            return p;
        }

        first = cx_slist_next(self->data, first);

    }

    return NULL;
}

/**
 * @brief
 *   Find a parameter with the given name in a parameter list.
 *
 * @param self  A parameter list.
 * @param name  The parameter name to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   name @em name, or @c NULL if no such parameter was found. If an
 *   error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>name</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter with the fully  qualified name @em name. If no parameter
 * with this name exists, the function returns @c NULL.
 */

cpl_parameter *
cpl_parameterlist_find(cpl_parameterlist *self, const char *name)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_parameter *par = (cpl_parameter *)cpl_parameterlist_find_const(self, name);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return par;

}

/**
 * @brief
 *   Find a parameter of the given type in a parameter list.
 *
 * @param self  A parameter list.
 * @param type  The parameter type to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   type @em type, or @c NULL if no such parameter was found. If an
 *   error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter whose value is of the type @em type. If no parameter
 * with this type exists, the function returns @c NULL.
 */

const cpl_parameter *
cpl_parameterlist_find_type_const(const cpl_parameterlist *self, cpl_type type)
{


    cx_slist_iterator first, last;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    first = cx_slist_begin(self->data);
    last = cx_slist_end(self->data);

    while (first != last) {

        cpl_parameter *p = cx_slist_get(self->data, first);


        if (cpl_parameter_get_type(p) == type) {
            return p;
        }

        first = cx_slist_next(self->data, first);

    }

    return NULL;
}

/**
 * @brief
 *   Find a parameter of the given type in a parameter list.
 *
 * @param self  A parameter list.
 * @param type  The parameter type to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   type @em type, or @c NULL if no such parameter was found. If an
 *   error occurs the function returns @c NULL and sets an appropriate
 *   error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter whose value is of the type @em type. If no parameter
 * with this type exists, the function returns @c NULL.
 */

cpl_parameter *
cpl_parameterlist_find_type(cpl_parameterlist *self, cpl_type type)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_parameter *par = (cpl_parameter *)cpl_parameterlist_find_type_const(self, type);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return par;

}

/**
 * @brief
 *   Find a parameter which belongs to the given context in a parameter list.
 *
 * @param self     A parameter list.
 * @param context  The parameter context to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   context @em context, or @c NULL if no such parameter was found.
 *   If an error occurs the function returns @c NULL and sets an
 *   appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>context</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter which belongs to the context @em context. If no parameter
 * with this type exists, the function returns @c NULL.
 */

const cpl_parameter *
cpl_parameterlist_find_context_const(const cpl_parameterlist *self,
                               const char *context)
{


    cx_slist_iterator first, last;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    first = cx_slist_begin(self->data);
    last = cx_slist_end(self->data);

    while (first != last) {

        cpl_parameter *p = cx_slist_get(self->data, first);

        const cxchar *_context = cpl_parameter_get_context(p);


        if ((context == NULL) && (_context == NULL)) {

            return p;

        }
        else {

            if (context != NULL && _context != NULL) {

                if (strcmp(_context, context) == 0) {
                    return p;
                }

            }

        }

        first = cx_slist_next(self->data, first);

    }

    return NULL;

}

/**
 * @brief
 *   Find a parameter which belongs to the given context in a parameter list.
 *
 * @param self     A parameter list.
 * @param context  The parameter context to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   context @em context, or @c NULL if no such parameter was found.
 *   If an error occurs the function returns @c NULL and sets an
 *   appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>context</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter which belongs to the context @em context. If no parameter
 * with this type exists, the function returns @c NULL.
 */

cpl_parameter *
cpl_parameterlist_find_context(cpl_parameterlist *self,
                               const char *context)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_parameter *par = (cpl_parameter *)cpl_parameterlist_find_context_const(self, context);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return par;

}


/**
 * @brief
 *   Find a parameter with the given tag in a parameter list.
 *
 * @param self  A parameter list.
 * @param tag   The parameter tag to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   tag @em tag, or @c NULL if no such parameter was found. If an
 *   error occurs the function returns @c NULL and sets an
 *   appropriate error code.
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
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter with the user tag @em tag. If no parameter with this tag
 * exists, the function returns @c NULL.
 */

const cpl_parameter *
cpl_parameterlist_find_tag_const(const cpl_parameterlist *self, const char *tag)
{

    cx_slist_iterator first, last;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    first = cx_slist_begin(self->data);
    last = cx_slist_end(self->data);

    while (first != last) {

        cpl_parameter *p = cx_slist_get(self->data, first);

        const cxchar *_tag = cpl_parameter_get_tag(p);


        if ((tag == NULL) && (_tag == NULL)) {

            return p;
        }
        else {

            if ((tag != NULL) && (_tag != NULL)) {

                if (strcmp(_tag, tag) == 0) {
                    return p;
                }

            }

        }

        first = cx_slist_next(self->data, first);

    }

    return NULL;

}

/**
 * @brief
 *   Find a parameter with the given tag in a parameter list.
 *
 * @param self  A parameter list.
 * @param tag   The parameter tag to search for.
 *
 * @return
 *   The function returns a handle for the first parameter with the
 *   tag @em tag, or @c NULL if no such parameter was found. If an
 *   error occurs the function returns @c NULL and sets an
 *   appropriate error code.
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
 * The function searches the parameter list @em self for the first occurrence
 * of a parameter with the user tag @em tag. If no parameter with this tag
 * exists, the function returns @c NULL.
 */

cpl_parameter *
cpl_parameterlist_find_tag(cpl_parameterlist *self, const char *tag)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_parameter *par = (cpl_parameter *)cpl_parameterlist_find_tag_const(self, tag);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return par;

}

/**
 * @brief
 *   Dump the contents of a parameter list to the given stream.
 *
 * @param self    The parameter list.
 * @param stream  The output stream to use.
 *
 * @return Nothing.
 *
 * The function dumps the debugging information for each parameter found in
 * the parameter list @em self to the output stream @em stream. The debugging
 * information for each individual parameter is dumped using
 * @b cpl_parameter_dump(). If @em self is @c NULL the function does nothing.
 *
 * @see cpl_parameter_dump()
 */

void
cpl_parameterlist_dump(const cpl_parameterlist *self, FILE *stream)
{

    if (self != NULL) {

        cx_slist_const_iterator pos = cx_slist_begin(self->data);

        if (stream == NULL) {
            stream = stdout;
        }

        while (pos != cx_slist_end(self->data)) {

           const cpl_parameter *p = cx_slist_get(self->data, pos);

            cpl_parameter_dump(p, stream);
            pos = cx_slist_next(self->data, pos);

        }

    }

    return;

}
/**@}*/
