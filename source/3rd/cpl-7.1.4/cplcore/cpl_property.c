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

#include <complex.h>

#include "cpl_property_impl.h"
#include "cpl_error_impl.h"

#include <cxmemory.h>
#include <cxmessages.h>

#include <string.h>

/**
 * @defgroup cpl_property Properties
 *
 * This module implements the property type. The type @c cpl_property is
 * basically a variable container which consists of a name, a type identifier
 * and a specific value of that type. The type identifier always determines
 * the type of the associated value. A property is similar to an ordinary
 * variable and its current value can be set or retrieved through its name. In
 * addition a property may have a descriptive comment associated.
 * A property can be created for the basic types char, bool (int), int,
 * long, float and double. Also C strings are supported. Support for
 * arrays in general is currently not available.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_property.h>
 * @endcode
 */

/**@{*/

/*-----------------------------------------------------------------------------
                             Macro definitions
 -----------------------------------------------------------------------------*/

/*
  @internal
  @def CPL_PROPERTY_SIZE
  @brief  The size of the data container, a few bytes more than a FITS card
  @see FLEN_CARD, cpl_type_get_sizeof()
  @note Must be able to hold any primitive type, e.g. a 16-byte double complex
  @hideinitializer

  The correctness is independent of its size (within some limits). Selecting a
  too small size causes extra allocations, selecting a too large size causes a
  too large amount of default allocation. A size similar to a FITS card ensures
  that a normal FITS card can be stored without extra allocation.
 */
#define CPL_PROPERTY_SIZE 88

#if CPL_PROPERTY_SIZE < 16
#error CPL_PROPERTY_SIZE is too small! How about the storage sweet spot of 88?
#elif CPL_PROPERTY_SIZE > 255
#error CPL_PROPERTY_SIZE is too large! How about the storage sweet spot of 88?
#endif

/*
  @internal
  @def CPL_PROPERTY_MEMBER_IS_OUTSIDE
  @brief  A boolean expression, whether a pointer member points outside
  @param SELF   The property to examine
  @param MEMBER The pointer member to examine
  @note Intended for use with cx_assert()
  @hideinitializer
 */
#define CPL_PROPERTY_MEMBER_IS_OUTSIDE(SELF, MEMBER)                    \
    ((SELF)->MEMBER != NULL &&                                          \
     ((const char*)(SELF)->MEMBER <  (const char*)(SELF) ||             \
      (const char*)(SELF)->MEMBER >= (const char*)(SELF) + sizeof(*self))) \

#define CPL_PROPERTY_SORT_UNDEF 0

/*-----------------------------------------------------------------------------
                                New types
 -----------------------------------------------------------------------------*/

/* This struct stores a typical property (e.g. one unpacked from a FITS card)
   without any additional memory allocation. On a 64-bit architecture, the
   struct has a size of 128 bytes, of which the data container has a fixed size
   (default of 88 bytes).

   When the property value fits inside it is stored at the start of the
   container, ensuring its alignment on a word-boundary. When a non-array
   property is created its value is guaranteed to be stored in the container.

   The property is always created with a name. When the name fits inside, it
   is stored at the end of the container.

   If the comment fits inside it is stored right after the value. The rationale
   for this is that the value does not change its size (except in the case of
   strings of changing lengths). If the size of the value does change and it and
   the comment both fit inside the container, the comment is moved so it remains
   stored right after the value.

   The memory layout of the struct can thus be visualized like this:
   PPPPPVVCCCUUNNNN
    - where each letter symbolizes (on 64-bit, 8 bytes for each capital letter):
   P: Property triplet of pointers and meta-data, 40 bytes
   V: Value (16 bytes, in this example)
   C: Comment string (24 bytes, in this example)
   U: Unused, undefined memory (16 bytes, in this example)
   N: Name string (32 bytes, in this example)
    
   If at any time one of the three pointer members is set without being able to
   store their data inside the container, it is stored outside using malloc().

   Once stored outside, a member's data remains outside the container.

   One member of the struct is for supporting effective sorting of a list
   of n properties according to this key, which is then set prior to the sorting
   at complexity O(n), as opposed to O(n*log(n)) of the sorting itself.
   This sorting key is just 1 byte, stored at the end where it does not
   change the alignment of the value member.

   */

typedef struct _cpl_property_ {
    char        * name;    /* A property is always created with the name set */
    char        * comment; /* NULL when not set */
    void        * value;   /* Null/all zero(s) before it is explicitly set */
    cpl_size      size;    /* Number of elements, 1 for non-array */
    uint32_t      type;    /* Property CPL-type (least significant 32 bits) */
    unsigned char vsize;   /* Container storage used for value, or 0   [byte] */
    unsigned char nsize;   /* Container storage used for name, or 0    [byte] */
    unsigned char csize;   /* Container storage used for comment, or 0 [byte] */
    unsigned char ksort;   /* The sorting key for efficient sorting */
    char          data[CPL_PROPERTY_SIZE]; /* Container for a normal property */
} _cpl_property_;


/*----------------------------------------------------------------------------*/
/**
  @enum     cpl_property_dicbtype
  @internal
  @brief    The FITS key types according to DICB
  @note This enum may have no more than 256 entries, among which the value zero
        is reserved to mean undefined.

  This enum stores all possible types for a FITS keyword. These determine
  the order of appearance in a header, they are a crucial point for
  DICB (ESO) compatibility. This classification is internal to this
  module.

 */
