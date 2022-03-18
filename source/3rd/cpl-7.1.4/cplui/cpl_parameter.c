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
#include <cxstrutils.h>

#include "cpl_error.h"
#include "cpl_parameter.h"


/**
 * @defgroup cpl_parameter Parameters
 *
 * This module implements a class of data types which can be used to manage
 * context specific, named parameters. They provide a standard way to pass,
 * for instance, command line information to different components of an
 * application.
 *
 * The fundamental parts of a parameter are its name, a context to which it
 * belongs (a specific component of an application for instance), its
 * current value and a default value.
 *
 * The implementation supports three classes of parameters:
 *   - A plain value
 *   - A range of values
 *   - An enumeration value
 *
 * When a parameter is created it is created for a particular value type.
 * The type of a parameter's current and default value may be (cf.
 * @ref cpl_type):
 *   - @c CPL_TYPE_BOOL
 *   - @c CPL_TYPE_INT
 *   - @c CPL_TYPE_DOUBLE
 *   - @c CPL_TYPE_STRING
 *
 * When a value is assigned to a parameter, the different parameter classes
 * behave differently. For a plain value, no checks are applied to the
 * data value to be assigned. For a range or an enumeration, on the other
 * hand, the value to be stored is validated with respect to the minimum and
 * maximum value of the range and the list of enumeration alternatives
 * respectively. If the value is invalid, an error is reported.
 *
 * @note
 *   The validation of parameter values on assignment is not yet implemented.
 *
 * The functions to create a new parameter of a particular class are
 * polymorphic and accept any value type mentioned above to initialise the
 * parameter to create. This is accomplished by using a variable argument
 * list. The number and type of the parameters expected by these functions
 * is determined from the parameter class and the indicated value type.
 *
 * The constructor of a plain value expects a single value, the parameter's
 * default value, which @em must be of the appropriate type, as argument while
 * the constructor of a range parameter expects the default value, followed
 * by the minimum value and the maximum value. The constructor of an
 * enumeration parameter expects as arguments the default value, the number
 * of possible enumeration values followed by the list of the possible
 * enumeration values. Note that the default value must be a member of the
 * list of possible enumeration values.
 *
 * An example of how parameters of the different classes are created and
 * destroyed is shown below:
 * @code
 *     cpl_parameter *value;
 *     cpl_parameter *range;
 *     cpl_parameter *enumeration;
 *
 *     ...
 *
 *     value = cpl_parameter_new_value("Value", CPL_TYPE_INT,
 *                                     "This is a plain value of type integer",
 *                                     "Example",
 *                                     -1);
 *
 *     range = cpl_parameter_new_range("Range", CPL_TYPE_DOUBLE,
 *                                     "This is a value range of type double",
 *                                     "Example",
 *                                     0.5, 0.0, 1.0);
 *
 *     enumeration = cpl_parameter_new_enum("Enum", CPL_TYPE_STRING,
 *                                          "This is an enumeration of type "
 *                                          "string",
 *                                          "Example",
 *                                          "first", 3, "first", "second",
 *                                          "third");
 *
 *     ...
 *
 *     cpl_parameter_delete(value);
 *     cpl_parameter_delete(range);
 *     cpl_parameter_delete(enumeration);
 * @endcode
 *
 * After the parameter has been created and initialised the parameters current
 * value equals its default value. The initialisation values are copied into
 * the parameter, i.e. if an initialiser value resides in dynamically allocated
 * memory, it can be deallocated after the parameter has been initialised
 * without affecting the created parameter.
 *
 * @note A variable argument list allows only limited type checking. Therefore,
 *   code explicitly what you mean and double check the constructor calls that
 *   the number of arguments and the types of the elements of the variable
 *   argument list are what the constructor expects. For instance, if a
 *   constructor takes a floating-point argument do not forget to put the
 *   decimal point at the end!
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_parameter.h>
 * @endcode
 */

/**@{*/

/*
 * Utility data type storing parameter setup flags
 */

typedef struct _cpl_parameter_info_ cpl_parameter_info;

struct _cpl_parameter_info_ {
    cxbool f_active;
    cxchar *f_key;

    cxbool e_active;
    cxchar *e_key;

    cxbool c_active;
    cxchar *c_key;

    cxint32 id;
    cxbool present;
};


/*
 * Utility data types to store parameter values
 */

union _cpl_parameter_data_ {
    cxchar c;
    cxbool b;
    cxint i;
    cxlong l;
    cxfloat f;
    cxdouble d;
    cxchar *s;
};

typedef union _cpl_parameter_data_ cpl_parameter_data;


struct _cpl_parameter_value_ {
    cpl_type type;
    cpl_parameter_data data;
    cpl_parameter_data preset;
};

typedef struct _cpl_parameter_value_ cpl_parameter_value;


struct _cpl_parameter_range_ {
    cpl_parameter_value value;
    cpl_parameter_data min;
    cpl_parameter_data max;
};

typedef struct _cpl_parameter_range_ cpl_parameter_range;


struct _cpl_parameter_enum_ {
    cpl_parameter_value value;
    cpl_parameter_data *vlist;
    cxsize sz;
};

typedef struct _cpl_parameter_enum_ cpl_parameter_enum;


struct _cpl_parameter_ {
    cxchar *name;
    cxchar *context;
    cxchar *description;
    cxchar *usertag;

    cpl_parameter_class class;
    cpl_parameter_value *data;

    cpl_parameter_info info;
};


/*
 * Private functions
 */

inline static cxbool
_cpl_parameter_validate_type(cpl_type type)
{

    switch (type) {
        case CPL_TYPE_BOOL:
        case CPL_TYPE_INT:
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_STRING:
            break;

        default:
            return FALSE;
            break;
    }

    return TRUE;

}


inline static cxint
_cpl_parameter_data_create(cpl_parameter_data *item, cpl_type type,
                           va_list *ap)
{

    switch (type) {
    case CPL_TYPE_CHAR:
    {
        item->c = (cxchar)va_arg(*ap, cxint);
        break;
    }

    case CPL_TYPE_BOOL:
    {
        cxint arg = va_arg(*ap, cxint);
        item->b = arg ? TRUE : FALSE;
        break;
    }

    case CPL_TYPE_INT:
    {
        item->i = va_arg(*ap, cxint);
        break;
    }

    case CPL_TYPE_LONG:
    {
        item->l = va_arg(*ap, cxlong);
        break;
    }

    case CPL_TYPE_FLOAT:
    {
        item->f = (float)va_arg(*ap, cxdouble);
        break;
    }

    case CPL_TYPE_DOUBLE:
    {
        item->d = va_arg(*ap, cxdouble);
        break;
    }

    case CPL_TYPE_STRING:
    {
        const cxchar *arg = va_arg(*ap, const cxchar *);

        if (item->s) {
            cx_free(item->s);
            item->s = NULL;
        }

        if (arg) {
            item->s = cx_strdup(arg);
        }
        break;
    }

    default:
        return 1;
        break;
    }

    return 0;
}


inline static cxint
_cpl_parameter_data_set(cpl_parameter_data *item, cpl_type type, cxptr value)
{

    switch (type) {
    case CPL_TYPE_CHAR:
    {
        item->c = *(cxchar *)value;
        break;
    }

    case CPL_TYPE_BOOL:
    {
        cxint arg = *(cxint *)value;
        item->b = arg ? TRUE : FALSE;
        break;
    }

    case CPL_TYPE_INT:
    {
        item->i = *(cxint *)value;
        break;
    }

    case CPL_TYPE_LONG:
    {
        item->l = *(cxlong *)value;
        break;
    }

    case CPL_TYPE_FLOAT:
    {
        item->f =  *(cxfloat *)value;
        break;
    }

    case CPL_TYPE_DOUBLE:
    {
        item->d =  *(cxdouble *)value;
        break;
    }

    case CPL_TYPE_STRING:
    {
        const cxchar *const arg = *(const cxchar *const *)value;

        if (item->s) {
            cx_free(item->s);
            item->s = NULL;
        }

        if (arg) {
            item->s = cx_strdup(arg);
        }
        break;
    }

    default:
        return 1;
        break;
    }

    return 0;
}


