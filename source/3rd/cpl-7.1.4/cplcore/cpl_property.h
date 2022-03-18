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

#ifndef CPL_PROPERTY_H
#define CPL_PROPERTY_H

#include <cpl_type.h>
#include <cpl_error.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_property
 *
 * @brief
 *   The opaque property data type.
 */

typedef struct _cpl_property_ cpl_property;


/*
 * Create, copy and destroy operations.
 */

cpl_property *cpl_property_new(const char *name,
                               cpl_type type) CPL_ATTR_ALLOC;
cpl_property *cpl_property_new_array(const char *name, cpl_type type,
                                     cpl_size size) CPL_ATTR_ALLOC;
cpl_property *cpl_property_duplicate(const cpl_property *other) CPL_ATTR_ALLOC;
void cpl_property_delete(cpl_property *self);

/*
 * Non modifying operations
 */

cpl_size cpl_property_get_size(const cpl_property *self);
cpl_type cpl_property_get_type(const cpl_property *self);

/*
 * Assignment operations
 */

cpl_error_code cpl_property_set_name(cpl_property *self, const char *name);
cpl_error_code cpl_property_set_comment(cpl_property *self,
                                        const char *comment);
cpl_error_code cpl_property_set_char(cpl_property *self, char value);
cpl_error_code cpl_property_set_bool(cpl_property *self, int value);
cpl_error_code cpl_property_set_int(cpl_property *self, int value);
cpl_error_code cpl_property_set_long(cpl_property *self, long value);
cpl_error_code cpl_property_set_long_long(cpl_property *self, long long value);
cpl_error_code cpl_property_set_float(cpl_property *self, float value);
cpl_error_code cpl_property_set_double(cpl_property *self, double value);
cpl_error_code cpl_property_set_string(cpl_property *self, const char *value);
cpl_error_code cpl_property_set_double_complex(cpl_property *self,
                                               _Complex double value);
cpl_error_code cpl_property_set_float_complex(cpl_property *self,
                                              _Complex float value);

/*
 * Element access
 */

const char *cpl_property_get_name(const cpl_property *self);
const char *cpl_property_get_comment(const cpl_property *self);
char cpl_property_get_char(const cpl_property *self);
int cpl_property_get_bool(const cpl_property *self);
int cpl_property_get_int(const cpl_property *self);
long cpl_property_get_long(const cpl_property *self);
long long cpl_property_get_long_long(const cpl_property *self);
float cpl_property_get_float(const cpl_property *self);
double cpl_property_get_double(const cpl_property *self);
const char *cpl_property_get_string(const cpl_property *self);
_Complex float cpl_property_get_float_complex(const cpl_property *self);
_Complex double cpl_property_get_double_complex(const cpl_property *self);

CPL_END_DECLS

#endif /* CPL_PROPERTY_H */
