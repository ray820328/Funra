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

#ifndef CPL_MEMORY_H
#define CPL_MEMORY_H

#include <sys/types.h>
#include <stdarg.h>

#include <cpl_macros.h>

/*----------------------------------------------------------------------------
                            Function prototypes
 ----------------------------------------------------------------------------*/

CPL_BEGIN_DECLS

void *cpl_malloc(size_t) CPL_ATTR_MALLOC;
void *cpl_calloc(size_t, size_t) CPL_ATTR_CALLOC;
void *cpl_realloc(void *, size_t) CPL_ATTR_REALLOC;

void cpl_free(void *);

char *cpl_strdup(const char *) CPL_ATTR_ALLOC;

char *cpl_vsprintf(const char *, va_list) CPL_ATTR_ALLOC CPL_ATTR_PRINTF(1, 0);
char *cpl_sprintf (const char *, ...)     CPL_ATTR_ALLOC CPL_ATTR_PRINTF(1, 2);

int cpl_memory_is_empty(void) CPL_ATTR_PURE;

void cpl_memory_dump(void);

CPL_END_DECLS

#endif /* CPL_MEMORY_H */