inline static cpl_parameter_value *
_cpl_parameter_value_new(cpl_type type, cxsize size)
{

    cpl_parameter_value *self = (cpl_parameter_value *)cx_calloc(1, size);

    self->type = type;
    return self;

}


inline static cxint
_cpl_parameter_value_create(cpl_parameter *self, va_list *ap)
{

    cpl_parameter_value *data = NULL;
    cpl_type type = CPL_TYPE_INVALID;


    cx_assert(self != NULL);

    data = self->data;
    cx_assert(data != NULL);

    type = data->type;

    if (_cpl_parameter_data_create(&data->preset, type, ap)) {
        return 1;
    }

    if (_cpl_parameter_data_set(&data->data, type, &data->preset)) {
        return 1;
    }

    return 0;

}


inline static int
_cpl_parameter_value_copy(cpl_parameter *self, const cpl_parameter *other)
{

    cpl_parameter_value *data  = NULL;
    cpl_parameter_value *_data = NULL;


    cx_assert((self != NULL) && (other != NULL));

    data  = (cpl_parameter_value *)self->data;
    _data = (cpl_parameter_value *)other->data;

    cx_assert((data != NULL) && (_data != NULL));

    data->type = _data->type;

    if (_cpl_parameter_data_set(&data->data, data->type, &_data->data)) {
        return 1;
    }

    if (_cpl_parameter_data_set(&data->preset, data->type, &_data->preset)) {
        return 1;
    }

    return 0;

}


inline static void
_cpl_parameter_value_destroy(cpl_parameter_value *self)
{

    if (self->type == CPL_TYPE_STRING) {
        if (self->data.s) {
            cx_free(self->data.s);
            self->data.s = NULL;
        }

        if (self->preset.s) {
            cx_free(self->preset.s);
            self->preset.s = NULL;
        }
    }

    return;

}


inline static cxint
_cpl_parameter_range_create(cpl_parameter *self, va_list *ap)
{

    cpl_parameter_range *data = NULL;
    cpl_type type = CPL_TYPE_INVALID;


    cx_assert(self != NULL);

    data = (cpl_parameter_range *)self->data;
    cx_assert(data != NULL);

    type = data->value.type;

    if (_cpl_parameter_data_create(&data->value.preset, type, ap)) {
        return 1;
    }

    if (_cpl_parameter_data_set(&data->value.data, type,
                                &data->value.preset)) {
        return 1;
    }

    if (_cpl_parameter_data_create(&data->min, type, ap)) {
        return 1;
    }

    if (_cpl_parameter_data_create(&data->max, type, ap)) {
        return 1;
    }

    return 0;

}


inline static int
_cpl_parameter_range_copy(cpl_parameter *self, const cpl_parameter *other)
{

    cpl_parameter_range *data  = NULL;
    cpl_parameter_range *_data = NULL;


    cx_assert((self != NULL) && (other != NULL));


    /*
     * Copy the value part first
     */

    if (_cpl_parameter_value_copy(self, other)) {
        return 1;
    }


    /*
     * Copy domain limits of the range
     */

    data  = (cpl_parameter_range *)self->data;
    _data = (cpl_parameter_range *)other->data;

    cx_assert((data != NULL) && (_data != NULL));

    if (_cpl_parameter_data_set(&data->min, _data->value.type, &_data->min)) {
        return 1;
    }

    if (_cpl_parameter_data_set(&data->max, _data->value.type, &_data->max)) {
        return 1;
    }

    return 0;

}


inline static void
_cpl_parameter_range_destroy(cpl_parameter_value *self)
{

    cpl_parameter_range *_self = (cpl_parameter_range *)self;
    cpl_type type = self->type;

    if (type == CPL_TYPE_STRING) {
        if (_self->min.s) {
            cx_free(_self->min.s);
            _self->min.s = NULL;
        }

        if (_self->max.s) {
            cx_free(_self->max.s);
            _self->max.s = NULL;
        }
    }

    _cpl_parameter_value_destroy(self);

    return;

}


inline static cxint
_cpl_parameter_enum_create(cpl_parameter *self, va_list *ap)
{

    register cxsize i;

    cpl_parameter_enum *data = NULL;
    cpl_type type = CPL_TYPE_INVALID;


    cx_assert(self != NULL);

    data = (cpl_parameter_enum *)self->data;
    cx_assert(data != NULL);

    type = data->value.type;

    if (_cpl_parameter_data_create(&data->value.preset, type, ap)) {
        return 1;
    }

    if (_cpl_parameter_data_set(&data->value.data, type,
                                &data->value.preset)) {
        return 1;
    }


    /*
     * Copy the enum alternatives from the argument list. A zero length
     * list of alternatives is not allowed.
     */

    data->sz = va_arg(*ap, cxint);

    if (data->sz < 1) {
        return -2;
    }

    data->vlist = cx_calloc(data->sz, sizeof(cpl_parameter_data));

    for (i = 0; i < data->sz; ++i) {
        if (_cpl_parameter_data_create(data->vlist + i, type, ap)) {
            return 1;
        }
    }

    return 0;

}


inline static int
_cpl_parameter_enum_copy(cpl_parameter *self, const cpl_parameter *other)
{

    register cxsize i = 0;

    cpl_parameter_enum *data  = NULL;
    cpl_parameter_enum *_data = NULL;


    cx_assert((self != NULL) && (other != NULL));


    /*
     * Copy the value part first
     */

    if (_cpl_parameter_value_copy(self, other)) {
        return 1;
    }


    /*
     * Copy the enum alternatives
     */

    data  = (cpl_parameter_enum *)self->data;
    _data = (cpl_parameter_enum *)other->data;

    cx_assert((data != NULL) && (_data != NULL));

    data->sz = _data->sz;
    data->vlist = cx_calloc(data->sz, sizeof(cpl_parameter_data));

    for (i = 0; i < data->sz; ++i) {
        if (_cpl_parameter_data_set(data->vlist + i, data->value.type,
                                    _data->vlist + i)) {
            cx_free(data->vlist);
            return 1;
        }
    }

    return 0;
}


inline static void
_cpl_parameter_enum_destroy(cpl_parameter_value *self)
{

    cpl_parameter_enum *_self = (cpl_parameter_enum *)self;
    cpl_type type = self->type;

    if (type == CPL_TYPE_STRING) {
        register cxsize i;

        for (i = 0; i < _self->sz; i++) {
            if (_self->vlist[i].s) {
                cx_free(_self->vlist[i].s);
            }
        }
    }

    if (_self->vlist) {
        cx_free(_self->vlist);
    }

    _cpl_parameter_value_destroy(self);

    return;

}


inline static cxint
_cpl_parameter_default_set(cpl_parameter *self, cxptr value)
{

    cpl_parameter_value *data = self->data;
    cpl_type type = data->type;


    cx_assert(value != NULL);

    if (_cpl_parameter_data_set(&data->preset, type, value)) {
        return 1;
    }

    return 0;
}


inline static cxint
_cpl_parameter_value_set(cpl_parameter *self, cxptr value)
{

    cpl_parameter_value *data = self->data;
    cpl_type type = data->type;


    cx_assert(value != NULL);

    if (_cpl_parameter_data_set(&data->data, type, value)) {
        return 1;
    }

    return 0;

}