/*----------------------------------------------------------------------------*/
typedef enum cpl_property_dicbtype {
    CPL_DICB_UNDEF = CPL_PROPERTY_SORT_UNDEF,

    CPL_DICB_TOP,

    /* Mandatory keywords */
    /* All FITS files */
    CPL_DICB_BITPIX,

    /* Per the FITS standard, NAXISn can go from 1 to 999. */
    CPL_DICB_NAXIS,
    CPL_DICB_NAXISn,

    /* Random groups only */
    CPL_DICB_GROUP,
    /* Extensions */
    CPL_DICB_PCOUNT,
    CPL_DICB_GCOUNT,
    /* Main header */
    CPL_DICB_EXTEND,
    /* Images */
    CPL_DICB_BSCALE,
    CPL_DICB_BZERO,
    /* Tables */
    CPL_DICB_TFIELDS,

    /* Per the FITS standard, TBCOLn is indexed and starts with 1 */
    CPL_DICB_TBCOLn,

    /* Per the FITS standard, TBCOLn is indexed and starts with 1 */
    CPL_DICB_TFORMn,

    /* Other primary keywords */
    CPL_DICB_PRIMARY,

    /* HIERARCH ESO keywords ordered according to DICB */
    /* Only the two first (3-letter) words count in the orderting */
    CPL_DICB_HIERARCH_DPR,
    CPL_DICB_HIERARCH_OBS,
    CPL_DICB_HIERARCH_TPL,
    CPL_DICB_HIERARCH_GEN,
    CPL_DICB_HIERARCH_TEL,
    CPL_DICB_HIERARCH_INS,
    CPL_DICB_HIERARCH_DET,
    CPL_DICB_HIERARCH_LOG,
    CPL_DICB_HIERARCH_PRO,
    /* Other HIERARCH keywords */
    /* Only the first (3-letter "ESO") word count in the orderting */
    CPL_DICB_HIERARCH_ESO,
    /* Except, in principle, there could be non-ESO cards */
    CPL_DICB_HIERARCH,

    /* HISTORY and COMMENT */
    CPL_DICB_HISTORY,
    CPL_DICB_COMMENT,
    /* END */
    CPL_DICB_END
} cpl_property_dicbtype;

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static cpl_property *
cpl_property_new_(const char *, cpl_type, cpl_size, size_t)
    CPL_ATTR_ALLOC CPL_ATTR_NONNULL;

/*
 * Private methods
 */

/**
 * @internal
 * @brief
 *    Create an empty property of a given type.
 *
 * @param name     Property name.
 * @param type     Property type flag.
 * @param size     Property size, 1 for non-array type, string length for string
 * @param namelen  Property name length (excl. terminating null), zero when NULL
 *
 * @return
 *   The newly created property, or @c NULL if it could not be created. In the
 *   latter case an appropriate error code is set.
 *
 * @see cpl_property_new()
 */

static cpl_property *
cpl_property_new_(const char *name, cpl_type type, cpl_size size,
                  size_t namelen)
{
    cpl_property *self;
    const size_t  allocsz = 1 + namelen; /* Include terminating null-byte */
    size_t        elemsz;

    /* Type size is 0 for invalid types */

    elemsz = cpl_type_get_sizeof(type);

    if (elemsz == 0) {
        (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "name=%s", name);
        return NULL;
    }

    if (size <= 0) {
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "name=%s", name);
        return NULL;
    }

    /* Allocate space for property and for typically sized name/value/comment */
    self = (cpl_property*)cx_malloc(sizeof(*self));

    self->type = (uint32_t)type;
    /* Ensure no data loss in truncated representation */
    cx_assert((cpl_size)self->type == type);

    /* An unset comment points to NULL */
    self->comment = NULL;
    self->csize = 0;

    self->ksort = CPL_PROPERTY_SORT_UNDEF; /* Member for fast sorting */

    if (type & CPL_TYPE_FLAG_ARRAY) {
        /* Value is an array - while unset it points to NULL */

        self->value = NULL;
        self->vsize = 0;

        self->size = size; /* Number of array elements */

    } else {
        /* Value is not an array - store inside struct */

        self->value = self->data;
        self->vsize = (unsigned char)elemsz;

        /* Ensure no data loss in truncated representation */
        cx_assert((size_t)self->vsize == elemsz);

        self->size = 1; /* Disregard caller supplied size, if any */

        /* Clear memory of value */
        (void)memset(self->data, 0, self->vsize);
    }

    if (self->vsize + allocsz <= CPL_PROPERTY_SIZE) {
        self->name  = self->data + CPL_PROPERTY_SIZE - allocsz;
        self->nsize = allocsz;
        cx_assert((size_t)self->nsize == allocsz); /* Ensure no data loss */
    } else {
        /* Name is too long... */
        self->name  = (char*)cx_malloc(allocsz);
        self->nsize = 0;
    }

    /* Copy name into place - it may be without null-terminator */
    (void)memcpy(self->name, name, allocsz - 1);
    self->name[allocsz - 1] = '\0';

    cx_assert(self->vsize + self->nsize + self->csize <= CPL_PROPERTY_SIZE);

    return self;

}



/**
 * @internal
 * @brief
 *    Create an empty property of a given type.
 *
 * @param key      Property name with lengh.
 * @param type     Property type flag.
 * @param namelen  Property name size (excl. terminating null byte)
 *
 * @return
 *   The newly created property, or @c NULL if it could not be created. In the
 *   latter case an appropriate error code is set.
 *
 * @see cpl_property_new()
 *
 * @note No error checking, the keylen must match the name
 */

inline cpl_property *
cpl_property_new_cx(const cpl_cstr *key, cpl_type type)
{
    cpl_property *self = cpl_property_new_(cx_string_get_(key), type, 1,
                                           cx_string_size_(key));

    if (self == NULL) {
        (void)cpl_error_set_where_();
    }

    return self;
}

/*
 * Public methods
 */

/**
 * @brief
 *    Create an empty property of a given type.
 *
 * @param name     Property name.
 * @param type     Property type flag.
 *
 * @return
 *   The newly created property, or @c NULL if it could not be created. In the
 *   latter case an appropriate error code is set.
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
 *         The requested property type <i>type</i> is not supported.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function allocates memory for a property of type @em type and assigns
 * the identifier string @em name to the newly created property.
 *
 * The returned property must be destroyed using the property destructor
 * @b cpl_property_delete().
 *
 * @see cpl_property_delete()
 */

