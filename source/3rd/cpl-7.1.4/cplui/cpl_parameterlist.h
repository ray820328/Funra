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

#ifndef CPL_PARAMETERLIST_H
#define CPL_PARAMETERLIST_H

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_parameter.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_parameterlist
 *
 * @brief
 *   The opaque parameter list data type.
 */

typedef struct _cpl_parameterlist_ cpl_parameterlist;


/*
 * Create, copy and destroy operations
 */

cpl_parameterlist *cpl_parameterlist_new(void) CPL_ATTR_ALLOC;
void cpl_parameterlist_delete(cpl_parameterlist *self);


/*
 * Non modifying operations
 */

cpl_size cpl_parameterlist_get_size(const cpl_parameterlist *self);

/*
 * Element insertion.
 */

cpl_error_code cpl_parameterlist_append(cpl_parameterlist *self,
                                        cpl_parameter *parameter);

/*
 * Element access
 */

const cpl_parameter *cpl_parameterlist_get_first_const(const cpl_parameterlist *self);
cpl_parameter *cpl_parameterlist_get_first(cpl_parameterlist *self);

const cpl_parameter *cpl_parameterlist_get_next_const(const cpl_parameterlist *self);
cpl_parameter *cpl_parameterlist_get_next(cpl_parameterlist *self);

const cpl_parameter *cpl_parameterlist_get_last_const(const cpl_parameterlist *self);
cpl_parameter *cpl_parameterlist_get_last(cpl_parameterlist *self);

const cpl_parameter *cpl_parameterlist_find_const(const cpl_parameterlist *self,
                                      const char *name);
cpl_parameter *cpl_parameterlist_find(cpl_parameterlist *self,
                                      const char *name);

const cpl_parameter *cpl_parameterlist_find_type_const(const cpl_parameterlist *self,
                                           cpl_type type);
cpl_parameter *cpl_parameterlist_find_type(cpl_parameterlist *self,
                                           cpl_type type);

const cpl_parameter *cpl_parameterlist_find_context_const(const cpl_parameterlist *self,
                                              const char *context);
cpl_parameter *cpl_parameterlist_find_context(cpl_parameterlist *self,
                                              const char *context);

const cpl_parameter *cpl_parameterlist_find_tag_const(const cpl_parameterlist *self,
                                          const char *tag);
cpl_parameter *cpl_parameterlist_find_tag(cpl_parameterlist *self,
                                          const char *tag);

/*
 * Debugging
 */

void cpl_parameterlist_dump(const cpl_parameterlist *self, FILE *stream);

CPL_END_DECLS

#endif /* CPL_PARAMETERLIST_H */