inline static cpl_parameter *
_cpl_parameter_create(const cxchar *name, const cxchar *description,
                      const cxchar *context)
{
    cpl_parameter *self = NULL;


    cx_assert(name != 0);

    self = (cpl_parameter *) cx_malloc(sizeof(cpl_parameter));

    self->name = cx_strdup(name);
    self->context = context ? cx_strdup(context) : NULL;
    self->description = description ? cx_strdup(description) : NULL;

    self->usertag = NULL;

    self->class = CPL_PARAMETER_CLASS_INVALID;
    self->data = NULL;

    self->info.f_active = TRUE;
    self->info.e_active = TRUE;
    self->info.c_active = TRUE;

    self->info.f_key = cx_strdup(self->name);
    self->info.e_key = cx_strdup(self->name);
    self->info.c_key = cx_strdup(self->name);

    self->info.present = FALSE;
    self->info.id = 0;

    return self;
}


inline static int
_cpl_parameter_init(cpl_parameter *parameter, va_list *ap)
{

    cxint status;

    cx_assert(parameter != NULL);
    cx_assert(ap != NULL);

    switch (parameter->class) {
    case CPL_PARAMETER_CLASS_VALUE:
        status = _cpl_parameter_value_create(parameter, ap);
        break;

    case CPL_PARAMETER_CLASS_RANGE:
        status = _cpl_parameter_range_create(parameter, ap);
        break;

    case CPL_PARAMETER_CLASS_ENUM:
        status = _cpl_parameter_enum_create(parameter, ap);
        break;

    default:
        cpl_error_set(cpl_func, CPL_ERROR_INVALID_TYPE);
        status = -1;
        break;
    }

    if (status) {
        switch (status) {
        case -1:
            cx_warning("%s: parameter at %p has invalid class id %02x",
                       cpl_func, (const void*)parameter, parameter->class);
            break;

        case -2:
            cx_warning("%s: parameter at %p is corrupted due to invalid "
                       "initialization data", cpl_func,
                       (const void*)parameter);
            break;

        default:
            cx_warning("%s: parameter at %p has illegal combination of "
                       "class id (%02x) and type id (%02x)", cpl_func,
                       (const void*)parameter,
                       parameter->class, parameter->data->type);
            break;
        }
        cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_INPUT);
        return 1;
    }

    return 0;

}


inline static int
_cpl_parameter_copy(cpl_parameter *self, const cpl_parameter *other)
{

    cxint status;

    cx_assert((self != NULL) && (other != NULL));

    switch (self->class) {
    case CPL_PARAMETER_CLASS_VALUE:
        status = _cpl_parameter_value_copy(self, other);
        break;

    case CPL_PARAMETER_CLASS_RANGE:
        status = _cpl_parameter_range_copy(self, other);
        break;

    case CPL_PARAMETER_CLASS_ENUM:
        status = _cpl_parameter_enum_copy(self, other);
        break;

    default:
        cpl_error_set(cpl_func, CPL_ERROR_INVALID_TYPE);
        status = -1;
        break;
    }

    if (status) {
        if (status == -1) {
            cx_warning("%s: parameter at %p has invalid class id %02x", cpl_func,
                       (const void *)self, self->class);
        }
        else {
            cx_warning("%s: parameter at %p has illegal combination of"
                       "class id (%02x) and type id (%02x)", cpl_func,
                       (const void *)self, self->class, self->data->type);
        }
        return 1;
    }

    return 0;

}


/**
 * @brief
 *   Create a new value parameter.
 *
 * @param name         The parameter's unique name
 * @param type         The type of the parameter's current and default value.
 * @param description  An optional comment or description of the parameter.
 * @param context      The context to which the parameter belongs.
 * @param ...          Arguments used for parameter initialisation.
 *
 * @return
 *   The function returns the newly created parameter, or @c NULL if
 *   the parameter could not be created. In the latter case an appropriate
 *   error code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>name</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         An unsupported type was passed as <i>type</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The provided parameter initialisation data is invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function allocates memory for a value parameter with the name @em name
 * and a value type @em type. Optionally a comment describing the parameter
 * may be passed by @em description and the context to which the parameter
 * belongs may be given by @em context.
 *
 * The newly created parameter is initialised with the default value passed
 * to the function as the only variable argument list argument. The type
 * of the value must match the parameter's value type as it is indicated by
 * @em type.
 *
 * The following example creates an integer value parameter with the
 * unique name @c application.setup.value which belongs to the context
 * @c application.setup with the default value @em def
 *
 * @par Example:
 *   @code
 *      int def = 1;
 *
 *      cpl_parameter *p = cpl_parameter_new_value("application.setup.value",
 *                                                 CPL_TYPE_INT,
 *                                                 "An integer value",
 *                                                 "application.setup",
 *                                                 def);
 *   @endcode
 *
 * @see cpl_parameter_new_range(), cpl_parameter_new_enum()
 */

cpl_parameter *
cpl_parameter_new_value(const char *name, cpl_type type,
                        const char *description, const char *context, ...)
{


    va_list ap;
    cpl_parameter *p;


    if (!name) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (_cpl_parameter_validate_type(type) != TRUE) {
        cpl_error_set_message(cpl_func, CPL_ERROR_INVALID_TYPE,
                              "Unsupported parameter type %s",
                              cpl_type_get_name(type));
        return NULL;
    }

    p  = _cpl_parameter_create(name, description, context);
    cx_assert(p != NULL);

    p->class = CPL_PARAMETER_CLASS_VALUE;
    p->data = _cpl_parameter_value_new(type, sizeof(cpl_parameter_value));
    cx_assert(p->data != NULL);

    va_start(ap, context);

    if (_cpl_parameter_init(p, &ap)) {
        cpl_parameter_delete(p);
        va_end(ap);
        return NULL;
    }

    va_end(ap);

    return p;
}


/**
 * @brief
 *   Create a new range parameter.
 *
 * @param name         The parameter's unique name
 * @param type         The type of the parameter's current and default value.
 * @param description  An optional comment or description of the parameter.
 * @param context      The context to which the parameter belongs.
 * @param ...          Arguments used for parameter initialisation.
 *
 * @return
 *   The function returns the newly created parameter, or @c NULL if
 *   the parameter could not be created. In the latter case an appropriate
 *   error code is set.
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
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         An unsupported type was passed as <i>type</i>.
 *       </td>
 *     </tr>
  *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The provided parameter initialisation data is invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function allocates memory for a range parameter with the name @em name
 * and a value type @em type. Optionally a comment describing the parameter
 * may be passed by @em description and the context to which the parameter
 * belongs may be given by @em context.
 *
 * To properly initialise the newly created range the parameters default
 * value together with the minimum and maximum value of the range has to
 * be passed as the variable argument list arguments. The type of all
 * initialisation arguments must match the parameter's value type as it
 * is indicated by @em type. Range parameters can only be created for the
 * value types integer or double.
 *
 * The following example creates a double range parameter with the
 * unique name @c application.setup.range which belongs to the context
 * @c application.setup with the default @em def and the range boundary
 * values @em minimum and @em maximum.
 *
 * @par Example:
 *   @code
 *      double def = 0.5;
 *      double min = 0.0;
 *      double max = 1.0;
 *
 *      cpl_parameter *p = cpl_parameter_new_range("application.setup.range",
 *                                                 CPL_TYPE_DOUBLE,
 *                                                 "A range of type double",
 *                                                 "application.setup",
 *                                                 def, min, max);
 *   @endcode
 *
 * @see cpl_parameter_new_value(), cpl_parameter_new_enum()
 */

cpl_parameter *
cpl_parameter_new_range(const char *name, cpl_type type,
                        const char *description, const char *context, ...)
{

    va_list ap;
    cpl_parameter *p;


    if (!name) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (_cpl_parameter_validate_type(type) != TRUE) {
        cpl_error_set_message(cpl_func, CPL_ERROR_INVALID_TYPE,
                              "Unsupported parameter type %s",
                              cpl_type_get_name(type));
        return NULL;
    }

    /*
     * A range parameter of type bool or string does not make sense
     * and is not supported
     */

    if ((type == CPL_TYPE_BOOL) || (type == CPL_TYPE_STRING)) {
        cpl_error_set_message(cpl_func, CPL_ERROR_INVALID_TYPE,
                              "Unsupported parameter type %s",
                              cpl_type_get_name(type));
        return NULL;
    }

    p  = _cpl_parameter_create(name, description, context);
    cx_assert(p != NULL);

    p->class = CPL_PARAMETER_CLASS_RANGE;
    p->data = _cpl_parameter_value_new(type, sizeof(cpl_parameter_range));
    cx_assert(p->data != NULL);

    va_start(ap, context);

    if (_cpl_parameter_init(p, &ap)) {
        cpl_parameter_delete(p);
        va_end(ap);
        return NULL;
    }

    va_end(ap);

    return p;
}


