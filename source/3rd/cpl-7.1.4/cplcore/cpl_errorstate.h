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

#ifndef CPL_ERRORSTATE_H
#define CPL_ERRORSTATE_H

#include <cpl_macros.h>

#include <cpl_type.h>
 
CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                           Defines
 -----------------------------------------------------------------------------*/

#ifndef CPL_ERROR_HISTORY_SIZE
#define CPL_ERROR_HISTORY_SIZE 20
#endif

typedef const void * cpl_errorstate;

/*-----------------------------------------------------------------------------
                           Prototypes of functions 
 -----------------------------------------------------------------------------*/

cpl_errorstate cpl_errorstate_get(void) CPL_ATTR_PURE;
void cpl_errorstate_set(cpl_errorstate);
cpl_boolean cpl_errorstate_is_equal(cpl_errorstate) CPL_ATTR_PURE;

void cpl_errorstate_dump(cpl_errorstate,
                         cpl_boolean,
                         void (*)(unsigned, unsigned, unsigned));

void cpl_errorstate_dump_one(unsigned, unsigned, unsigned);
void cpl_errorstate_dump_one_warning(unsigned, unsigned, unsigned);
void cpl_errorstate_dump_one_info(unsigned, unsigned, unsigned);
void cpl_errorstate_dump_one_debug(unsigned, unsigned, unsigned);

CPL_END_DECLS

#endif
/* end of cpl_errorstate.h */
