/*
 * This file is part of the ESO C Extension Library
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

#ifndef CX_SNPRINTF_H
#define CX_SNPRINTF_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_STDDEF_H
#  include <stdarg.h>
#endif

#ifdef HAVE_STDDEF_H
#  include <stddef.h>
#endif

/* Just for defining the CX_BEGIN_DECLS and CX_END_DECLS symbols. */
#include <cxtypes.h>


/* Define aliases for the replacement functions */

#if ! defined(HAVE_SNPRINTF)
#  define snprintf   rpl_snprintf
#endif

#if ! defined(HAVE_VSNPRINTF)
#  define vsnprintf  rpl_vsnprintf
#endif

#if ! defined(HAVE_ASPRINTF)
#  define asprintf  rpl_asprintf
#endif

#if ! defined(HAVE_VASPRINTF)
#  define vasprintf  rpl_vasprintf
#endif


CX_BEGIN_DECLS

#if ! defined(HAVE_SNPRINTF)
int rpl_snprintf(char *str, size_t size, const char *fmt, ...);
#endif

#if ! defined(HAVE_VSNPRINTF)
int rpl_vsnprintf(char *str, size_t size, const char *fmt, va_list args);
#endif

#if ! defined(HAVE_ASPRINTF)
int rpl_asprintf(char **str, const char *fmt, ...);
#endif

#if ! defined(HAVE_VASPRINTF)
int rpl_vasprintf(char **str, const char *fmt, va_list args);
#endif

CX_END_DECLS

#endif /* CX_SNPRINTF_H */