/**
 * @brief
 *   Create a new enumeration parameter.
 *
 * @param name         The parameter's unique name
 * @param type         The type of the parameter's current and default value.
 * @param description  An optional comment or description of the parameter.
 * @param context      The context to which the parameter belongs.
 * @param ...          Arguments used for parameter initialisation.
 *
 * @return
 *   The function returns the newly created parameter, or @c NULL if
 *   the parameter could not be created. In the latter case an appropriate
 *   error code is set.
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
 *       <td class="ecl">CPL_ERROR_INVALID_TYPE</td>
 *       <td class="ecr">
 *         An unsupported type was passed as <i>type</i>.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The provided parameter initialisation data is invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function allocates memory for an enumeration parameter with the name
 * @em name and a value type @em type. Optionally a comment describing the
 * parameter may be passed by @em description and the context to which the
 * parameter belongs may be given by @em context.
 *
 * To properly initialise the newly created enumeration, the parameter's
 * default value (together with the number of alternatives following) and
 * the list of enumeration alternatives must be passed as the variable
 * argument list arguments. The list of enumeration alternatives must
 * contain the default value! Apart from the number of possible alternatives,
 * which must be an argument of type @c int, the type of all initialisation
 * arguments must match the parameter's value type as it is indicated by
 * @em type. Enumeration parameters can only be created for the value types
 * integer, double or string.
 *
 * The following example creates a string enumeration parameter with the
 * unique name @c application.setup.enum which belongs to the context
 * @c application.setup with the default @em def and 3 enumeration
 * alternatives.
 *
 * @par Example:
 *   @code
 *      const char *first = "first";
 *      const char *second = "second";
 *      const char *third = "third";
 *
 *      cpl_parameter *p = cpl_parameter_new_enum("application.setup.enum",
 *                                                 CPL_TYPE_STRING,
 *                                                 "An enum of type string",
 *                                                 "application.setup",
 *                                                 first, 3, first, second,
 *                                                 third);
 *   @endcode
 *
 * @see cpl_parameter_new_value(), cpl_parameter_new_range()
 */

cpl_parameter *
cpl_parameter_new_enum(const char *name, cpl_type type,
                       const char *description, const char *context, ...)
{

    va_list ap;
    cpl_parameter *p;


    if (!name) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (_cpl_parameter_validate_type(type) != TRUE) {
        cpl_error_set_message(cpl_func, CPL_ERROR_INVALID_TYPE,
                              "Unsupported parameter type %s",
                              cpl_type_get_name(type));
        return NULL;
    }

    /*
     * A enum parameter of type bool or string does not make sense
     * and is not supported
     */

    if (type == CPL_TYPE_BOOL) {
        cpl_error_set_message(cpl_func, CPL_ERROR_INVALID_TYPE,
                              "Unsupported parameter type %s",
                              cpl_type_get_name(type));
        return NULL;
    }

    p  = _cpl_parameter_create(name, description, context);
    cx_assert(p != NULL);

    p->class = CPL_PARAMETER_CLASS_ENUM;
    p->data = _cpl_parameter_value_new(type, sizeof(cpl_parameter_enum));
    cx_assert(p->data != NULL);

    va_start(ap, context);

    if (_cpl_parameter_init(p, &ap)) {
        cpl_parameter_delete(p);
        va_end(ap);
        return NULL;
    }

    va_end(ap);

    return p;
}


/**
 * @brief
 *   Create a copy of a parameter.
 *
 * @param self  The parameter to copy.
 *
 * @return
 *   The copy of the given parameter, or @c NULL in case of an error.
 *   In the latter case an appropriate error code is set.
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
 * The function returns a copy of the parameter @em self. The copy is a
 * deep copy, i.e. all parameter properties are copied.
 */