cpl_property *
cpl_property_new(const char *name, cpl_type type)
{
    cpl_property *self;

    if (name == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    self = cpl_property_new_(name, type, 1, strlen(name));

    if (self == NULL) {
        (void)cpl_error_set_where_();
    }

    return self;
}

/**
 * @brief
 *    Create an empty property of a given type and size.
 *
 * @param name     Property name.
 * @param type     Property type flag.
 * @param size     Size of the property value.
 *
 * @return
 *   The newly created property, or @c NULL if it could not be created. in
 *   the latter case an appropriate error code is set.
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
 *         The requested property type <i>type</i> is not supported.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The requested <i>size</i> is non-positive.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function allocates memory for a property of type @em type and assigns
 * the identifier string @em name to the newly created property. The
 * property value is created such that @em size elements of type @em type
 * can be stored.
 *
 * The returned property must be destroyed using the property destructor
 * @b cpl_property_delete().
 *
 * @see cpl_property_delete()
 */

cpl_property *
cpl_property_new_array(const char *name, cpl_type type, cpl_size size)
{
    cpl_property *self;


    if (name == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    self = cpl_property_new_(name, type, size, strlen(name));

    if (self == NULL) {
        (void)cpl_error_set_where_();
    }

    return self;
}

/**
 * @brief
 *    Create a copy of a property.
 *
 * @param other  The property to copy.
 *
 * @return
 *   The copy of the given property, or @c NULL in case of an error.
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
 * The function returns a copy of the property @em self. The copy is a
 * deep copy, i.e. all property members are copied.
 */

cpl_property *
cpl_property_duplicate(const cpl_property *other)
{
    cpl_property *self;


    if (other == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    /* Allocate and copy everything up to any value + comment, after which the
       container may hold unused (and undefined) storage. If the name is stored
       inside the container then it too will be copied, see below */
    self = memcpy(cx_malloc(sizeof(*self)), other,
                  sizeof(*self) - CPL_PROPERTY_SIZE
                  + other->vsize + other->csize);

    /* Each of the three pointer members now need to be updated. */

    if (self->vsize > 0) {
        /* Member is stored inside the container, data already copied */
        self->value = self->data;
    } else if (other->value != NULL) {
        /* Member is stored outside the container: 
           Need to allocate, copy and update pointer */

        const size_t mysize = cpl_type_get_sizeof((cpl_type)self->type);

        const size_t size = self->size * mysize;

        cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(other, value));

        self->value = memcpy(cx_malloc(size), other->value, size);
    } else {
        cx_assert(self->value == NULL);
    }

    if (self->nsize > 0) {
        /* Member is stored inside the container, copy it and update pointer */
        self->name = memcpy(self->data  + CPL_PROPERTY_SIZE - self->nsize,
                            other->data + CPL_PROPERTY_SIZE - self->nsize,
                            self->nsize);
    } else {
        /* Member is stored outside the container:
           Need to allocate, copy and update pointer */
        const size_t size = 1 + strlen(other->name);

        cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(other, name));

        self->name = memcpy(cx_malloc(size), other->name, size);
    }

    if (self->csize > 0) {
        /* Member is stored inside the container, on top of value */
        self->comment = self->data + self->vsize;
    } else if (self->comment != NULL) {
        /* Member is stored outside the container:
           Need to allocate, copy and update pointer */
        const size_t size = 1 + strlen(other->comment);

        cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(other, comment));

        self->comment = memcpy(cx_malloc(size), other->comment, size);
    } else {
        cx_assert(self->comment == NULL);
    }

    cx_assert(self->vsize + self->nsize + self->csize <= CPL_PROPERTY_SIZE);

    return self;
}

/**
 * @brief
 *    Destroy a property.
 *
 * @param self  The property.
 *
 * @return Nothing.
 *
 * The function destroys a property of any kind. All property members
 * including their values are properly deallocated. If the property @em self
 * is @c NULL, nothing is done and no error is set.
 */

void
cpl_property_delete(cpl_property *self)
{

    if (self) {
#ifdef CPL_PROPERTY_DEBUG
        cpl_msg_warning(cpl_func, "FREE(%d+%d+%d<=>%d<=>%zu:%zu) %s-property "
                        "%s (sort=%d)",
                        (int)self->nsize, (int)self->vsize, (int)self->csize,
                        CPL_PROPERTY_SIZE - (int)self->nsize - (int)self->vsize,
                        sizeof(*self),
                        (size_t)self->size * cpl_type_get_sizeof(self->type),
                        cpl_type_get_name(self->type),
                        self->name, self->ksort);
#endif

        if (self->csize == 0 && self->comment != NULL) {
            cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, comment));
#ifdef CPL_PROPERTY_DEBUG
            cpl_msg_warning(cpl_func, "OUTSIZED(%zu) comment in %s-type %s",
                            1 + strlen(self->comment),
                            cpl_type_get_name(self->type),
                            self->name);
#endif
            cx_free((void*)self->comment);
        }

        if (self->vsize == 0 && self->value != NULL) {
            cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, value));
#ifdef CPL_PROPERTY_DEBUG
            cpl_msg_warning(cpl_func, "OUTSIZED(%zu) value in %s-type %s",
                            (size_t)self->size * cpl_type_get_sizeof(self->type),
                            cpl_type_get_name(self->type),
                            self->name);
#endif
            cx_free(self->value);
        }

        if (self->nsize == 0) {
            cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, name));
#ifdef CPL_PROPERTY_DEBUG
            cpl_msg_warning(cpl_func, "OUTSIZED(%zu) name %s of %s-type",
                            1 + strlen(self->name), self->name,
                            cpl_type_get_name(self->type));
#endif
            cx_free((void*)self->name);
        }

        cx_free((void*)self);
    }

    return;
}


/**
 * @internal
 * @brief
 *   Get the current number of elements a property contains.
 *
 * @param self  The property.
 *
 * @return
 *   The current number of elements or -1 in case of an error. If an
 *   error occurred an appropriate error code is set.
 *
 * @see cpl_property_get_size()
 *
 */

inline cpl_size
cpl_property_get_size_(const cpl_property *self)
{
    return self->size;
}


/**
 * @brief
 *   Get the current number of elements a property contains.
 *
 * @param self  The property.
 *
 * @return
 *   The current number of elements or -1 in case of an error. If an
 *   error occurred an appropriate error code is set.
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
 * The function returns the current number of elements the property
 * @em self contains. This is the array size of the property's
 * value and in particular, for a property of the type
 * @c CPL_TYPE_STRING, it is the length of the string as
 * given by the @b strlen() function plus 1.
 *
 * Testing whether the value of a string property equals a given string
 * can be done like this:
 *   @code
 *   if (cpl_property_get_size(property) == 8 &&
 *       memcmp(cpl_property_get_string(property), "EXAMPLE", 7) == 0) {
 *       my_strings_are_equal();
 *   } else {
 *       my_strings_are_not_equal();
 *   }
 *   @endcode
 *
 * When the length of the string to compare to is known at compile time,
 * the call to memcmp() will typically be inlined (and no calls to strlen()
 * et al are needed). Further, if the length of the property string value
 * is greater than 1, then it is not necessary to test whether its type is
 * CPL_TYPE_STRING, since all properties of a numerical type has size 1.
 *
 * @see cpl_property_get_string()
 */

