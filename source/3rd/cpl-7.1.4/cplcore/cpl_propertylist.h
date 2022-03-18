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

#ifndef CPL_PROPERTYLIST_H
#define CPL_PROPERTYLIST_H

#include <stdio.h>

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_property.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_propertylist
 *
 * @brief
 *   The opaque property list data type.
 */

typedef struct _cpl_propertylist_ cpl_propertylist;


/**
 * @ingroup cpl_propertylist
 *
 * @brief
 *   The property comparison function data type.
 */

typedef int (*cpl_propertylist_compare_func)(const cpl_property *first,
                                             const cpl_property *second);


/*
 * Create, copy and destroy operations.
 */

cpl_propertylist *
cpl_propertylist_new(void) CPL_ATTR_ALLOC;

cpl_propertylist *
cpl_propertylist_duplicate(const cpl_propertylist *other) CPL_ATTR_ALLOC;

void
cpl_propertylist_delete(cpl_propertylist *self);


/*
 * Non modifying operations
 */

cpl_size
cpl_propertylist_get_size(const cpl_propertylist *self);

int
cpl_propertylist_is_empty(const cpl_propertylist *self);

cpl_type
cpl_propertylist_get_type(const cpl_propertylist *self, const char *name);

int
cpl_propertylist_has(const cpl_propertylist *self, const char *name);


/*
 * Assignment operations
 */

cpl_error_code
cpl_propertylist_set_comment(cpl_propertylist *self, const char *name,
                             const char *comment);
cpl_error_code
cpl_propertylist_set_char(cpl_propertylist *self, const char *name,
                          char value);
cpl_error_code
cpl_propertylist_set_bool(cpl_propertylist *self, const char *name ,
                          int value);
cpl_error_code
cpl_propertylist_set_int(cpl_propertylist *self, const char *name,
                         int value);
cpl_error_code
cpl_propertylist_set_long(cpl_propertylist *self, const char *name,
                          long value);
cpl_error_code
cpl_propertylist_set_long_long(cpl_propertylist *self, const char *name,
                               long long value);
cpl_error_code
cpl_propertylist_set_float(cpl_propertylist *self, const char *name,
                           float value);
cpl_error_code
cpl_propertylist_set_double(cpl_propertylist *self, const char *name,
                            double value);
cpl_error_code
cpl_propertylist_set_string(cpl_propertylist *self, const char *name,
                            const char *value);
cpl_error_code
cpl_propertylist_set_float_complex(cpl_propertylist *self, const char *name,
                                   _Complex float value);
cpl_error_code
cpl_propertylist_set_double_complex(cpl_propertylist *self, const char *name,
                                    _Complex double value);


/*
 * Element access
 */

const cpl_property *
cpl_propertylist_get_const(const cpl_propertylist *self, long position);

cpl_property *
cpl_propertylist_get(cpl_propertylist *self, long position);

const cpl_property *
cpl_propertylist_get_property_const(const cpl_propertylist *self, const char *name);

cpl_property *
cpl_propertylist_get_property(cpl_propertylist *self, const char *name);

const char *
cpl_propertylist_get_comment(const cpl_propertylist *self, const char *name);

char
cpl_propertylist_get_char(const cpl_propertylist *self, const char *name);

int
cpl_propertylist_get_bool(const cpl_propertylist *self, const char *name);

int
cpl_propertylist_get_int(const cpl_propertylist *self, const char *name);

long
cpl_propertylist_get_long(const cpl_propertylist *self, const char *name);

long long
cpl_propertylist_get_long_long(const cpl_propertylist *self, const char *name);

float
cpl_propertylist_get_float(const cpl_propertylist *self, const char *name);

double
cpl_propertylist_get_double(const cpl_propertylist *self, const char *name);

const char *
cpl_propertylist_get_string(const cpl_propertylist *self, const char *name);

_Complex float
cpl_propertylist_get_float_complex(const cpl_propertylist *self,
                                   const char *name);

_Complex double
cpl_propertylist_get_double_complex(const cpl_propertylist *self,
                                    const char *name);


/*
 * Inserting and removing elements
 */

cpl_error_code
cpl_propertylist_insert_char(cpl_propertylist *self, const char *here,
                             const char *name, char value);

