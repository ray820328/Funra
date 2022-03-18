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

#ifndef CPL_XMEMORY_H
#define CPL_XMEMORY_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cpl_macros.h>

/*-----------------------------------------------------------------------------
                               Function prototypes
 -----------------------------------------------------------------------------*/

void * cpl_xmemory_malloc_count(size_t)          CPL_ATTR_MALLOC;
void * cpl_xmemory_calloc_count(size_t, size_t)  CPL_ATTR_CALLOC;
void * cpl_xmemory_realloc_count(void *, size_t) CPL_ATTR_REALLOC;
void   cpl_xmemory_free_count(void *);

void * cpl_xmemory_malloc(size_t)          CPL_ATTR_MALLOC;
void * cpl_xmemory_calloc(size_t, size_t)  CPL_ATTR_CALLOC;
void * cpl_xmemory_realloc(void *, size_t) CPL_ATTR_REALLOC;
void   cpl_xmemory_free(void *);

void cpl_xmemory_status(int);
int  cpl_xmemory_is_empty(int) CPL_ATTR_PURE;

#endif