cpl_size
cpl_property_get_size(const cpl_property *self)
{
    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return -1;
    }

    return cpl_property_get_size_(self);
}

/**
 * @brief
 *   Get the type of a property.
 *
 * @param self  The property.
 *
 * @return
 *   The type code of this property. In case of an error the returned
 *   type code is @c CPL_TYPE_INVALID and an appropriate error code is
 *   set.
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
 * This function returns the type of the data value stored in the
 * property @em self.
 */

cpl_type
cpl_property_get_type(const cpl_property *self)
{

    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return CPL_TYPE_INVALID;
    }

    return (cpl_type)self->type;
}


/**
 * @internal
 * @brief
 *   Modify the name of a property.
 *
 * @param self  The property.
 * @param name  New name.
 *
 * @see cpl_property_set_name
 *
 */

inline void
cpl_property_set_name_cx(cpl_property *self, const cpl_cstr *name)
{
    /* Include terminating null-byte */
    const size_t allocsz = cx_string_size_(name) + 1;


    if (self->nsize == 0) {
        /* Outside space already allocated, keep it like that */

        /* Include terminating null-byte */
        const size_t oldsize = 1 + strlen(self->name);

        cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, name));

        if (allocsz > oldsize) {
            cx_free((void*)self->name);
            self->name = (char*)cx_malloc(allocsz);
        }
    } else if (self->vsize + self->csize + allocsz > CPL_PROPERTY_SIZE) {
        /* Inside storage exhausted, move member outside */
        self->nsize = 0;
        self->name = (char*)cx_malloc(allocsz);

    } else if (allocsz != self->nsize) {
        /* Name stays inside, but size and thus location changes */

        self->nsize = allocsz;

        self->name = self->data + CPL_PROPERTY_SIZE - self->nsize;
    }

    /* Copy member into place - it may be without null-terminator */
    (void)memcpy(self->name, cx_string_get_(name), allocsz - 1);
    self->name[allocsz - 1] = '\0';

    cx_assert(self->vsize + self->nsize + self->csize <= CPL_PROPERTY_SIZE);
}


/**
 * @brief
 *   Modify the name of a property.
 *
 * @param self  The property.
 * @param name  New name.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 * The function replaces the current name of @em self with a copy
 * of the string @em name. The function returns an error if @em name is
 * @c NULL.
 */

cpl_error_code
cpl_property_set_name(cpl_property *self, const char *name)
{
    const cpl_cstr * cxstr;

    if (self == NULL) {
        return name != NULL
            ? cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "name=%s", name)
            : cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (name == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "self->name=%s",
                                      self->name);
    }

    cxstr = CXSTR(name, strlen(name));

    cpl_property_set_name_cx(self, cxstr);

    return CPL_ERROR_NONE;
}

/**
 * @internal
 * @brief
 *   Modify a property's comment.
 *
 * @param self     The property.
 * @param comment  New comment.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>comment</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current comment field of @em self with a
 * copy of the string @em comment. The new comment may be @c NULL. In this
 * case the function effectively deletes the current comment.
 */

inline void 
cpl_property_set_comment_cx(cpl_property *self, const cpl_cstr *comment)
{
    size_t allocsz;

    if (cx_string_get_(comment) == NULL) {
        if (self->csize > 0) {
            /* Reset old, inside stored comment */
            cx_assert(!CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, comment));
            self->csize = 0;
            self->comment = NULL;
        } else if (self->comment != NULL) {
            /* Free old, outside stored comment */
            cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, comment));
            cx_free((void*)self->comment);
            self->comment = NULL;
        }
    }

    allocsz = 1 + cx_string_size_(comment); /* Include terminating null-byte */

    if (self->csize == 0 && self->comment != NULL) {
        /* Outside space already allocated, keep it like that */

        /* Include terminating null-byte */
        const size_t oldsize = 1 + strlen(self->comment);

        cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, comment));

        if (allocsz > oldsize) {
            cx_free((void*)self->comment);
            self->comment = (char*)cx_malloc(allocsz);
        }
    } else if (self->vsize + self->nsize + allocsz > CPL_PROPERTY_SIZE) {
        /* Inside storage exhausted, move member outside */
        self->csize = 0;
        self->comment = (char*)cx_malloc(allocsz);
    } else if (allocsz != self->csize) {
        /* Inside storage OK, need to adjust comment location */

        self->comment = self->data + self->vsize;
        self->csize = allocsz;
    } else {
        /* Inside storage OK, with size and location unchanged */
        cx_assert(self->csize > 0);
        cx_assert(self->comment == self->data + self->vsize);
    }

    /* Copy member into place - it may be without null-terminator */
    (void)memcpy(self->comment, cx_string_get_(comment), allocsz - 1);
    self->comment[allocsz - 1] = '\0';

    cx_assert(self->vsize + self->nsize + self->csize <= CPL_PROPERTY_SIZE);
}

/**
 * @brief
 *   Modify a property's comment.
 *
 * @param self     The property.
 * @param comment  New comment.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>comment</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current comment field of @em self with a
 * copy of the string @em comment. The new comment may be @c NULL. In this
 * case the function effectively deletes the current comment.
 */

cpl_error_code
cpl_property_set_comment(cpl_property *self, const char *comment)
{
    const cpl_cstr * cxstr;

    if (self == NULL) {
        return comment == NULL
            ? cpl_error_set_(CPL_ERROR_NULL_INPUT)
            : cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "comment=%s",
                                     comment);
    }

    cxstr = CXSTR(comment, comment ? strlen(comment) : 0);

    cpl_property_set_comment_cx(self, cxstr);

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a character property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_CHAR</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current character value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_char(cpl_property *self, char value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_CHAR) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a boolean property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_BOOL</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current boolean value of @em self with a
 * a 0, if @em value is equal to 0. If @em value is different from 0 any
 * previous value of @em self is replaced by a 1.
 */