cpl_error_code
cpl_propertylist_insert_bool(cpl_propertylist *self, const char *here,
                             const char *name, int value);

cpl_error_code
cpl_propertylist_insert_int(cpl_propertylist *self, const char *here,
                            const char *name, int value);

cpl_error_code
cpl_propertylist_insert_long(cpl_propertylist *self, const char *here,
                             const char *name, long value);

cpl_error_code
cpl_propertylist_insert_long_long(cpl_propertylist *self, const char *here,
                                  const char *name, long long value);

cpl_error_code
cpl_propertylist_insert_float(cpl_propertylist *self, const char *here,
                              const char *name, float value);

cpl_error_code
cpl_propertylist_insert_double(cpl_propertylist *self, const char *here,
                               const char *name, double value);

cpl_error_code
cpl_propertylist_insert_string(cpl_propertylist *self, const char *here,
                               const char *name, const char *value);

cpl_error_code
cpl_propertylist_insert_float_complex(cpl_propertylist *self, const char *here,
                                      const char *name, _Complex float value);

cpl_error_code
cpl_propertylist_insert_double_complex(cpl_propertylist *self, const char *here,
                                       const char *name, _Complex double value);

cpl_error_code
cpl_propertylist_insert_after_char(cpl_propertylist *self, const char *after,
                                   const char *name, char value);

cpl_error_code
cpl_propertylist_insert_after_bool(cpl_propertylist *self, const char *after,
                                   const char *name, int value);

cpl_error_code
cpl_propertylist_insert_after_int(cpl_propertylist *self, const char *after,
                                  const char *name, int value);

cpl_error_code
cpl_propertylist_insert_after_long(cpl_propertylist *self, const char *after,
                                   const char *name, long value);

cpl_error_code
cpl_propertylist_insert_after_long_long(cpl_propertylist *self,
                                        const char *after, const char *name,
                                        long long value);

cpl_error_code
cpl_propertylist_insert_after_float(cpl_propertylist *self, const char *after,
                                    const char *name, float value);

cpl_error_code
cpl_propertylist_insert_after_double(cpl_propertylist *self, const char *after,
                                     const char *name, double value);

cpl_error_code
cpl_propertylist_insert_after_string(cpl_propertylist *self, const char *after,
                                     const char *name, const char *value);

cpl_error_code
cpl_propertylist_insert_after_float_complex(cpl_propertylist *self,
                                            const char *after, const char *name,
                                            _Complex float value);

cpl_error_code
cpl_propertylist_insert_after_double_complex(cpl_propertylist *self,
                                             const char *after,
                                             const char *name,
                                             _Complex double value);

cpl_error_code
cpl_propertylist_prepend_char(cpl_propertylist *self, const char *name,
                              char value);

cpl_error_code
cpl_propertylist_prepend_bool(cpl_propertylist *self, const char *name,
                              int value);
cpl_error_code
cpl_propertylist_prepend_int(cpl_propertylist *self, const char *name,
                             int value);

cpl_error_code
cpl_propertylist_prepend_long(cpl_propertylist *self, const char *name,
                              long value);

cpl_error_code
cpl_propertylist_prepend_long_long(cpl_propertylist *self, const char *name,
                                   long long value);

cpl_error_code
cpl_propertylist_prepend_float(cpl_propertylist *self, const char *name,
                               float value);

cpl_error_code
cpl_propertylist_prepend_double(cpl_propertylist *self, const char *name,
                                double value);

cpl_error_code
cpl_propertylist_prepend_string(cpl_propertylist *self, const char *name,
                                const char *value);

cpl_error_code
cpl_propertylist_prepend_float_complex(cpl_propertylist *self,
                                       const char *name,
                                       _Complex float value);

cpl_error_code
cpl_propertylist_prepend_double_complex(cpl_propertylist *self,
                                        const char *name,
                                        _Complex double value);


cpl_error_code
cpl_propertylist_append_char(cpl_propertylist *self, const char *name,
                             char value);

cpl_error_code
cpl_propertylist_append_bool(cpl_propertylist *self, const char *name,
                             int value);

