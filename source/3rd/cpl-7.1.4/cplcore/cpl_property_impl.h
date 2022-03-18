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

#ifndef CPL_PROPERTY_IMPL_H
#define CPL_PROPERTY_IMPL_H

#include <cpl_property.h>

#include <cxstring.h>
#include <stdio.h>

/* This internal struct is for creating a constant cx_string */
struct cpl_cstr {
    const cxchar *data;
    cxsize sz;
};

typedef struct cpl_cstr cpl_cstr;

/* CXSTR() allocates stack memory, so any pointer to it must have same scope */

#ifdef CPL_CXSTR_DEBUG
const cpl_cstr * cxstr_abort(const char*, cxsize);
#define CXSTR(STRING, SIZE) ((size_t)(SIZE) == ((STRING) ? strlen(STRING) : 0) \
                             ? &(const cpl_cstr){STRING, SIZE}         \
                             : cxstr_abort(STRING, SIZE))
#else
#define CXSTR(STRING, SIZE) (&(const cpl_cstr){STRING, SIZE})
#endif

/* Inline'd accessors not available here ... */
#define cx_string_get_(SELF) (SELF)->data
#define cx_string_size_(SELF) (SELF)->sz
#define cx_string_equal_(SELF, OTHER)                           \
    (cx_string_size_(SELF) == cx_string_size_(OTHER) &&         \
     !memcmp(cx_string_get_(SELF), cx_string_get_(OTHER),       \
             cx_string_size_(SELF)))                            \

CPL_BEGIN_DECLS

/**
 * @ingroup cpl_property
 */

/*
 * Private methods
 */

void
cpl_property_dump(const cpl_property *, FILE *)
    CPL_ATTR_NONNULL;

int cpl_property_compare_sortkey(const cpl_property *,
                                 const cpl_property *)
    CPL_INTERNAL CPL_ATTR_PURE CPL_ATTR_NONNULL;

void cpl_property_set_sort_dicb(cpl_property * self)
    CPL_INTERNAL CPL_ATTR_NONNULL;

size_t cpl_property_get_size_name(const cpl_property *)
     CPL_INTERNAL CPL_ATTR_PURE CPL_ATTR_NONNULL;

size_t cpl_property_get_size_comment(const cpl_property *)
     CPL_INTERNAL CPL_ATTR_PURE CPL_ATTR_NONNULL;

const char *
cpl_property_get_name_(const cpl_property *)
    CPL_INTERNAL CPL_ATTR_PURE CPL_ATTR_NONNULL;

cpl_size
cpl_property_get_size_(const cpl_property *)
    CPL_INTERNAL CPL_ATTR_PURE CPL_ATTR_NONNULL;

const char *
cpl_property_get_string_(const cpl_property *)
    CPL_INTERNAL CPL_ATTR_PURE CPL_ATTR_NONNULL;

const char *
cpl_property_get_comment_(const cpl_property *)
    CPL_INTERNAL CPL_ATTR_PURE CPL_ATTR_NONNULL;

cpl_property *
cpl_property_new_cx(const cpl_cstr *, cpl_type)
    CPL_ATTR_NONNULL CPL_ATTR_ALLOC;

void cpl_property_set_name_cx(cpl_property *, const cpl_cstr *)
    CPL_ATTR_NONNULL;

void cpl_property_set_string_cx(cpl_property *, const cpl_cstr *)
    CPL_ATTR_NONNULL;

void cpl_property_set_comment_cx(cpl_property *, const cpl_cstr *)
    CPL_ATTR_NONNULL;

CPL_END_DECLS

#endif /* CPL_PROPERTY_IMPL_H */