cpl_error_code
cpl_property_set_bool(cpl_property *self, int value)
{
    const cpl_boolean myvalue = value ? CPL_TRUE : CPL_FALSE;

    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_BOOL) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &myvalue, sizeof(myvalue)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of an integer property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current integer value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_int(cpl_property *self, int value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a long property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current long value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_long(cpl_property *self, long value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_LONG) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a long long property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_LONG</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current long long value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_long_long(cpl_property *self, long long value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_LONG_LONG) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a float property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current float value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_float(cpl_property *self, float value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_FLOAT) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a double property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current double value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_double(cpl_property *self, double value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_DOUBLE) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @internal
 * @brief
 *   Set the value of a string property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @see cpl_property_set_string()
 *
 */

inline void
cpl_property_set_string_cx(cpl_property *self, const cpl_cstr *value)
{
    /* Include terminating null-byte */
    const size_t allocsz = cx_string_size_(value) + 1;
    cpl_boolean move_comment = CPL_FALSE;

    if (self->vsize == 0 && self->value != NULL) {
        /* Outside space already allocated, keep it like that */

        cx_assert(CPL_PROPERTY_MEMBER_IS_OUTSIDE(self, value));

        if ((cpl_size)allocsz > self->size) {
            cx_free((void*)self->value);
            self->value = (char*)cx_malloc(allocsz);
        }
    } else if (self->nsize + self->csize + allocsz > CPL_PROPERTY_SIZE) {
        /* Inside storage exhausted, move member outside */

        /* May need to move non-empty comment down to where old string was */
        move_comment = self->csize > 0 && self->vsize > 0;

        self->vsize = 0;
        self->value = (char*)cx_malloc(allocsz);

    } else if (self->vsize != allocsz) {
        /* Inside storage OK, update location and size */

        /* Move non-empty comment based on new value size */
        move_comment = self->csize > 0;

        self->vsize = allocsz;
        self->value = self->data; /* Redundant all but the first time */
    }

    if (move_comment)
        self->comment = memmove(self->data + self->vsize,
                                self->comment, self->csize);

    /* Copy member into place */
    (void)memcpy(self->value, cx_string_get_(value), allocsz - 1);
    ((char*)self->value)[allocsz - 1] = '\0';

    self->size = allocsz;

    cx_assert(self->vsize + self->nsize + self->csize <= CPL_PROPERTY_SIZE);
}

/**
 * @brief
 *   Set the value of a string property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>value</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current string value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_string(cpl_property *self, const char *value)
{
    const cpl_cstr * cxstr;


    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_STRING) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    if (value == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT, "self->name=%s",
                                      self->name);
    }

    cxstr = CXSTR(value, strlen(value));

    cpl_property_set_string_cx(self, cxstr);

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a complex float property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type
 *            <tt>CPL_TYPE_COMPLEX_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current complex float value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_float_complex(cpl_property *self, float complex value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_FLOAT_COMPLEX) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @brief
 *   Set the value of a double complex property.
 *
 * @param self   The property.
 * @param value  New value.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success and an appropriate
 *   error code if an error occurred. In the latter case the error code is
 *   also set in the error handling system.
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
 *         The property <i>self</i> is not of type
 *            <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the current double value of @em self with a
 * copy of the value @em value.
 */

cpl_error_code
cpl_property_set_double_complex(cpl_property *self, double complex value)
{
    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->type != CPL_TYPE_DOUBLE_COMPLEX) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "name=%s, type=%s",
                                      self->name,
                                      cpl_type_get_name(self->type));
    }

    cx_assert(sizeof(value) == cpl_type_get_sizeof((cpl_type)self->type));

    (void)memcpy(self->data, &value, sizeof(value)); /* Inlined */

    return CPL_ERROR_NONE;
}

/**
 * @internal
 * @brief
 *   Get the property name.
 *
 * @param self  The property.
 *
 * @return
 *   The name of the property.
 *
 * @see cpl_property_get_name()
 *
 * @note No error checking in this internal function
 */

inline const char *
cpl_property_get_name_(const cpl_property *self)
{
    return self->name;
}

/**
 * @internal
 * @brief
 *   Get the string property value.
 *
 * @param self  The string property.
 *
 * @return
 *   The value of the string property.
 *
 * @see cpl_property_get_string()
 *
 * @note No error checking in this internal function
 */

inline const char *
cpl_property_get_string_(const cpl_property *self)
{
    return self->value;
}

/**
 * @internal
 * @brief
 *   Get the property comment.
 *
 * @param self  The property.
 *
 * @return
 *   The comment of the property.
 *
 * @see cpl_property_get_name()
 *
 * @note No error checking in this internal function
 */

inline const char *
cpl_property_get_comment_(const cpl_property *self)
{
    return self->comment;
}

/**
 * @brief
 *   Get the property name.
 *
 * @param self  The property.
 *
 * @return
 *   The name of the property or @c NULL if an error occurred. In the
 *   latter case an appropriate error code is set.
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
 * The function returns a handle for the read-only identifier string of @em self.
 */

const char *
cpl_property_get_name(const cpl_property *self)
{

    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return cpl_property_get_name_(self);
}

/**
 * @brief
 *   Get the property comment.
 *
 * @param self  The property.
 *
 * @return
 *   The comment of the property if it is present or @c NULL. If an error
 *   occurrs the function returns @c NULL and sets an appropriate error code.
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
 * The function returns a handle for the read-only comment string of @em self.
 */

const char *
cpl_property_get_comment(const cpl_property *self)
{

    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return self->comment;
}

/**
 * @brief
 *   Get the value of a character property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   '\\0' and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_CHAR</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the character value currently stored in the
 * property @em self.
 */

char
cpl_property_get_char(const cpl_property *self)
{
    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return '\0';
    }

    if (self->type != CPL_TYPE_CHAR) {
        (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                     "name=%s, type=%s",
                                     self->name,
                                     cpl_type_get_name(self->type));
        return '\0';
    }

    return *((const char*)self->data);
}

/**
 * @brief
 *   Get the value of a boolean property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0 and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_BOOL</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the boolean value currently stored in the
 * property @em self.
 */