cpl_error_code
cpl_propertylist_append_int(cpl_propertylist *self, const char *name,
                            int value);

cpl_error_code
cpl_propertylist_append_long(cpl_propertylist *self, const char *name,
                             long value);

cpl_error_code
cpl_propertylist_append_long_long(cpl_propertylist *self, const char *name,
                                  long long value);

cpl_error_code
cpl_propertylist_append_float(cpl_propertylist *self, const char *name,
                              float value);

cpl_error_code
cpl_propertylist_append_double(cpl_propertylist *self, const char *name,
                               double value);

cpl_error_code
cpl_propertylist_append_string(cpl_propertylist *self, const char *name,
                               const char *value);

cpl_error_code
cpl_propertylist_append_float_complex(cpl_propertylist *self, const char *name,
                                      _Complex float value);

cpl_error_code
cpl_propertylist_append_double_complex(cpl_propertylist *self, const char *name,
                                       _Complex double value);

cpl_error_code
cpl_propertylist_append(cpl_propertylist *self,
                        const cpl_propertylist *other);

int
cpl_propertylist_erase(cpl_propertylist *self, const char *name);

int
cpl_propertylist_erase_regexp(cpl_propertylist *self, const char *regexp,
                              int invert);

void
cpl_propertylist_empty(cpl_propertylist *self);


/*
 * Convenience functions
 */

cpl_error_code
cpl_propertylist_update_char(cpl_propertylist *self, const char *name,
                             char value);
cpl_error_code
cpl_propertylist_update_bool(cpl_propertylist *self, const char *name,
                             int value);
cpl_error_code
cpl_propertylist_update_int(cpl_propertylist *self, const char *name,
                            int value);
cpl_error_code
cpl_propertylist_update_long(cpl_propertylist *self, const char *name,
                             long value);
cpl_error_code
cpl_propertylist_update_long_long(cpl_propertylist *self, const char *name,
                                  long long value);
cpl_error_code
cpl_propertylist_update_float(cpl_propertylist *self, const char *name,
                              float value);
cpl_error_code
cpl_propertylist_update_double(cpl_propertylist *self, const char *name,
                               double value);
cpl_error_code
cpl_propertylist_update_string(cpl_propertylist *self, const char *name,
                               const char *value);
cpl_error_code
cpl_propertylist_update_float_complex(cpl_propertylist *self, const char *name,
                                      _Complex float value);
cpl_error_code
cpl_propertylist_update_double_complex(cpl_propertylist *self, const char *name,
                                       _Complex double value);


/*
 * Working on properties
 */

cpl_error_code
cpl_propertylist_copy_property(cpl_propertylist *self,
                               const cpl_propertylist *other,
                               const char *name);
cpl_error_code
cpl_propertylist_copy_property_regexp(cpl_propertylist *self,
                                      const cpl_propertylist *other,
                                      const char *regexp,
                                      int invert);

cpl_error_code
cpl_propertylist_append_property(cpl_propertylist *self,
                                 const cpl_property *property);

cpl_error_code
cpl_propertylist_prepend_property(cpl_propertylist *self,
                                 const cpl_property *property);
cpl_error_code
cpl_propertylist_insert_property(cpl_propertylist *self,
                                 const char *here,
                                 const cpl_property *property);
cpl_error_code
cpl_propertylist_insert_after_property(cpl_propertylist *self,
                                       const char *after,
                                       const cpl_property *property);

/*
 * Sorting
 */

cpl_error_code
cpl_propertylist_sort(cpl_propertylist *self,
                      cpl_propertylist_compare_func compare);

/*
 * Loading, saving and conversion operations.
 */

cpl_propertylist *
cpl_propertylist_load(const char *name, cpl_size position) CPL_ATTR_ALLOC;

cpl_propertylist *
cpl_propertylist_load_regexp(const char *name, cpl_size position,
                             const char *regexp, int invert) CPL_ATTR_ALLOC;
cpl_error_code
cpl_propertylist_save(const cpl_propertylist *self, const char *filename,
                      unsigned int mode);

void cpl_propertylist_dump(const cpl_propertylist *self, FILE *stream);

CPL_END_DECLS

#endif /* CPL_PROPERTYLIST_H */