cpl_parameter *cpl_parameter_duplicate(const cpl_parameter *self)
{

    cpl_parameter *_self = NULL;


    if (self == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    _self = cx_malloc(sizeof(cpl_parameter));

    _self->name = cx_strdup(self->name);
    _self->context = cx_strdup(self->context);
    _self->description = cx_strdup(self->description);
    _self->usertag = cx_strdup(self->usertag);

    _self->class = self->class;
    _self->data = NULL;

    switch (_self->class) {
    case CPL_PARAMETER_CLASS_VALUE:
        _self->data = _cpl_parameter_value_new(self->data->type,
                                               sizeof(cpl_parameter_value));
        break;

    case CPL_PARAMETER_CLASS_RANGE:
        _self->data = _cpl_parameter_value_new(self->data->type,
                                               sizeof(cpl_parameter_range));
        break;

    case CPL_PARAMETER_CLASS_ENUM:
        _self->data = _cpl_parameter_value_new(self->data->type,
                                               sizeof(cpl_parameter_enum));
        break;

    default:
        cpl_parameter_delete(_self);
        cpl_error_set(cpl_func, CPL_ERROR_INVALID_TYPE);
        return NULL;
        break;
    }

    cx_assert(_self->data != NULL);

    if (_cpl_parameter_copy(_self, self)) {
        cpl_parameter_delete(_self);
        return NULL;
    }

    _self->info.f_active = self->info.f_active;
    _self->info.e_active = self->info.e_active;
    _self->info.c_active = self->info.c_active;

    _self->info.f_key = cx_strdup(self->info.f_key);
    _self->info.e_key = cx_strdup(self->info.e_key);
    _self->info.c_key = cx_strdup(self->info.c_key);

    _self->info.present = self->info.present;
    _self->info.id = self->info.id;

    return _self;

}


/**
 * @brief
 *   Delete a parameter.
 *
 * @param self  The parameter to delete.
 *
 * @return Nothing.
 *
 * The function deletes a parameter of any class and type. All memory resources
 * acquired by the parameter during its usage are released. If the parameter @em self
 * is @c NULL, nothing is done and no error is set.
 */

void
cpl_parameter_delete(cpl_parameter *self)
{

    if (self != NULL) {
        if (self->data != NULL) {

            switch (self->class) {
                case CPL_PARAMETER_CLASS_VALUE:
                    _cpl_parameter_value_destroy(self->data);
                    break;

                case CPL_PARAMETER_CLASS_RANGE:
                    _cpl_parameter_range_destroy(self->data);
                    break;

                case CPL_PARAMETER_CLASS_ENUM:
                    _cpl_parameter_enum_destroy(self->data);
                    break;

                default:
                    break;
            }
            cx_free(self->data);
            self->data = NULL;
        }

        if (self->usertag != NULL) {
            cx_free(self->usertag);
            self->usertag = NULL;
        }

        if (self->description != NULL) {
            cx_free(self->description);
            self->description = NULL;
        }

        if (self->context != NULL) {
            cx_free(self->context);
            self->context = NULL;
        }

        if (self->name != NULL) {
            cx_free(self->name);
            self->name = NULL;
        }

        if (self->info.f_key != NULL) {
            cx_free(self->info.f_key);
            self->info.f_key = NULL;
        }

        if (self->info.e_key != NULL) {
            cx_free(self->info.e_key);
            self->info.e_key = NULL;
        }

        if (self->info.c_key != NULL) {
            cx_free(self->info.c_key);
            self->info.c_key = NULL;
        }

        cx_free(self);
    }

    return;
}


/**
 * @brief
 *   Assign a boolean value to a parameter.
 *
 * @param self   The parameter the value is assigned to.
 * @param value  The boolean value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_BOOL.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function assigns a boolean value @em value to a parameter of type
 * @c CPL_TYPE_BOOL. Any non-zero value passed as @em value is interpreted
 * as @em true.
 */

cpl_error_code
cpl_parameter_set_bool(cpl_parameter *self, int value)
{



    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_BOOL) {
        return cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                     "type %s", cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_value_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Assign an integer value to a parameter.
 *
 * @param self   The parameter the value is assigned to.
 * @param value  The integer value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function assigns an integer value @em value to a parameter of type
 * @c CPL_TYPE_INT.
 */

cpl_error_code
cpl_parameter_set_int(cpl_parameter *self, int value)
{



    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_INT) {
        return cpl_error_set_message(cpl_func,
                                     CPL_ERROR_TYPE_MISMATCH, "%s of type %s",
                                     cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_value_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Assign a double value to a parameter.
 *
 * @param self   The parameter the value is assigned to.
 * @param value  The double value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_DOUBLE.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function assigns a double value @em value to a parameter of type
 * @c CPL_TYPE_DOUBLE.
 */

cpl_error_code
cpl_parameter_set_double(cpl_parameter *self, double value)
{



    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_DOUBLE) {
        return cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                     "type %s", cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_value_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Assign a string value to a parameter.
 *
 * @param self   The parameter the value is assigned to.
 * @param value  The string value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_STRING.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function assigns a string value @em value to a parameter of type
 * @c CPL_TYPE_STRING.
 */

cpl_error_code
cpl_parameter_set_string(cpl_parameter *self, const char *value)
{



    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_STRING) {
        return cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                     "type %s", cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_value_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Modify the default value of a boolean parameter.
 *
 * @param self   The parameter whose default value is modified.
 * @param value  The boolean value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_BOOL.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function changes the default value of the parameter @em self to the
 * boolean value @em value. The parameter @em self must be of type
 * @c CPL_TYPE_BOOL. Any non-zero value passed as @em value is interpreted
 * as @em true.
 */

cpl_error_code
cpl_parameter_set_default_bool(cpl_parameter *self, int value)
{


    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_BOOL) {
        return cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                     "type %s", cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_default_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Modify the default value of an integer parameter.
 *
 * @param self   The parameter whose default value is modified.
 * @param value  The integer value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function changes the default value of the parameter @em self to the
 * integer value @em value. The parameter @em self must be of type
 * @c CPL_TYPE_INT.
 */

cpl_error_code
cpl_parameter_set_default_int(cpl_parameter *self, int value)
{


    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_INT) {
        return cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                     "type %s", cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_default_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Modify the default value of a double parameter.
 *
 * @param self   The parameter whose default value is modified.
 * @param value  The double value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_DOUBLE.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function changes the default value of the parameter @em self to the
 * double value @em value. The parameter @em self must be of type
 * @c CPL_TYPE_DOUBLE.
 */

cpl_error_code
cpl_parameter_set_default_double(cpl_parameter *self, double value)
{


    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_DOUBLE) {
        return cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                     "type %s", cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_default_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Modify the default value of a string parameter.
 *
 * @param self   The parameter whose default value is modified.
 * @param value  The string value to be assigned.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_STRING.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function changes the default value of the parameter @em self to the
 * string value @em value. The parameter @em self must be of type
 * @c CPL_TYPE_STRING.
 */

cpl_error_code
cpl_parameter_set_default_string(cpl_parameter *self, const char *value)
{


    if (!self) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_STRING) {
        return cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                     "type %s", cpl_parameter_get_name(self),
                                     cpl_type_get_name(cpl_parameter_get_type
                                                       (self)));
    }

    _cpl_parameter_default_set(self, &value);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the name of a parameter.
 *
 * @param self  A parameter.
 *
 * @return
 *   The function returns the parameter's unique name, or @c NULL if
 *   @em self is not a valid parameter but @c NULL. In the latter case
 *   an appropriate error code is set.
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
 * The function returns the read-only unique name of the parameter @em self. The
 * parameter name @b must not be modified using the returned pointer.
 */

const char *
cpl_parameter_get_name(const cpl_parameter *self)
{


    if (self == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->name != NULL);

    return self->name;

}


/**
 * @brief
 *   Get the context of a parameter.
 *
 * @param self  A parameter.
 *
 * @return
 *   The function returns the context of the parameter, or @c NULL if
 *   @em self is not a valid parameter but @c NULL. In the latter case
 *   an appropriate error code is set.
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
 * The function returns the read-only context to which the parameter @em self belongs.
 * The parameter's context @b must not be modified using the returned pointer.
 */

const char *
cpl_parameter_get_context(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->context;

}


/**
 * @brief
 *   Get the description of a parameter.
 *
 * @param self  A parameter.
 *
 * @return
 *   The function returns the parameter description, or @c NULL if
 *   no description has been set. The function returns @c NULL if an error
 *   occurs and sets an appropriate error code.
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
 * The function returns the read-only short help of the parameter @em self. The
 * parameter description @b must not be modified using the returned pointer.
 */

const char *
cpl_parameter_get_help(const cpl_parameter *self)
{

    if (!self) {
        (void)cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->description;

}



/**
 * @brief
 *   Get the parameter's value type.
 *
 * @param self  A parameter.
 *
 * @return
 *   The function returns the parameter's value type. The function
 *   returns @c CPL_TYPE_INVALID if @em self is not a valid parameter,
 *   but @c NULL. In this case an appropriate error is also set.
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
 * The type identifier of the parameter's value is retrieved from the
 * given parameter @em self.
 */

cpl_type
cpl_parameter_get_type(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return CPL_TYPE_INVALID;
    }

    return self->data->type;

}


/**
 * @brief
 *   Get the parameter's class.
 *
 * @param self  A parameter.
 *
 * @return The function returns the parameter's class identifier. The
 *   function returns @c CPL_PARAMETER_CLASS_INVALID if @em self is not a
 *   valid parameter, but @c NULL. In this case an appropriate error code
 *   is also set.
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
 * The class identifier of the parameter @em self is retrieved.
 */

cpl_parameter_class
cpl_parameter_get_class(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return CPL_PARAMETER_CLASS_INVALID;
    }

    return self->class;

}


/**
 * @brief
 *   Get the value of the given boolean parameter.
 *
 * @param self  A boolean parameter.
 *
 * @return
 *   The function returns the integer representation of the parameter's
 *   boolean value. @c TRUE is represented as non-zero value; whereas 0
 *   indicates @c FALSE. If an error occurs the function returns 0 and sets
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_BOOL.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current boolean value of the parameter @em self is retrieved.
 */

int
cpl_parameter_get_bool(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_BOOL) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    return self->data->data.b;

}


/**
 * @brief
 *   Get the value of the given integer parameter.
 *
 * @param self  An integer parameter.
 *
 * @return
 *   The function returns the parameter's integer value. If an error
 *   occurs the function returns 0 and sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current integer value of the parameter @em self is retrieved.
 */

int
cpl_parameter_get_int(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_INT) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    return self->data->data.i;

}


/**
 * @brief
 *   Get the value of the given double parameter.
 *
 * @param self  An double parameter.
 *
 * @return
 *   The function returns the parameter's double value. If an error
 *   occurs the function returns 0 and sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current double value of the parameter @em self is retrieved.
 */

double
cpl_parameter_get_double(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0.;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_DOUBLE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    return self->data->data.d;

}


/**
 * @brief
 *   Get the value of the given string parameter.
 *
 * @param self  A string parameter.
 *
 * @return
 *   The function returns the parameter's string value. If an error
 *   occurs the function returns @c NULL and sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_STRING.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current string value of the parameter @em self is retrieved.
 */

const char *
cpl_parameter_get_string(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_STRING) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return NULL;
    }

    return self->data->data.s;

}


/**
 * @brief
 *   Get the default value of the given boolean parameter.
 *
 * @param self  A boolean parameter.
 *
 * @return
 *   The function returns the integer representation of the parameter's
 *   default boolean value. @c TRUE is represented as non-zero value;
 *   whereas 0 indicates @c FALSE. If an error occurs the function returns
 *   0 and sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_BOOL.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current boolean default value of the parameter @em self is retrieved.
 */

int
cpl_parameter_get_default_bool(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_BOOL) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    return self->data->preset.b;

}


/**
 * @brief
 *   Get the default value of the given integer parameter.
 *
 * @param self  An integer parameter.
 *
 * @return
 *   The function returns the parameter's default integer value. If an
 *   error occurs the function returns 0 and sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current integer default value of the parameter @em self is retrieved.
 */

int
cpl_parameter_get_default_int(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_INT) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    return self->data->preset.i;

}


/**
 * @brief
 *   Get the default value of the given double parameter.
 *
 * @param self  An double parameter.
 *
 * @return
 *   The function returns the parameter's default double value. If an
 *   error occurs the function returns 0 and sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_DOUBLE.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current double default value of the parameter @em self is retrieved.
 */

double
cpl_parameter_get_default_double(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0.;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_DOUBLE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    return self->data->preset.d;

}


/**
 * @brief
 *   Get the default value of the given string parameter.
 *
 * @param self  An string parameter.
 *
 * @return
 *   The function returns the parameter's default string value. If an
 *   error occurs the function returns 0 and sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The type of the parameter <i>self</i> is not of type
 *         @c CPL_TYPE_STRING.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The current read-only string default value of the parameter @em self is retrieved.
 * If @em self is not a valid pointer the error code @c CPL_ERROR_NULL_INPUT
 * is set on return. Also, if the parameter @em self is not an string
 * parameter, i.e. the parameter's type is different from @c CPL_TYPE_INT, the
 * error code @c CPL_ERROR_TYPE_MISMATCH is set.
 */

const char *
cpl_parameter_get_default_string(const cpl_parameter *self)
{



    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    cx_assert(self->data);

    if (self->data->type != CPL_TYPE_STRING) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return NULL;
    }

    return self->data->preset.s;

}