int
cpl_property_get_bool(const cpl_property *self)
{
    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    if (self->type != CPL_TYPE_BOOL) {
        (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                     "name=%s, type=%s",
                                     self->name,
                                     cpl_type_get_name(self->type));
        return 0;
    }

    return *((const cpl_boolean*)self->data);
}

/**
 * @brief
 *   Get the value of an integer property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0 and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the integer value currently stored in the
 * property @em self.
 */

int
cpl_property_get_int(const cpl_property *self)
{
    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    if (self->type != CPL_TYPE_INT) {
        (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                     "name=%s, type=%s",
                                     self->name,
                                     cpl_type_get_name(self->type));
        return 0;
    }

    return *((const int*)self->data);
}

/**
 * @brief
 *   Get the value of a long property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0 and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_LONG</tt>
 *         or <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the value currently stored in the property @em self
 * as a long integer. The function accepts properties of integer type
 * with a rank less or equal than the function's return type.
 */

long
cpl_property_get_long(const cpl_property *self)
{
    long value = 0;


    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return value;
    }

    /*
     * If the property self is of the same kind, but of a smaller rank,
     * promote the property's value to the target type.
     */

    switch (self->type) {
        case CPL_TYPE_INT:
        {
            value = *((const int*)self->data);
            break;
        }

        case CPL_TYPE_LONG:
        {
            value = *((const long*)self->data);
            break;
        }

        default:
        {
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                         "name=%s, type=%s",
                                         self->name,
                                         cpl_type_get_name(self->type));
            break;
        }
    }

    return value;
}

/**
 * @brief
 *   Get the value of a long long property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0 and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_LONG_LONG</tt>,
 *         <tt>CPL_TYPE_LONG</tt>, or <tt>CPL_TYPE_INT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the value currently stored in the property @em self
 * as a long long integer. The function accepts properties of integer type
 * with a rank less or equal than the function's return type.
 */

long long
cpl_property_get_long_long(const cpl_property *self)
{
    long long value = 0;


    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return value;
    }

    /*
     * If the property self is of the same kind, but of a smaller rank,
     * promote the property's value to the target type.
     */

    switch (self->type) {
        case CPL_TYPE_INT:
        {
            value = *((const int*)self->data);
            break;
        }

        case CPL_TYPE_LONG:
        {
            value = *((const long*)self->data);
            break;
        }

        case CPL_TYPE_LONG_LONG:
        {
            value = *((const long long*)self->data);
            break;
        }

        default:
        {
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                         "name=%s, type=%s",
                                         self->name,
                                         cpl_type_get_name(self->type));
            break;
        }
    }

    return value;
}

/**
 * @brief
 *   Get the value of a float property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0. and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_FLOAT</tt>
 *         or <tt>CPL_TYPE_DOUBLE</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the value currently stored in the property @em self
 * as a float. The function also accepts properties of type double and returns
 * the property value as float.
 *
 * @note
 *  Calling the function for a property of type double may lead to
 *  truncation errors!
 */

float
cpl_property_get_float(const cpl_property *self)
{
    float value = 0.0;


    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return value;
    }

    /*
     * If the property self is of type double cast it to float.
     * This may lead to truncation errors!
     */

    switch (self->type) {
        case CPL_TYPE_FLOAT:
        {
            cx_assert(sizeof(value) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&value, self->data, sizeof(value));
            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            double pvalue;
            cx_assert(sizeof(pvalue) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&pvalue, self->data, sizeof(pvalue));
            value = pvalue;
            break;
        }

        default:
        {
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                         "name=%s, type=%s",
                                         self->name,
                                         cpl_type_get_name(self->type));
            break;
        }
    }

    return value;
}

/**
 * @brief
 *   Get the value of a double property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0. and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_DOUBLE</tt>
 *         or <tt>CPL_TYPE_FLOAT</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the value currently stored in the property @em self
 * as a double. The function accepts properties of a floating-point type
 * with a rank less or equal than the function's return type.
 */

double
cpl_property_get_double(const cpl_property *self)
{
    double value = 0.0;


    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return value;
    }

    /*
     * If the property self is of the same kind, but of a smaller rank,
     * promote the property's value to the target type.
     */

    switch (self->type) {
        case CPL_TYPE_FLOAT:
        {
            float pvalue;
            cx_assert(sizeof(pvalue) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&pvalue, self->data, sizeof(pvalue));
            value = pvalue;
            break;
        }

        case CPL_TYPE_DOUBLE:
        {
            cx_assert(sizeof(value) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&value, self->data, sizeof(value));
            break;
        }

        default:
        {
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                         "name=%s, type=%s",
                                         self->name,
                                         cpl_type_get_name(self->type));
            break;
        }
    }

    return value;
}

/**
 * @brief
 *   Get the value of a string property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   @c NULL and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type <tt>CPL_TYPE_STRING</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the string value currently stored in the
 * property @em self.
 *
 * @see cpl_property_get_size()
 */

const char *
cpl_property_get_string(const cpl_property *self)
{
    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (self->type != CPL_TYPE_STRING) {
        (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                     "name=%s, type=%s",
                                     self->name,
                                     cpl_type_get_name(self->type));
        return NULL;
    }

    return self->value;
}

/**
 * @brief
 *   Get the value of a float complex property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0. and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type
 *            <tt>CPL_TYPE_FLOAT_COMPLEX</tt> or
 *            <tt>CPL_TYPE_DOUBLE_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the value currently stored in the property @em self
 * as a float complex. The function also accepts properties of type
 * double complex and returns the property value as float complex.
 *
 * @note
 *  Calling the function for a property of type double complex may lead to
 *  truncation errors!
 */

float complex
cpl_property_get_float_complex(const cpl_property *self)
{
    float complex value = 0.0;


    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return value;
    }

    /*
     * If the property self is of type double complex cast it to float complex.
     * This may lead to truncation errors!
     */

    switch (self->type) {
        case CPL_TYPE_FLOAT_COMPLEX:
        {
            cx_assert(sizeof(value) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&value, self->data, sizeof(value));
            break;
        }

        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            double complex pvalue;
            cx_assert(sizeof(pvalue) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&pvalue, self->data, sizeof(pvalue));
            value = pvalue;
            break;
        }

        default:
        {
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                         "name=%s, type=%s",
                                         self->name,
                                         cpl_type_get_name(self->type));
            break;
        }
    }

    return value;
}


