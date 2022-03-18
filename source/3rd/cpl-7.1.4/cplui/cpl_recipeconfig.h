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

#ifndef CPL_RECIPECONFIG_H
#define CPL_RECIPECONFIG_H

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_framedata.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_cplrecipeconfig
 *
 * @brief
 *  The opaque recipe configuration object type.
 */

typedef struct _cpl_recipeconfig_ cpl_recipeconfig;


/*
 * Create, copy and destroy operations
 */

cpl_recipeconfig* cpl_recipeconfig_new(void) CPL_ATTR_ALLOC;
void cpl_recipeconfig_delete(cpl_recipeconfig* self);
void cpl_recipeconfig_clear(cpl_recipeconfig* self);

/*
 * Non modifying operations
 */

char** cpl_recipeconfig_get_tags(const cpl_recipeconfig* self);
char** cpl_recipeconfig_get_inputs(const cpl_recipeconfig* self,
                                   const char* tag);
char** cpl_recipeconfig_get_outputs(const cpl_recipeconfig* self,
                                    const char* tag);

cpl_size cpl_recipeconfig_get_min_count(const cpl_recipeconfig* self,
                                        const char* tag, const char* input);
cpl_size cpl_recipeconfig_get_max_count(const cpl_recipeconfig* self,
                                        const char* tag, const char* input);
int cpl_recipeconfig_is_required(const cpl_recipeconfig* self,
                                 const char* tag, const char* input);

/*
 * Assignment operations
 */

int cpl_recipeconfig_set_tag(cpl_recipeconfig* self, const char* tag,
                             cpl_size min_count, cpl_size max_count);
int cpl_recipeconfig_set_input(cpl_recipeconfig* self, const char* tag,
                               const char* input, cpl_size min_count,
                               cpl_size max_count);
int cpl_recipeconfig_set_output(cpl_recipeconfig* self, const char* tag,
                                const char* output);

int cpl_recipeconfig_set_tags(cpl_recipeconfig* self,
                              const cpl_framedata* data);
int cpl_recipeconfig_set_inputs(cpl_recipeconfig* self, const char* tag,
                                const cpl_framedata* data);
int cpl_recipeconfig_set_outputs(cpl_recipeconfig* self, const char* tag,
                                 const char** outputs);

CPL_END_DECLS

#endif /* CPL_RECIPECONFIG_H */