/**
 * @brief
 *   Get the number of alternatives of an enumeration parameter.
 *
 * @param self  An enumeration parameter.
 *
 * @return
 *   The number of possible values the parameter @em self can take,
 *   or 0 if @em self does not point to a valid parameter, or the
 *   parameter is not an enumeration parameter. In case of an error the
 *   function also sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_ENUM.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the number of possible alternatives for an
 * enumeration parameter, which is always greater or equal than 1.
 */

int
cpl_parameter_get_enum_size(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    if (self->class != CPL_PARAMETER_CLASS_ENUM) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    cx_assert(self->data);

    return ((cpl_parameter_enum *)self->data)->sz;

}


/**
 * @brief
 *   Get the possible values for an integer enumeration.
 *
 * @param self      An enumeration parameter
 * @param position  Position to look up in the list of possible values.
 *
 * @return
 *   The integer value at position @em position in the list of possible
 *   values of the enumeration @em self. In case of an error the
 *   function returns 0 and an appropriate error code is set.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_ENUM, or <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The requested index position <i>position</i> is out of range.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the integer value at position @em position from the
 * list of enumeration values the parameter @em self can possibly take.
 * For any valid enumeration parameter this list contains at least one value.
 * The possible values are counted starting from 0.
 */

int
cpl_parameter_get_enum_int(const cpl_parameter *self, int position)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    if (self->class != CPL_PARAMETER_CLASS_ENUM) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    cx_assert(self->data);

    if (((cpl_parameter_enum *)self->data)->value.type != CPL_TYPE_INT) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    if ((position < 0) && (position > cpl_parameter_get_enum_size(self))) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_ACCESS_OUT_OF_RANGE,
                                    "%s, pos=%d", cpl_parameter_get_name(self),
                                    position);
        return 0;
    }

    return ((cpl_parameter_enum *)self->data)->vlist[position].i;

}


/**
 * @brief
 *   Get the possible values for a double enumeration.
 *
 * @param self      An enumeration parameter
 * @param position  Position to look up in the list of possible values.
 *
 * @return
 *   The double value at position @em position in the list of possible
 *   values of the enumeration @em self. In case of an error the
 *   function returns 0. and an appropriate error code is set.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_ENUM, or <i>self</i> is not of type
 *         @c CPL_TYPE_DOUBLE.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The requested index position <i>position</i> is out of range.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the double value at position @em position from the
 * list of enumeration values the parameter @em self can possibly take.
 * For any valid enumeration parameter this list contains at leas one value.
 * The possible values are counted starting from 0.
 */

double
cpl_parameter_get_enum_double(const cpl_parameter *self, int position)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0.;
    }

    if (self->class != CPL_PARAMETER_CLASS_ENUM) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    cx_assert(self->data);

    if (((cpl_parameter_enum *)self->data)->value.type !=
        CPL_TYPE_DOUBLE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    if ((position < 0) && (position > cpl_parameter_get_enum_size(self))) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_ACCESS_OUT_OF_RANGE,
                                    "%s, pos=%d", cpl_parameter_get_name(self),
                                    position);
        return 0.;
    }

    return ((cpl_parameter_enum *)self->data)->vlist[position].d;

}


/**
 * @brief
 *   Get the possible values for a string enumeration.
 *
 * @param self      An enumeration parameter
 * @param position  Position to look up in the list of possible values.
 *
 * @return The string value at position @em position in the list of possible
 *   values of the enumeration @em self. In case of an error the
 *   function returns @c NULL and an appropriate error code is set.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_ENUM, or <i>self</i> is not of type
 *         @c CPL_TYPE_STRING.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The requested index position <i>position</i> is out of range.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the read-only string value at position @em position from the
 * list of enumeration values the parameter @em self can possibly take.
 * For any valid enumeration parameter this list contains at least one value.
 * The possible values are counted starting from 0.
 */

const char *
cpl_parameter_get_enum_string(const cpl_parameter *self, int position)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (self->class != CPL_PARAMETER_CLASS_ENUM) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return NULL;
    }

    cx_assert(self->data);

    if (((cpl_parameter_enum *)self->data)->value.type !=
        CPL_TYPE_STRING) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return NULL;
    }

    if ((position < 0) && (position > cpl_parameter_get_enum_size(self))) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_ACCESS_OUT_OF_RANGE,
                                    "%s, pos=%d", cpl_parameter_get_name(self),
                                    position);
        return NULL;
    }

    return ((cpl_parameter_enum *)self->data)->vlist[position].s;

}


/**
 * @brief
 *   Get the minimum value of an integer range parameter.
 *
 * @param self  An integer range parameter.
 *
 * @return
 *   The function returns the minimum integer value the range parameter
 *   @em self can take. In case of an error, the function returns 0 and
 *   sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_RANGE, or <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the integer value defined to be the lower limit
 * of the integer range parameter @em self.
 */