/**
 * @brief
 *   Get the value of a double complex property.
 *
 * @param self  The property.
 *
 * @return
 *   The current property value. If an error occurs the function returns
 *   0. and an appropriate error code is set.
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
 *         The property <i>self</i> is not of type
 *            <tt>CPL_TYPE_DOUBLE_COMPLEX</tt> or
 *            <tt>CPL_TYPE_FLOAT_COMPLEX</tt>.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the value currently stored in the property @em self
 * as a double complex. The function accepts properties of a complex type
 * with a rank less or equal than the function's return type.
 */

double complex
cpl_property_get_double_complex(const cpl_property *self)
{
    double complex value = 0.0;


    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return value;
    }

    /*
     * If the property self is of the same kind, but of a smaller rank,
     * promote the property's value to the target type.
     */

    switch (self->type) {
        case CPL_TYPE_FLOAT_COMPLEX:
        {
            float complex pvalue;
            cx_assert(sizeof(pvalue) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&pvalue, self->data, sizeof(pvalue));
            value = pvalue;
            break;
        }

        case CPL_TYPE_DOUBLE_COMPLEX:
        {
            cx_assert(sizeof(value) == cpl_type_get_sizeof(self->type));
            (void)memcpy(&value, self->data, sizeof(value));
            break;
        }

        default:
        {
            (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                         "name=%s, type=%s",
                                         self->name,
                                         cpl_type_get_name(self->type));
            break;
        }
    }

    return value;
}

/**
 * @internal
 * @brief
 *   Compare two properties according to their pre-set sort key
 * @param    p1      the first  property, non-NULL
 * @param    p2      the second property, non-NULL
 * @return   -1 if p1 < p2, 0 if p1 == p2, +1 if p1 > p2
 * @note This internal function has no error checking.
 * @see cpl_property_set_sort_dicb(), cpl_propertylist_sort()
 * 
 */
int cpl_property_compare_sortkey(const cpl_property * p1,
                                 const cpl_property * p2)
{
    /* Cannot order properties with an undefined sorting order */
    if (p1->ksort == CPL_PROPERTY_SORT_UNDEF ||
        p2->ksort == CPL_PROPERTY_SORT_UNDEF) return 0;

    if (p1->ksort > p2->ksort) return 1;
    else if (p1->ksort < p2->ksort) return -1;
    else return 0;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Set the DICB type of the name
  @param self  The property, must be non-NULL
  @return   nothing
  @note This internal function has no error checking
  @see cpl_property_compare_sortkey(), cpl_propertylist_sort()

  The function does not validate its input according to the FITS standard,
  it merely provides the type specified for DICB in order to sort the FITS
  keys.

  The correctness of the ordering in this function is difficult to verify
  because CFITSIO itself orders several cards in accordance with the FITS
  standard. For the same reason, any incorrectness regarding such keys will
  have no effect on any written FITS header.

 */
/*----------------------------------------------------------------------------*/
void cpl_property_set_sort_dicb(cpl_property * self)
{
    const size_t  size = self->nsize > 0 ? self->nsize : 1 + strlen(self->name);
    const char  * key  = self->name;
    cpl_property_dicbtype kt;

    /* The sort key must fit in a single char */
    cx_assert(CPL_DICB_END == (unsigned char)CPL_DICB_END);

    /* First matching the length of the key to the length of the string literal
       allows for a static-length memcmp() which should be inline-able. */

    /* NB: The indentation that aligns the calls to memcmp() helps to
       ensure that strings of identical length share the correct branch */

    if (size > 3 && !memcmp(key, "ESO", 3)) {
        kt = CPL_DICB_HIERARCH_ESO; /* Default HIERARCH ESO type */

        if (size > 7) {
            /* Specific HIERARCH ESO types take precedence */
            if      (!memcmp(key, "ESO DPR", 7)) kt = CPL_DICB_HIERARCH_DPR;
            else if (!memcmp(key, "ESO OBS", 7)) kt = CPL_DICB_HIERARCH_OBS;
            else if (!memcmp(key, "ESO TPL", 7)) kt = CPL_DICB_HIERARCH_TPL;
            else if (!memcmp(key, "ESO GEN", 7)) kt = CPL_DICB_HIERARCH_GEN;
            else if (!memcmp(key, "ESO TEL", 7)) kt = CPL_DICB_HIERARCH_TEL;
            else if (!memcmp(key, "ESO INS", 7)) kt = CPL_DICB_HIERARCH_INS;
            else if (!memcmp(key, "ESO DET", 7)) kt = CPL_DICB_HIERARCH_DET;
            else if (!memcmp(key, "ESO LOG", 7)) kt = CPL_DICB_HIERARCH_LOG;
            else if (!memcmp(key, "ESO PRO", 7)) kt = CPL_DICB_HIERARCH_PRO;
        }

        self->ksort = kt;

        return;
    } 

    /* The default value for a FITS key of length less than 9 */
    kt = CPL_DICB_PRIMARY;

    /* Switch on the size. The number of characters is one less */
    switch (size) {
    case 4:
        if (!memcmp(key, "END", 3)) kt = CPL_DICB_END;
        break;

    case 6:
        if      (!memcmp(key, "BZERO", 5)) kt = CPL_DICB_BZERO;
        else if (!memcmp(key, "NAXIS", 5)) kt = CPL_DICB_NAXIS;
        else if (!memcmp(key, "GROUP", 5)) kt = CPL_DICB_GROUP;

        break;

    case 7:
        if      (!memcmp(key, "BITPIX", 6)) kt = CPL_DICB_BITPIX;
        else if (!memcmp(key, "BSCALE", 6)) kt = CPL_DICB_BSCALE;
        else if (!memcmp(key, "EXTEND", 6)) kt = CPL_DICB_EXTEND;
        else if (!memcmp(key, "PCOUNT", 6)) kt = CPL_DICB_PCOUNT;
        else if (!memcmp(key, "GCOUNT", 6)) kt = CPL_DICB_GCOUNT;
        else if (!memcmp(key, "SIMPLE", 6)) kt = CPL_DICB_TOP;
        else {
            cpl_property_dicbtype mykt = kt;

            /* NAXISn/TFORMn/TBCOLn, n = 1..9 */

            if      (!memcmp(key, "NAXIS", 5)) mykt = CPL_DICB_NAXISn;
            else if (!memcmp(key, "TFORM", 5)) mykt = CPL_DICB_TFORMn;
            else if (!memcmp(key, "TBCOL", 5)) mykt = CPL_DICB_TBCOLn;

            if (mykt != kt &&
                '1' <= key[5] && key[5] <= '9') {
                kt = mykt;
            }
        }
        break;
    case 8:
        if      (!memcmp(key, "HISTORY", 7)) kt = CPL_DICB_HISTORY;
        else if (!memcmp(key, "COMMENT", 7)) kt = CPL_DICB_COMMENT;
        else if (!memcmp(key, "TFIELDS", 7)) kt = CPL_DICB_TFIELDS;
        else {
            cpl_property_dicbtype mykt = kt;

            /* NAXISn/TFORMn/TBCOLn, n = 10..99 */

            if      (!memcmp(key, "NAXIS", 5)) mykt = CPL_DICB_NAXISn;
            else if (!memcmp(key, "TFORM", 5)) mykt = CPL_DICB_TFORMn;
            else if (!memcmp(key, "TBCOL", 5)) mykt = CPL_DICB_TBCOLn;

            if (mykt != kt &&
                '1' <= key[5] && key[5] <= '9' &&
                '0' <= key[6] && key[6] <= '9') {
                kt = mykt;
            }
        }
        break;

    case 9:
        if (!memcmp(key, "XTENSION", 8)) kt = CPL_DICB_TOP;
        else {
            cpl_property_dicbtype mykt = kt;

            /* NAXISn/TFORMn/TBCOLn, n = 100..999 */

            if      (!memcmp(key, "NAXIS", 5)) mykt = CPL_DICB_NAXISn;
            else if (!memcmp(key, "TFORM", 5)) mykt = CPL_DICB_TFORMn;
            else if (!memcmp(key, "TBCOL", 5)) mykt = CPL_DICB_TBCOLn;

            if (mykt != kt &&
                '1' <= key[5] && key[5] <= '9' &&
                '0' <= key[6] && key[6] <= '9' &&
                '0' <= key[7] && key[7] <= '9') {
                kt = mykt;
            }
        }
        break;

    default:
        if (size > 9) {
            /* With a length of 9 or more, this is not a standard key... */

            kt = CPL_DICB_HIERARCH;
        }
        break;
    }

    self->ksort = kt;
}


/**
 * @internal
 * @brief
 *   Get the size of the property name
 * @param self  The property
 * @return The size of the name (excl. terminating null)
 * @note This internal function has no error checking.
 *
 */
inline size_t cpl_property_get_size_name(const cpl_property * self)
{
    return self->nsize > 0 ? (size_t)self->nsize - 1
        : strlen(self->name);   /* Name not in container. :-( */
}

/**
 * @internal
 * @brief
 *   Get the size of the property comment
 * @param self  The property
 * @return The size of the comment (excl. terminating null)
 * @note This internal function has no error checking.
 *
 */
inline size_t cpl_property_get_size_comment(const cpl_property * self)
{
    return self->csize > 0 ? (size_t)self->csize - 1
        : strlen(self->comment);   /* Comment not in container. :-( */
}

/**@}*/

/**
 * @brief
 *   Print a property.
 *
 * @param property Pointer to property
 * @param stream   The output stream
 *
 * @return Nothing.
 *
 * This function is mainly intended for debug purposes.
 * If the specified stream is @c NULL, it is set to @em stdout.
 * The function used for printing is the standard C @c fprintf().
 */

void cpl_property_dump(const cpl_property *property, FILE * stream)
{

    const char *name = cpl_property_get_name(property);
    const char *comment = cpl_property_get_comment(property);

    char c;

    cpl_size size = cpl_property_get_size(property);

    cpl_type type = cpl_property_get_type(property);


    fprintf(stream, "Property at address %p\n", (const void*)property);
    fprintf(stream, "\tname   : %p '%s'\n", name, name);
    fprintf(stream, "\tcomment: %p '%s'\n", comment, comment);
    fprintf(stream, "\ttype   : %#09x\n", type);
    fprintf(stream, "\tsize   : %lld\n", size);
    fprintf(stream, "\tvalue  : ");


    switch (type) {
        case CPL_TYPE_CHAR:
            c = cpl_property_get_char(property);
            if (!c)
                fprintf(stream, "''");
            else
                fprintf(stream, "'%c'", c);
            break;

        case CPL_TYPE_BOOL:
            fprintf(stream, "%d", cpl_property_get_bool(property));
            break;

        case CPL_TYPE_INT:
            fprintf(stream, "%d", cpl_property_get_int(property));
            break;

        case CPL_TYPE_LONG:
            fprintf(stream, "%ld", cpl_property_get_long(property));
            break;

        case CPL_TYPE_LONG_LONG:
            fprintf(stream, "%lld", cpl_property_get_long_long(property));
            break;

        case CPL_TYPE_FLOAT:
            fprintf(stream, "%g", cpl_property_get_float(property));
            break;

        case CPL_TYPE_DOUBLE:
            fprintf(stream, "%g", cpl_property_get_double(property));
            break;

        case CPL_TYPE_STRING:
            fprintf(stream, "'%s'", cpl_property_get_string(property));
            break;

        default:
            fprintf(stream, "unknown.");
            break;

    }

    fprintf(stream, "\n");

    return;

}

#ifdef CPL_CXSTR_DEBUG

/**
 * @internal
 * @brief  Abort on inconsistent CXSTR() call
 * @param  string  The string to test
 * @param  length  The length of the string to test
 * @note   Only for testing, will abort()!
 */
const cpl_cstr * cxstr_abort(const char* string, cxsize length)
{
    const size_t allocsz = string ? strlen(string) : 0;

    cpl_msg_error(cpl_func, "CXSTR(\"%s\", %d) <=> %zd",
                  string ? string : "<NULL>",
                  (int)length, allocsz);

    cx_assert(allocsz == length);
    abort();

    return NULL;
}

#endif