int
cpl_parameter_get_range_min_int(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    if (self->class != CPL_PARAMETER_CLASS_RANGE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    cx_assert(self->data);

    if (((cpl_parameter_range *)self->data)->value.type != CPL_TYPE_INT) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    return ((cpl_parameter_range *)self->data)->min.i;

}


/**
 * @brief
 *   Get the minimum value of a double range parameter.
 *
 * @param self  A double range parameter.
 *
 * @return
 *   The function returns the minimum double value the range parameter
 *   @em self can take. In case of an error, the function returns 0. and
 *   sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_RANGE, or <i>self</i> is not of type
 *         @c CPL_TYPE_DOUBLE.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the double value defined to be the lower limit
 * of the double range parameter @em self.
 */

double
cpl_parameter_get_range_min_double(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0.;
    }

    if (self->class != CPL_PARAMETER_CLASS_RANGE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    cx_assert(self->data);

    if (((cpl_parameter_range *)self->data)->value.type !=
        CPL_TYPE_DOUBLE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    return ((cpl_parameter_range *)self->data)->min.d;

}


/**
 * @brief
 *   Get the maximum value of an integer range parameter.
 *
 * @param self  An integer range parameter.
 *
 * @return
 *   The function returns the maximum integer value the range parameter
 *   @em self can take. In case of an error, the function returns 0 and
 *   sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_RANGE, or <i>self</i> is not of type
 *         @c CPL_TYPE_INT.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the integer value defined to be the upper limit
 * of the integer range parameter @em self.
 */

int
cpl_parameter_get_range_max_int(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    if (self->class!=CPL_PARAMETER_CLASS_RANGE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    cx_assert(self->data);

    if (((cpl_parameter_range *)self->data)->value.type != CPL_TYPE_INT) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0;
    }

    return ((cpl_parameter_range *)self->data)->max.i;

}


/**
 * @brief
 *   Get the maximum value of a double range parameter.
 *
 * @param self  A double range parameter.
 *
 * @return
 *   The function returns the maximum double value the range parameter
 *   @em self can take. In case of an error, the function returns 0. and
 *   sets an appropriate error code.
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
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The class of the parameter <i>self</i> is not of the kind
 *         @c CPL_PARAMETER_CLASS_RANGE, or <i>self</i> is not of type
 *         @c CPL_TYPE_DOUBLE.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the double value defined to be the upper limit
 * of the double range parameter @em self.
 */

double
cpl_parameter_get_range_max_double(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0.;
    }

    if (self->class != CPL_PARAMETER_CLASS_RANGE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    cx_assert(self->data);

    if (((cpl_parameter_range *)self->data)->value.type !=
        CPL_TYPE_DOUBLE) {
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_TYPE_MISMATCH, "%s of "
                                    "type %s", cpl_parameter_get_name(self),
                                    cpl_type_get_name(cpl_parameter_get_type
                                                      (self)));
        return 0.;
    }

    return ((cpl_parameter_range *)self->data)->max.d;

}


/**
 * @brief
 *   Get the presence status flag of the given parameter.
 *
 * @param self  A parameter.
 *
 * @return
 *   The function returns the current setting of the parameters
 *   presence flag. If the parameter is present the function returns 1 and
 *   0 otherwise. If an error occurs the function returns 0 and sets an
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
 * The function indicates whether the given parameter @em self was seen
 * by an application while processing the input from the command line, the
 * environment and/or a configuration file. If the parameter was seen the
 * application may set this status flag and query it later using this
 * function.
 *
 * @see cpl_parameter_set_default_flag()
 */

int
cpl_parameter_get_default_flag(const cpl_parameter *self)
{



    if (self == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return self->info.present == TRUE ? 1 : 0;

}


/**
 * @brief
 *   Change the presence status flag of the given parameter.
 *
 * @param self    A parameter.
 * @param status  The presence status value to assign.
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
 * The function sets the presence status flag of the given parameter
 * @em self to the value @em status. Any non-zero value means that
 * the parameter is present. If the presence status should be changed to
 * `not present' the argument @em status must be 0.
 *
 * @see cpl_parameter_get_default_flag()
 */

cpl_error_code
cpl_parameter_set_default_flag(cpl_parameter *self, int status)
{



    if (self == NULL) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    self->info.present = status == 0 ? FALSE : TRUE;
    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the numerical identifier of the given parameter.
 *
 * @param self  A parameter.
 *
 * @return
 *   The function returns the parameter's numerical identifier. If an error
 *   occurs the function returns 0 and sets an appropriate error code.
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
 * The function looks up the numerical identifier of the given parameter
 * @em self. A numerical identifier may be assigned to a parameter
 * using @b cpl_parameter_set_id().
 *
 * @see cpl_parameter_set_id()
 */

int
cpl_parameter_get_id(const cpl_parameter *self)
{



    if (self == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return self->info.id;

}


/**
 * @brief
 *   Set the numerical identifier of the given parameter.
 *
 * @param self  A parameter.
 * @param id    Numerical identifier to assign.
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
 * The function assigns a numerical identifier to the parameter @em self.
 * The numerical value to be assigned to the parameter's numerical identifier
 * member is passed as the argument @em id. The function does not do any
 * checks on the numerical value of @em id. The numerical identifier may
 * be used by an application to, for instance, assign a sequence number to
 * the parameter.
 *
 * @see cpl_parameter_get_id()
 */

cpl_error_code
cpl_parameter_set_id(cpl_parameter *self, int id)
{



    if (self == NULL) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    self->info.id = id;
    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the parameter's user tag.
 *
 * @param self  A parameter.
 *
 * @return
 *   The function returns the parameter's user tag, or @c NULL if no
 *   user tag was previously set. The function returns @c NULL if an error
 *   occurs and sets an appropriate error code.
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
 * The current read-only setting of the user definable tag of the given parameter
 * @em self is retrieved.
 */

const char *
cpl_parameter_get_tag(const cpl_parameter *self)
{


    if (!self) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->usertag;

}


/**
 * @brief
 *   Set the tag of the given parameter.
 *
 * @param self  A parameter.
 * @param tag   Tag string to assign.
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
 * The function assigns a user definable tag @em tag to the parameter
 * @em self. The function does not check the passed string @em tag in any way.
 * The tag may be used by an application but it cannot rely on the contents
 * of the parameter's tag.
 *
 * @see cpl_parameter_get_tag()
 */

cpl_error_code
cpl_parameter_set_tag(cpl_parameter *self, const char *tag)
{



    if (self == NULL || tag == NULL) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    if (self->usertag != NULL) {
        cx_free(self->usertag);
        self->usertag = NULL;
    }

    self->usertag = cx_strdup(tag);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Set alias names for the given parameter.
 *
 * @param self   A parameter
 * @param mode   The parameter mode for which the alias should be set.
 * @param alias  The parameter's alias.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter mode <i>mode</i> is not supported.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function assigns an alternative name to the parameter @em self for
 * the given mode @em mode. This alias name may be used instead of the
 * fully qualified parameter name in the context for which they have been
 * set. If the alias name is @c NULL a previously set alias is deleted.
 *
 * The context for which an alias is set is selected by the @em mode. The
 * functions accepts the parameter modes @c CPL_PARAMETER_MODE_CFG,
 * @c CPL_PARAMETER_MODE_CLI and @c CPL_PARAMETER_MODE_ENV.
 *
 * @see cpl_parameter_mode
 */

cpl_error_code
cpl_parameter_set_alias(cpl_parameter *self, cpl_parameter_mode mode,
                        const char *alias)
{



    if (self == NULL) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    switch (mode) {
    case CPL_PARAMETER_MODE_CLI:
        if (self->info.c_key) {
            cx_free(self->info.c_key);
        }
        self->info.c_key = cx_strdup(alias);
        break;

    case CPL_PARAMETER_MODE_ENV:
        if (self->info.e_key) {
            cx_free(self->info.e_key);
        }
        self->info.e_key = cx_strdup(alias);
        break;

    case CPL_PARAMETER_MODE_CFG:
        if (self->info.f_key) {
            cx_free(self->info.f_key);
        }
        self->info.f_key = cx_strdup(alias);
        break;

    default:
        return cpl_error_set_message(cpl_func, CPL_ERROR_ILLEGAL_INPUT,
                                     "%s, mode=%d, alias=%s",
                                     cpl_parameter_get_name(self), (int)mode,
                                     alias);
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the parameter's alias name for the given mode.
 *
 * @param self  A parameter.
 * @param mode  The parameter mode.
 *
 * @return
 *   The function returns the parameter's context specific alias name.
 *   If no alias name was previously assigned the function returns
 *   @c NULL. If an error occurs the function returns @c NULL and
 *   sets an appropriate error code.
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
 *         The parameter mode <i>mode</i> is not supported.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the read-only alias name of the parameter @em self.
 * The context for which an alias is retrieved is selected by the @em mode.
 * The functions accepts the parameter modes @c CPL_PARAMETER_MODE_CFG,
 * @c CPL_PARAMETER_MODE_CLI and @c CPL_PARAMETER_MODE_ENV.
 *
 * @see cpl_parameter_mode
 */

const char *
cpl_parameter_get_alias(const cpl_parameter *self, cpl_parameter_mode mode)
{


    const cxchar *alias = NULL;


    if (self == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    switch (mode) {
    case CPL_PARAMETER_MODE_CLI:
        alias = self->info.c_key;
        break;

    case CPL_PARAMETER_MODE_ENV:
        alias = self->info.e_key;
        break;

    case CPL_PARAMETER_MODE_CFG:
        alias = self->info.f_key;
        break;

    default:
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_ILLEGAL_INPUT,
                                    "%s, mode=%d",
                                    cpl_parameter_get_name(self), (int)mode);
        break;
    }

    return alias;

}


/**
 * @brief
 *   Get the parameter's activity status for the environment context.
 *
 * @param self  A parameter.
 * @param mode  The parameter mode.
 *
 * @return
 *   The function returns a non-zero value if the parameter is enabled
 *   for the environment context, or 0 if the parameter is not active.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter mode <i>mode</i> is not supported.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns whether the parameter is enabled for the
 * environment context or not. If the parameter is enabled for the
 * context the application may modify the parameter's value if the
 * parameter is referenced in the context specific input of the application.
 * If the parameter is disabled, the application must not modify the
 * parameter's value.
 */

int
cpl_parameter_is_enabled(const cpl_parameter *self, cpl_parameter_mode mode)
{


    cxbool active = FALSE;


    if (self == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return 0;
    }

    switch (mode) {
    case CPL_PARAMETER_MODE_CLI:
        active = self->info.c_active;
        break;

    case CPL_PARAMETER_MODE_ENV:
        active = self->info.e_active;
        break;

    case CPL_PARAMETER_MODE_CFG:
        active = self->info.f_active;
        break;

    default:
        (void)cpl_error_set_message(cpl_func, CPL_ERROR_ILLEGAL_INPUT,
                                    "%s, mode=%d",
                                    cpl_parameter_get_name(self), (int)mode);
        break;
    }

    return active == TRUE ? 1 : 0;

}


/**
 * @brief
 *   Activates a parameter for the given mode.
 *
 * @param self  A parameter.
 * @param mode  The parameter mode.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter mode <i>mode</i> is not supported.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function enables the parameter @em self for the given mode @em mode.
 */

cpl_error_code
cpl_parameter_enable(cpl_parameter *self, cpl_parameter_mode mode)
{



    if (self == NULL) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    switch (mode) {
    case CPL_PARAMETER_MODE_CLI:
        self->info.c_active = TRUE;
        break;

    case CPL_PARAMETER_MODE_ENV:
        self->info.e_active = TRUE;
        break;

    case CPL_PARAMETER_MODE_CFG:
        self->info.f_active = TRUE;
        break;

    default:
        return cpl_error_set_message(cpl_func, CPL_ERROR_ILLEGAL_INPUT,
                                     "%s, mode=%d",
                                     cpl_parameter_get_name(self), (int)mode);
        break;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Deactivate a parameter for the given mode.
 *
 * @param self  A parameter.
 * @param mode  The parameter mode.
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
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter mode <i>mode</i> is not supported.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function disables the parameter @em self for the given mode @em mode.
 */

cpl_error_code
cpl_parameter_disable(cpl_parameter *self, cpl_parameter_mode mode)
{



    if (self == NULL) {
        return cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
    }

    switch (mode) {
    case CPL_PARAMETER_MODE_CLI:
        self->info.c_active = FALSE;
        break;

    case CPL_PARAMETER_MODE_ENV:
        self->info.e_active = FALSE;
        break;

    case CPL_PARAMETER_MODE_CFG:
        self->info.f_active = FALSE;
        break;

    default:
        return cpl_error_set_message(cpl_func, CPL_ERROR_ILLEGAL_INPUT,
                                     "%s, mode=%d",
                                     cpl_parameter_get_name(self), (int)mode);
        break;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Dump the parameter debugging information to the given stream.
 *
 * @param self    The parameter.
 * @param stream  The output stream to use.
 *
 * @return Nothing.
 *
 * The function dumps the contents of of the parameter @em self to the
 * output stream @em stream. If @em stream is @c NULL the function writes
 * to the standard output. If @em self is @c NULL the function does nothing.
 */

void
cpl_parameter_dump(const cpl_parameter *self, FILE *stream)
{

    if (self != NULL) {

        const cxchar *class;
        const cxchar *type;

        const cpl_parameter_value *value = NULL;


        if (stream == NULL) {
            stream = stdout;
        }

        switch (self->class) {
        case CPL_PARAMETER_CLASS_VALUE:
            class = "Value";
            break;

        case CPL_PARAMETER_CLASS_RANGE:
            class = "Range";
            break;

        case CPL_PARAMETER_CLASS_ENUM:
            class = "Enum";
            break;

        default:
            class = "Unknown";
            break;
        }

        value = self->data;

        switch (value->type) {
        case CPL_TYPE_BOOL:
            type = "bool";
            break;

        case CPL_TYPE_INT:
            type = "int";
            break;

        case CPL_TYPE_DOUBLE:
            type = "double";
            break;

        case CPL_TYPE_STRING:
            type = "string";
            break;

        default:
            type = "unknown";
            break;
        }

        fprintf(stream, "Parameter at %p: class %02x (%s), type %02x (%s)\n"
                "  Attributes:", (const void *)self, self->class, class,
                value->type, type);

        fprintf(stream, "    name (at %p): %s", self->name, self->name);

        if (self->description) {
            fprintf(stream, "    description (at %p): %s", self->description,
                    self->description);
        }
        else {
            fprintf(stream, "    description (at %p): %p", self->description,
                    self->description);
        }

        if (self->context) {
            fprintf(stream, "    context (at %p): %s", self->context,
                    self->context);
        }
        else {
            fprintf(stream, "    context (at %p): %p", self->context,
                    self->context);
        }

        if (self->usertag) {
            fprintf(stream, "    user tag (at %p): %s", self->usertag,
                    self->usertag);
        }
        else {
            fprintf(stream, "    user tag (at %p): %p", self->usertag,
                    self->usertag);
        }

        fprintf(stream, "\n  Values:    ");

        switch (value->type) {
        case CPL_TYPE_BOOL:
            fprintf(stream, "    Current: %d    Default: %d", value->data.b,
                    value->preset.b);
            break;

        case CPL_TYPE_INT:
            fprintf(stream, "    Current: %d    Default: %d", value->data.i,
                    value->preset.i);
            break;

        case CPL_TYPE_DOUBLE:
            fprintf(stream, "    Current: %g    Default: %g", value->data.d,
                    value->preset.d);
            break;

        case CPL_TYPE_STRING:
            if (value->data.s) {
                if (value->preset.s) {
                    fprintf(stream, "    Current: %s    Default: %s",
                            value->data.s, value->preset.s);
                }
                else {
                    fprintf(stream, "    Current: %s    Default: %p",
                            value->data.s, value->preset.s);
                }
            }
            else {
                if (value->preset.s) {
                    fprintf(stream, "    Current: %p    Default: %s",
                            value->data.s, value->preset.s);
                }
                else {
                    fprintf(stream, "    Current: %p    Default: %p",
                            value->data.s, value->preset.s);
                }
            }

            break;

        default:
            fprintf(stream, "    Current: invalid    Default: invalid");
            break;
        }

        fprintf(stream, "\n");

    }

    return;

}
/**@}*/
